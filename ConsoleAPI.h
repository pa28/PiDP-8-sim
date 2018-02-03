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
    enum DataTypes : uint8_t
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


        void write(const CharT *buf, size_t l) {
            encoder.sputn(buf, l);
        }


        template <class T, size_t N>
        struct PutImpl {
#if 0
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, const T value) {
                throw std::exception();
            }
#endif
        };

        template <class T>
        struct PutImpl<T,1> {
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, const T value) {
                CharT c = static_cast<CharT>(value);
                api.write(&c, 1);
                return api;
            }
        };

        template <class T>
        struct PutImpl<T,2> {
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, const T value) {
                uint16_t tvalue = ::htons(value);
                api.write(reinterpret_cast<const char*>(&tvalue), 2);
                return api;
            }
        };

        template <class T>
        struct PutImpl<T,4> {
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, const T value) {
                uint32_t tvalue = ::htonl(value);
                api.write(reinterpret_cast<const char*>(&tvalue), 4);
                return api;
            }
        };

        template<class T>
        ApiConnection<CharT, Traits> &put(const T& value) {
            PutImpl<T,sizeof(T)>{}(*this, value);
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


        template <class T, size_t N>
        struct GetImpl {
#if 0
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, const T value) {
                throw std::exception();
            }
#endif
        };


        template <class T>
        struct GetImpl<T,1> {
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, T& value) {
                CharT c;
                api.read(&c, 1);
                value = static_cast<T>(c);
                return api;
            }
        };


        template <class T>
        struct GetImpl<T,2> {
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, T& value) {
                api.read(reinterpret_cast<CharT*>(&value), 2);
                value = static_cast<T>(::ntohs(value));
                return api;
            }
        };


        template <class T>
        struct GetImpl<T,4> {
            ApiConnection<CharT, Traits> &operator()(ApiConnection<CharT, Traits> &api, T& value) {
                api.read(reinterpret_cast<CharT*>(&value), 4);
                value = static_cast<T>(::ntohl(value));
                return api;
            }
        };


        template<class T>
        ApiConnection<CharT, Traits> &get(T &value) {
            GetImpl<T,sizeof(T)>{}(*this, value);
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
