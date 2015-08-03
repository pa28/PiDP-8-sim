/*
 * Console.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */


#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

//#define DEBUG_LEVEL 5
#include "PDP8.h"

#include "Console.h"
#include "Chassis.h"

using namespace std;

namespace ca
{
    namespace pdp8
    {
		Console * Console::_instance = NULL;

        Console::Console() :
                Device("CONS", "Console"),
				runConsole(false),
				stopMode(false),
				switchPipe(-1),
				stopCount(0),
				M(*(Memory::instance())),
				cpu(*(CPU::instance())),
				consoleTerm(new VirtualPanel())
        {
        }

        Console::~Console()
        {
        }

		Console * Console::instance() {
			if (_instance == NULL) {
				_instance = new Console();
			}

			return _instance;
		}

		int Console::printf( const char * format, ... ) {
			va_list args;
			va_start (args, format);
			int n = consoleTerm->vconf(format, args);
			va_end (args);
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

					FD_SET(consoleTerm->fdOfInput(), &rd_set); n = consoleTerm->fdOfInput() + 1;

					if (switchPipe >= 0) {
						FD_SET(switchPipe, &rd_set);
						n = max(n,switchPipe + 1);
					}

					int s;
#ifdef HAS_PSELECT
					if (stopMode) {
                        struct timespec timeout;

                        timeout.tv_sec = 1;
                        timeout.tv_nsec = 0;
                        s = pselect( n, &rd_set, NULL, NULL, &timeout, NULL);
					} else {
                        s = pselect( n, &rd_set, NULL, NULL, NULL, NULL);
					}
#else
                    if (stopMode) {
                        struct timeval timeout;

                        timeout.tv_sec = 1;
                        timeout.tv_usec = 0;
                        s = select( n, &rd_set, NULL, NULL, &timeout);
                    } else {
                        s = select( n, &rd_set, NULL, NULL, NULL);
                    }
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
						if (FD_ISSET(consoleTerm->fdOfInput(), &rd_set)) {
							consoleTerm->processStdin();
							/*
						if (FD_ISSET(0, &rd_set)) {
							int ch = getch();
							switch (ch) {
								case KEY_F(1):
									//wprintw(console, "F1\n");
									//wrefresh(console);
									break;
								default:
									Chassis::instance()->stop();
							}
							*/
						} else if (FD_ISSET(switchPipe, &rd_set)) {
							// Panel
						    int32_t    switchReport[2];

							int n = read(switchPipe, switchReport, sizeof(switchReport));
							if (n == sizeof(switchReport)) {
								// handle switches
							    switchReport[1] ^= 07777;

							    switch (switchReport[0]) {
							        case 0: // Switch register
                                        switchstatus[0] = switchReport[1];
							            debug(1, "SR: %04o\n", switchstatus[0]);
							            consoleTerm->updatePanel( switchstatus );
							            break;
							        case 1: // DF and IF
                                        switchstatus[1] = switchReport[1];
                                        debug(1, "DF: %1o  IF: %1o\n", ((switchstatus[1] >> 9) & 07), ((switchstatus[1] >> 6) & 07) );
                                        consoleTerm->updatePanel( switchstatus );
                                        break;
							        case 2: // Command switches
                                        switchstatus[2] = switchReport[1];
                                        //consoleTerm->updatePanel( switchstatus );
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
											break;
										case PanelLoadAdr:
											CPU::instance()->setPC(switchstatus[0]);
											CPU::instance()->setDF((switchstatus[1] >> 9) & 07);
											CPU::instance()->setIF((switchstatus[1] >> 6) & 07);
	                                        consoleTerm->updatePanel( switchstatus );
											break;
										case PanelDeposit:
											M[cpu.getIF() | cpu.getPC()] = switchstatus[0];
											cpu.setPC( (cpu.getPC() + 1) & 07777 );
											cpu.setState(DepositState);
	                                        consoleTerm->updatePanel( switchstatus );
											break;
										case PanelExamine:
											switchReport[1] = M[cpu.getIF() | cpu.getPC()];
											cpu.setPC( (cpu.getPC() + 1) & 07777 );
											cpu.setState(ExamineState);
	                                        consoleTerm->updatePanel( switchstatus );
											break;
										case PanelContinue:
										    cpu.cpuContinue();
											break;
										case PanelStop:
											break;
									}
								}
							}
						}
					} else {
						// timeout
					    if (stopMode) {
					        --stopCount;
					        if (stopCount < 0) {
					            Chassis::instance()->stop();
					        }
					    }
					}
//#endif

				//getch();
				//runConsole = false;
			}
			printf("Console exiting.\n");
			delete consoleTerm;
			return 0;
		}
    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
