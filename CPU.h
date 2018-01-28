//
// Created by richard on 26/01/18.
//

#ifndef PIDP_8_SIM_CPU_H
#define PIDP_8_SIM_CPU_H

#include <cstdint>
#include "PDP8.h"
#include "Device.h"
#include "Thread.h"
#include "Registers.h"

using namespace hw_sim;

namespace pdp8
{
    extern int32_t int_enable, int_req, dev_done, stop_inst;

    /* IOT subroutine return codes */

    constexpr int32_t IOT_V_SKP =       12;                              /* skip */
    constexpr int32_t IOT_V_REASON =   13;                              /* reason */
    constexpr int32_t IOT_SKP =        (1 << IOT_V_SKP);
    constexpr int32_t IOT_REASON =     (1 << IOT_V_REASON);
    
    constexpr int32_t OP_KSF =         06031;                           /* for idle */
    constexpr int32_t OP_CLSC =        06133;                           /* for idle */
    
    constexpr int32_t INT_V_START =    0;                               /* enable start */
    constexpr int32_t INT_V_LPT =      (INT_V_START+0);                 /* line printer */
    constexpr int32_t INT_V_PTP =      (INT_V_START+1);                 /* tape punch */
    constexpr int32_t INT_V_PTR =      (INT_V_START+2);                 /* tape reader */
    constexpr int32_t INT_V_TTO =      (INT_V_START+3);                 /* terminal */
    constexpr int32_t INT_V_TTI =      (INT_V_START+4);                 /* keyboard */
    constexpr int32_t INT_V_CLK =      (INT_V_START+5);                 /* clock */
    constexpr int32_t INT_V_TTO1 =     (INT_V_START+6);                 /* tto1 */
    constexpr int32_t INT_V_TTO2 =     (INT_V_START+7);                 /* tto2 */
    constexpr int32_t INT_V_TTO3 =     (INT_V_START+8);                 /* tto3 */
    constexpr int32_t INT_V_TTO4 =     (INT_V_START+9);                 /* tto4 */
    constexpr int32_t INT_V_TTI1 =     (INT_V_START+10);                /* tti1 */
    constexpr int32_t INT_V_TTI2 =     (INT_V_START+11);                /* tti2 */
    constexpr int32_t INT_V_TTI3 =     (INT_V_START+12);                /* tti3 */
    constexpr int32_t INT_V_TTI4 =     (INT_V_START+13);                /* tti4 */
    constexpr int32_t INT_V_DIRECT =   (INT_V_START+14);                /* direct start */
    constexpr int32_t INT_V_RX =       (INT_V_DIRECT+0);                /* RX8E */
    constexpr int32_t INT_V_RK =       (INT_V_DIRECT+1);                /* RK8E */
    constexpr int32_t INT_V_RF =       (INT_V_DIRECT+2);                /* RF08 */
    constexpr int32_t INT_V_DF =       (INT_V_DIRECT+3);                /* DF32 */
    constexpr int32_t INT_V_MT =       (INT_V_DIRECT+4);                /* TM8E */
    constexpr int32_t INT_V_DTA =      (INT_V_DIRECT+5);                /* TC08 */
    constexpr int32_t INT_V_RL =       (INT_V_DIRECT+6);                /* RL8A */
    constexpr int32_t INT_V_CT =       (INT_V_DIRECT+7);                /* TA8E int */
    constexpr int32_t INT_V_PWR =      (INT_V_DIRECT+8);                /* power int */
    constexpr int32_t INT_V_UF =       (INT_V_DIRECT+9);                /* user int */
    constexpr int32_t INT_V_TSC =      (INT_V_DIRECT+10);               /* TSC8-75 int */
    constexpr int32_t INT_V_FPP =      (INT_V_DIRECT+11);               /* FPP8 */
    constexpr int32_t INT_V_OVHD =     (INT_V_DIRECT+12);               /* overhead start */
    constexpr int32_t INT_V_NO_ION_PENDING = (INT_V_OVHD+0);             /* ion pending */
    constexpr int32_t INT_V_NO_CIF_PENDING = (INT_V_OVHD+1);             /* cif pending */
    constexpr int32_t INT_V_ION =      (INT_V_OVHD+2);                  /* interrupts on */
    
