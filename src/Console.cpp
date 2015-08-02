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

        Console::Console() :
                Device("CONS", "Console"),
				runConsole(false),
				stopMode(false),
				switchPipe(-1),
				stopCount(0),
				consoleMode(CommandMode),
				M(*(Memory::instance())),
				cpu(*(CPU::instance())),
				consoleTerm( 0, 1)
        {
        }

        Console::~Console()
        {
            // TODO Auto-generated destructor stub
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
			//int n = vfprintf (stdout, format, args);
            int n = consoleTerm.vprintw( format, args );
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

#ifdef USE_NCURSES
			// Setup ncurses
			initscr();
			raw();
			keypad(stdscr, TRUE);
			noecho();
#endif

			while (runConsole) {
					FD_ZERO(&rd_set);
					FD_ZERO(&wr_set);

					int	n = 0;

					//FD_SET(0, &rd_set); n = 0 + 1;

					if (switchPipe >= 0) {
						FD_SET(switchPipe, &rd_set);
						n = switchPipe + 1;
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
						if (FD_ISSET(0, &rd_set)) {
							// stdin
							processStdin();
						} else if (FD_ISSET(switchPipe, &rd_set)) {
							// Panel
							uint32_t switchstatus[SWITCHSTATUS_COUNT] = { 0 };
							int n = read(switchPipe, switchstatus, sizeof(switchstatus));
							if (n == sizeof(switchstatus)) {
								// handle switches
								for (int i = 0; i < SWITCHSTATUS_COUNT; ++i) {
									switchstatus[i] ^= 07777;
								}

								debug(1,"%d SR:%04o DF:%1o IF:%1o %02o SS:%1o\n", n,
									switchstatus[0],
									((switchstatus[1] >> 9) & 07),
									((switchstatus[1] >> 6) & 07),
									(switchstatus[2] >> 6) & 077,
									(switchstatus[2] >> 4) & 03);

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
											break;
										case PanelDeposit:
											M[cpu.getIF() | cpu.getPC()] = switchstatus[0];
											cpu.setPC( (cpu.getPC() + 1) & 077777 );
											cpu.setState(DepositState);
											break;
										case PanelExamine:
											switchstatus[0] = M[cpu.getIF() | cpu.getPC()];
											cpu.setPC( (cpu.getPC() + 1) & 077777 );
											cpu.setState(ExamineState);
											break;
										case PanelContinue:
											break;
										case PanelStop:
											break;
									}
								}
							}
						}
					} else {
					    if (stopMode) {
					        --stopCount;
					        if (stopCount < 0) {
					            Chassis::instance()->stop();
					        }
					    }
						// timeout
					}
			}
			return 0;
		}

		void Console::processStdin() {
			int	ch = getch();

			switch (ch) {
				case KEY_F(1):	// Set and clear virtual panel mode
					break;
				case KEY_F(2):	// Set and clear command mode
					break;
				default:
					switch (consoleMode) {
						case PanelMode:
							processPanelMode(ch);
							break;
						case CommandMode:
							processCommandMode(ch);
							break;
					}
			}
		}

		void Console::processPanelMode(int ch) {
			debug(5, "%d", ch);
		}

		void Console::processCommandMode(int ch) {
			debug(5, "%d", ch);
		}

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
