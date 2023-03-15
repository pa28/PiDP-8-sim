//#include <csignal>
//#include <Pdp8Terminal.h>
//#include <DECWriter.h>

#include <Terminal.h>
#include <Pdp8Terminal.h>
#include <DECWriter.h>

using namespace pdp8;

int main() {
    signal(SIGCHLD, SIG_IGN);

    PDP8 pdp8{};
    auto decWriter = std::make_shared<DECWriter>();
    pdp8.iotDevices[3] = decWriter;
    pdp8.iotDevices[4] = decWriter;

    pdp8.terminalManager.push_back(std::make_shared<Pdp8Terminal>(pdp8));

    while (!pdp8.terminalManager.empty())
        pdp8.terminalManager.serviceTerminals();

    return 0;
}