    constexpr int32_t INT_LPT =        (1 << INT_V_LPT);
    constexpr int32_t INT_PTP =        (1 << INT_V_PTP);
    constexpr int32_t INT_PTR =        (1 << INT_V_PTR);
    constexpr int32_t INT_TTO =        (1 << INT_V_TTO);
    constexpr int32_t INT_TTI =        (1 << INT_V_TTI);
    constexpr int32_t INT_CLK =        (1 << INT_V_CLK);
    constexpr int32_t INT_TTO1 =       (1 << INT_V_TTO1);
    constexpr int32_t INT_TTO2 =       (1 << INT_V_TTO2);
    constexpr int32_t INT_TTO3 =       (1 << INT_V_TTO3);
    constexpr int32_t INT_TTO4 =       (1 << INT_V_TTO4);
    constexpr int32_t INT_TTI1 =       (1 << INT_V_TTI1);
    constexpr int32_t INT_TTI2 =       (1 << INT_V_TTI2);
    constexpr int32_t INT_TTI3 =       (1 << INT_V_TTI3);
    constexpr int32_t INT_TTI4 =       (1 << INT_V_TTI4);
    constexpr int32_t INT_RX =         (1 << INT_V_RX);
    constexpr int32_t INT_RK =         (1 << INT_V_RK);
    constexpr int32_t INT_RF =         (1 << INT_V_RF);
    constexpr int32_t INT_DF =         (1 << INT_V_DF);
    constexpr int32_t INT_MT =         (1 << INT_V_MT);
    constexpr int32_t INT_DTA =        (1 << INT_V_DTA);
    constexpr int32_t INT_RL =         (1 << INT_V_RL);
    constexpr int32_t INT_CT =         (1 << INT_V_CT);
    constexpr int32_t INT_PWR =        (1 << INT_V_PWR);
    constexpr int32_t INT_UF =         (1 << INT_V_UF);
    constexpr int32_t INT_TSC =        (1 << INT_V_TSC);
    constexpr int32_t INT_FPP =        (1 << INT_V_FPP);
    constexpr int32_t INT_NO_ION_PENDING = (1 << INT_V_NO_ION_PENDING);
    constexpr int32_t INT_NO_CIF_PENDING = (1 << INT_V_NO_CIF_PENDING);
    constexpr int32_t INT_ION =        (1 << INT_V_ION);
    constexpr int32_t INT_DEV_ENABLE = ((1 << INT_V_DIRECT) - 1);       /* devices w/enables */
    constexpr int32_t INT_ALL =        ((1 << INT_V_OVHD) - 1);         /* all interrupts */
    constexpr int32_t INT_INIT_ENABLE = (INT_TTI+INT_TTO+INT_PTR+INT_PTP+INT_LPT) | \
                        (INT_TTI1+INT_TTI2+INT_TTI3+INT_TTI4) | \
                        (INT_TTO1+INT_TTO2+INT_TTO3+INT_TTO4);
    constexpr int32_t INT_PENDING =    (INT_ION+INT_NO_CIF_PENDING+INT_NO_ION_PENDING);
#define               INT_UPDATE =     ((int_req & ~INT_DEV_ENABLE) | (dev_done & int_enable));

    enum CPUState {
        NoState,
        Fetch,
        Execute,
        Defer,
        WordCount,
        CurrentAddress,
        Break,
        DepositState,
        ExamineState,
        FetchExecute,
        FetchDeferExecute,
    };

    enum CPUCondition {
        CPURunning = 0,
        CPUIdle,
        CPUStopped,
        CPUMemoryBreak,
    };

    enum CPUStepping {
        NotStepping = 0,
        SingleInstruction,
        SingleStep,
        PanelCommand,
    };

    enum CpuStopReason {
        STOP_NO_REASON = 0,
        STOP_IDLE,
        STOP_ENDLESS_LOOP,
        STOP_ILL_INS,
        STOP_IOT_REASON,
        STOP_HLT,
    };

