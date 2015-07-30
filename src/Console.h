/*
 * Console.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "Device.h"
#include "Thread.h"
#include "Panel.h"

namespace ca
{
    namespace pdp8
    {
		
		enum ConsoleMode {
			PanelMode,
			CommandMode,
		}

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
			int	getSwitchFd() { return switchPipe[1]; }
			
		protected:
			static	Console * _instance;
			bool	runConsole;
			
			int	switchPipe[2];
			ConsoleMode	consoleMode;
			
			void processStdin();
			void processPanelMode(int);
			void processCommandMode(int);
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CONSOLE_H_ */
