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

        class CPU: public Device, Thread
        {
            CPU();

		public:
            virtual ~CPU();

			static CPU * instance();
			virtual void initialize() { start(); }
			virtual void reset() {}

			uint16_t    getPC() const { return PC; }
			uint16_t    getIF() const { return IF; }
			uint16_t    getDF() const { return DF; }
			uint16_t    getLAC() const { return LAC & 017777; }
			uint16_t    getMQ() const { return MQ; }
			uint16_t    getSC() const { return SC; }
			uint16_t	getIR() const { return IR; }

			void    setPC(uint16_t v) { PC = v & 07777; }
            void    setIF(uint16_t v) { IF = (v & 07) << 12; }
            void    setDF(uint16_t v) { DF = (v & 07) << 12; }

			CPUState		getState() const { return cpuState; }
			CPUCondition	getCondition() const { return cpuCondition; }
			CPUStepping		getStepping() const { return cpuStepping; }

			void	setState(CPUState s) { cpuState = s; }
			void	setStepping(CPUStepping s) { cpuStepping = s; }

			void	cpuContinue();
			void	cpuStop() { cpuCondition = CPUStopped; }
			void	cpuMemoryBreak() { cpuCondition = CPUMemoryBreak; }

			virtual int run();
			virtual void stop() { threadRunning = false; }
			virtual bool waitCondition();

		protected:
			static CPU *    _instance;
			//Memory &        M;

			int32_t        PC, IF, DF, LAC, MQ, SC, IR, MA;
	        int32_t IB;                                           /* Instruction Buffer */
	        int32_t SF;                                           /* Save Field */
	        int32_t emode;                                        /* EAE mode */
	        int32_t gtf;                                          /* EAE gtf flag */
	        int32_t UB;                                           /* User mode Buffer */
	        int32_t UF;                                           /* User mode Flag */
	        int32_t OSR;                                          /* Switch Register */
	        int32_t tsc_ir;                                       /* TSC8-75 IR */
	        int32_t tsc_pc;                                       /* TSC8-75 PC */
	        int32_t tsc_cdf;                                      /* TSC8-75 CDF flag */
	        int32_t tsc_enb;                                      /* TSC8-75 enabled */
	        int32_t int_req;                                      /* intr requests */
	        int32_t dev_done;                                     /* dev done flags */
	        int32_t int_enable;                                   /* intr enables */


	        CPUState	    cpuState;
			CPUCondition	cpuCondition;
			CPUStepping		cpuStepping;
			bool		    threadRunning;
			CpuStopReason   reason;

			ConditionWait	runConditionWait;

			bool throttleTimerReset;

			long cycleCpu();

            void reset_all(int i) { /* TODO: call chassis reset all */ }

        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CPU_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
