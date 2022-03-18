/**
 * @file cpu.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-11
 */

#include "cpu.h"

namespace sim {

    void PDP8I::fetch() {
        // Instruction address in field from program counter
        cpma[wordIndex] = program_counter[wordIndex]();
        // Read memory into the memory buffer
        memory_buffer[wordIndex] = readInstructionCore()[wordIndex]();
        // Pull out the instruction code
        instruction_register = static_cast<Instruction>(memory_buffer[instruction_index]());
        // Increment the program counter
        ++program_counter[wordIndex];

        if (instruction_register < Instruction::IOT) {
            // This instruction address 7 LSB comes from the instruction in the memory buffer
            cpma[addr_index] = memory_buffer[addr_index]();

            // If the Memory Page flag is 0 then the instruction address is on page 0
            // Otherwise it is the same page that the instruction was fetched from.
            if (!memory_buffer[memPageIndex]())
                cpma[page_index] = 0u;
        }
    }

    void PDP8I::defer() {
        if (cpma[page_index]() == 0 && (cpma[addr_index]() & 0170u) == 0010u) {
            writeInstructionCore(readInstructionCore()[wordIndex]() + 1);
        }
        memory_buffer[wordIndex] = readInstructionCore()[wordIndex]();
        cpma[wordIndex] = memory_buffer[wordIndex]();
    }

    void PDP8I::instruction_step() {
        auto cycleToInterruptState = [this]() {
            while (cycleState != CycleState::Interrupt)
                instruction_cycle();
        };

        if (cycleState != CycleState::Interrupt)
            cycleToInterruptState();
        instruction_cycle();
        cycleToInterruptState();
    }

    void PDP8I::instruction_cycle() {
        switch (cycleState) {
            case CycleState::Fetch:
                fetch();
                // sim_time(16us);
                cycleState = instruction_register < Instruction::IOT && memory_buffer[indirectIndex]() ?
                             CycleState::Defer : CycleState::Execute;
                break;
            case CycleState::Defer:
                defer();
                // sim_time(16us);
                cycleState = CycleState::Execute;
                break;
            case CycleState::Execute:
                execute();
                cycleState = CycleState::Interrupt;
                break;
            case CycleState::Interrupt:
                if ((!interrupt_deferred) && interrupt_enable && interrupt_request) {
                    interrupt_buffer = field_register;
                    field_register[instruction_field] = 0u;
                    field_register[data_field] = 0u;
                    interrupt_enable = false;
                    cpma[wordIndex] = 0u;
                    writeInstructionCore(program_counter[wordIndex]());
                    program_counter[wordIndex] = 1u;
                }

                if (interrupt_delayed)
                    interrupt_enable = (--interrupt_delayed) == 0;

                cycleState = CycleState::Fetch;
                break;
            case CycleState::Pause: // LCOV_EXCL_LINE
                break; // LCOV_EXCL_LINE
        }
    }

    void PDP8I::execute() {
        switch (instruction_register) {
            case Instruction::AND:
                memory_buffer[wordIndex] = readDataCore()[wordIndex]();
                link_accumulator[wordIndex] = link_accumulator[wordIndex]() & memory_buffer[wordIndex]();
                break;
            case Instruction::TAD:
                memory_buffer[wordIndex] = readDataCore()[wordIndex]();
                link_accumulator[arithmetic] = link_accumulator[arithmetic]() + memory_buffer[wordIndex]();
                break;
            case Instruction::ISZ:
                memory_buffer[wordIndex] = readDataCore()[wordIndex]();
                ++memory_buffer[wordIndex];
                if (not memory_buffer[wordIndex]())
                    ++program_counter[wordIndex];
                writeDataCore(memory_buffer[wordIndex]());
                break;
            case Instruction::DCA:
                writeDataCore(link_accumulator[wordIndex]());
                link_accumulator[wordIndex] = 0u;
                break;
            case Instruction::JMS:
                writeInstructionCore(program_counter[wordIndex]());
                program_counter[wordIndex] = cpma[wordIndex]();
                ++program_counter[wordIndex];
                interrupt_deferred = false;
                field_register[instruction_field] = instruction_buffer[instruction_field]();
                break;
            case Instruction::JMP:
                if (not memory_buffer[indirectIndex]()) {
                    if ((program_counter[wordIndex]() - 2) == cpma[wordIndex]()) { // JMP .-1
                        auto wait_inst = readInstructionCore();
                        if (wait_inst[wordIndex]() == OP_KSF || wait_inst[wordIndex]() == OP_CLSC) {
                            idle_flag = true;   // idle loop detected
                        }
                    } else if ((program_counter[wordIndex]() - 1) == cpma[wordIndex]()) { // JMP .
                        if (interrupt_enable || interrupt_delayed > 0) {
                            interrupt_enable = true;
                            interrupt_delayed = 0;
                            idle_flag = short_jmp_flag = true;
                        } else {
                            halt_flag = true;   // endless loop
                        }
                    }
                }
                if (short_jmp_flag) {
                    short_jmp_flag = false;
                } else {
                    program_counter[wordIndex] = cpma[wordIndex]();
                    interrupt_deferred = false;
                    field_register[instruction_field] = instruction_buffer[instruction_field]();
                }
                break;
            case Instruction::IOT:
                execute_iot();
                break;
            case Instruction::OPR:
                execute_opr();
                break;
        }
    }

