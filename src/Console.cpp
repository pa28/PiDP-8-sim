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

using namespace std;

namespace ca
{
    namespace pdp8
    {
		Console * Console::_instance = NULL;

        Console::Console() :
                Device("CONS", "Console"),
                Thread(),
				runConsole(false),
				consoleMode(CommandMode)
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
			int n = vfprintf (stdout, format, args);
			va_end (args);
			return n;
		}

		void Console::initialize() {
			runConsole = true;
			start();
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

					debug(1, "switchPipe: %d\n", switchPipe);
					if (switchPipe >= 0) {
						FD_SET(switchPipe, &rd_set);
						n = switchPipe + 1;
					}

#ifdef HAS_PSELECT
					int s = pselect( n, &rd_set, NULL, NULL, NULL, NULL);
#else
                    int s = select( n, &rd_set, NULL, NULL, NULL);
#endif

					debug(1,"select: %d\n", s);

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
									(switchstatus[2] >> 6),
									(switchstatus[2] >> 4) & 03);
							}
						}
					} else {
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
