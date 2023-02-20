/*
 * CodeFragmentTest.cpp Created by Richard Buckley (C) 19/02/23
 */

/**
 * @file TestOprInst.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 19/02/23
 */

#include <PDP8.h>
#include <assembler/Assembler.h>
#include "libs/CodeFragmentTest.h"

using namespace pdp8;

int main() {
    CodeFragmentTest test1{};
    test1.runTestCode("CodeFrag", "/ Simple test program.\nOCTAL\n*0200\nCLA CLL CMA IAC\nHLT\n*0200\n",
                      [](PDP8 &pdp8) { return pdp8.accumulator.getAcc() == 0; },
                      [](PDP8 &pdp8) { return pdp8.accumulator.getLink() == 1; }
    );

    DirectTest test2{};
    test2.setupTest("OPR Grp 3", [](PDP8& pdp8) {
                        pdp8.accumulator.setAcc(01234);
                        pdp8.mulQuotient.setWord(06543);
                        return true;
                    },
                    [](PDP8& pdp8) {
                        pdp8.instructionReg.value = pdp8asm::generateOpCode("CAM", 0);
                        pdp8.execute();
                        return pdp8.accumulator.getAcc() == 0 && pdp8.mulQuotient.getWord() == 0;
                    },
                    [](PDP8& pdp8) {
                        pdp8.instructionReg.value = pdp8asm::generateOpCode("MQA", 0);
                        pdp8.execute();
                        return pdp8.accumulator.getAcc() == 07777 && pdp8.mulQuotient.getWord() == 06543;
                    },
                    [](PDP8& pdp8) {
                        pdp8.instructionReg.value = pdp8asm::generateOpCode("CLA MQA", 0);
                        pdp8.execute();
                        return pdp8.accumulator.getAcc() == 06543 && pdp8.mulQuotient.getWord() == 06543;
                    },
                    [](PDP8& pdp8) {
                        pdp8.instructionReg.value = pdp8asm::generateOpCode("MQL", 0);
                        pdp8.execute();
                        return pdp8.accumulator.getAcc() == 0 && pdp8.mulQuotient.getWord() == 01234;
                    },
                    [](PDP8& pdp8) {
                        pdp8.instructionReg.value = pdp8asm::generateOpCode("SWP", 0);
                        pdp8.execute();
                        return pdp8.accumulator.getAcc() == 06543 && pdp8.mulQuotient.getWord() == 01234;
                    }
    );

}