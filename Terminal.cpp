/*
 * Terminal.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: H Richard Buckley
   Portions of this program are based substantially on work by Robert M Supnik
   The license for Mr Supnik's work follows:
   Copyright (c) 1993-2013, Robert M Supnik
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.
 */

#include "Terminal.h"

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
