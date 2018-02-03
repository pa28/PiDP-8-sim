//
// Created by richard on 31/01/18.
//

#ifndef PIDP_SERVER_H
#define PIDP_SERVER_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <list>
#include <algorithm>
#include <streambuf>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>
#include "Thread.h"

namespace util {
    class ServerException : public std::runtime_error
    {
    public:
        explicit ServerException(const char * w) : std::runtime_error(w) {}
        explicit ServerException(const std::string && w) : std::runtime_error(w) {}
    };

    /**
     * @brief A template type safe wrapper around htons and htonl
     * @tparam T the type of the argument
     * @param v the value of the argument
     * @return the transformed value
     */
    template<typename T>
    T hton(T v) {
        if constexpr(std::is_same<T, char>::value) {
            return v;
        }
        if constexpr(std::is_same<T, uint16_t>::value) {
            return htons(v);
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            return htonl(v);
        } else {
            static_assert(std::is_same<std::decay_t<T>, uint32_t>::value, "No implementation of hton for type.");
        }
    }


    /**
     * @brief A template type safe wrapper around ntohs and ntohl
     * @tparam T the type of the argument
     * @param v the value of the argument
     * @return the transformed value
     */
    template<typename T>
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


    /**
     * @brief A function to transorm a range from host to network format.
     * @tparam C The iterator type
     * @param first the beginning of the range
     * @param last the end of the range
     */
    template<class C>
    void Host2Net(C first, C last) {
        for (auto i = first; i != last; ++i) {
            auto v = hton(*i);
            *i = v;
        }
    }

    /**
     * @brief A function to transorm a range from network to host format.
     * @tparam C The iterator type
     * @param first the beginning of the range
     * @param last the end of the range
     */
    template<class C>
    void Net2Host(C first, C last) {
        for (auto i = first; i != last; ++i) {
            auto v = ntoh(*i);
            *i = v;
        }
    }


    /**
     * @brief A streambuf which abstracts the socket file descriptor allowing the use of
     * standard iostreams.
     */
    template <class CharT, class Traits = std::char_traits<CharT>>
    class fdstreambuf : public std::basic_streambuf<CharT, Traits>
    {
    public:

        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;

        fdstreambuf() : _fd(-1) {}

        explicit fdstreambuf(int fd) : _fd(fd) {
            this->setp(obuf, obuf+sizeof(obuf));
            this->setg(ibuf, ibuf+8, ibuf+8);
        }

        void set_fd(int fd) { _fd = fd; }

    protected:
        CharT obuf[4096];
        CharT ibuf[4096+8];
        int _fd;

        int sync() {
            if (_fd >= 0) {
                ssize_t n = ::send(_fd, obuf, this->pptr() - obuf, 0 );

                if (n < 0) {
                    return -1;
                } else if (n == (this->pptr() - obuf)) {
                    this->setp(obuf, obuf + sizeof(obuf));
                } else {
                    for (ssize_t cp = n; cp < (sizeof(obuf) - n); ++cp) {
                        obuf[cp - n] = obuf[cp];
                    }
                    this->setp(obuf, obuf + sizeof(obuf));
                    this->pbump(static_cast<int>(sizeof(obuf)-n));
                }
                return 0;
            }
            return -1;
        }

        int_type overflow( int_type c ) {
            if (sync() < 0)
                return traits_type::eof();

            if (traits_type::not_eof(c)) {
                char_type cc = traits_type::to_char_type(c);
                this->xsputn(&cc, 1);
            }

            return traits_type::to_int_type(c);
        }

        int_type underflow( ) {
            if (_fd >= 0) {
                ssize_t n = ::recv(_fd, ibuf + 8, sizeof(ibuf) - 8, 0);

                if (n < 0) {
                    return traits_type::eof();
                }
                this->setg(ibuf, ibuf + 8, ibuf + 8 + n);
                if (n) {
                    return traits_type::to_int_type(*(ibuf + 8));
                }
            }
            return traits_type::eof();
        }
    };

    typedef fdstreambuf<char> fd_streambuf;
    typedef fdstreambuf<wchar_t> wfd_streambuf;

    template <class CharT, class Traits = std::char_traits<CharT>>
    class Connection {
    public:
        explicit Connection(int fd = -1) :
                clientfd{fd},
                client_addr{},
                length{},
                pendingWrite{false}
        {
        }

