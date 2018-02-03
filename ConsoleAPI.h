//
// Created by richard on 31/01/18.
//

#ifndef PIDP_CONSOLEAPI_H
#define PIDP_CONSOLEAPI_H

#include <iostream>
#include <array>
#include <iterator>
#include <sstream>
#include "PDP8.h"
#include "Server.h"
#include "Encoder.h"

using namespace util;

namespace pdp8
{
    enum DataTypes : u_int8_t
    {
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
                encoder(strmbuf),
                ostrm{&encoder},
                istrm{&encoder},
                ss{},
                loop{true}
        {
            start();
        }

        ApiConnection(int fd, struct sockaddr_in &addr, socklen_t &len) :
                Connection<CharT,Traits>(fd, addr, len),
                strmbuf{fd},
                encoder{strmbuf},
                ostrm{&encoder},
                istrm{&encoder},
                ss{},
                loop{true}
        {
            start();
        }

        void * run() override {
            stop();
        }

        void stop() override {
            loop = false;
        }


        ApiConnection<CharT, Traits> &beginEncoding() {
            encoder.beginEncoding();
            return *this;
        };


        ApiConnection<CharT, Traits> &endEncoding() {
            encoder.endEncoding();
            encoder.pubsync();
            return *this;
        };


        ApiConnection<CharT, Traits> &beginDecoding() {
            encoder.beginDecoding();
            return *this;
        };


        ApiConnection<CharT, Traits> &endDecoding() {
            encoder.endDecoding();
            return *this;
        };


        void write(CharT *buf, size_t l) {
            encoder.sputn(buf, l);
        }

        template<class T>
        ApiConnection<CharT, Traits> &put(const T value) {
            if constexpr(std::is_same<T, CharT>::value) {
                ostrm.put(value);
            } else if constexpr(std::is_same<std::decay_t<T>, uint16_t>::value) {
                T tvalue = hton(value);
                write(reinterpret_cast<CharT *>(&tvalue), sizeof(T));
            } else if constexpr(std::is_same<std::decay_t<T>, uint32_t>::value) {
                T tvalue = hton(value);
                write(reinterpret_cast<CharT *>(&tvalue), sizeof(T));
            } else if constexpr(std::is_same<std::decay_t<T>, std::string>::value) {
                ostrm << value;
            } else {
                static_assert(std::is_same<std::decay_t<T>, uint32_t>::value,
                              "No implementation of operation for type.");
            }
            return *this;
        }


        template<class C>
        ApiConnection<CharT, Traits> &put(C first, C last) {
            for (auto i = first; i != last; ++i) {
                put(*i);
            }

            return *this;
        }


        template<class T>
        ApiConnection<CharT, Traits> &operator<<(const T value) {
            return put(value);
        };


        int read(CharT *buf, size_t l) {
            int n{0};

            while (not encoder.isAtEnd() && n < l) {
                if (istrm.eof())
                    throw DecodingError("Unexpected EOF in packet.");

                buf[n] = istrm.get();
                ++n;
            }

            if (n != l)
                throw DecodingError("Unexpected end of packet.");

            return n;
        }

        void readString(std::string &value) {
            value.clear();

            while (not encoder.isAtEnd() && not istrm.eof()) {
                CharT c = istrm.get();
                if (isspace(c))
                    return;
                value.push_back(istrm.get());
            }
        }

        template<class T>
        ApiConnection<CharT, Traits> &get(T &value) {
            if constexpr(std::is_same<T, CharT>::value) {
                istrm.get(value);
            } else if constexpr(std::is_same<std::decay_t<T>, uint16_t>::value) {
                T tvalue{};
                read(reinterpret_cast<CharT *>(&tvalue), sizeof(T));
                T value = ntoh(tvalue);
            } else if constexpr(std::is_same<std::decay_t<T>, uint32_t>::value) {
                T tvalue{};
                read(reinterpret_cast<CharT *>(&tvalue), sizeof(T));
                T value = ntoh(tvalue);
            } else if constexpr(std::is_same<std::decay_t<T>, std::string>::value) {
                readString(value);
            } else {
                static_assert(std::is_same<std::decay_t<T>, uint32_t>::value,
                              "No implementation of operation for type.");
            }
            return *this;
        }


        template<class C>
        ApiConnection<CharT, Traits> &get(C first, C last) {
            while (not encoder.isAtEnd() && first != last) {
                if (istrm.eof())
                    throw DecodingError("Unexpeded end of file in data.");
                get(*first);
                ++first;
            }
            return *this;
        };


        template<class T>
        ApiConnection<CharT, Traits> &operator>>(const T value) {
            return get(value);
        };


    protected:
        fdstreambuf<CharT>  strmbuf;
        Encoder<CharT> encoder;

    public:
        std::basic_ostream<CharT> ostrm;
        std::basic_istream<CharT> istrm;

    protected:
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
