#include <csignal>
#include <Pdp8Terminal.h>
#include <DECWriter.h>

using namespace pdp8;

int main() {

    signal(SIGCHLD, SIG_IGN);
    TerminalSocket terminalSocket;
    terminalSocket.open();
    Pdp8Terminal terminal(terminalSocket);
    terminal.pdp8.iotDevices.push_back(DECWriterPrinter());
    terminal.pdp8.iotDevices.push_back(DECWriterKeyBoard());
    terminal.console();
}
