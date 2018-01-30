/*
 * Terminal.h
 *
 *  Created on: Jul 31, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_TERMINAL_H
#define PIDP_TERMINAL_H


#include <cstdio>
#include <cstdarg>
#include <ncurses.h>

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

        void start();
        void stop();

    protected:
        WINDOW	* mainwin;

    };

} /* namespace pdp8 */


#endif //PIDP_TERMINAL_H
