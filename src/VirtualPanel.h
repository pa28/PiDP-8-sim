/*
 * VirtualPanel.h
 *
 *  Created on: Aug 2, 2015
 *      Author: richard
 */

#ifndef VIRTUALPANEL_H_
#define VIRTUALPANEL_H_

#include <stdint.h>
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
            enum VirtualPanelConstants {
                BufferSize = 256,
            };

            VirtualPanel();
            virtual ~VirtualPanel();

            virtual void processStdin();
			int vconf(const char * format, va_list args);

			int panelf( int y, int x, const char * format, ... );

			void updatePanel(uint32_t sx[3]);

        protected:
            WINDOW      *vPanel, *console, *command;
            ConsoleMode consoleMode;

            Memory  &M;
            CPU     &cpu;

			uint32_t	switches[3];

			char *      cmdBuffer;
			size_t      cmdBufSize, cmdCurLoc;

			void updatePanel();
			void updateSwitchRegister(uint32_t o);
            void processPanelMode(int);
            void processCommandMode(int);
            void updateCommandDisplay();
			void setCursorLocation();
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* VIRTUALPANEL_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
