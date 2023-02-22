#include <csignal>
#include <Pdp8Terminal.h>
#include <DECWriter.h>

using namespace pdp8;

int main() {
    signal(SIGCHLD, SIG_IGN);

    TerminalManager terminalManager{};

    auto terminalSocket = TerminalSocket();
    terminalSocket.open();

    terminalManager.push_back(std::make_unique<Pdp8Terminal>(terminalSocket));

    while (!terminalManager.empty())
        terminalManager.serviceTerminals();
}
