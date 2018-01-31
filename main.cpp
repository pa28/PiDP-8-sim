#include <iostream>
#include <iterator>

#include "ConsoleAPI.h"

using namespace util;
using namespace pdp8;

int main() {
    std::cout << "Hello, World!" << std::endl;

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
}