/*
 * Console.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "Device.h"

namespace ca
{
    namespace pdp8
    {

        class Console: public Device
        {
        public:
            Console();
            virtual ~Console();
			
			static Console * instance();
			
			int printf( const char *format, ... );
			
		protected:
			static Console * _instance;
			
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CONSOLE_H_ */
