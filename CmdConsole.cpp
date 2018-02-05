//
// Created by richard on 04/02/18.
//

#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "CmdConsole.h"

int CmdConsole::open() {

    unsigned char serveraddr[sizeof(struct in6_addr)];
    struct addrinfo hints{}, *res{nullptr};

    n_port = static_cast<uint16_t>(stoul(serverPort));

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rc = inet_pton(AF_INET, serverHost.c_str(), serveraddr);
    if (rc == 1) {
        hints.ai_family = AF_INET;
        hints.ai_flags |= AI_NUMERICHOST;
    } else {
        rc = inet_pton(AF_INET6, serverHost.c_str(), serveraddr);
        if (rc == 1) {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
        }
    }

    rc = getaddrinfo(serverHost.c_str(), serverPort.c_str(), &hints, &res);
    if (rc != 0) {
        std::cerr << "Host not found: " << gai_strerror(rc) << std::endl;
        if (rc == EAI_SYSTEM)
            std::cerr << "getaddrinfo() failed: " << strerror(errno) << std::endl;
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return fd;
    }


    do {
        rc = connect(fd, res->ai_addr, res->ai_addrlen);
        if (rc < 0) {
            std::cerr << "connect() failed: " << strerror(errno) << std::endl;
            res = res->ai_next;
        }
    } while (rc < 0 && res != nullptr);

    if (res == nullptr) {
        std::cerr << "All found addresses failed." << std::endl;
        ::close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char **argv) {
    CmdConsole  console{};
    char buf[128];

    console.setPort("8000");
    console.setHost("::1");
    console.setHost("localhost");

    int fd = console.open();
    if (fd < 0) {
        std::cerr << strerror(errno) << std::endl;
        return 1;
    }

    ssize_t n;
    n = ::write(fd, "Hello World!\n", 13);

    while ((n = ::read(fd, buf, sizeof(buf))) > 0) {
        std::cout.write(buf, n);
    }
    return 0;
}
