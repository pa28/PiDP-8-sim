/*
 * Terminal.h
 *
 *  Created on: Jul 31, 2015
 *      Author: richard
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <stdio.h>
#include <stdarg.h>
#include <ncurses.h>

namespace ca
{
    namespace pdp8
    {

        class Terminal
        {
        public:
            Terminal();
            virtual ~Terminal();

			int vprintw( const char * format, va_list list );
            virtual int fdOfInput() { return fileno(stdin); }
            virtual int fdOfOutput() { return fileno(stdout); }

		protected:
			WINDOW	* mainwin;
			
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* TERMINAL_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
