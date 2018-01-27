#include <iostream>

#include "Memory.h"
#include "FileUtilities.h"

using namespace hw_sim;

void f(std::shared_ptr<Memory<MAXMEMSIZE, uint16_t, 12>> &memory) {
    std::cout << "Ref to mem: " << memory.use_count() << std::endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    auto memory = std::make_shared<Memory<MAXMEMSIZE, uint16_t, 12>>();

    pdp8::readFileToMemory("../TestData/Test.bin", memory);

    for (auto m = memory->begin(); m != memory->end(); ++m) {
        if (m->flags() & MemoryFlag::MemFlagInitialized) {
            std::cout << std::oct << setw(5) << setfill('0') << m - memory->begin() << ' '
                      << m->flags() << ' ' << *m << std::endl;
        }
    }
}