/*
 * VirtualPanel.cpp
 *
 *  Created on: Aug 2, 2015
 *      Author: richard
 */

#include <ncurses.h>
#include <ctype.h>
#include <string.h>
#include "PDP8.h"
#include "VirtualPanel.h"
#include "Chassis.h"

using namespace std;

namespace ca
{
    namespace pdp8
    {

        VirtualPanel::VirtualPanel() :
                vPanel(NULL),
                console(NULL),
                command(NULL),
                consoleMode(PanelMode),
                M(*(Memory::instance())),
                cpu(*(CPU::instance())),
                cmdBuffer(),
                cmdCurLoc(string::npos)

        {
			switches[0] = 0;
			switches[1] = 0;
			switches[2] = 0;

			vPanel=subwin(stdscr,4,80,0,0);
			scrollok(vPanel,true);
			keypad(vPanel,true);
			wbkgd(vPanel,COLOR_PAIR(1));
			wrefresh(vPanel);

			console = subwin(stdscr, 20, 80, 5, 0);
			scrollok(console,true);
			keypad(console,true);
			wbkgd(console, COLOR_PAIR(2));
			wrefresh(console);

			command = subwin(stdscr, 1, 80, 26, 0);
			keypad(command, true);
			wbkgd(command, COLOR_PAIR(3));
			updateCommandDisplay();
            wrefresh(command);

			curs_set(0);
        }

        VirtualPanel::~VirtualPanel()
        {
			delwin(console);
    		delwin(vPanel);
        }

		int VirtualPanel::vconf( const char *format, va_list list ) {
			int n = vwprintw( console, format, list );
			wrefresh( console );
			setCursorLocation();
			return n;
		}

		int VirtualPanel::panelf( int y, int x, const char * format, ... ) {
			va_list args;
			va_start (args, format);
			wmove( vPanel, y, x );
			int n = vwprintw( vPanel, format, args );
			wrefresh( vPanel );
			va_end (args);
			setCursorLocation();
			return n;
		}

		void VirtualPanel::updatePanel(uint32_t sx[3]) {
			switches[0] = sx[0];
			switches[1] = sx[1];
			switches[2] = sx[2];
			updatePanel();
		}

		void VirtualPanel::updatePanel() {
		    wmove( vPanel, 1, 5 );
		    wprintw( vPanel, " Sw Reg    Df If  PC    MA    MB" );
		    wmove( vPanel, 2, 5 );
		    wprintw( vPanel, "%1o %1o %04o    %1o  %1o %04o  %04o  %04o",
	                ((switches[1] >> 9) & 07),
	                ((switches[1] >> 6) & 07),
	                switches[0] & 07777,
	                cpu.getDF(), cpu.getIF(), cpu.getPC(), M.MA(), M.MB()
                );
		    wrefresh( vPanel );
			setCursorLocation();
		}

