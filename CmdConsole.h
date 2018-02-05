//
// Created by richard on 04/02/18.
//

#ifndef PIDP_CMDCONSOLE_H
#define PIDP_CMDCONSOLE_H

#include "Server.h"

using namespace std;

class CmdConsole {
public:
    CmdConsole() = default;
    ~CmdConsole() = default;

    void setHost(const string &host) { serverHost = host; }
    void setPort(const string &port) { serverPort = port; }

    int open();

    int close() {
        return ::close(fd);
    }

protected:
    int fd;
    uint16_t n_port;
    string  serverHost;
    string  serverPort;

};


#endif //PIDP_CMDCONSOLE_H
