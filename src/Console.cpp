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
				runConsole(false),
				consoleMode(CommandMode)
        {
			if (pipe(switchPipe)) {
				switchPipe[0] = -1;
				switchPipe[1] = -1;
			}
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
			Panel::instance()->setSwitchFd(switchPipe[1]);
		}
		
		int Console::run() {
			fd_set	rd_set, wr_set;
			
			// Setup ncurses
			initscr();
			raw();
			keypad(stdscr, TRUE);
			noecho();
			
			while (runConsole) {
					FD_CLEAR(&rd_set);
					FD_CLEAR(&wr_set);
					
					int	n = 0;
					
					FD_SET(0, &rd_set); n = 0 + 1;
					
					if (switchPipe[0] >= 0) {
						FD_SET(switchPipe[0], &rd_set);
						n = switchPipe[0] + 1;
					}
					
					int s = pselect( n, &rd_set, &wr_set, NULL, NULL, NULL );
					
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
						} else if (FD_ISSET(switchPipe[0], &rd_set)) {
							// Panel
							uint32 switchstatus[SWITCHSTATUS_COUNT] = { 0 };
							int n = read(switchPipe[0], sizeof(switchstatus));
							if (n == sizeof(switchstatus)) {
								// handle switches
							}
						}
					} else {
						// timeout
					}
			}
			return 0;
		}
		
		void Console::processStdin() {
			int	ch = getchr();
			
			switch (ch) {
				case KEY_F1:	// Set and clear virtual panel mode
					break;
				case KEY_F2:	// Set and clear command mode
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
