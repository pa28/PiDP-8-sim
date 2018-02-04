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

        void * run() override {
            while (_runFlag) {
                char buf[2048];
                ssize_t n = ::recv(this->clientfd, buf, sizeof(buf), 0);
                if (n > 0) {
                    ssize_t m = ::send(this->clientfd, buf, n, 0);
                    if (m < n) {
                        return nullptr;
                    }
                }
            }
            return nullptr;
        }

        void stop() override {
            _runFlag = false;
        }

    };

    static char BusyMsg[] = "Console connection in use.";

    template <class Cmd>
    class CommandServer :
            public Server<Cmd>
    {
    public:
        explicit CommandServer(uint16_t port = 8000, in_addr_t listenAddress = INADDR_LOOPBACK) :
                Server<Cmd>{port, listenAddress}
        {}

        void * run() override {
            while (this->_runFlag) {
                int fd = this->accept();
                if (fd >= 0) {
                    auto c = this->begin();
                    while (c != this->end()) {
                        if ((*c)->threadComplete()) {
                            c = this->close(c);
                        } else {
                            ++c;
                        }
                    }

                    if (this->clients.size()) {
                        ::send((*c)->fd(), BusyMsg, sizeof(BusyMsg), 0);
                    } else {
                        c = this->begin();
                        while (c != this->end()) {
                            if ((*c)->fd() == fd) {
                                (*c)->start();
                                break;
                            }
                            ++c;
                        }
                    }
                }
            }
            return nullptr;
        }

        void stop() override {
            for (auto &&c: this->clients) {
                c->stop();
            }
            this->_runFlag = false;
        }

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
