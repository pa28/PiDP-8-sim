/*
 * CPU.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
   Portions of this program are based substantially on work by Robert M Supnik
   The license for Mr Supnik's work follows:
   Copyright (c) 1993-2013, Robert M Supnik
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.
 */

#include "CPU.h"
#include "Memory.h"
#include "Chassis.h"

#include "PDP8.h"

using namespace hw_sim;

namespace pdp8
{

#ifndef NOT_DEF
    int32_t PC = 0;										// Program Counter
    int32_t IF = 0;										// Instruction Field
    int32_t DF = 0;										// Data Field
    int32_t LAC = 0;									// Link Accumulator
    int32_t MQ = 0;										// Multiplier Quotient
    int32_t SC = 0;										// Step Counter
    int32_t IR = 0;										// Instruction Register
    int32_t MA = 0;										// Memory Address
    int32_t IB = 0;                                     /* Instruction Buffer */
#endif
    int32_t SF = 0;                                     /* Save Field */
    int32_t emode = 0;                                  /* EAE mode */
    int32_t gtf = 0;                                    /* EAE gtf flag */
    int32_t UB = 0;                                     /* User mode Buffer */
    int32_t UF = 0;                                     /* User mode Flag */
    int32_t OSR = 0;                                    /* Switch Register */
    int32_t tsc_ir = 0;                                 /* TSC8-75 IR */
    int32_t tsc_pc = 0;                                 /* TSC8-75 PC */
    int32_t tsc_cdf = 0;                                /* TSC8-75 CDF flag */
    int32_t tsc_enb = 0;                                /* TSC8-75 enabled */
    int32_t int_req = 0;                                /* intr requests */
    int32_t dev_done = 0;                               /* dev done flags */
    int32_t int_enable = INT_INIT_ENABLE;               /* intr enables */
    int32_t cpuLoadControl = 0;
    int32_t stop_inst = 0;                              /* trap on ill inst */


    CPU::CPU() :
            Device("CPU", "Central Processing Unit"),
            M(),
            rPC(0), rMQ(0), rIR(0), rIB(0), rOSR(0), rLAC(0), rDF(0), rIF(0), rSC(0), rMA(0),
            PC(rPC), MQ(rMQ), IR(rIR), IB(rIB), OSR(rOSR), LAC(rLAC), DF(rDF), IF(rIF), SC(rSC), AC(rLAC), L(rLAC),
            ION(int_req), MA(rMA), MA_F(rMA), MA_W(rMA),
            cpuState(NoState),
            cpuCondition(CPUStopped),
            cpuStepping(NotStepping),
            threadRunning(false),
            reason(STOP_NO_REASON),
            timerTickFlag(false),
            timerTickMutex(),
            idleMutex(),
            throttleTimerReset(false)
    {
        pthread_mutex_init( &mutexCondition, nullptr );
        pthread_cond_init( &condition, nullptr );
    }

    CPU::~CPU()
    {
        pthread_mutex_destroy( &mutexCondition );
        pthread_cond_destroy( &condition );
    }

    std::shared_ptr<CPU> CPU::getCPU() {
        std::shared_ptr<CPU> cpu;
        auto devItr = Chassis::instance()->find(DEV_CPU);
        if (devItr != Chassis::instance()->end()) {
            cpu = std::dynamic_pointer_cast<CPU>(devItr->second);
        }

        return cpu;
    }


