/*
 * CPU.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#include "CPU.h"
#include <stdio.h>
#include <time.h>

namespace ca
{
    namespace pdp8
    {

		CPU * CPU::_instance = NULL;

        CPU::CPU() :
                Device("CPU", "Central Processing Unit"),
                PC(0), IF(0), DF(0), LAC(0), MQ(0), SC(0), IR(0),
				cpuState(NoState),
				cpuCondition(CPUStopped),
				cpuStepping(NotStepping),
				threadRunning(false),
				runConditionWait(this)
        {
            // TODO Auto-generated constructor stub

        }

        CPU::~CPU()
        {
            // TODO Auto-generated destructor stub
        }

		CPU * CPU::instance() {
			if (_instance == NULL) {
				_instance = new CPU();
			}

			return _instance;
		}

		int CPU::run() {
			bool throttleTimerReset = true;
			struct timespec throttleStart, throttleCheck;
			long	cpuTime = 0;
			threadRunning = true;

			while (threadRunning) {
				// If we are going to wait, reset throttling when we get going again.
				// Wait the thread if the condition is false (the CPU is not running).
				throttleTimerReset = runConditionWait.waitOnCondition();
				switch (cpuStepping) {
					case NotStepping:
					case SingleInstruction:
						cpuState = FetchExecute;
						break;
					case SingleStep:
						switch (cpuState) {
							case Fetch:			// Let the cpuCycle function decide what state to be in
							case Defer:
							case Execute:
								break;
							default:			// But if comming out of a non-running condition start with a fetch.
								cpuState = Fetch;
						}
				}
				cpuTime += cycleCpu();
				if (cpuStepping != NotStepping) {
					cpuCondition = CPUStopped;
				} else {
					if (throttleTimerReset) {
						cpuTime = 0;
						clock_gettime( CLOCK_MONOTONIC, &throttleStart );
						throttleTimerReset = false;
					} else {
						if (cpuTime > 1000000) {
							clock_gettime( CLOCK_MONOTONIC, &throttleCheck );

							time_t d_sec = throttleCheck.tv_sec - throttleStart.tv_sec;
							long d_nsec = (d_sec * 1000000000) + (throttleCheck.tv_nsec - throttleStart.tv_nsec);
							if ((cpuTime - d_nsec) > 1000000) {
								throttleCheck.tv_sec = 0;
								throttleCheck.tv_nsec = cpuTime - d_nsec;
								nanosleep( &throttleCheck, NULL );
							}
						}
					}
				}
			}

			return 0;
		}

		long CPU::cycleCpu() {
			return 1000;
		}

		void CPU::cpuContinue() {
			try {
				Lock	lock(runConditionWait);
				cpuCondition = CPURunning;
			} catch (LockException &le) {
				fprintf(stderr, le.what());
			}
			runConditionWait.releaseOnCondition();
		}

		bool CPU::waitCondition() {
			return cpuCondition == CPURunning;
		}

    } /* namespace pdp8 */
} /* namespace ca */
