//
// Created by richard on 31/01/18.
//

#ifndef PIDP_CONSOLEAPI_H
#define PIDP_CONSOLEAPI_H

#include <iostream>
#include "Server.h"

using namespace util;

namespace pdp8
{
    template <class CharT, class Traits = std::char_traits<CharT>>
    class ApiConnection : public Connection<CharT,Traits>, Thread
    {
    public:
        ApiConnection(int fd, struct sockaddr_in &addr, socklen_t &len) :
                Connection<CharT,Traits>(fd, addr, len),
                strmbuf{fd},
                loop{true}
        {
            start();
        }

        void * run() override {
            std::istream is{&strmbuf};
            while (not is.eof() && loop) {
                char c;
                while (is.get(c)) {
                    std::cout << c;
                }
            }

            stop();
        }

        void stop() override {
            loop = false;
        }

    protected:
        fdstreambuf<CharT>  strmbuf;
        bool loop;
    };

    class MyServer : public Server<ApiConnection<char>>
    {
    public:
        MyServer() : loop(false) {}

        void * run() override;

        void stop () override;

        bool loop;
    };

    void MyServer::stop() {
        loop = false;

        for (auto &&c: clients) {
            c->stop();
        }
    };

    void * MyServer::run() {

        loop = true;
        while (loop) {
            int s = select(MyServer::SC_None);
        }

    }

}

#endif //PIDP_CONSOLEAPI_H
