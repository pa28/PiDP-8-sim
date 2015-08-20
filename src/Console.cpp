/*
 * Console.cpp
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


#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define DEBUG_LEVEL 5
#include "PDP8.h"

#include "Console.h"
#include "Chassis.h"

using namespace std;

namespace ca
{
    namespace pdp8
    {
		Console * Console::_instance = NULL;

        Console::Console(bool headless) :
                Device("CONS", "Console"),
				runConsole(false),
				stopMode(false),
				switchPipe(-1),
				stopCount(0),
				M(*(Memory::instance())),
				cpu(*(CPU::instance())),
				consoleTerm(NULL)
        {
            if (!headless) {
                consoleTerm = new VirtualPanel();
            }
            pthread_mutex_init( &mutex, NULL );
        }

        Console::~Console()
        {
            pthread_mutex_destroy( &mutex );
        }

		Console * Console::instance(bool headless) {
			if (_instance == NULL) {
				_instance = new Console(headless);
			}

			return _instance;
		}

		int Console::printf( const char * format, ... ) {
		    if (consoleTerm == NULL) {
		        return 0;
		    }

		    int n = -1;
		    try {
                va_list args;
                va_start (args, format);
                int n = consoleTerm->vconf(format, args);
                va_end (args);
		    } catch (LockException &le ) {
		        consoleTerm->printw( le.what() );
		    }
			return n;
		}

		void Console::initialize() {
			runConsole = true;
		}

		void Console::reset() {

		}

		int Console::run() {
			fd_set	rd_set, wr_set;

            uint32_t switchstatus[SWITCHSTATUS_COUNT] = { 0 };
			while (runConsole) {
					FD_ZERO(&rd_set);
					FD_ZERO(&wr_set);

					int	n = 0;

					if (consoleTerm != NULL) {
                        FD_SET(consoleTerm->fdOfInput(), &rd_set); n = consoleTerm->fdOfInput() + 1;
					}

                    if (switchPipe >= 0) {
                        FD_SET(switchPipe, &rd_set);
                        n = max(n,switchPipe + 1);
                    }

                    int s;

#ifdef HAS_PSELECT
                    s = pselect( n, &rd_set, NULL, NULL, NULL, NULL);
#else
                    s = select( n, &rd_set, NULL, NULL, NULL);
#endif

					if (s < 0) {
						switch(errno) {
						case EBADF:	// invalid FD
							break;
						case EINTR: // Signal was caught
							break;
						case EINVAL:	// invalid n or timeout
							break;
						case ENOMEM:	// no memory
							break;
						}
					} else if (s > 0) {
						// one or more fd ready
					    if (consoleTerm != NULL) {
                            if (FD_ISSET(consoleTerm->fdOfInput(), &rd_set)) {
                                consoleTerm->processStdin();
                            }
						} else if (FD_ISSET(switchPipe, &rd_set)) {
							// Panel
						    int    switchReport[2];

							int n = read(switchPipe, switchReport, sizeof(switchReport));
							if (n == sizeof(switchReport)) {
								// handle switches
							    switchReport[1] ^= 07777;

							    switch (switchReport[0]) {
							        case 0: // Switch register
                                        switchstatus[0] = switchReport[1];
							            debug(1, "SR: %04o\n", switchstatus[0]);
							            if (consoleTerm) consoleTerm->updatePanel( switchstatus );
										CPU::instance()->setOSR(switchstatus[0]);
							            break;
							        case 1: // DF and IF
                                        switchstatus[1] = switchReport[1];
                                        debug(1, "DF: %1o  IF: %1o\n", ((switchstatus[1] >> 9) & 07), ((switchstatus[1] >> 6) & 07) );
                                        if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                        break;
							        case 2: // Command switches
                                        switchstatus[2] = switchReport[1];
                                        debug(1, "Cmd: %02o  SS: %1o\n", (switchstatus[2] >> 6) & 077, (switchstatus[2] >> 4) & 03 );
                                        break;
							    }

								debug(1, "Sx: %04o %04o %04o\n", switchstatus[0],  switchstatus[2], switchstatus[2]);

								CPU::instance()->setStepping(static_cast<CPUStepping>((switchstatus[2] >> 4) & 03));

								if (CPU::instance()->getStepping() == PanelCommand) {
									switch ((switchstatus[2] >> 6) & 077) {
									    case PanelNoCmd:
											stopMode = false;
											stopCount = 8;
									        break;
										case PanelStart:
											break;
										case PanelLoadAdr:
											break;
										case PanelDeposit:
											break;
										case PanelExamine:
											break;
										case PanelContinue:
											break;
										case PanelStop:
										    stopMode = true;
										    stopCount = 8;
											break;
									}
								} else {
									switch ((switchstatus[2] >> 6) & 077) {
										case PanelStart:
											debug(1, "%s\n", "PanelStart");
											Chassis::instance()->reset();
											cpu.setCondition(CPURunning);
											cpu.cpuContinue();
											break;
										case PanelLoadAdr:
											CPU::instance()->setPC(switchstatus[0]);
											CPU::instance()->setDF((switchstatus[1] >> 9) & 07);
											CPU::instance()->setIF((switchstatus[1] >> 6) & 07);
											if (consoleTerm) consoleTerm->updatePanel( switchstatus );
											break;
										case PanelDeposit:
											M[cpu.getIF() | cpu.getPC()] = switchstatus[0];
											cpu.setPC( (cpu.getPC() + 1) & 07777 );
											cpu.setState(DepositState);
											if (consoleTerm) consoleTerm->updatePanel( switchstatus );
											break;
										case PanelExamine:
											switchReport[1] = M[cpu.getIF() | cpu.getPC()];
											cpu.setPC( (cpu.getPC() + 1) & 07777 );
											cpu.setState(ExamineState);
											if (consoleTerm) consoleTerm->updatePanel( switchstatus );
											break;
										case PanelContinue:
											debug(1, "%s\n", "PanelContinue");
											cpu.setCondition(CPURunning);
										    cpu.cpuContinue();
										    if (consoleTerm) consoleTerm->updatePanel( switchstatus );
											break;
										case PanelStop:
											debug(1, "%s\n", "PanelStop");
											cpu.cpuStop();
											break;
									}
								}
							}
						}
					}
//#endif

				//getch();
				//runConsole = false;
			}
			printf("Console exiting.\n");
			if (consoleTerm) delete consoleTerm;
			return 0;
		}

		void Console::oneSecond() {
            // timeout
            debug(10, "Timeout mode %d, count %d\n", stopMode, stopCount );
            if (stopMode) {
                --stopCount;
                if (stopCount < 0) {
                    Chassis::instance()->stop();
                }
            }
		}
    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
