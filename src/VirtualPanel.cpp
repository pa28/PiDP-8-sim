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
                consoleMode(CommandMode)
        {
			wclear(stdscr);

			printw("Create windows\n");
			refresh();

            vPanel = subwin(mainwin, 3, 20, 0, 0);
            console = subwin(mainwin, 20, 80, 3, 0);
            command = subwin(mainwin, 1, 80, 23, 0);

            scrollok(vPanel,false);
            scrollok(console,true);
            scrollok(command,false);

			wclear(vPanel);
			wclear(console);
			wclear(command); 

			wprintw(command, 0, 0, "sim>");
			wrefresh(command);
        }

        VirtualPanel::~VirtualPanel()
        {
            // TODO Auto-generated destructor stub
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
