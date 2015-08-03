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

            virtual int vprintw(const char *fmt, va_list list) { return vwprintw(stdscr, fmt, list ); }
            virtual int vmvprintw(int y, int x, const char *fmt, va_list list) { return vmvwprintw(stdscr, y, x, fmt, list); }
            int vwprintw(WINDOW *w, const char *fmt, va_list list);
            int vmvwprintw(WINDOW *w, int y, int x, const char *fmt, va_list list);

            virtual void processStdin() = 0;

            virtual int fdOfInput() { return fileno(stdin); }
            virtual int fdOfOutput() { return fileno(stdout); }

        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* TERMINAL_H_ */
/* vim: set ts=4 sw=4  noet autoindent : */
