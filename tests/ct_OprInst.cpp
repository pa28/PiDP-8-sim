//
// Created by richard on 15/03/23.
//

#include <PDP8.h>
#include <assembler/Assembler.h>
#include "libs/CodeFragmentTest.h"
#include <clean-test/clean-test.h>
#include <numeric>

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
        try {
            if (pass1 = assembler.pass1(); pass1) {
                std::stringstream bin{};
                std::stringstream list{};
                if (pass2 = assembler.pass2(bin, list); pass2) {
                    pdp8.readBinaryFormat(bin);
                    pdp8.instructionCycle();
                }
            }
        } catch (std::invalid_argument &ia) {
            fmt::print("Assembly exception: {}\n", ia.what());
        }
    }

};

static constexpr std::string_view  testCode0{"/ Simple test program.\nOCTAL\n*0200\nCLA CLL CMA IAC\nHLT\n*0200\n"};

static constexpr std::string_view   autoIncCode{R"(
                OCTAL
                *0200
                CLA CLL
                DCA 010
                TAD I 010
                CLA CLL
                TAD 010
                HLT
                *0200
)"};

struct Operate {
    PDP8 pdp8{};
    bool opCode{false};
    std::function<void(Operate&)> setup{};
    explicit operator bool () const { return opCode; }

    template<class Str>
    requires StringLike<Str>
    void initialize(Str opStr, unsigned pc) {
        if (setup)
            setup(*this);

        try {
            if (auto op = pdp8asm::generateOpCode(opStr, pc); op) {
                opCode = op.has_value();
                pdp8.instructionReg.value = op.value();
                pdp8.execute();
            }
        } catch (AssemblyException &ae) {
            fmt::print("Assembly Exception: {}\n", ae.what());
            throw;
        } catch (std::invalid_argument &ia) {
            fmt::print("{}\n", ia.what());
            throw;
        }
    }

    template<class Str>
    requires StringLike<Str>
    explicit Operate(Str opStr, unsigned pc = 0u) {
        initialize(opStr, pc);
    }

    template<class Str>
            requires StringLike<Str>
    Operate(Str opStr, std::function<void(Operate&)> setup, unsigned pc = 0u) : setup(std::move(setup)) {
        initialize(opStr, pc);
    }
};

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

struct AutoIncTest {
    PDP8 pdp8{};
    bool opCode{false};
    bool result{false};

    explicit operator bool () const { return opCode; }
    explicit AutoIncTest(unsigned int addr) {
        addr = addr & 07 | 010;
        auto opStr = fmt::format("TAD I 0{:o}", addr);
        if (auto op = pdp8asm::generateOpCode(opStr, 0u); op) {
            opCode = op.has_value();
            pdp8.instructionReg.value = op.value();
            pdp8.defer();
            result = pdp8.memory.read(0u, addr).getData() == 1u;
        }
    }
};

std::ostream& operator<<(std::ostream& strm, const AccInstruction& acc) {
    return strm << fmt::format("[{}, {}-{:o}]", acc.opCode, acc.pdp8.accumulator.getLink(), acc.pdp8.accumulator.getAcc());
}

#if 1

