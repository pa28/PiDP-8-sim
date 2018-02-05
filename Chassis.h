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
                Thread{},
                strm{fd},
                istrm{&strm},
                ostrm{&strm}
        {}

        CommandConnection(int fd, struct sockaddr_in &addr, socklen_t &len) :
                Connection<CharT>{fd, addr, len},
                Thread{},
                strm{fd},
                istrm{&strm},
                ostrm{&strm}
        {}

        void * run() override {
            while (_runFlag) {
                if (istrm.eof())
                    break;

                CharT c = istrm.get();
                switch (c) {
                    case '\t':
                        ostrm << "[tab]";
                        break;
                    default:
                        ostrm << c;
                        std::cout << c;
                }

                ostrm.flush();
            }
            std::cout << "Connection closed." << std::endl;
            return nullptr;
        }

        void stop() override {
            _runFlag = false;
        }

    protected:
        fdstreambuf<CharT>  strm;
        std::basic_istream<CharT> istrm;
        std::basic_ostream<CharT> ostrm;
    };

    static char BusyMsg[] = "Console connection in use.\r\n";

    template <class Cmd>
    class CommandServer :
            public Server<Cmd>
    {
    public:
        explicit CommandServer(uint16_t port = 8000, const in6_addr listenAddress = in6addr_any) :
                Server<Cmd>{port, listenAddress}
        {}

        void * run() override {
            char buf[2048];

            while (this->_runFlag) {

                this->clean_clients();

                int fd = this->select_accept();
                if (fd >= 0) {
                    std::cout << "Connection on " << fd << std::endl;
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
