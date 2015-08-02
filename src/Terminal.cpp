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

        Terminal::Terminal()
        {
			initscr();			/* Start curses mode 		  */
			raw();
			noecho();
			keypad(stdscr,TRUE);
			scrollok(stdscr,TRUE);

			printw("%d x %d\n", LINES, COLS);
			refresh();
        }

        Terminal::~Terminal()
        {
			endwin();			/* End curses mode		  */
        }

        int Terminal::vwprintw(WINDOW *w, const char *fmt, va_list list) {
            int n = ::vwprintw(w, fmt, list);
			refresh();
            return n;
        }

        int Terminal::vmvwprintw(WINDOW *w, int y, int x, const char *fmt, va_list list) {
            wmove(w, y, x);
            int n = ::vwprintw(w, fmt, list);
			refresh();
            return n;
        }

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
