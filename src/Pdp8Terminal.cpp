/**
 * @file Pdp8Terminal.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#include <chrono>
#include "Pdp8Terminal.h"
#include "assembler/Assembler.h"
#include "assembler/TestPrograms.h"
#include "src/cpu.h"

namespace sim {

    void Pdp8Terminal::console() {
        print("\033[1;1H\033[2J");
        print("\033]0;PiDP-8/I Console\007");
        setCharacterMode();
        negotiateAboutWindowSize();

        bool runConsole = true;
        std::chrono::microseconds selectTimeout(10000);
        inputBufferChanged();
        while (runConsole) {
            setCursorPosition();
            auto[read, write, timeout] = select(true, false, selectTimeout.count());
            if (read == SelectStatus::Data) {
                parseInput();
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
//        terminal.displayInputBuffer();
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

}
