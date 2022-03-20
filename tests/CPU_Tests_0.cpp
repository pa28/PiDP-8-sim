//
// Created by richard on 2022-03-13.
//

#include <iostream>
#include <variant>
#include "../src/cpu.h"

using namespace sim;

static constexpr std::string_view TEST_FAIL = "\u001b[31m[ Fail    ]\u001b[0m";
static constexpr std::string_view TEST_OK =   "\u001b[32m[      OK ]\u001b[0m";
static constexpr std::string_view TESTS_PASS = "\u001b[32m[  PASSED ]\u001b[0m";
static constexpr std::string_view TESTS_FAIL = "\u001b[31m[  FAILED ]\u001b[0m";
static constexpr std::string_view TESTS_WARN = "\u001b[33m[ WARNING ]\u001b[0m";
static constexpr std::string_view TEST_HDR = "\u001b[32m[=========]\u001b[0m";
static constexpr std::string_view TEST_SEP = "\u001b[32m[---------]\u001b[0m";

static constexpr uint16_t PAGE = 00200;     // Current page flag
static constexpr uint16_t IND = 00400;      // Indirect addressing
static constexpr uint16_t AND = 00000;      // Bitwise AND
static constexpr uint16_t TAD = 01000;      // Two's complement add
static constexpr uint16_t ISZ = 02000;      // Increment and skip if Zero
static constexpr uint16_t DCA = 03000;      // Deposit and clear accumulator
static constexpr uint16_t HLT = 07402;      // HLT instruction
static constexpr uint16_t NOP = 07000;      // OPR instruction

bool compareTestResult(const test_result lhs, const test_result rhs, CompareCriteria criteria) {
    return std::visit([criteria](auto &&lhs, auto &&rhs) -> bool {
        switch (criteria) {
            case Less:
                return lhs < rhs;
            case LessEq:
                return lhs <= rhs;
            case Equal:
                return lhs == rhs;
            case GreaterEq:
                return lhs >= rhs;
            case Greater:
                return lhs > rhs;
            case NotEqual:
                return lhs == rhs;
        }
        return false;
    }, lhs, rhs);
}

template<size_t length>
void TestCPU::loadTestProgram(uint16_t start, std::array<uint16_t, length> code) {
    loadAddress(start);
    for (auto &word: code) {
        deposit(word);
    }
    loadAddress(start);
}

template<typename Title, size_t length>
requires std::is_convertible_v<Title, std::string_view>
std::pair<int, int> TestCPU::singleInstructionTests(Title title, std::array<SingleInstructionTest, length> tests) {
    int passed = 0, failed = 0;
    fmt::print("{} {}\n", TEST_HDR, title);
    for (auto &test: tests) {
        if (test.resetCPU)
            reset();
        loadAddress(0200u);
//        fmt::print("Instruction {:05o}\n", test.instruction);
        deposit(test.instruction);
        loadAddress(0200u);
        instruction_step();
        auto result = test.test_function(*this);
        if (compareTestResult(result, test.expected, test.criteria)) {
            ++passed;
            fmt::print("{} {}\n", TEST_OK, test.name);
        } else {
            ++failed;
            fmt::print("{} {}", TEST_FAIL, test.name);
            auto criteria = CompareCriteriaText[test.criteria];
            std::visit([&criteria](auto &&lhs, auto &&rhs) {
                fmt::print(" -- Expected: {:o} {} {:o}\n", lhs, criteria, rhs);
                }, result, test.expected);
        }
    }
    if (failed == 0)
        fmt::print("{} ", TESTS_PASS);
    else if (passed == 0)
        fmt::print("{} ", TESTS_FAIL);
    else
        fmt::print("{} ", TESTS_WARN);
    if (failed)
        fmt::print("Passed {}, Failed {}\n", passed, failed);
    else
        fmt::print("Passed {}\n", passed);
    return {passed, failed};
}

std::pair<int, int> TestCPU::testOprInstructions_1() {
    auto testHaltFlag = [](TestCPU &cpu) -> test_result { return static_cast<bool>(cpu.halt_flag); };
    std::array<SingleInstructionTest, 2> tests =
            {{
                     {HLT, "HLTI", testHaltFlag, true, Equal, true},
                     {NOP, "NOPI", testHaltFlag, false, Equal, true},
             }};

    return singleInstructionTests("OPR Instruction Tests.", tests);
}

