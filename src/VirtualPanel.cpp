/*
 * VirtualPanel.cpp
 *
 *  Created on: Aug 2, 2015
 *      Author: richard
 */

#include <ncurses.h>
#include "pdp8.h"
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
            vPanel = newwin(3, COLS, 0, 0);
            console = newwin(LINES-4, COLS, 3, 0);
            command = newwin(1, COLS, LINES-1, 0);

            raw();
            noecho();
            keypad(vPanel,TRUE);
            scrollok(vPanel,FALSE);

            keypad(console,TRUE);
            scrollok(console,TRUE);

            keypad(command,TRUE);
            scrollok(command,FALSE);

        }

        VirtualPanel::~VirtualPanel()
        {
            // TODO Auto-generated destructor stub
        }

        void VirtualPanel::processStdin() {
            int ch;

            while ((ch = getch()) > 0) {

            wprintw(console, "ch: %d\n", ch);
                switch (ch) {
                    case KEY_F(1):  // Set and clear virtual panel mode
                        wprintw(console, "F1\n");
                        break;
                    case KEY_F(2):  // Set and clear command mode
                        wprintw(console, "F2\n");
                        break;
                    case 'q':   // Set and clear command mode
                        wprintw(console, "quit\n");
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
