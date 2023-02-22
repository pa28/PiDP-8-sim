#include <csignal>
#include <Pdp8Terminal.h>
#include <DECWriter.h>

using namespace pdp8;

int main() {


    signal(SIGCHLD, SIG_IGN);
    auto terminalSocket = TerminalSocket();
    terminalSocket.open();

    TerminalManager terminalManager{};

    terminalManager.emplace_back(terminalSocket);

    while (!terminalManager.empty())
        terminalManager.serviceTerminals();
}
