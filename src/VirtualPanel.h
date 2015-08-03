/*
 * VirtualPanel.h
 *
 *  Created on: Aug 2, 2015
 *      Author: richard
 */

#ifndef VIRTUALPANEL_H_
#define VIRTUALPANEL_H_

#include "Terminal.h"

namespace ca
{
    namespace pdp8
    {

        // User input mode to the console.
        enum ConsoleMode {
            PanelMode,
            CommandMode,
        };

        class VirtualPanel: public Terminal
        {
        public:
            VirtualPanel();
            virtual ~VirtualPanel();

            virtual void processStdin();
            virtual int vprintw(const char *fmt, va_list list) { return vwprintw(console, fmt, list ); wrefresh(console); }
            virtual int vmvprintw(int y, int x, const char *fmt, va_list list) { return vmvwprintw(console, y, x, fmt, list); wrefresh(console); }

        protected:
            WINDOW      *vPanel, *console, *command;
            ConsoleMode consoleMode;

            void processPanelMode(int);
            void processCommandMode(int);
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* VIRTUALPANEL_H_ */
