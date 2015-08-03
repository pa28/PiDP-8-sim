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
			int vconf(const char * format, va_list args);

			int panelf( int y, int x, const char * format, ... );

        protected:
            WINDOW      *vPanel, *console, *command;
            ConsoleMode consoleMode;

            void processPanelMode(int);
            void processCommandMode(int);
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* VIRTUALPANEL_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
