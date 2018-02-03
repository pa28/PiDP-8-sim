#include <iostream>
#include <iomanip>

#include "ConsoleAPI.h"

using namespace util;
using namespace pdp8;

int main() {

    int s[2]{};

    socketpair(AF_UNIX, SOCK_STREAM, 0, s);
    ApiConnection<char> tx{s[1]};
    ApiConnection<char> rx{s[0]};

    SXStatus_t sxStatus{0x0123, 0x4567, 0x89ab};

    tx.beginEncoding().put(sxStatus.begin(), sxStatus.end()).endEncoding();
    rx.beginDecoding().get(sxStatus.begin(), sxStatus.end()).endDecoding();

    for (auto sx: sxStatus) {
        cout << hex << setw(4) << setfill('0') << sx << "  ";
    }

    cout << endl;

}