    void PDP8I::execute_opr() {
        auto bits = memory_buffer[opr_bits]();

        if (bits == 0) // NOP
            return;

        if ((bits & 0400) == 0) { // Group 1
            if (bits & 0200) link_accumulator[wordIndex] = 0u; //acl << cpu_word.clear();
            if (bits & 0100) link_accumulator[linkIndex] = 0u; //acl << link.clear();
            if (bits & 0040) link_accumulator[wordIndex] = ~link_accumulator[wordIndex](); //acl << ~acl[cpu_word];
            if (bits & 0020) link_accumulator[linkIndex] = ~link_accumulator[linkIndex](); //acl << ~acl[link];
            if (bits & 0001) ++link_accumulator[wordIndex]; // acl << ++acl[cpu_word];
            switch (bits & 016) {
                case 000: // NOP
                    break;
                case 012: // RTR
                    link_accumulator[arithmetic] = (link_accumulator[leastSignificant]() << 12) |
                                                   (link_accumulator[arithmetic]() >> 1);
                case 010: // RAR
                    link_accumulator[arithmetic] = (link_accumulator[leastSignificant]() << 12) |
                                                   (link_accumulator[arithmetic]() >> 1);
                    break;
                case 006: // RTL
                    link_accumulator[arithmetic] =
                            (link_accumulator[arithmetic]() << 1) | link_accumulator[linkIndex]();
                case 004: // RAL
                    link_accumulator[arithmetic] =
                            (link_accumulator[arithmetic]() << 1) | link_accumulator[linkIndex]();
                    break;
                case 002: // BSW
                    link_accumulator[wordIndex] =
                            (link_accumulator[lowerNibble]() << 6) | link_accumulator[upperNibble]();
                    break;
                default: // LCOV_EXCL_LINE
                    throw std::logic_error("Group 1 OPR error."); // LCOV_EXCL_LINE
            }
        } else if ((bits & 01) == 0) { // Group 2
            bool skip = false;
            switch (bits & 0170) {
                case 0010: // SKP
                    skip = true;
                    break;
                case 0020: // SNL
                    skip = link_accumulator[linkIndex]() != 0;
                    break;
                case 0030: // SZL
                    skip = link_accumulator[linkIndex]() == 0;
                    break;
                case 0040: // SZA
                    skip = link_accumulator[wordIndex]() == 0;
                    break;
                case 0050: // SNA
                    skip = link_accumulator[wordIndex]() != 0;
                    break;
                case 0100: // SMA
                    skip = link_accumulator[mostSignificant]() != 0;
                    break;
                case 0110: // SPA
                    skip = link_accumulator[mostSignificant]() == 0;
                    break;
                case 0000: // Not one of this group.
                    break;
                default: // LCOV_EXCL_LINE
                    throw std::logic_error("Group 2 OPR error."); // LCOV_EXCL_LINE
            }
            if (skip)
                ++program_counter[wordIndex];

            if (bits & 0200) link_accumulator[wordIndex] = 0u; // CLA
            if (bits & 0004) link_accumulator[wordIndex] = link_accumulator[wordIndex]() | switch_register[wordIndex]();
            if (bits & 0002)
                halt_flag = true; // HLT
        } else { // Group 3

        }
    }

