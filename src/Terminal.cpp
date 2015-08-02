/*
 * Terminal.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: richard
 */

#include "Terminal.h"

namespace ca
{
    namespace pdp8
    {

        Terminal::Terminal( int _fd0, int _fd1 ) : fd0(_fd0), fd1(_fd1)
        {
            fin = fdopen( fd0, "r" );
            fout = fdopen( fd1, "w" );
            screen = newterm( NULL, fin, fout );
            def_shell_mode();
        }

        Terminal::~Terminal()
        {
            set_term(screen);
            reset_shell_mode();
        }

        int Terminal::wprintw(WINDOW *w, const char *fmt, ...) {
            set_term(screen);
            va_list args;
            va_start(args, fmt);
            int n = vwprintw(w, fmt, args);
            va_end(args);
            return n;
        }

        int Terminal::mvwprintw(WINDOW *w, int y, int x, const char *fmt, ...) {
            set_term(screen);
            va_list args;
            va_start(args, fmt);
            wmove(w, y, x);
            int n = vwprintw(w, fmt, args);
            va_end(args);
            return n;
        }

        int Terminal::printw(const char *fmt, ...) {
            set_term(screen);
            va_list args;
            va_start(args, fmt);
            int n = vwprintw(stdscr, fmt, args);
            va_end(args);
            return n;
        }

        int Terminal::mvprintw(int y, int x, const char *fmt, ...) {
            set_term(screen);
            va_list args;
            va_start(args, fmt);
            wmove(stdscr, y, x);
            int n = vwprintw(stdscr, fmt, args);
            va_end(args);
            return n;
        }

        int Terminal::vwprintw(WINDOW *w, const char *fmt, va_list list) {
            set_term(screen);
            int n = vwprintw(w, fmt, list);
            return n;
        }

        int Terminal::vmvwprintw(WINDOW *w, int y, int x, const char *fmt, va_list list) {
            set_term(screen);
            wmove(w, y, x);
            int n = vwprintw(w, fmt, list);
            return n;
        }

    } /* namespace pdp8 */
} /* namespace ca */
