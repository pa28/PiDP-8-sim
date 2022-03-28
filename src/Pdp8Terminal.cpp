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
        print("\033c").flush();
        print("\033[1;1H").flush();
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
            auto[timeoutRemainder, selectResults] = selectOnAll(selectTimeout);
            for (auto const &selectResult: selectResults) {
                if (selectResult.listIndex < 0) {
                    if (selectResult.selectRead || selectResult.selectWrite) {
                        selected(selectResult.selectRead, selectResult.selectWrite);
                    }
                } else {
                    if (selectResult.selectRead) {
                        if (selectResult.selectRead || selectResult.selectWrite) {
                            auto c = managedTerminals[selectResult.listIndex].terminal->
                                    selected(selectResult.selectRead, selectResult.selectWrite);
                            if (c == EOF) {
                                managedTerminals[selectResult.listIndex].disconnected = true;
                            }
                        }
                    }
                }
            }

            auto removeCount = std::count_if(managedTerminals.begin(), managedTerminals.end(),
                                             [](const TelnetTerminalSet &t) {
                                                 return t.disconnected;
                                             });

            if (removeCount) {
                managedTerminals.erase(std::remove_if(managedTerminals.begin(), managedTerminals.end(),
                                                      [](const TelnetTerminalSet &t) {
                                                          return t.disconnected;
                                                      }), managedTerminals.end());
            }

            if (cpu.runFlag()) {
                cpu.instruction_step();
                printPanel();
                auto pc = cpu.PC()[cpu.wordIndex]();
                auto fpc = cpu.readCore(0, 010)[cpu.wordIndex]();
                auto fop = cpu.readCore(0, fpc)[cpu.wordIndex]();
                std::cout << fmt::format("{:04o} {:04o} {:04o}\n", pc, fpc, fop);
                if (cpu.PC()[cpu.wordIndex]() == 0245)
                    cpu.stop();
            }

            std::this_thread::sleep_for(timeoutRemainder);
        }
    }

    int Pdp8Terminal::selected(bool selectedRead, bool selectedWrite) {
        if (selectedRead) {
            parseInput();
            inputBufferChanged();
        }

        return 0;
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
            } else if (command == "FORTH") {
                loadForth();
                commandHistory.emplace_back("Load FORTH");
                return;
            } else if (command == "RIM") {
                cpu.rimLoader();
                printPanel();
                return;
            }

            switch (command.front()) {
                case 'l':
                case 'L': {
                    if (auto address = parseArgument(command.substr(1)); address) {
                        commandHistory.push_back(command);
                        cpu.loadAddress(address.value());
                    }
                    printPanel();
                }
                    break;
                case 'd':
                case 'D': {
                    if (auto code = parseArgument(command.substr(1)); code) {
                        commandHistory.push_back(command);
                        cpu.deposit(code.value());
                    }
                    printPanel();
                }
                    break;
                case 'E': {
                    if (auto pc = parseArgument(command.substr(1)); pc) {
                        auto word = cpu.examineAt(pc.value());
                        commandHistory.push_back(fmt::format("Examine {:04o} -> {:04o}", pc.value(), word));
                    }
                }
                    break;
                case 'e': {
                    auto pc = cpu.PC()[cpu.wordIndex]();
                    auto code = cpu.examine();
                    commandHistory.push_back(fmt::format("Examine {:04o} -> {:04o}", pc, code));
                }
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
        std::tie(line, column) = printPanelField(line + 3u, 2u, cpu.SC()[cpu.step_counter_index]);
        std::tie(line, column) = printPanelField(line, 14u, cpu.MQ()[cpu.wordIndex]);

        printPanelFlag(2u, 44u, cpu.InstReg() == PDP8I::Instruction::AND);
        printPanelFlag(4u, 44u, cpu.InstReg() == PDP8I::Instruction::TAD);
        printPanelFlag(6u, 44u, cpu.InstReg() == PDP8I::Instruction::ISZ);
        printPanelFlag(8u, 44u, cpu.InstReg() == PDP8I::Instruction::DCA);
        printPanelFlag(10u, 44u, cpu.InstReg() == PDP8I::Instruction::JMS);
        printPanelFlag(12u, 44u, cpu.InstReg() == PDP8I::Instruction::JMP);
        printPanelFlag(14u, 44u, cpu.InstReg() == PDP8I::Instruction::IOT);
        printPanelFlag(16u, 44u, cpu.InstReg() == PDP8I::Instruction::OPR);
        print("  Managed terms: {:02}", managedTerminals.size());

        printPanelFlag(2u, 56u, cpu.cycleState() == PDP8I::CycleState::Fetch ||
                                cpu.cycleState() == PDP8I::CycleState::Interrupt);
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
        print("\033[{};{}H{:^10}", 14u, 2u, "Step Cnt");
        print("\033[{};{}H{:^24}", 14u, 14u, "Multiplier Quotient");

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
        print("\033[{};{}H{}", 4u, 26u, Bar);
        print("\033[{};{}H{}", 7u, 14u, Bar);
        print("\033[{};{}H{}", 7u, 26u, Bar);
        print("\033[{};{}H{}", 10u, 14u, Bar);
        print("\033[{};{}H{}", 10u, 26u, Bar);
        print("\033[{};{}H{}", 13u, 14u, Bar);
        print("\033[{};{}H{}", 13u, 26u, Bar);
        print("\033[{};{}H{}", 16u, 6u, Bar);
        print("\033[{};{}H{}", 16u, 14u, Bar);
        print("\033[{};{}H{}", 16u, 26u, Bar);

        print("{}", color(Regular));

        setCursorPosition();
        out().flush();
    }

    void Pdp8Terminal::loadSourceStream(std::istream &sourceCode, const std::string &title) {
        assembler.pass1(sourceCode);
        sourceCode.clear();
        sourceCode.seekg(0);
        std::stringstream binary;
        std::ostream nullStrm(&nullStreamBuffer);

        managedTerminals.emplace_back();

        managedTerminals.back().terminal->print("\033c");
        managedTerminals.back().terminal->print("\033[1;1H");
        managedTerminals.back().terminal->print("\033]0;PiDP-8/I Source Listing {}\007", title).flush();

        assembler.pass2(sourceCode, binary, managedTerminals.back().terminal->out());

        assembler.dump_symbols(managedTerminals.back().terminal->out());
        managedTerminals.back().terminal->out().flush();
        auto startAddress = cpu.readBinaryFormat(binary);
        printPanel();
    }

    void Pdp8Terminal::loadPingPong() {
        assembler.clear();
        std::stringstream sourceCode(std::string{pdp8asm::PingPong});
        loadSourceStream(sourceCode, "Ping Pong");
    }

    void Pdp8Terminal::loadForth() {
        try {
            assembler.clear();
            std::stringstream sourceCode(std::string{pdp8asm::Forth});
            loadSourceStream(sourceCode, "Fourth");
        } catch (const pdp8asm::AssemblerAbort& aa) {
            commandHistory.emplace_back(aa.what());
        }
    }


    void Pdp8Terminal::printCommandHistory() {
        if (commandHistory.empty())
            return;

        while (commandHistory.size() > 43 - 18)
            commandHistory.erase(commandHistory.begin());

        unsigned long availableLines = termHeight - 1 - 18;
        unsigned long startLine = 18;
        if (commandHistory.size() < availableLines)
            startLine = startLine + availableLines - commandHistory.size();

        auto itr = commandHistory.begin();
        if (commandHistory.size() > availableLines) {
            itr += static_cast<int>(commandHistory.size() - availableLines);
        }

        for (; itr != commandHistory.end(); ++itr, ++startLine) {
            print("\033[{};{}H{:<80}\n", startLine, 1, *itr);
        }
        setCursorPosition();
        out().flush();
    }

    void Pdp8Terminal::commandHelp() {
        for (auto help: CommandLineHelp) {
            commandHistory.emplace_back(help);
        }
    }

    std::optional<register_type> Pdp8Terminal::parseArgument(const std::string &argument) {
        try {
            return std::stoul(argument, nullptr, 8);

        } catch (const std::invalid_argument &ia) {
            commandHistory.push_back(fmt::format("Error: invalid argument: {}", argument));
//            try {
//                std::string arg(argument.substr(argument.find_first_not_of(' ')));
//                assembler.setNumberRadix(pdp8asm::Assembler::Radix::OCTAL);
//                if (auto value = assembler.find_symbol(arg); value) {
//                    return value.value();
//                } else if (auto cmd = assembler.parse_command(arg, cpu.PC()[cpu.wordIndex]())) {
//                    return cmd.value();
//                }
//                assembler.setNumberRadix(pdp8asm::Assembler::Radix::AUTOMATIC);
//                commandHistory.push_back(fmt::format("Error: argument not octal number, no symbol found: {}", arg));
//            } catch (const pdp8asm::AssemblerException &ae) {
//                commandHistory.emplace_back(ae.what());
//            }
        } catch (const std::out_of_range &oor) {
            commandHistory.push_back(fmt::format("Error: argument out of range: {}", argument));
            return std::nullopt;
        }
        return std::nullopt;
    }

    std::tuple<std::chrono::microseconds, std::vector<Pdp8Terminal::SelectAllResult>>
    Pdp8Terminal::selectOnAll(std::chrono::microseconds timeoutUs) {

        fd_set readFds, writeFds, exceptFds;
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);
        FD_ZERO(&exceptFds);
        struct timeval timeout{};

        timeout.tv_sec = 0;
        timeout.tv_usec = timeoutUs.count();

        std::vector<SelectAllResult> selectResults{};

        selectResults.emplace_back(-1, ifd, ofd, false, false);

        for (int i = 0; i < managedTerminals.size(); ++i) {
            auto tifd = managedTerminals[i].terminal->getReadFd();
            auto tofd = managedTerminals[i].terminal->getWriteFd();
            selectResults.emplace_back(i, tifd, tofd, false, false);
        }

        for (auto const &selectResult: selectResults) {
            if (selectResult.readFd >= 0) {
                FD_SET(selectResult.readFd, &readFds);
            }

            if (selectResult.writeFd >= 0) {
                FD_SET(selectResult.writeFd, &writeFds);
            }
        }

        if (auto stat = ::select(FD_SETSIZE, &readFds, &writeFds, &exceptFds, &timeout); stat == -1) {
            throw TerminalConnectionException("Call to select failed.");
        } else if (stat > 0) {
            for (auto &selectResult: selectResults) {
                if (FD_ISSET(selectResult.readFd, &readFds))
                    selectResult.selectRead = true;
                if (FD_ISSET(selectResult.writeFd, &writeFds))
                    selectResult.selectWrite = true;
            }
        }

        std::chrono::microseconds timeoutRemainder{timeout.tv_usec};

        return {timeoutRemainder, selectResults};
    }
}
