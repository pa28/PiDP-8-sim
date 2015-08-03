/*
 * VirtualPanel.cpp
 *
 *  Created on: Aug 2, 2015
 *      Author: richard
 */

#include <ncurses.h>
#include "PDP8.h"
#include "VirtualPanel.h"
#include "Chassis.h"

namespace ca
{
    namespace pdp8
    {

        VirtualPanel::VirtualPanel() :
                vPanel(NULL),
                console(NULL),
                command(NULL),
                consoleMode(CommandMode),
                M(*(Memory::instance())),
                cpu(*(CPU::instance()))

        {
			vPanel=subwin(stdscr,4,80,0,0);
			scrollok(vPanel,true);
			wbkgd(vPanel,COLOR_PAIR(1));
			wrefresh(vPanel);

			console = subwin(stdscr, 20, 80, 5, 0);
			scrollok(console,true);
			wprintw(console,"sim>");
			wbkgd(console, COLOR_PAIR(2));
			wrefresh(console);
        }

        VirtualPanel::~VirtualPanel()
        {
			delwin(console);
    		delwin(vPanel);
        }

		int VirtualPanel::vconf( const char *format, va_list list ) {
			int n = vwprintw( console, format, list );
			wrefresh( console );
			return n;
		}

		int VirtualPanel::panelf( int y, int x, const char * format, ... ) {
			va_list args;
			va_start (args, format);
			wmove( vPanel, y, x );
			int n = vwprintw( vPanel, format, args );
			wrefresh( vPanel );
			va_end (args);
			return n;
		}

		void VirtualPanel::updatePanel(uint32_t sx[3]) {
		    wmove( vPanel, 1, 5 );
		    wprintw( vPanel, " Sw Reg    Df If  PC    MA    MB" );
		    wmove( vPanel, 2, 5 );
		    wprintw( vPanel, "%1o %1o %04o    %1o  %1o %04o  %04o  %04o",
	                ((sx[1] >> 9) & 07),
	                ((sx[1] >> 6) & 07),
	                sx[0] & 07777,
	                cpu.getDF(), cpu.getIF(), cpu.getPC(), M.MA(), M.MB()
                );
		    wrefresh( vPanel );
		}

        void VirtualPanel::processStdin() {
            int ch;

            while ((ch = getch()) > 0) {

            wprintw(console, "ch: %d\n", ch);
			refresh();
                switch (ch) {
                    case KEY_F(1):  // Set and clear virtual panel mode
                        wprintw(console, "F1\n");
						refresh();
                        break;
                    case KEY_F(2):  // Set and clear command mode
                        wprintw(console, "F2\n");
						refresh();
                        break;
                    case 'q':   // Set and clear command mode
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
        }

        void VirtualPanel::processPanelMode(int ch) {
            debug(5, "%d", ch);
        }

        void VirtualPanel::processCommandMode(int ch) {
            debug(5, "%d", ch);
        }

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
