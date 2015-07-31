/*
 * CPU.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
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
		}
		
		enum CPUSteping {
			NotStepping = 0,
			SingleStep,
			SingleInstruction,
		}

        class CPU: public Device, Thread
        {
            CPU();

		public:
            virtual ~CPU();

			static CPU * instance();
			virtual void initialize() {}

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
			
			void	cpuContinue() { cpuCondition = CPURunning; runConditionWait.releaseOnCondition(); }
			void	cpuStop() { cpuCondition = CPUStopped; }
			void	cpuMemoryBreak() { cpuCondition = CPUMemoryBreak; }
			
			virtual int run();
			virtual void stop() { threadRunning = false; }
			virtual bool waitCondition();

		protected:
			static CPU * _instance;

			uint16_t    PC, IF, DF, LAC, MQ, SC, IR;
			CPUState	cpuState;
			CPUCondition	cpuCondition;
			CPUStepping		cpuStepping;
			bool		threadRunning;
			
			ConditionWait	runConditionWait;
			
			void cycleCpu();
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CPU_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
