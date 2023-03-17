//
// Created by richard on 15/03/23.
//

#include <PDP8.h>
#include <assembler/Assembler.h>
#include "libs/CodeFragmentTest.h"
#include <clean-test/clean-test.h>

constexpr auto sum(auto... vs) { return (0 + ... + vs); }

namespace ct = clean_test;
using namespace ct::literals;
using namespace pdp8;
using namespace pdp8asm;

template<class Str>
        concept StringLike = std::is_convertible_v<Str, std::string_view>;

struct TestAssembly {
    PDP8 pdp8{};
    Assembler assembler{};
    bool pass1{false};
    bool pass2{false};

    template<typename Str>
    requires StringLike<Str>
    explicit TestAssembly(Str s) {
        std::stringstream testCode{std::string(s)};
        assembler.readProgram(testCode);
        if (pass1 = assembler.pass1(); pass1) {
            std::stringstream bin{};
            std::stringstream list{};
            if (pass2 = assembler.pass2(bin,list); pass2) {
                pdp8.readBinaryFormat(bin);
                pdp8.instructionCycle();
            }
        }
    }

};

static constexpr std::string_view  testCode0{"/ Simple test program.\nOCTAL\n*0200\nCLA CLL CMA IAC\nHLT\n*0200\n"};

struct AccInstruction {
    PDP8 pdp8{};

    bool opCode{false};

    explicit operator bool () const { return opCode; }

    template<class Str>
            requires StringLike<Str>
    AccInstruction(unsigned acc, unsigned link, Str opStr, unsigned pc = 0) {
        pdp8.accumulator.setAcc(acc);
        pdp8.accumulator.setLink(link);
        if (auto op = pdp8asm::generateOpCode(opStr, pc); op) {
            opCode = op.has_value();
            pdp8.instructionReg.value = op.value();
            pdp8.execute();
        }
    }
};

std::ostream& operator<<(std::ostream& strm, const AccInstruction& acc) {
    return strm << fmt::format("[{}, {}-{:o}]", acc.opCode, acc.pdp8.accumulator.getLink(), acc.pdp8.accumulator.getAcc());
}

auto const suite0 = ct::Suite{"ACC Link", []{
    "BSW"_test = [] { AccInstruction acc{01234u, 0u, "BSW"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getAcc() == 03412_i); };
    "RTR"_test = [] { AccInstruction acc{01236u, 0u, "RTR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 010247_i); };
    "RAR"_test = [] { AccInstruction acc{01235u, 0u, "RAR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 010516_i); };
    "RTL"_test = [] { AccInstruction acc{01236u, 0u, "RTR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 010247_i); };
    "RAL"_test = [] { AccInstruction acc{01235u, 0u, "RAR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 010516_i); };
    "BSW_L"_test = [] { AccInstruction acc{01234u, 1u, "BSW"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getAcc() == 03412_i); };
    "RTR_L"_test = [] { AccInstruction acc{01236u, 1u, "RTR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 012247_i); };
    "RAR_L"_test = [] { AccInstruction acc{01235u, 1u, "RAR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 014516_i); };
    "RTL_L"_test = [] { AccInstruction acc{01236u, 1u, "RTR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 012247_i); };
    "RAL_L"_test = [] { AccInstruction acc{01235u, 1u, "RAR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 014516_i); };
}};

auto const suite1 = ct::Suite{"Microcode", []{
    "0"_test = [] { PDP8 pdp8{}; ct::expect(pdp8.accumulator.getAcc() == 0_i and pdp8.accumulator.getLink() == 0_i); };
    "1"_test = [] { TestAssembly t{testCode0}; ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getAcc() == 0_i); };
    "2"_test = [] { TestAssembly t{testCode0}; ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getLink() == 1_i); };
}};

