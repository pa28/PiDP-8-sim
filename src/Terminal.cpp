/**
 * @file terminal.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-15
 */

#include <sys/un.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include "Terminal.h"

namespace sim {
    NullStreamBuffer Terminal::nullStreamBuffer;


    void TerminalPipe::open(const std::string& title) {
        // Open a pseudo-terminal master

        auto ptmx = ::open("/dev/ptmx", O_RDWR | O_NOCTTY);
        ptsName = ptsname(ptmx);

        if (ptmx == -1) {
            throw TerminalPipeException("Failed to open pseudo-terminal master-slave for use with xterm. Aborting...");
        } else if (unlockpt(ptmx) != 0) {
            close(ptmx);
            throw TerminalPipeException("Failed to unlock pseudo-terminal master-slave for use with xterm. Aborting...");
        } else if (grantpt(ptmx) != 0) {
            close(ptmx);
            throw TerminalPipeException("Failed to grant access rights to pseudo-terminal master-slave for use with xterm Aborting...");
        }

        // open the corresponding pseudo-terminal slave (that's us)
        ptsName = ptsname(ptmx);
        fmt::print("Slave-master terminal: {}\n", ptsName);
        terminalFd = ::open(ptsName.c_str(), O_RDWR | O_NOCTTY);

        if (terminalFd == -1) {
            close(ptmx);
            throw TerminalPipeException("Failed to open client side terminal endpoint: " + ptsName);
        }

        childPid = fork();
        if (childPid == -1) {
            close(ptmx);
            close(terminalFd);
            throw TerminalPipeException("Failed to fork terminal process.");
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
            throw TerminalPipeException("Failed to exec terminal program.");
        } else {
            close(ptmx);
        }

        iBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::in);
        oBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::out);
    }

    void TerminalSocket::open() {
        if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
            throw TerminalPipeException("socket failed.");

        int options = 1;
        if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &options, sizeof(options)))
            throw TerminalPipeException("setsockopt failed.");

        sock_address.sin_family = AF_INET;
        sock_address.sin_addr.s_addr = inet_addr("127.0.0.1");
        sock_address.sin_port = htons(8888);

        if (bind(socket, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1) {
            perror("bind");
            throw TerminalPipeException("bind failed.");
        }

        if (listen(socket, 1) == -1)
            throw TerminalPipeException("listen failed.");

        childPid = fork();
        if (childPid == -1) {
            throw TerminalPipeException("Failed to fork terminal process.");
        }

        if (childPid == 0) {
            char *argv[4];
            asprintf(argv+0, "mate-terminal");
            asprintf(argv+1, "-e");
            asprintf(argv+2, "telnet localhost %d", 8888);
            argv[3] = nullptr;
            close(0);
            close(1);
            close(2);

            execvp(argv[0], argv);
            throw TerminalPipeException("Failed to exec terminal program.");
        } else {
            if ((terminalFd = accept(socket, (struct sockaddr *) &client_address, (socklen_t *) &client_addr_len)) ==
                -1)
                throw TerminalPipeException("accept failed.");

            iBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::in);
            oBuffer = std::make_unique<stdio_filebuf>(terminalFd, std::ios::out);
        }

        // mate-terminal seems to fork to detach from its parent so we must reap the process.
        close(socket);
        int waitStatus;
        auto c = wait(&waitStatus);
    }
}
