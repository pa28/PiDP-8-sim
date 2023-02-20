//
// Created by richard on 19/02/23.
//

#include <iostream>
#include <fstream>
#include <assembler/TestPrograms.h>
#include <assembler/Assembler.h>

using namespace pdp8asm;

int main() {
//    std::stringstream source("\tOCTAL\n\t*0200\n\tone,\ntwo,\tcla\n\tnop\n");
    std::ifstream source("../src/assembler/samples/Test.pal");
    if (source) {
        try {
            Assembler assembler;
            assembler.readProgram(source);
            assembler.pass1();

            std::stringstream binary;
            assembler.pass2(binary, std::cout);
            assembler.dumpSymbols(std::cout);
        } catch (std::invalid_argument& e) {
            fmt::print("Error: {}\n", e.what());
        }
    }
}