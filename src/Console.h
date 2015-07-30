/*
 * Console.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "Device.h"
#include "Panel.h"
#include "CPU.h"
#include "Memory.h"

namespace ca
{
    namespace pdp8
    {

        // User input mode to the console.
		enum ConsoleMode {
			PanelMode,
			CommandMode,
		};

        enum PanelCmdButton {
            PanelStart = 040,
            PanelLoadAdr = 020,
            PanelDeposit = 010,
            PanelExamine = 004,
            PanelContinue = 002,
            PanelStop = 001,
        };

        class Console: public Device
        {
            Console();

        public:
            virtual ~Console();

			static Console * instance();

			int printf( const char *format, ... );

			void initialize();

			virtual int run();
			void stop() { runConsole = false; }
			void setSwitchFd(int fd) { switchPipe = fd; }

		protected:
			static	Console * _instance;
			bool	runConsole;

			int	switchPipe;
			ConsoleMode	consoleMode;

			Memory  &M;
			CPU     &cpu;

			void processStdin();
			void processPanelMode(int);
			void processCommandMode(int);
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CONSOLE_H_ */
