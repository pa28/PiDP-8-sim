#include <iostream>

#include "Memory.h"

using namespace hw_sim;

void f(std::shared_ptr<Memory<MAXMEMSIZE, uint16_t, 12>> &memory) {
    std::cout << "Ref to mem: " << memory.use_count() << std::endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    auto memory = std::make_shared<Memory<MAXMEMSIZE, uint16_t, 12>>();

    f(memory);

    if (memory != nullptr) {
        auto m = memory;
        f(memory);
    }

    f(memory);

    memory->at(0) = 0077;
    std::cout << memory->at(0).flags() << ' ' << std::oct << memory->at(0) << std::endl;

    memory->clear();
    std::cout << memory->at(0).flags() << ' ' << std::oct << memory->at(0) << std::endl;

    return 0;
}