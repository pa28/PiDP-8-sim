#include <iostream>
#include <iterator>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "ConsoleAPI.h"

using namespace util;
using namespace pdp8;

int main() {
    std::cout << "Hello, World!" << std::endl;

    LEDStatus_t ledStatus { 0x0008, 0x0104, 0x0202, 0x0401, 0x0008, 0x0104, 0x0202, 0x0401 };

    std::vector<uint16_t> lv{ 0x0008, 0x0104, 0x0202, 0x0401, 0x0008, 0x0104, 0x0202, 0x0401 };

//    std::transform(ledStatus.begin(), ledStatus.end(), ledStatus.begin(), ntoh<uint16_t>);
//    std::transform(lv.begin(), lv.end(), lv.begin(), ntoh<uint16_t>);

    Host2Net(ledStatus.begin(), ledStatus.end());
    Net2Host(ledStatus.begin(), ledStatus.end());

    Host2Net(lv.begin(), lv.end());
    Net2Host(lv.begin(), lv.end());

    for (auto l: ledStatus) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << l << std::endl;
    }

    std::cout << std::endl;

    for (auto l: lv) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << l << std::endl;
    }

#if 0
    MyServer  server{};

    try {
        server.open();
        server.start();

        server.loop = true;
        while (server.loop)
            sleep(1);

    } catch (ServerException &se) {
        std::cerr << se.what() << std::endl;
    }
#endif
}