    /*
     * By waiting on the run condition between CPU execution states, and at the end of the cycle,
     * the run loop does not have to do anything with cpuStepping.
     */
    void * CPU::run() {
        throttleTimerReset = true;
        struct timespec throttleStart{}, throttleCheck{};
        long	cpuTime = 0;
        threadRunning = true;

        while (threadRunning) {
            // If we are going to wait, reset throttling when we get going again.
            // Wait the thread if the condition is false (the CPU is not running).
            // This call is the opportunity to trap on break or illegal instructions.
            if (cpuStepping == PanelCommand || reason != STOP_NO_REASON || cpuCondition == CPUStopped) {
                if (cpuStepping == PanelCommand || reason > STOP_IDLE) {
                    cpuCondition = CPUStopped;
                }
//                debug(1, "cpuStepping %d, reason %d, cpuCondition %d\n", cpuStepping, reason, cpuCondition);
                throttleTimerReset = waitOnCondition();
            }
            cpuTime += cycleCpu();
#ifdef THROTTLEING
            if (cpuStepping == NotStepping) {
                if (throttleTimerReset) {
                    cpuTime = 0;
                    clock_gettime( CLOCK_MONOTONIC, &throttleStart );
                    throttleTimerReset = false;
                } else {
                    if ( timerTickFlag ) {
                        clock_gettime( CLOCK_MONOTONIC, &throttleCheck );

                        time_t d_sec = throttleCheck.tv_sec - throttleStart.tv_sec;
                        long d_nsec = (d_sec * 1000000000) + (throttleCheck.tv_nsec - throttleStart.tv_nsec);
                        debug( 20, "%ld.%09ld\n", throttleCheck.tv_sec, throttleCheck.tv_nsec);
                        debug( 20, "%ld.%09ld\n", throttleStart.tv_sec, throttleStart.tv_nsec);
                        debug( 20, "cputime %ld, d_nsec %ld\n", cpuTime, d_nsec );
                        if ((cpuTime - d_nsec) > 1000000) {
                            throttleCheck.tv_sec = 0;
                            throttleCheck.tv_nsec = cpuTime - d_nsec;
                            nanosleep( &throttleCheck, NULL );
                        }
                        throttleTimerReset = true;
                    }
                }
            }
#endif
            timerTickFlag = false;
        }

        return nullptr;
    }