    void PDP8I::execute_iot() {
        if (memory_buffer[deviceSelect]() == 0u) {
            switch (memory_buffer[deviceOpr]()) {
                case 0: //SKON
                    if (interrupt_enable)
                        ++program_counter[wordIndex];
                case 2: //IOF
                    interrupt_enable = false;
                    break;
                case 1: //ION
                    interrupt_delayed = 2;
                    break;
                case 3: //SRQ
                    if (interrupt_request)
                        ++program_counter[wordIndex];
                    break;
                case 4: //GTF
                    link_accumulator[wordIndex] = 0u;
                    link_accumulator[bit0] = link_accumulator[linkIndex]();
                    link_accumulator[bit1] = greater_than_flag ? 1u : 0u;
                    link_accumulator[bit2] = interrupt_request ? 1u : 0u;
                    link_accumulator[bit4] = interrupt_enable ? 1u : 0u;
                    link_accumulator[instruction_field] = field_register[instruction_field]();
                    link_accumulator[data_field] = field_register[data_field]();
                    break;
                case 5: //RTF
                    link_accumulator[linkIndex] = link_accumulator[bit0]();
                    greater_than_flag = link_accumulator[bit1]() == 1u;
                    interrupt_request = link_accumulator[bit2]() == 1u;
                    interrupt_delayed = link_accumulator[bit4]() ? 2 : 0;
                    instruction_buffer[instruction_field] = link_accumulator[instruction_field]();
                    field_register[data_field] = link_accumulator[data_field]();
                    break;
                case 6: // SGT
                    if (greater_than_flag)
                        ++program_counter[wordIndex];
                    break;
                case 7: // CAF
                    program_counter[wordIndex] = 0u;
                    cycleState = CycleState::Interrupt;
                    break;
                default:
                    throw std::logic_error("IOT 00 error."); // LCOV_EXCL_LINE
            }
        } else if ((memory_buffer[wordIndex]() & 06200) == 06200) {
            if (memory_buffer[wordIndex]() & 1) {   // CDF
                field_register[data_field] = memory_buffer[instruction_field]();
            }

            if (memory_buffer[wordIndex]() & 2) {   // CIF
                instruction_buffer[instruction_field] = memory_buffer[instruction_field]();
            }
        }
        // Other IOT instructions not supported yet.
    }

    register_value PDP8I::readCore(register_type field, register_type address) {
        register_value word(coreMemory.readCore(field, address));
        return word;
    }

    void PDP8I::writeCore(register_type field, register_type address, register_type data) {
        coreMemory.writeCore(field, address, data);
    }

    void PDP8I::deposit(register_type value) {
        cpma[wordIndex] = program_counter[wordIndex]();
        memory_buffer[wordIndex] = value;
        writeCore(field_register[instruction_field](), cpma[wordIndex](), memory_buffer[wordIndex]());
        ++program_counter[wordIndex];
    }

    register_type PDP8I::examine() {
        cpma[wordIndex] = program_counter[wordIndex]();
        memory_buffer[wordIndex] = readCore(field_register[instruction_field](), cpma[wordIndex]())[wordIndex]();
        ++program_counter[wordIndex];
        return memory_buffer[wordIndex]();
    }

    bool PDP8I::readBinaryFormat(std::istream &istrm) {
        bool addressSet = false;
        register_type word;

        auto readWord = [&word](std::istream &s) -> bool {
            char buffer[2];
            s.read(buffer, 2);
            if (s.eof() || s.fail())
                return false;

            word = (buffer[0] & 0177) << 6 | (buffer[1] & 077);
            return true;
        };

        while (readWord(istrm)) {
            if (word & 010000) {
                program_counter[wordIndex] = word;
                addressSet = true;
            } else if (addressSet) {
                writeCore(field_register[instruction_field](), program_counter[wordIndex](), word);
                ++program_counter[wordIndex];
            }
        }

        return addressSet;
    }

    bool PDP8I::writeBinaryFormat(std::ostream &ostrm, register_type first, register_type last) {
        bool address_set = false;
        bool word_written = false;

        program_counter[wordIndex] = first;
        do {
            auto word = readCore(field_register[instruction_field](), program_counter[wordIndex]());
            bool word_set = word[memorySet]() == 1;
            if (!address_set) {
                if (word_set) {
                    ostrm << static_cast<char>(program_counter[upperNibble]() | 0100u)
                          << static_cast<char>(program_counter[lowerNibble]());
                    address_set = true;
                }
            }

            if (address_set && word_set) {
                ostrm << static_cast<char>(word[upperNibble]()) << static_cast<char>(word[lowerNibble]());
                ++program_counter[wordIndex];
                word_written = true;
            } else
                address_set = false;
        } while (program_counter[wordIndex]() != (last & 07777));
        return word_written;
    }

    void PDP8I::rimLoader() {
        program_counter[wordIndex] = RIM_LOADER_START;
        for (auto &word: RIM_LOADER) {
            register_type instruction(word);
            deposit(instruction);
        }
        program_counter[wordIndex] = RIM_LOADER_START;
    }

    void PDP8I::reset() {
        link_accumulator[arithmetic] = 0u;
        interrupt_delayed = 0u;
        interrupt_enable = false;
        interrupt_deferred = false;
        interrupt_request = false;
        error_flag = false;
        cycleState = CycleState::Interrupt;
        halt_flag = false;
    }