std::pair<int, int> TestCPU::testTadInstructions_1() {
    auto testLinkAcc = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.link_accumulator.value); };

    std::array<SingleInstructionTest, 15> tests =
            {{
                     {TAD | PAGE | IND | 012u, "TAD I 12", testLinkAcc, static_cast<uint16_t>(000002u), Equal, true},
                     {TAD | PAGE | IND | 013u, "TAD I 13", testLinkAcc, static_cast<uint16_t>(004002u), Equal, false},
                     {TAD | PAGE | IND | 013u, "TAD I 13", testLinkAcc, static_cast<uint16_t>(010002u), Equal, false},
                     {TAD | PAGE | IND | 013u, "TAD I 13", testLinkAcc, static_cast<uint16_t>(014002u), Equal, false},
                     {TAD | PAGE | IND | 013u, "TAD I 13", testLinkAcc, static_cast<uint16_t>(000002u), Equal, false},
                     {TAD | PAGE | 010u, "TAD 10", testLinkAcc, static_cast<uint16_t>(000002u), Equal, true},
                     {TAD | PAGE | 011u, "TAD 11", testLinkAcc, static_cast<uint16_t>(004002u), Equal, false},
                     {TAD | PAGE | 011u, "TAD 11 link -> 1", testLinkAcc, static_cast<uint16_t>(010002u), Equal, false},
                     {TAD | PAGE | 011u, "TAD 11 link", testLinkAcc, static_cast<uint16_t>(014002u), Equal, false},
                     {TAD | PAGE | 011u, "TAD 11 link -> 0", testLinkAcc, static_cast<uint16_t>(000002u), Equal, false},
                     {TAD | 030u, "TAD Z 30", testLinkAcc, static_cast<uint16_t>(000001u), Equal, true},
                     {TAD | 031u, "TAD Z 31", testLinkAcc, static_cast<uint16_t>(004001u), Equal, false},
                     {TAD | 031u, "TAD Z 31", testLinkAcc, static_cast<uint16_t>(010001u), Equal, false},
                     {TAD | 031u, "TAD Z 31", testLinkAcc, static_cast<uint16_t>(014001u), Equal, false},
                     {TAD | 031u, "TAD Z 31", testLinkAcc, static_cast<uint16_t>(000001u), Equal, false},
             }};

    loadAddress(00030u);
    deposit(1u);        // 1 at 00030
    deposit(04000u);    // 04000 at 00031
    loadAddress(00210u);
    deposit(2u);        // 2 at 00210
    deposit(04000u);    // 04000 at 00211
    deposit(00210u);    // 00210 at 00212;
    deposit(00211u);    // 00211 at 00213;

    return singleInstructionTests("TAD Instruction Tests.", tests);
}

std::pair<int, int> TestCPU::testAndInstructions_1() {
    auto testLinkAcc = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.link_accumulator.value); };
    auto dca0 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 033u)[cpu.wordIndex]()); };
    auto dca1 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 0233u)[cpu.wordIndex]()); };
    auto dca2 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 0212u)[cpu.wordIndex]()); };
    std::array<SingleInstructionTest, 15> tests =
            {{
                     {TAD | 030u, "TAD Z 30", testLinkAcc, static_cast<uint16_t>(07777u), Equal, true},
                     {AND | 031u, "AND Z 31", testLinkAcc, static_cast<uint16_t>(01234u), Equal, false},
                     {AND | 032u, "AND Z 32", testLinkAcc, static_cast<uint16_t>(01230u), Equal, false},
                     {TAD | PAGE | 010u, "TAD 10", testLinkAcc, static_cast<uint16_t>(07777u), Equal, true},
                     {AND | PAGE | 011u, "AND 11", testLinkAcc, static_cast<uint16_t>(01234u), Equal, false},
                     {AND | PAGE | 012u, "AND 12", testLinkAcc, static_cast<uint16_t>(01230u), Equal, false},
                     {TAD | PAGE | IND | 020u, "TAD I 20", testLinkAcc, static_cast<uint16_t>(07777u), Equal, true},
                     {AND | PAGE | IND | 021u, "AND I 21", testLinkAcc, static_cast<uint16_t>(01234u), Equal, false},
                     {AND | PAGE | IND | 022u, "AND I 22", testLinkAcc, static_cast<uint16_t>(01230u), Equal, false},
                     {TAD | 030u, "TAD Z 30", testLinkAcc, static_cast<uint16_t>(07777u), Equal, true},
                     {DCA | 033u, "DCA Z 33", dca0, static_cast<uint16_t>(07777u), Equal, false},
                     {TAD | 030u, "TAD Z 30", testLinkAcc, static_cast<uint16_t>(07777u), Equal, true},
                     {DCA | PAGE | 033u, "DCA 33", dca1, static_cast<uint16_t>(07777u), Equal, false},
                     {TAD | 030u, "TAD Z 30", testLinkAcc, static_cast<uint16_t>(07777u), Equal, true},
                     {DCA | PAGE | IND | 022u, "DCA I 22", dca2, static_cast<uint16_t>(07777u), Equal, false},
             }};

    loadAddress(00030u);
    deposit(07777u);    // 07777 at 00030
    deposit(01234u);    // 01234 at 00031
    deposit(05670u);    // 05670 at 00032
    loadAddress(00210u);
    deposit(07777u);    // 07777 at 00210
    deposit(01234u);    // 01234 at 00211
    deposit(05670u);    // 05678 at 00212
    loadAddress(00220u);
    deposit(00210u);    // 00210 at 00220
    deposit(00211u);    // 00211 at 00221
    deposit(00212u);    // 00212 at 00222

    return singleInstructionTests("AND-DCA Instruction Tests.", tests);
}

