#include <iostream>

#include "Registers.h"

using namespace hw_sim;

enum RegisterId
{
    AC, PC, IR, Count
};


using WordRegister_t = RegisterType<8, 12, 0>;

using RegisterSet_t = std::array<int32_t, RegisterId::Count>;

using rAC_t = RegisterAccessor<RegisterSet_t, AC, WordRegister_t>;
using rPC_t = RegisterAccessor<RegisterSet_t, PC, WordRegister_t>;


RegisterSet_t registerSet;

rAC_t rAC{registerSet};
rPC_t rPC{registerSet};


int main() {
    std::cout << "Hello, World!" << std::endl;

    rAC = 077;
    rPC = rAC;

    std::cout << rAC << ' ' << rPC << std::endl;

    return 0;
}