    long CPU::cycleCpu() {
        long cycleTime = 0;
        reason = STOP_NO_REASON;
        int32_t device, pulse, temp, iot_data;

//        debug(10, "int_req %o, INT_PENDING %o\n", int_req, INT_PENDING);
        if (int_req > INT_PENDING) {                        /* interrupt? */
            int_req = int_req & ~INT_ION;                   /* interrupts off */
            SF = (UF << 6) | (IF() << 3) | (DF());          /* form save field */
            IF = IB = DF = UF = UB = 0;                     /* clear mem ext */
            M[0] = PC;                                      /* save PC in 0 */
            PC = 1;                                         /* fetch next from 1 */
        }

        cpuState = Fetch;

        MA_W = PC;
        MA_F = IF;                // create memory address
        IR = M[MA]();               // fetch instruction
        ++PC;

        int_req = int_req | INT_NO_ION_PENDING;             /* clear ION delay */


        // End of the Fetch state
        if (cpuStepping == SingleStep) {
            cpuCondition = CPUStopped;
            throttleTimerReset = waitOnCondition();
        }

        cpuState = Execute;

        if ( (IR & 07000) <= 05000 ) {                                 // memory access function
            if ( IR & 0200 ) {      // current page
                MA = (MA & 077600) | (IR & 0177);
            } else {                // page zero
                setMA(IF, IR & 0177);
            }

            if ( IR & 0400 ) {      // indirect
                cpuState = Defer;
                if ((MA_W & 07770) != 00010) { // not autoincrement
                    setMA(DF, M[MA]);
                } else {
                    setMA(DF, ++M[MA]);
                }

                // End of the Defer state
                if (cpuStepping == SingleStep) {
                    cpuCondition = CPUStopped;
                    throttleTimerReset = waitOnCondition();
                }
            }

            cpuState = Execute;

            switch ( IR & 07000 ) {
                case 00000: // AND
                    cycleTime = 3000;
                    AC = AC & M[MA];
                    break;
                case 01000: // TAD
                    cycleTime = 3000;
                    LAC = (LAC + M[MA]);
                    break;
                case 02000: // ISZ
                    cycleTime = 3000;
                    if (++M[MA] == 0)
                        ++PC;
                    break;
                case 03000: // DCA
                    cycleTime = 3000;
                    M[MA] = AC;
                    AC = 0;
                    break;
                case 04000: // JMS
                    /* Opcode 4, JMS.  From Bernhard Baehr's description of the TSC8-75:
                       (In user mode) the current JMS opcode is moved to the ERIOT register, the ECDF
                       flag is cleared. The address of the JMS instruction is loaded into the ERTB
                       register and the TSC8-75 I/O flag is raised. When the TSC8-75 is enabled, the
                       target addess of the JMS is loaded into PC, but nothing else (loading of IF, UF,
                       clearing the interrupt inhibit flag, storing of the return address in the first
                       word of the subroutine) happens. When the TSC8-75 is disabled, the JMS is performed
                       as usual. */
                    cycleTime = 3000;
                    if (UF) {                                       /* user mode? */
                        tsc_ir = IR;                                /* save instruction */
                        tsc_cdf = 0;                                /* clear flag */
                    }
                    if (UF && tsc_enb) {                            /* user mode, TSC enab? */
                        tsc_pc = (PC - 1) & 07777;                  /* save PC */
                        int_req = int_req | INT_TSC;                /* request intr */
                    }
                    else {                                          /* normal */
                        IF = IB;                                    /* change IF */
                        UF = UB;                                    /* change UF */
                        int_req = int_req | INT_NO_CIF_PENDING;     /* clr intr inhibit */
                        MA_F = IF;
                        M[MA] = PC;
                        PC = MA_W;
                        ++PC;
                    }
                    break;
                case 05000: // JMP
                    cycleTime = 1500;
                    /* Opcode 4, JMS.  From Bernhard Baehr's description of the TSC8-75:
                       (In user mode) the current JMS opcode is moved to the ERIOT register, the ECDF
                       flag is cleared. The address of the JMS instruction is loaded into the ERTB
                       register and the TSC8-75 I/O flag is raised. When the TSC8-75 is enabled, the
                       target addess of the JMS is loaded into PC, but nothing else (loading of IF, UF,
                       clearing the interrupt inhibit flag, storing of the return address in the first
                       word of the subroutine) happens. When the TSC8-75 is disabled, the JMS is performed
                       as usual. */
                    if (UF) {                                       /* user mode? */
                        tsc_ir = IR;                                /* save instruction */
                        tsc_cdf = 0;                                /* clear flag */
                        if (tsc_enb) {                              /* TSC8 enabled? */
                            tsc_pc = (PC - 1) & 07777;              /* save PC */
                            int_req = int_req | INT_TSC;            /* request intr */
                        }
                    }

                    if ( !(IR & 0400) ) {   // direct jump test for idle
                        if ( IF == IB ) {
                            if (MA_W == ((PC - 2) & 07777)) {       // JMP .-1
                                int32_t no = M[MA];
                                if ( (no == OP_KSF) ||              // next instruction is KSF
                                     (no == OP_CLSC)                // next instruction is CLSC
                                        )  {
                                    reason = STOP_IDLE;
                                }
                            } else if (MA_W == ((PC - 1) & 07777)) {    // JMP .
                                if (!(int_req & INT_ION)) {             /*    iof? */
                                    reason = STOP_ENDLESS_LOOP;         /* then infinite loop */
                                } else if (!(int_req & INT_ALL)) {      /*    ion, not intr? */
                                    reason = STOP_IDLE;
                                }                                       /* end JMP */
                            }
                        }
                    }

                    IF = IB;                                        /* change IF */
                    UF = UB;                                        /* change UF */
                    int_req = int_req | INT_NO_CIF_PENDING;         /* clr intr inhibit */
                    PC = MA;

                    if (reason == STOP_IDLE) {
                        throttleTimerReset = waitOnCondition();
                        reason = STOP_NO_REASON;
                    }

                    break;
                default:
                    break;
            }
        } else if ( (IR & 07000) == 06000) {    // IOT
            cycleTime = 4250;

            /* Opcode 6, IOT.  From Bernhard Baehr's description of the TSC8-75:
               (In user mode) Additional to raising a user mode interrupt, the current IOT
               opcode is moved to the ERIOT register. When the IOT is a CDF instruction (62x1),
               the ECDF flag is set, otherwise it is cleared. */

            if (UF) {                                       /* privileged? */
                int_req = int_req | INT_UF;                 /* request intr */
                tsc_ir = IR;                                /* save instruction */
                if ((IR & 07707) == 06201)                  /* set/clear flag */
                    tsc_cdf = 1;
                else tsc_cdf = 0;
            } else {
                device = (IR >> 3) & 077;                   /* device = IR<3:8> */
                pulse = IR & 07;                            /* pulse = IR<9:11> */
                iot_data = AC;                              /* AC unchanged */
                switch (device) {                           /* decode IR<3:8> */

                    case 000:                               /* CPU control */
                        switch (pulse) {                    /* decode IR<9:11> */

                            case 0:                         /* SKON */
                                if (int_req & INT_ION)
                                    ++PC;
                                int_req = int_req & ~INT_ION;
                                break;

                            case 1:                         /* ION */
                                int_req = (int_req | INT_ION) & ~INT_NO_ION_PENDING;
                                break;

                            case 2:                         /* IOF */
                                int_req = int_req & ~INT_ION;
                                break;

                            case 3:                         /* SRQ */
                                if (int_req & INT_ALL)
                                    ++PC;
                                break;

                            case 4:                         /* GTF */
                                LAC = (LAC & 010000) |
                                      ((LAC & 010000) >> 1) | (gtf << 10) |
                                      (((int_req & INT_ALL) != 0) << 9) |
                                      (((int_req & INT_ION) != 0) << 7) | SF;
                                break;

                            case 5:                         /* RTF */
                                gtf = ((LAC & 02000) >> 10);
                                UB = (LAC & 0100) >> 6;
                                IB = (LAC & 0070) << 9;
                                DF = (LAC & 0007) << 12;
                                LAC = ((LAC & 04000) << 1) | iot_data;
                                int_req = (int_req | INT_ION) & ~INT_NO_CIF_PENDING;
                                break;

                            case 6:                         /* SGT */
                                if (gtf)
                                    ++PC;
                                break;

                            case 7:                         /* CAF */
                                gtf = 0;
                                emode = 0;
                                int_req = int_req & INT_NO_CIF_PENDING;
                                dev_done = 0;
                                int_enable = INT_INIT_ENABLE;
                                LAC = 0;
                                reset_all (1);              /* reset all dev */
                                break;
                            default:
                                break;
                        }                                   /* end switch pulse */
                        break;                              /* end case 0 */

                    case 020:case 021:case 022:case 023:
                    case 024:case 025:case 026:case 027:            /* memory extension */
                        switch (pulse) {                            /* decode IR<9:11> */

                            case 1:                                     /* CDF */
                                DF = (IR & 0070) << 9;
                                break;

                            case 2:                                     /* CIF */
                                IB = (IR & 0070) << 9;
                                int_req = int_req & ~INT_NO_CIF_PENDING;
                                break;

                            case 3:                                     /* CDF CIF */
                                DF = IB = (IR & 0070) << 9;
                                int_req = int_req & ~INT_NO_CIF_PENDING;
                                break;

                            case 4:
                                switch (device & 07) {                  /* decode IR<6:8> */

                                    case 0:                             /* CINT */
                                        int_req = int_req & ~INT_UF;
                                        break;

                                    case 1:                                 /* RDF */
                                        LAC = LAC() | (DF() >> 9);
                                        break;

                                    case 2:                                 /* RIF */
                                        LAC = LAC() | (IF() >> 9);
                                        break;

                                    case 3:                                 /* RIB */
                                        LAC = LAC() | SF;
                                        break;

                                    case 4:                                 /* RMF */
                                        UB = (SF & 0100) >> 6;
                                        IB = (SF & 0070) << 9;
                                        DF = (SF & 0007) << 12;
                                        int_req = int_req & ~INT_NO_CIF_PENDING;
                                        break;

                                    case 5:                                 /* SINT */
                                        if (int_req & INT_UF)
                                            ++PC;
                                        break;

                                    case 6:                                 /* CUF */
                                        UB = 0;
                                        int_req = int_req & ~INT_NO_CIF_PENDING;
                                        break;

                                    case 7:                                 /* SUF */
                                        UB = 1;
                                        int_req = int_req & ~INT_NO_CIF_PENDING;
                                        break;
                                    default:
                                        break;
                                }                                   /* end switch device */
                                break;

                            default:
                                reason = STOP_ILL_INS;
                                break;
                        }                                           /* end switch pulse */
                        break;                                      /* end case 20-27 */

                    case 010:                                       /* power fail */
                        switch (pulse) {                            /* decode IR<9:11> */

                            case 1:                                     /* SBE */
                                break;

                            case 2:                                     /* SPL */
                                if (int_req & INT_PWR)
                                    ++PC;
                                break;

                            case 3:                                     /* CAL */
                                int_req = int_req & ~INT_PWR;
                                break;

                            default:
                                reason = STOP_ILL_INS;
                                break;
                        }                                       /* end switch pulse */
                        break;                                      /* end case 10 */

                    default:                                        /* I/O device */
                        auto dev = Chassis::instance()->find(device);
                        if (dev != Chassis::instance()->end()) {                      /* dev present? */
                            iot_data = dev->second->dispatch(IR(), iot_data);
                            AC = iot_data;
                            if (iot_data & IOT_SKP)
                                ++PC;
                            if (iot_data >= IOT_REASON)
                                reason = STOP_IOT_REASON; //reason = iot_data >> IOT_V_REASON;
                        }
                        else reason = STOP_ILL_INS;                    /* stop on flag */
                        break;
                }                                           /* end switch device */
                /* end case IOT */
            }                                               /* end switch opcode */
        } else {                                // OPR


            /* Opcode 7, OPR group 1 */
            cycleTime = 1500;

            switch ((IR >> 7) & 037) {
                case 034:case 035:                                  /* OPR, group 1 */
                    switch ((IR >> 4) & 017) {                      /* decode IR<4:7> */
                        case 0:                                         /* nop */
                            break;
                        case 1:                                         /* CML */
                            L = L ^ 1;
                            break;
                        case 2:                                         /* CMA */
                            AC = AC ^ 07777;
                            break;
                        case 3:                                         /* CMA CML */
                            LAC = LAC ^ 017777;
                            break;
                        case 4:                                         /* CLL */
                            L = 0;
                            break;
                        case 5:                                         /* CLL CML = STL */
                            L = 1;
                            break;
                        case 6:                                         /* CLL CMA */
                            L = 0;
                            AC = AC ^ 07777;
                            break;
                        case 7:                                         /* CLL CMA CML */
                            L = 1;
                            AC = AC ^ 07777;
                            break;
                        case 010:                                       /* CLA */
                            AC = 0;
                            break;
                        case 011:                                       /* CLA CML */
                            L = L ^ 1;
                            AC = 0;
                            break;
                        case 012:                                       /* CLA CMA = STA */
                            AC = 07777;
                            break;
                        case 013:                                       /* CLA CMA CML */
                            L = L ^ 1;
                            AC = 07777;
                            break;
                        case 014:                                       /* CLA CLL */
                            LAC = 0;
                            break;
                        case 015:                                       /* CLA CLL CML */
                            L = 1;
                            AC = 0;
                            break;
                        case 016:                                       /* CLA CLL CMA */
                            L = 0;
                            AC = 07777;
                            break;
                        case 017:                                       /* CLA CLL CMA CML */
                            LAC = 017777;
                            break;
                        default:
                            break;
                    }                                           /* end switch opers */

                    if (IR & 01)                                    /* IAC */
                        ++AC;
                    switch ((IR >> 1) & 07) {                       /* decode IR<8:10> */
                        case 0:                                         /* nop */
                            break;
                        case 1:                                         /* BSW */
                            AC = ((AC >> 6) & 077) | ((AC & 077) << 6);
                            break;
                        case 2:                                         /* RAL */
                            LAC = (LAC << 1) | L;
                            break;
                        case 3:                                         /* RTL */
                            LAC = (LAC << 1) | L;
                            LAC = (LAC << 1) | L;
                            break;
                        case 4:                                         /* RAR */
                            LAC = (LAC >> 1) | (LAC << 12);
                            break;
                        case 5:                                         /* RTR */
                            LAC = (LAC >> 1) | (LAC << 12);
                            LAC = (LAC >> 1) | (LAC << 12);
                            break;
                        case 6:                                         /* RAL RAR - undef */
                            AC = AC & IR;                             /* uses AND path */
                            break;
                        case 7:                                         /* RTL RTR - undef */
                            AC = (MA & 07600) | (IR & 0177);
                            break;                                      /* uses address path */
                        default:
                            break;
                    }                                           /* end switch shifts */
                    break;                                          /* end group 1 */

                    /* OPR group 2.  From Bernhard Baehr's description of the TSC8-75:
                       (In user mode) HLT (7402), OSR (7404) and microprogrammed combinations with
                       HLT and OSR: Additional to raising a user mode interrupt, the current OPR
                       opcode is moved to the ERIOT register and the ECDF flag is cleared. */

                case 036:case 037:                                  /* OPR, groups 2, 3 */
                    if ((IR & 01) == 0) {                           /* group 2 */
                        switch ((IR >> 3) & 017) {                  /* decode IR<6:8> */
                            case 0:                                     /* nop */
                                break;
                            case 1:                                     /* SKP */
                                ++PC;
                                break;
                            case 2:                                     /* SNL */
                                if (L)
                                    ++PC;
                                break;
                            case 3:                                     /* SZL */
                                if (L == 0)
                                    ++PC;
                                break;
                            case 4:                                     /* SZA */
                                if (AC == 0)
                                    ++PC;
                                break;
                            case 5:                                     /* SNA */
                                if (AC)
                                    ++PC;
                                break;
                            case 6:                                     /* SZA | SNL */
                                if ((AC == 0) || (L == 1))
                                    ++PC;
                                break;
                            case 7:                                     /* SNA & SZL */
                                if ((AC != 0) && (L == 0))
                                    ++PC;
                                break;
                            case 010:                                   /* SMA */
                                if ((AC & 04000) != 0)
                                    ++PC;
                                break;
                            case 011:                                   /* SPA */
                                if ((AC & 04000) == 0)
                                    ++PC;
                                break;
                            case 012:                                   /* SMA | SNL */
                                if (L == 1 || (AC & 04000))
                                    ++PC;
                                break;
                            case 013:                                   /* SPA & SZL */
                                if (L == 0 || (AC & 04000) == 0)
                                    ++PC;
                                break;
                            case 014:                                   /* SMA | SZA */
                                if (((LAC & 04000) != 0) || ((LAC & 07777) == 0))
                                    ++PC;
                                break;
                            case 015:                                   /* SPA & SNA */
                                if (((LAC & 04000) == 0) && ((LAC & 07777) != 0))
                                    ++PC;
                                break;
                            case 016:                                   /* SMA | SZA | SNL */
                                if ((LAC >= 04000) || (LAC == 0))
                                    ++PC;
                                break;
                            case 017:                                   /* SPA & SNA & SZL */
                                if ((LAC < 04000) && (LAC != 0))
                                    ++PC;
                                break;
                            default:
                                break;
                        }                                       /* end switch skips */
                        if (IR & 0200)                              /* CLA */
                            LAC = LAC & 010000;
                        if ((IR & 06) && UF) {                      /* user mode? */
                            int_req = int_req | INT_UF;             /* request intr */
                            tsc_ir = IR;                          /* save instruction */
                            tsc_cdf = 0;                            /* clear flag */
                        }
                        else {
                            if (IR & 04)                            /* OSR */
                                AC = AC | OSR;
                            if (IR & 02)                            /* HLT */
                                reason = STOP_HLT;
                        }
                        break;
                    }                                           /* end if group 2 */

                    /* OPR group 3 standard
                       MQA!MQL exchanges AC and MQ, as follows:
                            temp = MQ;
                            MQ = LAC & 07777;
                            LAC = LAC & 010000 | temp;
                    */

                    temp = MQ();                                      /* group 3 */
                    if (IR & 0200)                                  /* CLA */
                        LAC = LAC & 010000;
                    if (IR & 0020) {                                /* MQL */
                        MQ = LAC & 07777;
                        LAC = LAC & 010000;
                    }
                    if (IR & 0100)                                  /* MQA */
                        LAC = LAC | temp;
#ifdef UNIT_NOEAE
                if ((IR & 0056) && (cpu_unit.flags & UNIT_NOEAE)) {
                            reason = STOP_ILL_INS;                         /* EAE not present */
                            break;
                            }
#endif //UNIT_NOEAE

                    /* OPR group 3 EAE
                       The EAE operates in two modes:
                            Mode A, PDP-8/I compatible
                            Mode B, extended capability
                       Mode B provides eight additional subfunctions; in addition, some
                       of the Mode A functions operate differently in Mode B.
                       The mode switch instructions are decoded explicitly and cannot be
                       microprogrammed with other EAE functions (SWAB performs an MQL as
                       part of standard group 3 decoding).  If mode switching is decoded,
                       all other EAE timing is suppressed.
                    */

                    if (IR == 07431) {                              /* SWAB */
                        emode = 1;                                  /* set mode flag */
                        break;
                    }
                    if (IR == 07447) {                              /* SWBA */
                        emode = gtf = 0;                            /* clear mode, gtf */
                        break;
                    }

                    /* If not switching modes, the EAE operation is determined by the mode
                       and IR<6,8:10>:
                       <6:10>       mode A          mode B          comments
                       0x000        NOP             NOP
                       0x001        SCL             ACS
                       0x010        MUY             MUY             if mode B, next = address
                       0x011        DVI             DVI             if mode B, next = address
                       0x100        NMI             NMI             if mode B, clear AC if
                                                                     result = 4000'0000
                       0x101        SHL             SHL             if mode A, extra shift
                       0x110        ASR             ASR             if mode A, extra shift
                       0x111        LSR             LSR             if mode A, extra shift
                       1x000        SCA             SCA
                       1x001        SCA + SCL       DAD
                       1x010        SCA + MUY       DST
                       1x011        SCA + DVI       SWBA            NOP if not detected earlier
                       1x100        SCA + NMI       DPSZ
                       1x101        SCA + SHL       DPIC            must be combined with MQA!MQL
                       1x110        SCA + ASR       DCM             must be combined with MQA!MQL
                       1x111        SCA + LSR       SAM
                       EAE instructions which fetch memory operands use the CPU's DEFER
                       state to read the first word; if the address operand is in locations
                       x0010 - x0017, it is autoincremented.
                    */

                    if (emode == 0)                                 /* mode A? clr gtf */
                        gtf = 0;
                    switch ((IR >> 1) & 027) {                      /* decode IR<6,8:10> */

                        case 020:                                       /* mode A, B: SCA */
                            LAC = LAC | SC;
                            break;
                        case 000:                                       /* mode A, B: NOP */
                            break;

                        case 021:                                       /* mode B: DAD */
                            if (emode) {
                                MA_W = PC;
                                MA_F = IF;
                                if ((MA & 07770) != 00010) {             /* indirect; autoinc? */
                                    MA_W = M[MA];
                                    MA_F = DF;
                                } else {
                                    MA_W = M[MA] = M[MA] + 1;
                                    MA_F = DF;
                                } /* incr before use */
                                MQ = MQ + M[MA];
                                ++MA_W;
                                MA_F = DF;
                                LAC = AC + M[MA] + (MQ >> 12);
                                MQ = MQ & 07777;
                                ++PC;
                                break;
                            }
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 001:                                       /* mode B: ACS */
                            if (emode) {
                                SC = LAC & 037;
                                LAC = LAC & 010000;
                            }
                            else {                                      /* mode A: SCL */
                                MA_W = PC;
                                MA_F = IF;
                                SC = (~M[MA]()) & 037;
                                ++PC;
                            }
                            break;

                        case 022:                                       /* mode B: DST */
                            if (emode) {
                                MA_W = PC;
                                MA_F = IF;
                                if ((MA & 07770) != 00010) {            /* indirect; autoinc? */
                                    MA_W = M[MA];
                                    MA_F = DF;
                                } else {
                                    MA_W = M[MA] = M[MA] + 1;
                                    MA_F = DF;
                                } /* incr before use */
                                M[MA] = MQ;
                                ++MA_W;
                                MA_F = DF;
                                M[MA] = AC;
                                ++PC;
                                break;
                            }
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 002:                                       /* MUY */
                            MA_W = PC;
                            MA_F = IF;
                            if (emode) {                                /* mode B: defer */
                                if ((MA & 07770) != 00010) {            /* indirect; autoinc? */
                                    MA_W = M[MA];
                                    MA_F = DF;
                                } else {
                                    MA_W = M[MA] = M[MA] + 1;
                                    MA_F = DF;
                                } /* incr before use */
                            }
                            temp = (MQ * M[MA]) + AC;
                            AC = (temp >> 12) & 07777;
                            MQ = temp & 07777;
                            ++PC;
                            SC = 014;                                   /* 12 shifts */
                            break;

                        case 023:                                       /* mode B: SWBA */
                            if (emode)
                                break;
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 003:                                       /* DVI */
                            MA_W = PC;
                            MA_F = IF;
                            if (emode) {                                /* mode B: defer */
                                if ((MA & 07770) != 00010) {            /* indirect; autoinc? */
                                    MA_W = M[MA];
                                    MA_F = DF;
                                } else {
                                    MA_W = M[MA] = M[MA] + 1;
                                    MA_F = DF;
                                } /* incr before use */
                            }
                            if (AC >= M[MA]) {               /* overflow? */
                                L = 1;                     /* set link */
                                MQ = ((MQ << 1) + 1) & 07777;           /* rotate MQ */
                                SC = 0;                                 /* no shifts */
                            }
                            else {
                                temp = (AC << 12) | MQ;
                                MQ = temp / M[MA];
                                LAC = temp % M[MA];
                                SC = 015;                               /* 13 shifts */
                            }
                            ++PC;
                            break;

                        case 024:                                       /* mode B: DPSZ */
                            if (emode) {
                                if ((AC | MQ) == 0)
                                    ++PC;
                                break;
                            }
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 004:                                       /* NMI */
                            temp = (LAC << 12) | MQ;                    /* preserve link */
                            for (SC = 0; ((temp & 017777777) != 0) &&
                                         (temp & 040000000) == ((temp << 1) & 040000000); ++SC)
                                temp = temp << 1;
                            LAC = (temp >> 12) & 017777;
                            MQ = temp & 07777;
                            if (emode && (AC == 04000) && (MQ == 0))
                                AC = 0;                     /* clr if 4000'0000 */
                            break;

                        case 025:                                       /* mode B: DPIC */
                            if (emode) {
                                temp = (LAC + 1) & 07777;               /* SWP already done! */
                                LAC = MQ + (temp == 0);
                                MQ = temp;
                                break;
                            }
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 5:                                         /* SHL */
                            MA_W = PC;
                            MA_F = IF;
                            SC = (M[MA] & 037) + (emode ^ 1);      /* shift+1 if mode A */
                            if (SC > 25)                                /* >25? result = 0 */
                                temp = 0;
                            else temp = ((LAC << 12) | MQ) << SC;       /* <=25? shift LAC:MQ */
                            LAC = (temp >> 12) & 017777;
                            MQ = temp & 07777;
                            ++PC;
                            SC = emode? 037: 0;                         /* SC = 0 if mode A */
                            break;

                        case 026:                                       /* mode B: DCM */
                            if (emode) {
                                temp = (-LAC) & 07777;                  /* SWP already done! */
                                LAC = (MQ ^ 07777) + (temp == 0);
                                MQ = temp;
                                break;
                            }
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 6:                                         /* ASR */
                            MA_W = PC;
                            MA_F = IF;
                            SC = (M[MA] & 037) + (emode ^ 1);      /* shift+1 if mode A */
                            temp = (AC << 12) | MQ;          /* sext from AC0 */
                            if (LAC & 04000)
                                temp = temp | ~037777777;
                            if (emode && (SC != 0))
                                gtf = (temp >> (SC - 1)) & 1;
                            if (SC() > 25)
                                temp = (LAC & 04000)? -1: 0;
                            else temp = temp >> SC;
                            LAC = (temp >> 12) & 017777;
                            MQ = temp & 07777;
                            ++PC;
                            SC = emode? 037: 0;                         /* SC = 0 if mode A */
                            break;

                        case 027:                                       /* mode B: SAM */
                            if (emode) {
                                temp = LAC & 07777;
                                LAC = MQ + (temp ^ 07777) + 1;          /* L'AC = MQ - AC */
                                gtf = (temp <= MQ) ^ ((temp ^ MQ) >> 11);
                                break;
                            }
                            LAC = LAC | SC;                             /* mode A: SCA then */
                            // no break
                        case 7:                                         /* LSR */
                            MA_W = PC;
                            MA_F = IF;
                            SC = (M[MA] & 037) + (emode ^ 1);      /* shift+1 if mode A */
                            temp = (AC << 12) | MQ;          /* clear link */
                            if (emode && (SC != 0))
                                gtf = (temp >> (SC - 1)) & 1;
                            if (SC > 24)                                /* >24? result = 0 */
                                temp = 0;
                            else temp = temp >> SC;                     /* <=24? shift AC:MQ */
                            LAC = (temp >> 12) & 07777;
                            MQ = temp & 07777;
                            ++PC;
                            SC = emode? 037: 0;                         /* SC = 0 if mode A */
                            break;
                        default:
                            break;
                    }                                           /* end switch */
                    break;                                          /* end case 7 */
                default:
                    break;
            }
        }
        // End of the Execute state
        // TODO: This is also where idle detection will pause the CPU
        if (cpuStepping == SingleStep || cpuStepping == SingleInstruction) {
            cpuCondition = CPUStopped;
            throttleTimerReset = waitOnCondition();
        }

        cpuState = NoState;

        // TODO: return the number of nanoseconds the cycle should have taken on the real machine.
        return cycleTime;
    }

    /*
     * CPU waits until releaseOnCondition is called
     */
    bool CPU::waitOnCondition() {
        if (pthread_mutex_lock( &mutexCondition ) == 0) {
            pthread_cond_wait( &condition, &mutexCondition );
            pthread_mutex_unlock( &mutexCondition );
        }

        return true;
    }

    void CPU::cpuContinueFromIdle() {
        if (cpuStepping == NotStepping && (cpuCondition == CPUIdle || cpuCondition == CPURunning)) {
            cpuContinue();
        }
    }

    void CPU::cpuContinue() {
        if (pthread_mutex_lock( &mutexCondition ) == 0) {
            pthread_cond_signal( &condition );
            pthread_mutex_unlock( &mutexCondition );
        }
    }

    void CPU::timerTick() {
        try {
            Lock	lock(timerTickMutex);
            timerTickFlag = true;
        } catch ( LockException &le ) {
            //Console::instance()->printf(le.what());
        }
    }

    bool CPU::testTick( bool clear ) {

        bool r = timerTickFlag;
        if (clear)
            timerTickFlag = false;
        return r;
    }

} /* namespace pdp8 */
