/**
 * @file terminal.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-15
 */

#include <sys/socket.h>
#include <bits/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "Terminal.h"

namespace sim {
//    NullStreamBuffer Terminal::nullStreamBuffer;


    void TerminalPipe::open(const std::string& title) {
        // Open a pseudo-terminal master

        auto ptmx = ::open("/dev/ptmx", O_RDWR | O_NOCTTY);
        ptsName = ptsname(ptmx);

        if (ptmx == -1) {
            throw TerminalConnectionException("Failed to open pseudo-terminal master-slave for use with xterm. Aborting...");
        } else if (unlockpt(ptmx) != 0) {
            close(ptmx);
            throw TerminalConnectionException("Failed to unlock pseudo-terminal master-slave for use with xterm. Aborting...");
        } else if (grantpt(ptmx) != 0) {
            close(ptmx);
            throw TerminalConnectionException("Failed to grant access rights to pseudo-terminal master-slave for use with xterm Aborting...");
        }

        // open the corresponding pseudo-terminal slave (that's us)
        ptsName = ptsname(ptmx);
        fmt::print("Slave-master terminal: {}\n", ptsName);
        terminalFd = ::open(ptsName.c_str(), O_RDWR | O_NOCTTY);

        if (terminalFd == -1) {
            close(ptmx);
            throw TerminalConnectionException("Failed to open client side terminal endpoint: " + ptsName);
        }

        childPid = fork();
        if (childPid == -1) {
            close(ptmx);
            close(terminalFd);
            throw TerminalConnectionException("Failed to fork terminal process.");
        }

        if (childPid == 0) {
            char *argv[7];
            asprintf(argv+0, "xterm");
            asprintf(argv+1, "-S%s/%d", ptsName.c_str(), ptmx);
            asprintf(argv+2, "-T");
            asprintf(argv+3, "%s", title.c_str());
            asprintf(argv+4, "-n");
            asprintf(argv+5, "%s", title.c_str());
            argv[6] = nullptr;

            execvp(argv[0], argv);
            throw TerminalConnectionException("Failed to exec terminal program.");
        } else {
            close(ptmx);
        }

        iBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::in);
        oBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::out);
    }

    void TerminalSocket::open() {
        if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
            throw TerminalConnectionException("socket failed.");

        int options = 1;
        if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &options, sizeof(options)))
            throw TerminalConnectionException("setsockopt failed.");

        sock_address.sin_family = AF_INET;
        sock_address.sin_addr.s_addr = inet_addr("127.0.0.1");
        sock_address.sin_port = htons(0);

        if (bind(socket, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1) {
            perror("bind");
            throw TerminalConnectionException("bind failed.");
        }

        if (listen(socket, 1) == -1)
            throw TerminalConnectionException("listen failed.");

        socklen_t len = sizeof(sock_address);
        getsockname(socket, (struct sockaddr *) &sock_address, &len);
        auto port = ntohs(sock_address.sin_port);

        childPid = fork();
        if (childPid == -1) {
            throw TerminalConnectionException("Failed to fork terminal process.");
        }

        if (childPid == 0) {
            char *argv[4];
            asprintf(argv+0, "mate-terminal");
            asprintf(argv+1, "-e");
            asprintf(argv+2, "telnet localhost %d", port);
            argv[3] = nullptr;
            close(0);
            close(1);
            close(2);

            execvp(argv[0], argv);
            throw TerminalConnectionException("Failed to exec terminal program.");
        } else {
            if ((terminalFd = accept(socket, (struct sockaddr *) &client_address, (socklen_t *) &client_addr_len)) ==
                -1)
                throw TerminalConnectionException("accept failed.");

            iBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::in);
            oBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::out);
        }

        // mate-terminal seems to fork to detach from its parent so we must reap the process.