        Connection(int fd, struct sockaddr_in &addr, socklen_t &len) :
                clientfd{-1},
                client_addr{},
                length{len},
                pendingWrite{false}
        {
            clientfd = fd;
            length = len;
            memcpy(&client_addr, &addr, len);
        }

        ~Connection() {
            if (clientfd >= 0)
                close(clientfd);
        }

        int fd() { return clientfd; }

        bool operator < (const Connection &other) {
            return clientfd < other.clientfd;
        }

    protected:
        int clientfd;
        struct sockaddr_in client_addr;
        socklen_t length;
        bool pendingWrite;

        template <class ConnectionT>
        friend class Server;
    };

    template <class CharT, class Traits = std::char_traits<CharT>>
    bool operator < (const std::unique_ptr<Connection<CharT,Traits>>& a, const std::unique_ptr<Connection<CharT,Traits>>& b) {
        return a->fd() < b->fd();
    }

    template <class ConnectionT>
    class Server : public Thread {
    public:

        using ConnectionList_t = std::list<std::unique_ptr<ConnectionT>>;

        enum SelectClients
        {
            SC_None = 0,
            SC_Read = 1,
            SC_Write = 2,
            SC_Except = 4,
            SC_All = 7
        };

        explicit Server(in_addr_t listenAddress = INADDR_ANY, uint16_t port = 8000) :
                rd_set{},
                wr_set{},
                ex_set{},
                sockfd{-1},
                listenCount{5},
                portno{port},
                server_in_addr{listenAddress},
                server_addr{}
        {

        }

        ~Server() override = default;

        int open() {
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd >= 0) {
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = server_in_addr;
                server_addr.sin_port = htons(portno);
            }

            if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
                ::close(sockfd);
                sockfd = -1;
                throw ServerException(std::string("bind() error: ") + strerror(errno));
            }

            listen(sockfd, listenCount);

            return sockfd;
        }

        int accept() {
            struct sockaddr_in client_addr{};
            socklen_t length;

            int clientfd = ::accept(sockfd, (struct sockaddr *) &client_addr, &length);
            if (clientfd >= 0)
                clients.push_back(std::make_unique<ConnectionT>(clientfd, client_addr, length));
        }

        int select(SelectClients selectClients = SC_Read | SC_Write | SC_Except,
                   struct timeval *timeout = nullptr) {
            FD_ZERO(&rd_set);
            FD_ZERO(&wr_set);
            FD_ZERO(&ex_set);

            FD_SET(sockfd, &rd_set);

            int n = sockfd + 1;

            for (auto &&c: clients) {
                if (selectClients & SC_All) {
                    if (selectClients & SC_Read)
                        FD_SET(c->fd(), &rd_set);
                    if (selectClients & SC_Write)
                        if (c->pendingWrite)
                            FD_SET(c->fd(), &wr_set);
                    if (selectClients & SC_Except)
                        FD_SET(c->fd(), &ex_set);

                    n = std::max(n, c->fd()) + 1;
                }
            }

            int s = ::select( n, &rd_set, &wr_set, &ex_set, timeout);

            if (s > 0) {
                if (FD_ISSET(sockfd, &rd_set)) {
                    accept();
                    s--;
                }
            }

            return s;
        }

        auto begin() { return clients.begin(); }
        auto end() { return clients.end(); }

        bool isRead(std::unique_ptr<ConnectionT> &c) {
            return FD_ISSET(c->fd(), &rd_set);
        }

        bool isWrite(std::unique_ptr<ConnectionT> &c) {
            return FD_ISSET(c->fd(), &wr_set);
        }

        bool isExcept(std::unique_ptr<ConnectionT> &c) {
            return FD_ISSET(c->fd(), &ex_set);
        }

        auto close(typename ConnectionList_t::iterator c) {
            clients.erase(c);
        }

        fd_set  rd_set, wr_set, ex_set;

    protected:
        int sockfd, listenCount;
        uint16_t portno;
        in_addr_t server_in_addr;
        struct sockaddr_in server_addr;

        std::list<std::unique_ptr<ConnectionT>> clients;
    };
}


#endif //PIDP_SERVER_H
