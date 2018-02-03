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
    template <typename T>
    T hton(T v) {
        if constexpr(std::is_same<T, char>::value) {
            return v;
        } if constexpr(std::is_same<T, uint16_t>::value) {
            return htons(v);
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            return htonl(v);
        } else {
            static_assert(std::is_same<std::decay_t<T>, uint32_t>::value, "No implementation of hton for type.");
        }
    }

    template <typename T>
    T ntoh(T v) {
        if constexpr(std::is_same<T, char>::value) {
            return v;
        } else if constexpr(std::is_same<T, uint16_t>::value) {
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


    enum DataTypes : u_int8_t
    {
        DT_STX = 2,             // Start of text
        DT_ETX = 3,             // End of text
        DT_SO = 14,             // Shift Out
        DT_SI = 15,             // Shift In
        DT_SYN = 22,            // Sychronous idle
        DT_String,
        DT_LED_Status,          // LED status - from Chassis
        DT_SX_Status,           // Switch status - to Chassis
    };


    using LEDStatus_t = std::array<register_base_t,LEDSTATUS_COUNT>;
    using SXStatus_t = std::array<register_base_t,SWITCHSTATUS_COUNT>;


    template <class CharT, class Traits = std::char_traits<CharT>>
    class ApiConnection : public Connection<CharT,Traits>, Thread
    {
    public:
        ApiConnection(int fd) :
                Connection<CharT,Traits>{fd},
                strmbuf{fd},
                istrm{&strmbuf},
                ostrm{&strmbuf},
                ss{},
                loop{true}
        {
            start();
        }

        ApiConnection(int fd, struct sockaddr_in &addr, socklen_t &len) :
                Connection<CharT,Traits>(fd, addr, len),
                strmbuf{fd},
                istrm{&strmbuf},
                ostrm{&strmbuf},
                ss{},
                loop{true}
        {
            start();
        }

        void * run() override {
            bool idle = true;
            bool error = false;
            bool ready = false;

            while (not istrm.eof()) {
                char c;
                while (istrm.get(c)) {
                    if (idle) {
                        switch (c) {
                            case DT_STX:
                                idle = false;
                                ss.clear();
                                break;
                            case DT_SYN:
                                break;
                            default:
                                break;          // Technically a protocol error.
                        }
                    } else {
                        switch (c) {
                            case DT_SO:
                            case DT_SI:
                                istrm.get(c);
                                ss.put(c);
                                break;
                            case DT_STX:
                            case DT_SYN:
                                error = true;
                            case DT_ETX:
                                idle = true;
                                ready = true;
                                break;
                            default:
                                ss.put(c);
                        }
                    }
                    if (ready && not error) {
                        ready = false;
                        std::cout << "Received packet " << ss.str().length() << " bytes." << std::endl;
                        // process ss
                    }
                }
            }

            stop();
        }

        void stop() override {
            loop = false;
        }

        void sendLeader() {
            ostrm.put(DT_SYN);
            ostrm.put(DT_STX);
        }

        void sendTrailer() {
            ostrm.put(DT_ETX);
            ostrm.put(DT_SYN);
        }

        template <typename T>
        void sendData(T d) {
            for (size_t i = 0; i < sizeof(d); ++i) {
                sendChar(static_cast<char>(d & 0xFF));
                d >>= 8;
            }
        }

        void sendChar(char c) {
            switch (c) {
                case DT_STX:
                case DT_ETX:
                case DT_SYN:
                    ostrm.put(DT_SO);
                    break;
                case DT_SO:
                    ostrm.put(DT_SI);
                    break;
                default:
                    break;
            }
            ostrm.put(c);
        }

        template <class InputIt>
        void send(DataTypes t, InputIt first, InputIt last) {
            sendLeader();
            sendChar(t);
            while (first != last) {
                auto l = hton(*first);
                sendData(l);
                ++first;
            }
            sendTrailer();
            ostrm.flush();
        }

    protected:
        fdstreambuf<CharT>  strmbuf;
        std::basic_istream<CharT,Traits>  istrm;
        std::basic_ostream<CharT,Traits>  ostrm;
        std::stringstream   ss;

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
