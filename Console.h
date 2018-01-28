/*
 * Console.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_CONSOLE_H
#define PIDP_CONSOLE_H


#include "Device.h"
#include "Panel.h"
#include "CPU.h"
#include "Memory.h"
#include "Thread.h"
#include "VirtualPanel.h"

namespace pdp8
{

    enum PanelCmdButton {
        PanelStart = 040,
        PanelLoadAdr = 020,
        PanelDeposit = 010,
        PanelExamine = 004,
        PanelContinue = 002,
        PanelStop = 001,
        PanelNoCmd = 0,
    };

    class Console: public Device
    {
        Console(bool headless);

    public:
        virtual ~Console();

        static Console * instance(bool headless = false);

        int printf( const char *format, ... );

        void initialize();
        void reset();

        virtual int run();
        void stop() { runConsole = false; }
        void setSwitchFd(int fd) { switchPipe = fd; }

        bool    getStopMode() const { return stopMode; }
        int     getStopCount() const { return stopCount; }
        void    oneSecond();

    protected:
        static	Console * _instance;
        bool	runConsole;
        bool    stopMode;

        int	    switchPipe;
        int     stopCount;

//        Memory  &M;
//        CPU     &cpu;

        VirtualPanel    *consoleTerm;
        pthread_mutex_t     mutex;

    };

} /* namespace pdp8 */


#endif //PIDP_CONSOLE_H
