/*
 * CPU.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>
#include "Device.h"
#include "Thread.h"

namespace ca
{
    namespace pdp8
    {

/* IOT subroutine return codes */

#define IOT_V_SKP       12                              /* skip */
#define IOT_V_REASON    13                              /* reason */
#define IOT_SKP         (1 << IOT_V_SKP)
#define IOT_REASON      (1 << IOT_V_REASON)

#define OP_KSF          06031                           /* for idle */
#define OP_CLSC         06133                           /* for idle */

#define INT_V_START     0                               /* enable start */
#define INT_V_LPT       (INT_V_START+0)                 /* line printer */
#define INT_V_PTP       (INT_V_START+1)                 /* tape punch */
#define INT_V_PTR       (INT_V_START+2)                 /* tape reader */
#define INT_V_TTO       (INT_V_START+3)                 /* terminal */
#define INT_V_TTI       (INT_V_START+4)                 /* keyboard */
#define INT_V_CLK       (INT_V_START+5)                 /* clock */
#define INT_V_TTO1      (INT_V_START+6)                 /* tto1 */
#define INT_V_TTO2      (INT_V_START+7)                 /* tto2 */
#define INT_V_TTO3      (INT_V_START+8)                 /* tto3 */
#define INT_V_TTO4      (INT_V_START+9)                 /* tto4 */
#define INT_V_TTI1      (INT_V_START+10)                /* tti1 */
#define INT_V_TTI2      (INT_V_START+11)                /* tti2 */
#define INT_V_TTI3      (INT_V_START+12)                /* tti3 */
#define INT_V_TTI4      (INT_V_START+13)                /* tti4 */
#define INT_V_DIRECT    (INT_V_START+14)                /* direct start */
#define INT_V_RX        (INT_V_DIRECT+0)                /* RX8E */
#define INT_V_RK        (INT_V_DIRECT+1)                /* RK8E */
#define INT_V_RF        (INT_V_DIRECT+2)                /* RF08 */
#define INT_V_DF        (INT_V_DIRECT+3)                /* DF32 */
#define INT_V_MT        (INT_V_DIRECT+4)                /* TM8E */
#define INT_V_DTA       (INT_V_DIRECT+5)                /* TC08 */
#define INT_V_RL        (INT_V_DIRECT+6)                /* RL8A */
#define INT_V_CT        (INT_V_DIRECT+7)                /* TA8E int */
#define INT_V_PWR       (INT_V_DIRECT+8)                /* power int */
#define INT_V_UF        (INT_V_DIRECT+9)                /* user int */
#define INT_V_TSC       (INT_V_DIRECT+10)               /* TSC8-75 int */
#define INT_V_FPP       (INT_V_DIRECT+11)               /* FPP8 */
#define INT_V_OVHD      (INT_V_DIRECT+12)               /* overhead start */
#define INT_V_NO_ION_PENDING (INT_V_OVHD+0)             /* ion pending */
#define INT_V_NO_CIF_PENDING (INT_V_OVHD+1)             /* cif pending */
#define INT_V_ION       (INT_V_OVHD+2)                  /* interrupts on */

#define INT_LPT         (1 << INT_V_LPT)
#define INT_PTP         (1 << INT_V_PTP)
#define INT_PTR         (1 << INT_V_PTR)
#define INT_TTO         (1 << INT_V_TTO)
#define INT_TTI         (1 << INT_V_TTI)
#define INT_CLK         (1 << INT_V_CLK)
#define INT_TTO1        (1 << INT_V_TTO1)
#define INT_TTO2        (1 << INT_V_TTO2)
#define INT_TTO3        (1 << INT_V_TTO3)
#define INT_TTO4        (1 << INT_V_TTO4)
#define INT_TTI1        (1 << INT_V_TTI1)
#define INT_TTI2        (1 << INT_V_TTI2)
#define INT_TTI3        (1 << INT_V_TTI3)
#define INT_TTI4        (1 << INT_V_TTI4)
#define INT_RX          (1 << INT_V_RX)
#define INT_RK          (1 << INT_V_RK)
#define INT_RF          (1 << INT_V_RF)
#define INT_DF          (1 << INT_V_DF)
#define INT_MT          (1 << INT_V_MT)
#define INT_DTA         (1 << INT_V_DTA)
#define INT_RL          (1 << INT_V_RL)
#define INT_CT          (1 << INT_V_CT)
#define INT_PWR         (1 << INT_V_PWR)
#define INT_UF          (1 << INT_V_UF)
#define INT_TSC         (1 << INT_V_TSC)
#define INT_FPP         (1 << INT_V_FPP)
#define INT_NO_ION_PENDING (1 << INT_V_NO_ION_PENDING)
#define INT_NO_CIF_PENDING (1 << INT_V_NO_CIF_PENDING)
#define INT_ION         (1 << INT_V_ION)
#define INT_DEV_ENABLE  ((1 << INT_V_DIRECT) - 1)       /* devices w/enables */
#define INT_ALL         ((1 << INT_V_OVHD) - 1)         /* all interrupts */
#define INT_INIT_ENABLE (INT_TTI+INT_TTO+INT_PTR+INT_PTP+INT_LPT) | \
                        (INT_TTI1+INT_TTI2+INT_TTI3+INT_TTI4) | \
                        (INT_TTO1+INT_TTO2+INT_TTO3+INT_TTO4)
#define INT_PENDING     (INT_ION+INT_NO_CIF_PENDING+INT_NO_ION_PENDING)
#define INT_UPDATE      ((int_req & ~INT_DEV_ENABLE) | (dev_done & int_enable))

        extern int32_t int_enable, int_req, dev_done, stop_inst;

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

		public:
            virtual ~CPU();

			static CPU * instance();
			virtual void initialize() { start(); }
			virtual void reset() {}

			#define X(nm,loc,r,w,o,d)	Index ## nm,
			enum RegisterIndex {
				CPU_REGISTERS
				RegisterCount,
			};
			#undef X
			//
			//
			#define X(nm,loc,r,w,o,d)	int32_t get ## nm (bool normal = false) const { return cpuRegisters[Index ## nm].get(normal); }
			CPU_REGISTERS
			#undef X

			#define X(nm,loc,r,w,o,d)	void set ## nm (int32_t v, bool normal = false) { cpuRegisters[Index ## nm].set(v, normal); }
			CPU_REGISTERS
			#undef X

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
			virtual bool waitCondition();

		protected:
			static CPU *    _instance;
			static Register cpuRegisters[];

			//Memory &        M;

	        CPUState	    cpuState;
			CPUCondition	cpuCondition;
			CPUStepping		cpuStepping;
			bool		    threadRunning;
			CpuStopReason   reason;
			bool			timerTickFlag;
			Mutex			timerTickMutex;
			Mutex           idleMutex;

			ConditionWait	runConditionWait;

			bool throttleTimerReset;

			long cycleCpu();
			void setReasonIdle();
			bool testReasonIdle();

            void reset_all(int i) { /* TODO: call chassis reset all */ }

        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CPU_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
