#include <iostream>
#include <iterator>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>

#include "ConsoleAPI.h"

using namespace util;
using namespace pdp8;

int main() {
    std::cout << "Hello, World!" << std::endl;

    LEDStatus_t ledStatus { 0x0008, 0x0104, 0x0202, 0x0401, 0x0008, 0x0104, 0x0202, 0x0401 };

    std::vector<uint16_t> lv{ 0x0008, 0x0104, 0x0202, 0x0401, 0x0008, 0x0104, 0x0202, 0x0401 };

    int p[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, p) == -1) {
//    int f;
//    f = open("test.dat", O_CREAT | O_WRONLY);
//    if (f < 0) {
        std::cerr << strerror(errno) << std::endl;
        return 1;
    }

    ApiConnection<char>   rx{p[0]};
    ApiConnection<char>   tx{p[1]};

    tx.send(DT_LED_Status, ledStatus.begin(), ledStatus.end());

    while(true) {
        sleep(1);
    }
}