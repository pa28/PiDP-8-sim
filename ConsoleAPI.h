//
// Created by richard on 31/01/18.
//

#ifndef PIDP_CONSOLEAPI_H
#define PIDP_CONSOLEAPI_H

#include <iostream>
#include <array>
#include <iterator>
#include "PDP8.h"
#include "Server.h"

using namespace util;

namespace pdp8
{
    enum DataTypes : u_int8_t
    {
        DT_LED_Status = 1,      // LED status - from Chassis
        DT_SX_Status,           // Switch status - to Chassis
    };

    using LEDStatus_t = std::array<register_base_t,LEDSTATUS_COUNT>;
    using SXStatus_t = std::array<register_base_t,SWITCHSTATUS_COUNT>;

    template <typename T>
    T hton(T v) {
        if constexpr(std::is_same<T, uint16_t>::value) {
            return htons(v);
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            return htonl(v);
        } else {
            static_assert(std::is_same<std::decay_t<T>, uint32_t>::value, "No implementation of hton for type.");
        }
    }

    template <typename T>
    T ntoh(T v) {
        if constexpr(std::is_same<T, uint16_t>::value) {
            return ntohs(v);
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            return ntohl(v);
        } else {
            static_assert(std::is_same<T, uint32_t>::value, "No implementation of ntoh for type.");
        }
    }

    template <class C>
    void Host2Net(C first, C last) {
        for (auto i = first; i != last; ++i) {
            auto v = hton(*i);
            *i = v;
        }
    }

    template <class C>
    void Net2Host(C first, C last) {
        for (auto i = first; i != last; ++i) {
            auto v = ntoh(*i);
            *i = v;
        }
    }


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
