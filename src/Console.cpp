/*
 * Console.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */


#include <stdio.h>
#include <stdarg.h>

#include "Console.h"

namespace ca
{
    namespace pdp8
    {
		Console * Console::_instance = NULL;

        Console::Console() :
                Device("CONS", "Console")
        {
            // TODO Auto-generated constructor stub

        }

        Console::~Console()
        {
            // TODO Auto-generated destructor stub
        }
		
		Console * Console::instance() {
			if (_instance == NULL) {
				_instance = new Console();
			}
			
			return _instance;
		}
		
		int Console::printf( const char * format, ... ) {
			va_list args;
			va_start (args, format);
			int n = vfprintf (stdout, format, args);
			va_end (args);
			return n;
		}

    } /* namespace pdp8 */
} /* namespace ca */
