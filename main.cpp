#include <iostream>
#include <iterator>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "Encoder.h"

using namespace util;

int main() {

    std::stringbuf sbo;
    Encoder<char> encoder{sbo};

    ostream os{&encoder};
    istream is{&encoder};

    encoder.beginEncoding();
    os << "Hello World!";
    encoder.endEncoding();
    flush(os);

    string s{};

    encoder.beginDecodeing();
    while (not encoder.isAtEnd()) {
        s.push_back(is.get());
    }
    encoder.endDecoding();
    cout << s << endl;
}