auto const suite0 = ct::Suite{"ACC Link", []{
    "CLA"_test = [] { AccInstruction acc{01234u, 0u, "CLA OSR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getAcc() == 0_i); };
    "BSW"_test = [] { AccInstruction acc{01234u, 0u, "BSW"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getAcc() == 03412_i); };
    "RTR"_test = [] { AccInstruction acc{01236u, 0u, "RTR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 010247_i); };
    "RAR"_test = [] { AccInstruction acc{01235u, 0u, "RAR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 010516_i); };
    "RTL"_test = [] { AccInstruction acc{01236u, 0u, "RTL"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 05170_i); };
    "RAL"_test = [] { AccInstruction acc{01235u, 0u, "RAL"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 02472_i); };
    "BSW_L"_test = [] { AccInstruction acc{01234u, 1u, "BSW"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getAcc() == 03412_i); };
    "RTR_L"_test = [] { AccInstruction acc{01236u, 1u, "RTR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 012247_i); };
    "RAR_L"_test = [] { AccInstruction acc{01235u, 1u, "RAR"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 014516_i); };
    "RTL_L"_test = [] { AccInstruction acc{01236u, 1u, "RTL"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 05172_i); };
    "RAL_L"_test = [] { AccInstruction acc{01235u, 1u, "RAL"}; ct::expect(ct::lift(acc) and acc.pdp8.accumulator.getArithmetic() == 02473_i); };
}};

auto const suite1 = ct::Suite{"Microcode", []{
    "0"_test = [] { PDP8 pdp8{}; ct::expect(pdp8.accumulator.getAcc() == 0_i and pdp8.accumulator.getLink() == 0_i); };
    "1"_test = [] { TestAssembly t{testCode0}; ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getAcc() == 0_i); };
    "2"_test = [] { TestAssembly t{testCode0}; ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getLink() == 1_i); };
}};

auto const suite2 = ct::Suite {"Auto Increment", []{
    static auto const data = std::vector<unsigned>{0, 1, 2, 3, 4, 5, 6, 7};
    ct::Test<const std::vector<unsigned>>{"addr", data} = [] (const unsigned a){
        AutoIncTest t{a | 010}; ct::expect(t.opCode and t.result);
    };
}};

auto const suite3 = ct::Suite {"Mem Inst", []{
    "JMP"_test = [] {
        TestAssembly t{"OCTAL\n*0200\nJMP 0202\nHLT\nNOP\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.memory.programCounter.getProgramCounter() == 0204_i);
    };
    "JMP"_test = [] {
        TestAssembly t{"OCTAL\n*0176\n00000\nJMP I 0176\nJMS 0176\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.memory.programCounter.getProgramCounter() == 0202_i);
    };
    "JMS"_test = [] {
        TestAssembly t{"OCTAL\n*0176\n00000\nHLT\nJMS 0176\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.memory.programCounter.getProgramCounter() == 0200_i);
    };
    "DCA"_test = [] {
        TestAssembly t{"OCTAL\n*0177\n00000\nCLA CMA\nDCA 0177\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getAcc() == 0_i and
            t.pdp8.memory.read(0u, 0177u).getData() == 07777u);
    };
    "ISZ"_test = [] {
        TestAssembly t{"OCTAL\n*0177\n07777\nISZ 0177\nHLT\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.memory.programCounter.getProgramCounter() == 00203_i);
    };
    "ISZ"_test = [] {
        TestAssembly t{"OCTAL\n*0177\n07776\nISZ 0177\nHLT\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.memory.programCounter.getProgramCounter() == 00202_i);
    };
    "AND"_test = [] {
        TestAssembly t{"OCTAL\n*0177\n01234\nCLA CMA\nAND 0177\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getAcc() == 01234_i);
    };
    "AND"_test = [] {
        TestAssembly t{"OCTAL\n*0177\n01234\nCLA\nAND 0177\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getAcc() == 0_i);
    };
    "TAD"_test = [] {
        TestAssembly t{"OCTAL\n*0177\n00001\nCLA CMA CLL\nTAD 0177\nHLT\n*0200\n"};
        ct::expect(t.pass1 and t.pass2 and t.pdp8.accumulator.getAcc() == 0_i and t.pdp8.accumulator.getLink() == 1_i);
    };
}};

auto const suite4 = ct::Suite {"Cycle Function", []{
    "Fetch"_test = [] {
        PDP8 pdp8{}; ct::expect(!ct::lift(pdp8.fetch())); // Test for condition fetch() uninitialized memory.
    };
}};

auto const suite5 = ct::Suite { "Special JMP", []{
    "Jmp ."_test = [] {
        PDP8 pdp8{};
        pdp8.interrupt_enable = false;
        pdp8.interrupt_delayed = 2;
        if (auto op = pdp8asm::generateOpCode("JMP .", 0200u); op) {
            // Simulate fetch from 0200
            pdp8.memory.memoryAddress.setPageWordAddress(0200u);
            pdp8.memory.programCounter.setProgramCounter(0201u);
            pdp8.instructionReg.value = op.value();
            pdp8.execute();
            ct::expect(ct::lift(pdp8.idle_flag));
        }
    };
    "Jmp ."_test = [] {
        PDP8 pdp8{};
        pdp8.interrupt_enable = false;
        pdp8.interrupt_delayed = 0;
        if (auto op = pdp8asm::generateOpCode("JMP .", 0200u); op) {
            // Simulate fetch from 0200
            pdp8.memory.memoryAddress.setPageWordAddress(0200u);
            pdp8.memory.programCounter.setProgramCounter(0201u);
            pdp8.instructionReg.value = op.value();
            pdp8.execute();
            ct::expect(ct::lift(pdp8.halt_flag));
        }
    };
    "JMP .-1"_test = [] {
        Assembler assembler{};
        PDP8 pdp8{};
        bool pass1{false}, pass2{false};

        std::stringstream testCode{std::string("OCTAL\n*0200\nKSF\nJMP 0200\nHLT\n*0201\n")};
        assembler.readProgram(testCode);
        try {
            if (pass1 = assembler.pass1(); pass1) {
                std::stringstream bin{};
                std::stringstream list{};
                if (pass2 = assembler.pass2(bin, list); pass2) {
                    pdp8.readBinaryFormat(bin);
                    pdp8.fetch();
                    pdp8.execute();
                }
            }
        } catch (std::invalid_argument &ia) {
            fmt::print("Assembly exception: {}\n", ia.what());
        }

        ct::expect(pass1 and pass2 and ct::lift(pdp8.idle_flag));
    };
}};

auto const suite6 = ct::Suite { "CPU", [] {
    "reset"_test = [] {
        PDP8 pdp8{};
        pdp8.accumulator.setArithmetic(017777);
        pdp8.interrupt_delayed = 2u;
        pdp8.interrupt_enable = true;
        pdp8.interrupt_deferred = true;
        pdp8.interrupt_request = true;
        pdp8.error_flag = true;
        pdp8.cycle_state = PDP8::CycleState::Fetch;
        pdp8.halt_flag = true;
        pdp8.run_flag = true;

        pdp8.reset();
        ct::expect(pdp8.accumulator.getArithmetic() == 0_i and pdp8.interrupt_delayed == 0);
        ct::expect(!pdp8.interrupt_enable);
        ct::expect(!pdp8.interrupt_deferred);
        ct::expect(!pdp8.interrupt_request);
        ct::expect(!pdp8.error_flag);
        ct::expect(!pdp8.halt_flag);
        ct::expect(!pdp8.run_flag);
        ct::expect(pdp8.cycle_state == PDP8::CycleState::Interrupt);
    };

}};

auto const suite7 = ct::Suite { "Interrupt", [] {
    "SKON"_test = []{ Operate o("SKON"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SKON"_test = []{ Operate o("SKON", [](Operate& opr){
        opr.pdp8.interrupt_enable = true;});
        ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SRQ"_test = []{ Operate o("SRQ"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SRQ"_test = []{ Operate o("SRQ", [](Operate& opr){
        opr.pdp8.interrupt_request = true;});
        ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "ION"_test = []{ Operate o("ION"); ct::expect(o.opCode and ct::lift(o.pdp8.interrupt_delayed == 2_i));};
    "IOF"_test = []{ Operate o("IOF", [](Operate& opr){
        opr.pdp8.interrupt_enable = true;
    }); ct::expect(o.opCode and ct::lift(!o.pdp8.interrupt_enable)); };
    // GTF - Get Flags
    "GTF_L"_test = []{ Operate o("GTF", [](Operate& opr){
        opr.pdp8.accumulator.setLink(1);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getAcc() == 04000_i));};
    "GTF_GT"_test = []{ Operate o("GTF", [](Operate& opr){
        opr.pdp8.greater_than_flag = true;
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getAcc() == 02000_i));};
    "GTF_IR"_test = []{ Operate o("GTF", [](Operate& opr){
        opr.pdp8.interrupt_request = true;
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getAcc() == 01000_i));};
    "GTF_GT"_test = []{ Operate o("GTF", [](Operate& opr){
        opr.pdp8.interrupt_enable = true;
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getAcc() == 00200_i));};
    "GTF_IF"_test = []{ Operate o("GTF", [](Operate& opr){
        opr.pdp8.memory.fieldRegister.setInstField(07u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getAcc() == 00070_i));};
    "GTF_DF"_test = []{ Operate o("GTF", [](Operate& opr){
        opr.pdp8.memory.fieldRegister.setDataField(07u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getAcc() == 00007_i));};
    // RTF - Restore Flags
    "RTF_L"_test = []{ Operate o("RTF", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(04000u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.accumulator.getLink() == 1_i));};
    "RTF_GT"_test = []{ Operate o("RTF", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(02000u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.greater_than_flag));};
    "RTF_IR"_test = []{ Operate o("RTF", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(01000u);
    }); ct::expect(o.opCode and ct::lift(!o.pdp8.interrupt_request));};
    "RTF_IE"_test = []{ Operate o("RTF", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(00200u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.interrupt_deferred) and ct::lift(o.pdp8.interrupt_delayed) == 2u);};
    "RTF_IF"_test = []{ Operate o("RTF", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(00070u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.memory.fieldRegister.getInstBuffer() == 07_i));};
    "RTF_DF"_test = []{ Operate o("RTF", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(00007u);
    }); ct::expect(o.opCode and ct::lift(o.pdp8.memory.fieldRegister.getDataField() == 00007_i));};
    "SGT_T"_test = []{ Operate o("SGT", [](Operate& opr){
        opr.pdp8.greater_than_flag = true;
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i);};
    "SGT_F"_test = []{ Operate o("SGT", [](Operate& opr){
        opr.pdp8.greater_than_flag = false;
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i);};
    "CAF"_test = []{ Operate o("CAF"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0000_i); };
    "CDF"_test = []{ Operate o("CDF 070"); ct::expect(o.opCode and o.pdp8.memory.fieldRegister.getDataField() == 07_i); };
    "CIF"_test = []{ Operate o("CIF 070"); ct::expect(o.opCode and o.pdp8.memory.fieldRegister.getInstBuffer() == 07_i); };
}};

auto const suite8 = ct::Suite { "Bool Skip", [] {
    "SPA_T"_test = [] { Operate o("SPA"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SNA_F"_test = [] { Operate o("SNA"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SPA_F"_test = [] { Operate o("SPA", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(01000u);
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SNA_T"_test = [] { Operate o("SNA", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(01000u);
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SZL_T"_test = [] { Operate o("SZL"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SZL_F"_test = [] { Operate o("SZL", [](Operate& opr){
        opr.pdp8.accumulator.setLink(01u);
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SZA_T"_test = [] { Operate o("SZA"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SZA_F"_test = [] { Operate o("SZA", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(01u);
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SNL_F"_test = [] { Operate o("SNL"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SNL_T"_test = [] { Operate o("SNL", [](Operate& opr){
        opr.pdp8.accumulator.setLink(01u);
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
    "SMA_F"_test = [] { Operate o("SMA"); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0200_i); };
    "SMA_T"_test = [] { Operate o("SMA", [](Operate& opr){
        opr.pdp8.accumulator.setAcc(04000u);
    }); ct::expect(o.opCode and o.pdp8.memory.programCounter.getProgramCounter() == 0201_i); };
}};

#endif

auto const suite9 = ct::Suite{ "RIM Loader", [] {
    ct::Test{"Code", PDP8::RIM_LOADER} = [](std::pair<uint16_t,uint16_t> const &n) {
        PDP8 pdp8{};
        pdp8.rimLoader();
        pdp8.memory.programCounter.setProgramCounter(n.first);
        auto e = pdp8.memory.examine().getData();
        auto d = n.second;
        ct::expect(ct::lift(e) == d);
    };
}};