        void VirtualPanel::processStdin() {
            int ch;

            //while ((ch = wgetch( consoleMode == PanelMode ? vPanel : console )) > 0) {
			while (( ch = getch()) > 0) {

//#define DEBUG_CHAR
#ifdef DEBUG_CHAR
				if (isgraph(ch)) {
					wprintw(console, "ch: %c\n", ch);
				} else {
            		wprintw(console, "ch: %04o\n", ch);
				}
				wrefresh(console);
#endif
                switch (ch) {
                    case KEY_F(1):  // Set and clear virtual panel mode
                        consoleMode = PanelMode;
                        wmove( vPanel, 2, 12);
                    break;
                    case KEY_F(2):  // Set and clear command mode
                        consoleMode = CommandMode;
                        wmove( vPanel, 2, 12);
                        break;
                    case KEY_F(3):   // Set and clear command mode
					case 'q':
                        wprintw(console, "quit\n");
						refresh();
                        Chassis::instance()->stop();
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
			setCursorLocation();
        }

		void VirtualPanel::setCursorLocation() {
			if (consoleMode == PanelMode) {
				curs_set(1);
				mvwprintw( vPanel, 2, 12, "");
				wrefresh(vPanel);
			} else {
				curs_set(1);
				if (cmdCurLoc == string::npos) {
			    	mvwprintw( command, 0, cmdBuffer.size() + 2, "");
				} else {
			    	mvwprintw( command, 0, cmdCurLoc + 2, "");
				}
			    wrefresh( command );
			}
		}

		void VirtualPanel::updateSwitchRegister(uint32_t o) {
			switches[1] = ((switches[1] & 0700) << 3) | ((switches[0] & 07000) >> 3);
			switches[0] = ((switches[0] << 3) & 07770) | (o & 07);
			updatePanel();
		}

        void VirtualPanel::processPanelMode(int ch) {
			uint32_t t;
			if (ch >= '0' && ch <= '7') {
				updateSwitchRegister( ch - '0' );
			} else {
				switch (ch) {
				case KEY_DC:
				case '.':
					switches[0] = 0;
					switches[1] = 0;
					updatePanel();
					break;
				case '*':	// Address Load
					cpu.setPC(switches[0]);
					cpu.setDF((switches[1] >> 9) & 07);
					cpu.setIF((switches[1] >> 6) & 07);
					switches[0] = switches[1] = 0;
					updatePanel();
					break;
				case 012:	// Enter deposit
					M[cpu.getIF() | cpu.getPC()] = switches[0];
					cpu.setPC( (cpu.getPC() + 1) & 07777 );
					cpu.setState(DepositState);
					switches[0] = switches[1] = 0;
					updatePanel();
					break;
				case '+':	// Examine
					t = M[cpu.getIF() | cpu.getPC()];
					cpu.setPC( (cpu.getPC() + 1) & 07777 );
					cpu.setState(ExamineState);
					switches[0] = switches[1] = 0;
					updatePanel();
					break;
				}
			}
        }

        void VirtualPanel::processCommandMode(int ch) {
			switch (ch) {
				case KEY_HOME:
					if (cmdBuffer.size() > 0) {
						cmdCurLoc = 0;
					}
					break;
				case KEY_END:
					cmdCurLoc = string::npos;
					break;
				case KEY_LEFT:
					if (cmdCurLoc == string::npos && cmdBuffer.size() > 1) {
						cmdCurLoc = cmdBuffer.size() - 1;
					} else if (cmdCurLoc > 0) {
						cmdCurLoc--;
					}
					break;
				case KEY_RIGHT:
					if (cmdCurLoc != string::npos) {
						cmdCurLoc++;
						if (cmdCurLoc >= cmdBuffer.size()) {
							cmdCurLoc = string::npos;
						}
					}
					break;
				case 012:
					wprintw( console, "> %s\n", cmdBuffer.c_str() );
					wrefresh( console );
					break;
				case 025:
					cmdBuffer.clear();
					cmdCurLoc = string::npos;
					break;
				case KEY_BACKSPACE:
					if (cmdCurLoc > 0 && (size_t)cmdCurLoc < cmdBuffer.size()) {
						cmdBuffer.erase( cmdCurLoc, 1 );
						cmdCurLoc--;
					} else {
						cmdBuffer.erase(cmdBuffer.end()-1);
					}
					break;
				default:
					if (isalnum(ch) || ispunct(ch) || (ch == ' ')) {
						if (cmdCurLoc == string::npos || (size_t)cmdCurLoc > cmdBuffer.size()) {
							cmdBuffer.append(1,ch);
						} else {
							cmdBuffer.insert(cmdCurLoc, 1, ch);
							cmdCurLoc++;
						}
					}
            }
            updateCommandDisplay();
        }

        void VirtualPanel::updateCommandDisplay() {
            werase( command );
            mvwprintw( command, 0, 0, "> %s", cmdBuffer.c_str() );
            wrefresh( command );
            setCursorLocation();
        }

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
