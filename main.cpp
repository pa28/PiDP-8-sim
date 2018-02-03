#include <iostream>
#include <iterator>
#include <iomanip>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>

#include "Encoder.h"

using namespace util;

int main() {

    std::stringbuf  sb;
    Encoder<char>   encoder{sb};

    ostream os{&encoder};
    istream is{&encoder};

    os << "Hello";
    flush(os);

    string  s;
    is >> s;

    cout << s << endl;
}