#include <csignal>
#include <Pdp8Terminal.h>
#include <DECWriter.h>

using namespace pdp8;

int main() {
    signal(SIGCHLD, SIG_IGN);

    PDP8 pdp8{};
    auto terminalSocket = TerminalSocket();
    terminalSocket.open();

    pdp8.terminalManager.push_back(std::make_unique<Pdp8Terminal>(pdp8, terminalSocket));

    while (!pdp8.terminalManager.empty())
        pdp8.terminalManager.serviceTerminals();
}
