/*
 * PDP8.cpp Created by Richard Buckley (C) 17/02/23
 */

/**
 * @file PDP8.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 17/02/23
 */

#include <fmt/format.h>
#include <ranges>
#include "PDP8.h"

namespace pdp8 {

    bool PDP8::readBinaryFormat(std::istream &iStream) {
        bool addressSet{false};
        small_register_t word{};

        auto readWord = [&word](std::istream& s) -> bool {
            char buffer[2];
            s.read(buffer, 2);
            if (s.eof() || s.fail())
                return false;

            word = (((buffer[0] & 0177) << 6) | (buffer[1] & 077)) & 017777;
            return true;
        };

        while (readWord(iStream)) {
            if (word & 010000) {
                memory.programCounter.setProgramCounter(word & 07777);
                addressSet = true;
            } else if (addressSet) {
                memory.deposit(word);
            }
        }

        return addressSet;
    }

    bool PDP8::fetch() {
        instructionReg.value = memory.examine().getData();
        if (memory.memoryBuffer.getInitialized()) {
            switch (static_cast<OpCode>(instructionReg.getOpCode())) {
                case OpCode::IOT:
                case OpCode::OPR:
                    break;
                default:
                    memory.memoryAddress.setWordAddress(instructionReg.getAddress());
                    if (instructionReg.getZeroPage()) {
                        memory.memoryAddress.setPageAddress(0u);
                    }
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Perform the defer stat for indirect addressing.
     * @details The immediate address of the operation points to a memory cell with the intended address. If
     * immediate address is on Page 0 and in the range 010..017 then the contents are incremented before use.
     */
    void PDP8::defer() {
        // Put the immediate address into the memory address register
        memory.memoryAddress.setWordAddress(instructionReg.getAddress());
        // Set the page of the memory address register to 0 if bit 4 of the instruction is 0
        if (instructionReg.getZeroPage()) {
            memory.memoryAddress.setPageAddress(0u);
        }
        memory.read();
        // TestCode for and action autoincrement memory registers.
        if (instructionReg.getZeroPage() && (memory.memoryAddress.getPageWordAddress() & 0170u) == 0010u) {
            memory.memoryBuffer.setData(memory.memoryBuffer.getData()+1);
            memory.write();
        }
        // Set the address in the memory address register.
        memory.memoryAddress.setPageWordAddress(memory.memoryBuffer.getData());
        // Finally set the field address to the data field for the four instructions that used it.
        switch (static_cast<OpCode>(instructionReg.getOpCode())) {
            case OpCode::AND:
            case OpCode::TAD:
            case OpCode::ISZ:
            case OpCode::DCA:
                memory.memoryAddress.setFieldAddress(memory.fieldRegister.getDataField());
                break;
            default:
                break;
        }
    }

    void PDP8::execute() {
        switch (static_cast<OpCode>(instructionReg.getOpCode())) {
            case OpCode::AND:
                accumulator.andOp(memory.read().getData());
                break;
            case OpCode::TAD:
                accumulator.addOp(memory.read().getData());
                break;
            case OpCode::ISZ:
                memory.read();
                memory.memoryBuffer.setData(memory.memoryBuffer.getData() + 1);
                memory.write();
                if (memory.memoryBuffer.getData() == 0)
                    ++memory.programCounter;
                break;
            case OpCode::DCA:
                memory.memoryBuffer.setData(accumulator.getAcc());
                memory.write();
                accumulator.setAcc(0);
                break;
            case OpCode::JMS:
                memory.memoryBuffer.setData(memory.programCounter.getProgramCounter());
                memory.write();
                memory.programCounter.setProgramCounter(memory.memoryAddress.getPageWordAddress() + 1);
                break;
            case OpCode::JMP: {
                bool short_jmp_flag = false;
                if (!instructionReg.getIndirect()) {
                    if ((memory.programCounter.getProgramCounter() - 2) == memory.memoryAddress.getPageWordAddress()) {
                        // JMP .-1
                        auto wait_inst = instructionReg.getWord();
                        if (std::ranges::find(WaitInstructions, wait_inst)) {
                            idle_flag = true; // idle loop detected
                        }
                    } else if ((memory.programCounter.getProgramCounter() - 1) ==
                               memory.memoryAddress.getPageWordAddress()) {
                        // JMP .
                        if (interrupt_enable || interrupt_delayed > 0) {
                            interrupt_enable = true;
                            interrupt_delayed = 0;
                            idle_flag = short_jmp_flag = true;
                        } else {
                            halt_flag = true; // endless loop;
                        }
                    }
                }
                if (!short_jmp_flag) {
                    memory.programCounter.setProgramCounter(memory.memoryAddress.getPageWordAddress());
                    interrupt_deferred = false;
                    memory.fieldRegister.setInstField(memory.fieldRegister.getInstBuffer());
                }
            }
                break;
            case OpCode::IOT:
                execute_iot();
                break;
            case OpCode::OPR:
                execute_opr();
                break;
        }
    }

    void PDP8::instructionStep() {
        switch (cycle_state) {
            case CycleState::Interrupt:
            case CycleState::Fetch:
                fetch();
                if (instructionReg.isIndirectInstruction())
                    cycle_state = CycleState::Defer;
                else
                    cycle_state = CycleState::Execute;
                break;
            case CycleState::Defer:
                defer();
                cycle_state = CycleState::Execute;
                break;
            case CycleState::Execute:
                execute();
                cycle_state = CycleState::Fetch;
                break;
            case CycleState::Pause:
                run_flag = false;
        }
    }

    void PDP8::instructionCycle() {
        while (!halt_flag) {
            fetch();
            if (instructionReg.getIndirect()) {
                defer();
            }
            execute();
        }
    }

    void PDP8::decodeInstruction() const {
        switch (static_cast<OpCode>(instructionReg.getOpCode())) {
            case pdp8::OpCode::IOT:
            case pdp8::OpCode::OPR:
                break;
            default:
                fmt::print("    {} {}{} {:4o}\n", instructionReg.getOpCodeStr(),
                           (instructionReg.getIndirect() ? 'I' : ' '),
                           (instructionReg.getZeroPage() ? 'Z' : ' '),
                           instructionReg.getAddress());
        }

    }

    void PDP8::execute_iot() {
        if (instructionReg.getDeviceSel() == 0u) {
            switch (instructionReg.getDeviceOpr()) {
                case 0: //SKON
                    if (interrupt_enable)
                        ++memory.programCounter;
                    break;
                case 2: //IOF
                    interrupt_enable = false;
                    break;
                case 1: //ION
                    interrupt_delayed = 2;
                    break;
                case 3: //SRQ
                    if (interrupt_request)
                        ++memory.programCounter;
                    break;
                case 4: //GTF
                    accumulator.setAcc(0);
                    accumulator.set<registers::register_t<1,0,12>>(accumulator.getLink());
                    accumulator.set<registers::register_t<1,1,12>>(greater_than_flag ? 1u : 0u);
                    accumulator.set<registers::register_t<1,2,12>>(interrupt_request ? 1u : 0u);
                    accumulator.set<registers::register_t<1,4,12>>(interrupt_enable ? 1u : 0u);
                    accumulator.set<registers::register_t<3,6,12>>(memory.fieldRegister.getInstField());
                    accumulator.set<registers::register_t<3,9,12>>(memory.fieldRegister.getDataField());
                    break;
                case 5: //RTF
                    accumulator.setLink(accumulator.get<registers::register_t<1,0,12>>());
                    greater_than_flag = accumulator.get<registers::register_t<1,1,12>>() != 0;
                    interrupt_request = accumulator.get<registers::register_t<1,2,12>>() != 0;
                    interrupt_enable = accumulator.get<registers::register_t<1,4,12>>() != 0;
                    memory.fieldRegister.setInstField(accumulator.get<registers::register_t<3,6,12>>());
                    memory.fieldRegister.setDataField(accumulator.get<registers::register_t<3,9,12>>());
                    break;
                case 6: // SGT
                    if (greater_than_flag)
                        ++memory.programCounter;
                    break;
                case 7: // CAF
                    memory.programCounter.setProgramCounter(0u);
                    reset();
                    break;
                default:
                    throw std::logic_error("IOT 00 error."); // LCOV_EXCL_LINE
            }
        } else if ((instructionReg.getWord() & 06200) == 06200) {
            if (instructionReg.getWord() & 1) {   // CDF
                memory.fieldRegister.setDataField(instructionReg.getFieldReg());
            }

            if (instructionReg.getWord() & 2) {   // CIF
                memory.fieldRegister.setInstBuffer(instructionReg.getFieldReg());
            }
        }

        auto deviceSel = instructionReg.getDeviceSel();
        auto devOp = instructionReg.getDeviceOpr();
        if (auto device = iotDevices.find(deviceSel); device != iotDevices.end()) {
            device->second->operation(*this, deviceSel, devOp);
        }
        // Other IOT instructions not supported yet.
    }

    void PDP8::execute_opr() {
        auto bits = instructionReg.getOprBits();

        if (bits == 0)  // NOP
            return;

        if ((bits & 0400) == 0) { // Group 1
            // Seq 1
            if (bits & 0200) accumulator.setAcc(0); //acl << cpu_word.clear();
            if (bits & 0100) accumulator.setLink(0); //acl << link.clear();
            // Seq 2
            if (bits & 0040) accumulator.setAcc(~accumulator.getAcc()); //acl << ~acl[cpu_word];
            if (bits & 0020) accumulator.setLink(~accumulator.getLink()); //acl << ~acl[link];
            // Seq 3
            if (bits & 0001) accumulator.setArithmetic(accumulator.getArithmetic()+1); // acl << ++acl[cpu_word];
            // Seq 4
            switch (bits & 016) {
                case 000: // NOP
                    break;
                case 012: // RTR
                    accumulator.setArithmetic((accumulator.getLeastSig() << 12) | (accumulator.getArithmetic() >> 1));
                case 010: // RAR
                    accumulator.setArithmetic((accumulator.getLeastSig() << 12) | (accumulator.getArithmetic() >> 1));
                    break;
                case 006: // RTL
                    accumulator.setArithmetic((accumulator.getArithmetic() << 1) | accumulator.getLink());
                case 004: // RAL
                    accumulator.setArithmetic((accumulator.getArithmetic() << 1) | accumulator.getLink());
                    break;
                case 002: // BSW
                    accumulator.setAcc((accumulator.getLowerNibble() << 6) | accumulator.getUpperNibble());
                    break;
                default: // LCOV_EXCL_LINE
                    throw std::logic_error("Group 1 OPR error."); // LCOV_EXCL_LINE
            }
        } else if ((bits & 0401) == 0400) { // Group 2
            bool skip;
            // Seq 1
            if (bits & 010) {    // SPA, SNA, SZL and
                bool spa{true}, sna{true}, szl{true};
                if (bits & 0100)
                    spa = accumulator.getMostSig() == 0;
                if (bits & 040)
                    sna = accumulator.getAcc() != 0;
                if (bits & 020)
                    szl = accumulator.getLink() == 0;
                skip = spa && sna && szl;
            } else {             // SMA, SZA, SNL
                bool sma{false}, sza{false}, snl{false};
                if (bits & 0100)
                    sma = accumulator.getMostSig() != 0;
                if (bits & 040)
                    sza = accumulator.getAcc() == 0;
                if (bits & 020)
                    snl = accumulator.getLink() != 0;
                skip = sma || sza || snl;
            }
            // Seq 2
            if (bits & 0200)    // CLA
                accumulator.setAcc(0);
            // Seq 3
            if (bits & 04)      // OSR
                accumulator.setAcc(accumulator.getAcc() | opSxReg.value);
            // Seq 4
            if (bits & 02) {    // HLT
                halt_flag = true;
                run_flag = false;
            }
            if (skip)
                ++memory.programCounter;
        } else if ((bits & 0401) == 0401){    // Group 3
            switch (bits & 0721) {
                case 0621: //CAM
                    accumulator.setAcc(0);
                    mulQuotient.setWord(0);
                    break;
                case 0701:
                    accumulator.setAcc(0);
                case 0501: // MQA
                    accumulator.setAcc(accumulator.getAcc() | mulQuotient.getWord());
                    break;
                case 0421: // MQL
                    mulQuotient.setWord(accumulator.getAcc());
                    accumulator.setAcc(0);
                    break;
                case 0521: { // SWP
                    auto acc = accumulator.getAcc();
                    accumulator.setAcc(mulQuotient.getWord());
                    mulQuotient.setWord(acc);
                }
                    break;
            }
        }
    }

    void PDP8::rimLoader() {
        memory.programCounter.setProgramCounter(RIM_LOADER_START);
        for (auto &word: RIM_LOADER) {
            memory.deposit(word);
        }
        memory.programCounter.setProgramCounter(RIM_LOADER_START);
    }

    [[maybe_unused]] void PDP8::reset() {
        accumulator.setArithmetic(0);
        interrupt_delayed = 0u;
        interrupt_enable = false;
        interrupt_deferred = false;
        interrupt_request = false;
        error_flag = false;
        cycle_state = CycleState::Interrupt;
        halt_flag = false;
        run_flag = false;
    }
} // pdp8