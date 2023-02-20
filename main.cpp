#include <csignal>
#include <Pdp8Terminal.h>

using namespace pdp8;

int main() {

    signal(SIGCHLD, SIG_IGN);
    TerminalSocket terminalSocket;
    terminalSocket.open();
    Pdp8Terminal terminal(terminalSocket);
    terminal.console();
}