    using AddressRegister_t = RegisterType<8, 15, 0>;
    using WordRegister_t = RegisterType<8, 12, 0>;
    using LinkAccRegister_t = RegisterType<8, 13, 0>;
    using LinkRegister_t = RegisterType<8, 1, 12>;
    using FieldRegister_t = RegisterType<8, 3, 12>;
    using SCRegister_t = RegisterType<8, 5, 0>;
    using IntReqRegister_t = RegisterType<8, 1, INT_V_ION>;

#define CPU_REGISTERS \
        X(PC, PC, 8, 12, 0, 12 ) \
        X(MQ, MQ, 8, 12, 0, 12 ) \
        X(IR, IR, 8, 12, 0, 12 ) \
        X(IB, IB, 8, 12, 0, 12 ) \
        X(OSR, OSR, 8, 12, 0, 12 ) \
        X(LAC, LAC, 8, 13, 0, 13 ) \
        X(L, LAC, 8, 1, 12, 1 ) \
        X(AC, LAC, 8, 12, 0, 12 ) \
        X(DF, DF, 8, 3, 12, 3 ) \
        X(IF, IF, 8, 3, 12, 3 ) \
        X(SC, SC, 8, 5, 0, 5 ) \
        X(ION, int_req, 8, 1, INT_V_ION, 1 )

#define IDLE_DETECT_MASK    0x1
#define THROTTLE_MASK       0X2

#define CPU_MODIFIERS \
        X(IDLE, ModifierValue, cpuLoadControl, IDLE_DETECT_MASK, IDLE_DETECT_MASK ) \
        X(NOIDLE, ModifierValue, cpuLoadControl, 0, IDLE_DETECT_MASK ) \
        X(THROTTLE, ModifierValue, cpuLoadControl, THROTTLE_MASK, THROTTLE_MASK ) \
        X(NOTHROTTLE, ModifierValue, cpuLoadControl, 0, THROTTLE_MASK )

    class CPU: public Device, Thread
    {
        CPU();
        ~CPU();

    public:
        virtual void initialize() { start(); }
        virtual void reset() {}

        CPUState		getState() const { return cpuState; }
        CPUCondition	getCondition() const { return cpuCondition; }
        CPUStepping		getStepping() const { return cpuStepping; }

        void	setState(CPUState s) { cpuState = s; }
        void	setStepping(CPUStepping s) { cpuStepping = s; }
        void	setCondition(CPUCondition c) { cpuCondition = c; }

        void	cpuContinue();
        void    cpuContinueFromIdle();
        void	cpuStop() { cpuCondition = CPUStopped; }
        void	cpuMemoryBreak() { cpuCondition = CPUMemoryBreak; }
        void	timerTick();
        bool	testTick(bool clear = true);

        virtual int run();
        virtual void stop() { threadRunning = false; }

        ScalarRegister<register_base_t, WordRegister_t> PC;
        ScalarRegister<register_base_t, WordRegister_t> MQ;
        ScalarRegister<register_base_t, WordRegister_t> IR;
        ScalarRegister<register_base_t, WordRegister_t> IB;
        ScalarRegister<register_base_t, WordRegister_t> OSR;
        ScalarRegister<register_base_t, WordRegister_t> AC;
        ScalarRegister<register_base_t, LinkAccRegister_t> LAC;
        ScalarRegister<register_base_t, LinkRegister_t> L;
        ScalarRegister<register_base_t, FieldRegister_t> DF;
        ScalarRegister<register_base_t, FieldRegister_t> IF;
        ScalarRegister<register_base_t, SCRegister_t> SC;
        ScalarRegister<int32_t, IntReqRegister_t> ION;
        ScalarRegister<register_base_t, AddressRegister_t> MA;
        ScalarRegister<register_base_t, FieldRegister_t> MA_F;
        ScalarRegister<register_base_t, WordRegister_t > MA_W;

    protected:
        register_base_t rPC, rMQ, rIR, rIB, rOSR, rLAC, rDF, rIF, rSC, rMA;
        CPUState	    cpuState;
        CPUCondition	cpuCondition;
        CPUStepping		cpuStepping;
        bool		    threadRunning;
        CpuStopReason   reason;
        bool			timerTickFlag;
        Mutex			timerTickMutex;
        Mutex           idleMutex;

        bool throttleTimerReset;

        pthread_mutex_t     mutexCondition;
        pthread_cond_t      condition;

        long cycleCpu();

        bool waitOnCondition();
        void releaseOnCondition();

        void reset_all(int i) { /* TODO: call chassis reset all */ }

    };

} /* namespace pdp8 */

#endif //PIDP_8_SIM_CPU_H
