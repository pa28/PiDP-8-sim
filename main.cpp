//#include <csignal>
//#include <Pdp8Terminal.h>
//#include <DECWriter.h>

#include <Terminal.h>

using namespace pdp8;

int main() {
//    signal(SIGCHLD, SIG_IGN);
//
//    PDP8 pdp8{};
//    auto decWriter = std::make_shared<DECWriter>();
//    pdp8.iotDevices[3] = decWriter;
//    pdp8.iotDevices[4] = decWriter;
//
//    auto pdp8PopUp = std::make_shared<PopupTerminal>();
//    pdp8PopUp->telnetTerminal = std::make_unique<Pdp8Terminal>(pdp8, pdp8PopUp->terminalSocket);
//
//    // ToDo: Roll TerminalSocket into TelnetTerminal
//    pdp8.terminalManager.push_back(pdp8PopUp);
//
//    while (!pdp8.terminalManager.empty())
//        pdp8.terminalManager.serviceTerminals();
    TerminalSocket terminalSocket{};
    {
        TerminalSocket ts{};
        ts.open();
        terminalSocket = std::move(ts);

    }
    std::ostream ostream{terminalSocket.oStrmBuf.get()};
    ostream << "Hello World\n";
    ostream.flush();

    return 0;
}