std::pair<int, int> TestCPU::testAutoIndexMemory() {
    auto noidx0 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 07u)[cpu.wordIndex]()); };
    auto noidx1 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 020u)[cpu.wordIndex]()); };
    auto idx0 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 010u)[cpu.wordIndex]()); };
    auto idx1 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 011u)[cpu.wordIndex]()); };
    auto idx2 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 012u)[cpu.wordIndex]()); };
    auto idx3 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 013u)[cpu.wordIndex]()); };
    auto idx4 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 014u)[cpu.wordIndex]()); };
    auto idx5 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 015u)[cpu.wordIndex]()); };
    auto idx6 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 016u)[cpu.wordIndex]()); };
    auto idx7 = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.readCore(0u, 017u)[cpu.wordIndex]()); };

    std::array<SingleInstructionTest, 10> tests =
            {{
                     { TAD | IND | 007u, "007 ", noidx0, static_cast<uint16_t>(00000u), Equal, true },
                     { TAD | IND | 010u, "010 ", idx0, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 011u, "011 ", idx1, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 012u, "012 ", idx2, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 013u, "013 ", idx3, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 014u, "014 ", idx4, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 015u, "015 ", idx5, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 016u, "016 ", idx6, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 017u, "017 ", idx7, static_cast<uint16_t>(00001u), Equal, true },
                     { TAD | IND | 007u, "020 ", noidx1, static_cast<uint16_t>(00000u), Equal, true },
            }};

    loadAddress(00007u);
    for (int i = 0; i < 10; ++i)
        deposit(0u);

    return singleInstructionTests("Auto indexing memory.", tests);
}

std::pair<int, int> TestCPU::testIszInstructions() {
    auto pc = [](TestCPU &cpu) -> test_result { return static_cast<uint16_t>(cpu.program_counter[cpu.wordIndex]());};

    std::array<SingleInstructionTest, 6> tests =
            {{
                     { ISZ | 030u, "ISZ Z 30 ", pc, static_cast<uint16_t>(00201u), Equal, true },
                     { ISZ | 030u, "ISZ Z 30 ", pc, static_cast<uint16_t>(00202u), Equal, false },
                     { ISZ | PAGE | 030u, "ISZ 30 ", pc, static_cast<uint16_t>(00201u), Equal, true },
                     { ISZ | PAGE | 030u, "ISZ 30 ", pc, static_cast<uint16_t>(00202u), Equal, false },
                     { ISZ | PAGE | IND | 010u, "ISZ I 10 ", pc, static_cast<uint16_t>(00201u), Equal, true },
                     { ISZ | PAGE | IND | 010u, "ISZ I 10 ", pc, static_cast<uint16_t>(00202u), Equal, false },
            }};

    loadAddress(00030u);
    deposit(07776u);

    loadAddress(00230u);
    deposit(07776u);
    deposit(07776u);

    loadAddress(00210);
    deposit(00231);

    return singleInstructionTests("ISZ instruction Tests.", tests);
}

static constexpr uint16_t TEST_START = 0200u;
static constexpr std::array<uint16_t, 3> TEST_PROGRAM =
        {
                01202, 05200, 04000
        };

int main() {
    int passed = 0, failed = 0;
    std::array<std::function<std::pair<int, int>(TestCPU &)>, 5> TestList =
            {&TestCPU::testOprInstructions_1, &TestCPU::testTadInstructions_1, &TestCPU::testAndInstructions_1,
             &TestCPU::testAutoIndexMemory, &TestCPU::testIszInstructions};

    TestCPU cpu{};

    for (auto &func: TestList) {
        auto[p, f] = func(cpu);
        passed += p;
        failed += f;
    }

    fmt::print("{} {}\n", TEST_HDR, "Summary");
    if (failed == 0)
        fmt::print("{} ", TESTS_PASS);
    else if (passed == 0)
        fmt::print("{} ", TESTS_FAIL);
    else
        fmt::print("{} ", TESTS_WARN);
    if (failed)
        fmt::print("Passed {}, Failed {}\n", passed, failed);
    else
        fmt::print("Passed {}\n", passed);

    return 0;
}