//        close(socket);
//        int waitStatus;
//        auto c = wait(&waitStatus);
    }

    std::tuple<Terminal::SelectStatus, Terminal::SelectStatus, unsigned int>
    Terminal::select(bool selRead, bool selWrite, unsigned int timeoutUs) {
        SelectStatus readSelect = Timeout, writeSelect = Timeout;

        fd_set readFds, writeFds, exceptFds;
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);
        FD_ZERO(&exceptFds);
        struct timeval timeout{};

        timeout.tv_sec = 0;
        timeout.tv_usec = timeoutUs;

        if (selRead)
            FD_SET(ifd, &readFds);
        if (selWrite)
            FD_SET(ofd, &writeFds);

        auto nfds = std::max(ifd,ofd) + 1;

        if (auto stat = ::select(nfds, &readFds, &writeFds, &exceptFds, &timeout); stat == -1) {
            throw TerminalConnectionException("Call to select failed.");
        } else if (stat > 0) {
            if (FD_ISSET(ifd, &readFds))
                readSelect = Data;

            if (FD_ISSET(ofd, &writeFds))
                writeSelect = Data;
        }

        return {readSelect, writeSelect, timeout.tv_usec};
    }

    void TelnetTerminal::setCharacterMode() {
        out().put(static_cast<char>(IAC)).put(static_cast<char>(WILL)).put(static_cast<char>(ECHO)).flush();
        out().put(static_cast<char>(IAC)).put(static_cast<char>(WILL)).put(static_cast<char>(SUPPRESS_GO_AHEAD)).flush();
    }

    void TelnetTerminal::negotiateAboutWindowSize() {
        out().put(static_cast<char>(IAC)).put(static_cast<char>(DO)).put(NAWS).flush();
    }

    void TelnetTerminal::parseInput() {
        enum ActionState { NONE, OFF, ON, BUFFER };
        // 0377 0375 0001 0377 0375 0003 0377 0373 0037 0377 0372 0037 0000 0120 0000 0030 0377 0360
        ActionState actionState = NONE;
        while (in().rdbuf()->in_avail() && in()) {
            // Process IAC communications
            auto c = in().get();
            if (c == IAC) {
                bool skip = false;
                c = in().get();
                switch (c) {
                    case SB: {
                        skip = true;
                        actionState = BUFFER;
                        std::vector<int> buffer{};
                        bool noSE = true;
                        while (in().rdbuf()->in_avail() && in() && noSE) {
                            c = in().get();
                            switch (c) {
                                case SE:
                                    parseIacBuffer(buffer);
                                    noSE = false;
                                    break;
                                case IAC:
                                    break;
                                default:
                                    buffer.push_back(c);
                            }
                        }
                    }
                        break;
                    case DO:
                    case WILL:
                        actionState = ON;
                        break;
                    case WONT:
                        actionState = OFF;
                        break;
                    default:
                        std::cout << fmt::format("Unhandled code 1 {}\n", c);
                }

                if (skip)
                    continue;

                c = in().get();
                switch (c) {
                    case NAWS:
                        naws = actionState == ON;
                        break;
                    case ECHO:
                        echoMode = actionState == ON;
                        break;
                    case SUPPRESS_GO_AHEAD:
                        suppressGoAhead = actionState == ON;
                        break;
                    default:
                        std::cout << fmt::format("Unhandled code 2 {}\n", c);
                        break;
                }
            } else if (std::isprint(c)) {
                inputLineBuffer.push_back(static_cast<char>(c));
                inputBufferChanged();
            } else {
                if (c == '\r') {
                    inputBufferReady();
                } else if (c == 127 && !inputLineBuffer.empty()) {
                    inputLineBuffer.pop_back();
                    inputBufferChanged();
                }
            }
        }
    }

    void TelnetTerminal::parseIacBuffer(const std::vector<int>& buffer) {
        auto itr = buffer.begin();
        switch (*itr) {
            case NAWS:
                ++itr;
                termWidth = *itr;
                ++itr;
                termWidth = (termWidth << 8) | *itr;
                ++itr;
                termHeight = *itr;
                ++itr;
                termHeight = (termHeight << 8) | *itr;
                windowSizeChanged();
                break;
            default:
                std::cout << fmt::format("Unhandled code IAC {}\n", *itr);
                break;
        }
    }
}
