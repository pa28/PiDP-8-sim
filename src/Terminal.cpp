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
			mainwin = initscr();
			noecho();
			//raw();
			timeout(0);
			keypad(stdscr,true);
			scrollok(stdscr,true);
			start_color();
			init_pair(1,COLOR_YELLOW,COLOR_BLUE);
			init_pair(2,COLOR_BLACK,COLOR_WHITE);
			init_pair(3,COLOR_BLUE,COLOR_WHITE); 
        }

        Terminal::~Terminal()
        {
			endwin();			/* End curses mode		  */
        }

		int Terminal::vprintw( const char * format, va_list list ) {
			int n = vwprintw( mainwin, format, list );
			wrefresh( mainwin );
			return n;
		}


    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
