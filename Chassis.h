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
                        break;
                    }
                } else {
                    break;
                }
            }
            return nullptr;
        }

        void stop() override {
            _runFlag = false;
        }

    };

    static char BusyMsg[] = "Console connection in use.\r\n";

    template <class Cmd>
    class CommandServer :
            public Server<Cmd>
    {
    public:
        explicit CommandServer(uint16_t port = 8000, const in6_addr listenAddress = in6addr_loopback) :
                Server<Cmd>{port, listenAddress}
        {}

        void * run() override {

            while (this->_runFlag) {

                this->clean_clients();

                int fd = this->select_accept();
                if (fd >= 0) {
                    if (this->clients.size() > 1) {
                        ::send(fd, BusyMsg, sizeof(BusyMsg), 0);
                        auto ci = this->find(fd);
                        (*ci)->stop();
                        this->close(ci);
                    } else {
                        auto ci = this->find(fd);
                        auto &&c = *ci;
                        c->start();
                    }
                } else if (fd < 0) {
                    switch (errno) {
                        case EINTR:     // A signal was caught
                            break;
                        default:
                            throw ServerException(strerror(errno));
                    }
                } // else a timeout occured.

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
