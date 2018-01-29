/*
 * Console.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_CONSOLE_H
#define PIDP_CONSOLE_H


#include "PDP8.h"
#include "Device.h"
#include "CPU.h"
#include "Memory.h"
#include "Thread.h"
#include "VirtualPanel.h"

namespace pdp8
{
    constexpr size_t SWITCHSTATUS_COUNT = 3;

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
    public:
        explicit Console(bool headless);

        virtual ~Console();

        int printf( const char *format, ... );

        virtual void initialize();
        virtual void reset();
        virtual void tick(int ticksPerSecond);
        virtual int run();
        virtual void stop(){ runConsole = false; }
        void setSwitchFd(int fd) { switchPipe = fd; }

        bool    getStopMode() const { return stopMode; }
        int     getStopCount() const { return stopCount; }
        void    oneSecond();
        void    update();

        static std::shared_ptr<Console> getConsole();

    protected:
        bool	runConsole;
        bool    stopMode;

        int	    switchPipe;
        int     stopCount;
        int     tickCount;

//        Memory  &M;
//        CPU     &cpu;

        VirtualPanel    *consoleTerm;
        pthread_mutex_t     accessMutex;

    };

} /* namespace pdp8 */


#endif //PIDP_CONSOLE_H
