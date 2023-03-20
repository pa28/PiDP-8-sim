/*
 * CodeFragmentTest.h Created by Richard Buckley (C) 19/02/23
 */

/**
 * @file CodeFragmentTest.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 19/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_CODEFRAGMENTTEST_H
#define PDP8_CODEFRAGMENTTEST_H

#include <functional>
#include <ranges>
#include <algorithm>
#include "assembler/Assembler.h"
#include "PDP8.h"

namespace pdp8 {
    using TestFunction = std::function<bool(PDP8& pdp8)>;

    /**
     * @class DirectTest
     * @brief Test PDP8 operation by direct manipulation.
     */
    class DirectTest {
        std::vector<bool> results{};

    public:
        template<class Name, class Setup, class Test>
        void gatherSetupResults(Name name, Setup setup, Test test) {
            PDP8 pdp8{};
            if (setup(pdp8)) {
                results.push_back(test(pdp8));
                auto passes = std::ranges::count(results, true);
                fmt::print("{}: {} of {} tests passed.\n", name, passes, results.size());
            }
        }

        template<class Name, class Setup, class Test, class ... Tests>
        void gatherSetupResults( Name name, Setup setup, Test test, Tests ... tests) {
            PDP8 pdp8{};
            if (setup(pdp8)) {
                results.push_back(test(pdp8));
                gatherSetupResults(name, setup, tests...);
            }
        }

        template<class Name, class Setup, class ... Tests>
        void setupTest(Name name, Setup setup, Tests ... tests) {
            gatherSetupResults( name, setup, tests...);
        }
    };


    /**
     * @class CodeFragmentTest
     * @brief Test PDP8 operation by running code fragments.
     */
    class CodeFragmentTest {
        std::vector<bool> results{};

    public:
        template<class Name, class Test>
        void gatherResults(PDP8& pdp8, Name name, Test test) {
            results.push_back(test(pdp8));
            auto passes = std::ranges::count(results, true);
            fmt::print("{}: {} of {} tests passed.\n", name, passes, results.size());
        }

        template<class Name, class Test, class ... Tests>
        void gatherResults(PDP8& pdp8, Name name, Test test, Tests ... tests ) {
            results.push_back(test(pdp8));
            gatherResults(pdp8, name, std::forward<Tests>(tests)...);
        }

        template<class Name, class Code, class ... Tests>
        void runTestCode(Name name, Code code, Tests ... tests) {
            std::stringstream testCode{code};
            std::stringstream list{};
            pdp8asm::Assembler assembler{};
            assembler.readProgram(testCode);
            if (assembler.pass1()) {
                std::stringstream bin{};
                if (assembler.pass2(bin, list)) {
                    PDP8 pdp8{};
                    if (pdp8.readBinaryFormat(bin)) {
                        while (pdp8.run_flag)
                            pdp8.instructionStep();
                        gatherResults(pdp8, name, tests...);
                    } else {
                        fmt::print("Binary would not load.\n");
                    }
                } else {
                    fmt::print("Assembler pass 2 failed:\n{}\n", list.str());
                }
            } else {
                fmt::print("Assembler pass 1 failed for:\n\t{}\n", code);
            }
        }
    };

} // pdp8

#endif //PDP8_CODEFRAGMENTTEST_H