    void PDP8I::printPanel(Terminal &terminal) {
        using namespace TerminalConsts;
        terminal.print("{}", color(Regular, Yellow));

        size_t margin = 1;
        size_t line = 3, column = 2;
        std::tie(line, column) = printPanelField(terminal, line, column, field_register[data_field]);
        std::tie(line, column) = printPanelField(terminal, line, column, field_register[instruction_field]);
        std::tie(line, column) = printPanelField(terminal, line, column, program_counter[wordIndex]);
        std::tie(line, column) = printPanelField(terminal, line + 3u, 14u, cpma[wordIndex]);
        std::tie(line, column) = printPanelField(terminal, line + 3u, 14u, memory_buffer[wordIndex]);
        std::tie(line, column) = printPanelField(terminal, line + 3u, 12u, link_accumulator[arithmetic]);

        printPanelFlag(terminal, 2u, 44u, instruction_register == Instruction::AND);
        printPanelFlag(terminal, 4u, 44u, instruction_register == Instruction::TAD);
        printPanelFlag(terminal, 6u, 44u, instruction_register == Instruction::ISZ);
        printPanelFlag(terminal, 8u, 44u, instruction_register == Instruction::DCA);
        printPanelFlag(terminal, 10u, 44u, instruction_register == Instruction::JMS);
        printPanelFlag(terminal, 12u, 44u, instruction_register == Instruction::JMP);
        printPanelFlag(terminal, 14u, 44u, instruction_register == Instruction::IOT);
        printPanelFlag(terminal, 16u, 44u, instruction_register == Instruction::OPR);

        printPanelFlag(terminal, 2u, 56u, cycleState == CycleState::Fetch || cycleState == CycleState::Interrupt);
        printPanelFlag(terminal, 4u, 56u, cycleState == CycleState::Execute);
        printPanelFlag(terminal, 6u, 56u, cycleState == CycleState::Defer);

        printPanelFlag(terminal, 2u, 66u, interrupt_enable);
        printPanelFlag(terminal, 4u, 66u, false);
        printPanelFlag(terminal, 6u, 66u, run_flag);

        terminal.setCursorPosition(24u, 1u);
        terminal.print("{}", color(Regular));

        terminal.flush();
    }

    void PDP8I::printPanelSilk(Terminal &terminal) {
        using namespace TerminalConsts;
        terminal.setCursorPosition(2u,2u);
        terminal.print("{:^6}{:^6}{:^24}", "Data", "Inst", "Program Counter");
        terminal.setCursorPosition(5u, 14u);
        terminal.print("{:^24}", "Memory Address");
        terminal.setCursorPosition(8u, 14u);
        terminal.print("{:^24}", "Memory Buffer");
        terminal.setCursorPosition(11u, 14u);
        terminal.print("{:^24}", "Link Accumulator");
        terminal.setCursorPosition(14u, 1u);

        terminal.setCursorPosition(2u, 40u);
        terminal.print("{:<8}{:<12}{:<6}", "And", "Fetch", "Ion");
        terminal.setCursorPosition(4u, 40u);
        terminal.print("{:<8}{:<12}{:<6}", "Tad", "Execute", "Pause");
        terminal.setCursorPosition(6u, 40u);
        terminal.print("{:<8}{:<12}{:<6}", "Isz", "Defer", "Run");
        terminal.setCursorPosition(8u, 40u);
        terminal.print("{:<8}{:<12}", "Dca", "Wrd Cnt");
        terminal.setCursorPosition(10u, 40u);
        terminal.print("{:<8}{:<12}", "Jms", "Cur Adr");
        terminal.setCursorPosition(12u, 40u);
        terminal.print("{:<8}{:<12}", "Jmp", "Break");
        terminal.setCursorPosition(14u, 40u);
        terminal.print("{:<8}", "Iot");
        terminal.setCursorPosition(16u, 40u);
        terminal.print("{:<8}", "Opr");

        terminal.print("{}", color(Regular, Yellow));
        terminal.setCursorPosition(4u, 2u); terminal.print(Bar);
        terminal.setCursorPosition(4u, 14u); terminal.print(Bar);
        terminal.setCursorPosition(7u, 14u); terminal.print(Bar);
        terminal.setCursorPosition(10u, 14u); terminal.print(Bar);
        terminal.setCursorPosition(13u, 14u); terminal.print(Bar);
        terminal.setCursorPosition(4u, 26u); terminal.print(Bar);
        terminal.setCursorPosition(7u, 26u); terminal.print(Bar);
        terminal.setCursorPosition(10u, 26u); terminal.print(Bar);
        terminal.setCursorPosition(13u, 26u); terminal.print(Bar);

        terminal.print("{}", color(Regular));
    }
}
