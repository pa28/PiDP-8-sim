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
            Terminal( int fd0, int fd1);
            virtual ~Terminal();

            int wprintw(WINDOW *w, const char *fmt, ...);
            int mvwprintw(WINDOW *w, int y, int x, const char *fmt, ...);
            int printw(const char *fmt, ...);
            int mvprintw(int y, int x, const char *fmt, ...);
            int vprintw(const char *fmt, va_list list) { return vwprintw(stdscr, fmt, list ); }
            int vmvprintw(int y, int x, const char *fmt, va_list list) { return vmvwprintw(stdscr, y, x, fmt, list); }
            int vwprintw(WINDOW *w, const char *fmt, va_list list);
            int vmvwprintw(WINDOW *w, int y, int x, const char *fmt, va_list list);

            void setTerm() { set_term(screen); }

        protected:
            int     fd0, fd1;
            FILE    *fin, *fout;
            SCREEN  *screen;
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* TERMINAL_H_ */
