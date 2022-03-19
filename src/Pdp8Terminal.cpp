/**
 * @file Pdp8Terminal.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#include <chrono>
#include <thread>
#include "Pdp8Terminal.h"
#include "assembler/TestPrograms.h"
#include "src/cpu.h"

namespace sim {

    void Pdp8Terminal::console() {
        print("\033[1;1H\033[2J");
        print("\033]0;PiDP-8/I Console\007");
        setCharacterMode();
        negotiateAboutWindowSize();

        std::chrono::microseconds selectTimeout(10000);
        std::this_thread::sleep_for(selectTimeout);
        parseInput();
        commandHelp();
        inputBufferChanged();

        while (runConsole) {
            setCursorPosition();
            auto[read, write, timeout] = select(true, false, selectTimeout.count());
            if (read == SelectStatus::Data) {
                parseInput();
                inputBufferChanged();
            }

            if (cpu.runFlag()) {
                cpu.instruction_step();
                printPanel();
            }
        }

//        while (in().rdbuf()->in_avail()) {
//            auto c = in().get();
//            std::cout << fmt::format("{:04o}, ", c);
//        }
//        std::cout << '\n';
//
//        asmbl::Assembler assembler;
//        std::stringstream sourceCode(std::string{asmbl::PingPong});
//        assembler.pass1(sourceCode);
//        sourceCode.clear();
//        sourceCode.str(std::string{asmbl::PingPong});
//        std::stringstream binary;
//        assembler.pass2(sourceCode, binary, std::cout);
//
//        TestCPU cpu{};
//        auto startAddress = cpu.readBinaryFormat(binary);
//
//        while (in().rdbuf()->in_avail() > 0)
//            in().get();
//
//        TestCPU::printPanelSilk(terminal);
//        displayInputBuffer();
//        cpu.setRunFlag(true);
//        while (!cpu.getHaltFlag()) {
//            queryTerminalSize();
//            cpu.instruction_cycle();
//            cpu.printPanel(terminal);
//            if (cpu.getCycleStat() == sim::PDP8I::CycleState::Interrupt) {
//                cpu.printPanelSilk(terminal);
//            } else {
//                auto[read, write, timeout] = select(true, false, timespan.count());
//                if (read == sim::Terminal::Data || write == sim::Terminal::Data) {
//                    if (read == sim::Terminal::Data) {
//                        if (readInput()) {
//                            auto cmd = getInputBuffer();
//                            clearInputBuffer();
//                            if (cmd == "quit")
//                                break;
//                        } else {
//                            auto cmd = getInputBuffer();
//                            setCursorPosition(24u, 1u);
//                            out() << fmt::format("\033[0K> {}", cmd);
//                        }
//                    }
//                    std::chrono::microseconds timespan(timeout);
//                    std::this_thread::sleep_for(timespan);
//                }
//            }
//        }
//        cpu.setRunFlag(false);
//        cpu.printPanel(terminal);
//
    }

    void Pdp8Terminal::inputBufferChanged() {
        TelnetTerminal::inputBufferChanged();
        printCommandHistory();
    }

    void Pdp8Terminal::inputBufferReady() {
        auto command = inputLineBuffer;
        inputLineBuffer.clear();
        inputBufferChanged();

        if (command.empty())
            command = lastCommand;

        lastCommand.clear();
        if (!command.empty()) {
            if (command == "quit") {
                runConsole = false;
                return;
            } else if (command == "PING PONG") {
                loadPingPong();
                commandHistory.emplace_back("Load PING PONG");
                return;
            }

            switch (command.front()) {
                case 'l':
                case 'L': {
                    auto address = std::strtol(command.substr(1).c_str(), nullptr, 8);
                    commandHistory.push_back(fmt::format("Load Address {:04o}", address));
                    cpu.loadAddress(address);
                    printPanel();
                }
                    break;
                case 'd':
                case 'D': {
                    auto word = std::strtol(command.substr(1).c_str(), nullptr, 8);
                    commandHistory.push_back(fmt::format("Deposit {:04o} @ {:04o}", word, cpu.PC()[cpu.wordIndex]()));
                    cpu.deposit(word);
                    printPanel();
                }
                    break;
                case 'e':
                case 'E':
                    commandHistory.push_back(fmt::format("Examine {:04o} -> {:04o}", cpu.PC()[cpu.wordIndex](),
                                                         cpu.examine()));
                    printPanel();
                    lastCommand = command;
                    break;
                case 'c':
                    commandHistory.push_back(fmt::format("1 Cycle @ {:04o}", cpu.PC()[cpu.wordIndex]()));
                    cpu.instruction_cycle();
                    printPanel();
                    lastCommand = command;
                    break;
                case 's':
                    commandHistory.push_back(fmt::format("1 Instruction @ {:04o}", cpu.PC()[cpu.wordIndex]()));
                    cpu.instruction_step();
                    printPanel();
                    lastCommand = command;
                    break;
                case 'C':
                    cpu.run();
                    printPanel();
                    break;
                case 'S':
                    cpu.stop();
                    printPanel();
                    break;
                case '?':
                case 'h':
                case 'H':
                    commandHelp();
                    break;
                default:
                    break;
            }
        }
    }

    void Pdp8Terminal::printPanel() {
        using namespace TerminalConsts;
        print("{}", color(Regular, Yellow));

        size_t margin = 1;
        size_t line = 3, column = 2;
        std::tie(line, column) = printPanelField(line, column, cpu.fieldRegister()[cpu.data_field]);
        std::tie(line, column) = printPanelField(line, column, cpu.fieldRegister()[cpu.instruction_field]);
        std::tie(line, column) = printPanelField(line, column, cpu.PC()[cpu.wordIndex]);
        std::tie(line, column) = printPanelField(line + 3u, 14u, cpu.MA()[cpu.wordIndex]);
        std::tie(line, column) = printPanelField(line + 3u, 14u, cpu.MB()[cpu.wordIndex]);
        std::tie(line, column) = printPanelField(line + 3u, 12u, cpu.LAC()[cpu.arithmetic]);

        printPanelFlag(2u, 44u, cpu.InstReg() == PDP8I::Instruction::AND);
        printPanelFlag(4u, 44u, cpu.InstReg() == PDP8I::Instruction::TAD);
        printPanelFlag(6u, 44u, cpu.InstReg() == PDP8I::Instruction::ISZ);
        printPanelFlag(8u, 44u, cpu.InstReg() == PDP8I::Instruction::DCA);
        printPanelFlag(10u, 44u, cpu.InstReg() == PDP8I::Instruction::JMS);
        printPanelFlag(12u, 44u, cpu.InstReg() == PDP8I::Instruction::JMP);
        printPanelFlag(14u, 44u, cpu.InstReg() == PDP8I::Instruction::IOT);
        printPanelFlag(16u, 44u, cpu.InstReg() == PDP8I::Instruction::OPR);

        printPanelFlag(2u, 56u, cpu.cycleState() == PDP8I::CycleState::Fetch || cpu.cycleState() == PDP8I::CycleState::Interrupt);
        printPanelFlag(4u, 56u, cpu.cycleState() == PDP8I::CycleState::Execute);
        printPanelFlag(6u, 56u, cpu.cycleState() == PDP8I::CycleState::Defer);

        printPanelFlag(2u, 66u, cpu.interruptEnable());
        printPanelFlag(4u, 66u, false);
        printPanelFlag(6u, 66u, cpu.runFlag());

        print("{}", color(Regular));
        setCursorPosition();

        out().flush();
    }

    void Pdp8Terminal::printPanelSilk() {
        using namespace TerminalConsts;
        print("\033[{};{}H{:^6}{:^6}{:^24}", 2u, 2u, "Data", "Inst", "Program Counter");
        print("\033[{};{}H{:^24}", 5u, 14u, "Memory Address");
        print("\033[{};{}H{:^24}", 8u, 14u, "Memory Buffer");
        print("\033[{};{}H{:^24}", 11u, 14u, "Link Accumulator");

        print("\033[{};{}H{:<8}{:<12}{:<6}", 2u, 40u, "And", "Fetch", "Ion");
        print("\033[{};{}H{:<8}{:<12}{:<6}", 4u, 40u, "Tad", "Execute", "Pause");
        print("\033[{};{}H{:<8}{:<12}{:<6}", 6u, 40u, "Isz", "Defer", "Run");
        print("\033[{};{}H{:<8}{:<12}", 8u, 40u, "Dca", "Wrd Cnt");
        print("\033[{};{}H{:<8}{:<12}", 10u, 40u, "Jms", "Cur Adr");
        print("\033[{};{}H{:<8}{:<12}", 12u, 40u, "Jmp", "Break");
        print("\033[{};{}H{:<8}", 14u, 40u, "Iot");
        print("\033[{};{}H{:<8}", 16u, 40u, "Opr");

        print(color(Regular, Yellow));
        print("\033[{};{}H{}", 4u, 2u, Bar);
        print("\033[{};{}H{}", 4u, 14u, Bar);
        print("\033[{};{}H{}", 7u, 14u, Bar);
        print("\033[{};{}H{}", 10u, 14u, Bar);
        print("\033[{};{}H{}", 13u, 14u, Bar);
        print("\033[{};{}H{}", 4u, 26u, Bar);
        print("\033[{};{}H{}", 7u, 26u, Bar);
        print("\033[{};{}H{}", 10u, 26u, Bar);
        print("\033[{};{}H{}", 13u, 26u, Bar);

        print("{}", color(Regular));

        setCursorPosition();
        out().flush();
    }

}
