/*
 * Chassis.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_CHASSIS_H
#define PIDP_CHASSIS_H


#include <csignal>
#include <map>
#include <memory>
#include <sys/time.h>

#include "Device.h"
#include "SignalHandler.h"
#include "Server.h"
#include "ConsoleAPI.h"
#include "Encoder.h"

using namespace util;

namespace hw_sim
{
    template <typename CharT>
    class CommandConnection :
            public Connection<CharT>,
            public Thread
    {
    public:
        explicit CommandConnection(int fd = -1) :
                Connection<CharT>{fd},
                Thread{}
        {}

        CommandConnection(int fd, struct sockaddr_in &addr, socklen_t &len) :
                Connection<CharT>{fd, addr, len},
                Thread{}
        {}

        void * run() override;

        void stop() override;

    };


    template <class Cmd>
    class CommandServer :
            public Server<Cmd>
    {
    public:
        explicit CommandServer(uint16_t port = 8000, in_addr_t listenAddress = INADDR_LOOPBACK) :
                Server<Cmd>{port, listenAddress}
        {}

        void * run() override;

        void stop() override;

    };

    class Chassis : public std::map<int32_t, std::shared_ptr<Device>>
    {
        Chassis();

    public:

        static 		Chassis * instance();

        void        start();
        void    	stop(bool halt=false);
        void		timerHandler();
        void		reset();
        void        initialize();

        void		setTimerFreq( bool f120 = true );

    protected:
        static      Chassis * _instance;
        int         timeoutCounter;
        bool        timerFreq;

        CommandServer<CommandConnection<char>> commandServer;

        util::SignalHandler<SIGALRM> sigalrm;
    };

} /* namespace hw_sim */


#endif //PIDP_CHASSIS_H
