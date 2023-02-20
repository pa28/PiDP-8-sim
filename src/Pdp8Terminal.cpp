/**
 * @file Pdp8Terminal.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#include <chrono>
#include <thread>
#include "Pdp8Terminal.h"

namespace pdp8 {

    void Pdp8Terminal::console() {
        ostrm << fmt::format("\033c"); ostrm.flush();
        ostrm << fmt::format("\033[1;1H"); ostrm.flush();
        ostrm << fmt::format("\033]0;PiDP-8/I Console\007");
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

            if (pdp8.run_flag) {
                pdp8.instructionStep();
                printPanel();
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
                pdp8.rimLoader();
                printPanel();
                return;
            }

            switch (command.front()) {
                case 'l':
                case 'L': {
                    if (auto address = parseArgument(command.substr(1)); address) {
                        commandHistory.push_back(command);
                        pdp8.memory.memoryAddress.setPageWordAddress(address.value());
                        pdp8.memory.programCounter.setProgramCounter(address.value());
                    }
                    printPanel();
                }
                    break;
                case 'd':
                case 'D': {
                    if (auto code = parseArgument(command.substr(1)); code) {
                        commandHistory.push_back(command);
                        pdp8.memory.deposit(code.value());
                    }
                    printPanel();
                }
                    break;
                case 'E': {
                    if (auto pc = parseArgument(command.substr(1)); pc) {
                        auto word = pdp8.memory.read(pdp8.memory.fieldRegister.getInstField(), pc.value()).getData();
                        commandHistory.push_back(fmt::format("Examine {:04o} -> {:04o}", pc.value(), word));
                    }
                }
                    break;
                case 'e': {
                    auto pc = pdp8.memory.programCounter.getProgramCounter();
                    auto code = pdp8.memory.examine().getData();
                    commandHistory.push_back(fmt::format("Examine {:04o} -> {:04o}", pc, code));
                }
                    printPanel();
                    lastCommand = command;
                    break;
                case 'c':
                    commandHistory.push_back(fmt::format("1 Cycle @ {:04o}", pdp8.memory.programCounter.getProgramCounter()));
                    pdp8.instructionStep();
                    printPanel();
                    lastCommand = command;
                    break;
                case 's':
                    commandHistory.push_back(fmt::format("1 Instruction @ {:04o}", pdp8.memory.programCounter.getProgramCounter()));
                    pdp8.instructionStep();
                    printPanel();
                    lastCommand = command;
                    break;
                case 'C':
                    pdp8.run_flag = true;
                    printPanel();
                    break;
                case 'S':
                    pdp8.run_flag = false;
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
        ostrm << fmt::format("{}", color(Regular, Yellow));

        size_t margin = 1;
        size_t line = 3, column = 2;
        std::tie(line, column) = printPanelField(line, column, 3, pdp8.memory.fieldRegister.getDataField());
        std::tie(line, column) = printPanelField(line, column, 3, pdp8.memory.fieldRegister.getInstField());
        std::tie(line, column) = printPanelField(line, column, 12, pdp8.memory.programCounter.getProgramCounter());
        std::tie(line, column) = printPanelField(line + 3u, 14u, 12, pdp8.memory.memoryAddress.getPageWordAddress());
        std::tie(line, column) = printPanelField(line + 3u, 14u, 12, pdp8.memory.memoryBuffer.getData());
        std::tie(line, column) = printPanelField(line + 3u, 12u, 13, pdp8.accumulator.getArithmetic());
        std::tie(line, column) = printPanelField(line + 3u, 2u, 5, pdp8.stepCounter.value);
        std::tie(line, column) = printPanelField(line, 14u, 12, pdp8.mulQuotient.getWord());

        printPanelFlag(2u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::AND);
        printPanelFlag(4u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::TAD);
        printPanelFlag(6u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::ISZ);
        printPanelFlag(8u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::DCA);
        printPanelFlag(10u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::JMS);
        printPanelFlag(12u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::JMP);
        printPanelFlag(14u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::IOT);
        printPanelFlag(16u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::OPR);
        ostrm << fmt::format("  Managed terms: {:02}", managedTerminals.size());

        printPanelFlag(2u, 56u, pdp8.cycle_state == PDP8::CycleState::Fetch ||
                                pdp8.cycle_state == PDP8::CycleState::Interrupt);
        printPanelFlag(4u, 56u, pdp8.cycle_state == PDP8::CycleState::Execute);
        printPanelFlag(6u, 56u, pdp8.cycle_state == PDP8::CycleState::Defer);

        printPanelFlag(2u, 66u, pdp8.interrupt_enable);
        printPanelFlag(4u, 66u, false);
        printPanelFlag(6u, 66u, pdp8.run_flag);

        ostrm << fmt::format("{}", color(Regular));
        setCursorPosition();

        out().flush();
    }

    void Pdp8Terminal::printPanelSilk() {
        using namespace TerminalConsts;
        ostrm << fmt::format("\033[{};{}H{:^6}{:^6}{:^24}", 2u, 2u, "Data", "Inst", "Program Counter");
        ostrm << fmt::format("\033[{};{}H{:^24}", 5u, 14u, "Memory Address");
        ostrm << fmt::format("\033[{};{}H{:^24}", 8u, 14u, "Memory Buffer");
        ostrm << fmt::format("\033[{};{}H{:^24}", 11u, 14u, "Link Accumulator");
        ostrm << fmt::format("\033[{};{}H{:^10}", 14u, 2u, "Step Cnt");
        ostrm << fmt::format("\033[{};{}H{:^24}", 14u, 14u, "Multiplier Quotient");

        ostrm << fmt::format("\033[{};{}H{:<8}{:<12}{:<6}", 2u, 40u, "And", "Fetch", "Ion");
        ostrm << fmt::format("\033[{};{}H{:<8}{:<12}{:<6}", 4u, 40u, "Tad", "Execute", "Pause");
        ostrm << fmt::format("\033[{};{}H{:<8}{:<12}{:<6}", 6u, 40u, "Isz", "Defer", "Run");
        ostrm << fmt::format("\033[{};{}H{:<8}{:<12}", 8u, 40u, "Dca", "Wrd Cnt");
        ostrm << fmt::format("\033[{};{}H{:<8}{:<12}", 10u, 40u, "Jms", "Cur Adr");
        ostrm << fmt::format("\033[{};{}H{:<8}{:<12}", 12u, 40u, "Jmp", "Break");
        ostrm << fmt::format("\033[{};{}H{:<8}", 14u, 40u, "Iot");
        ostrm << fmt::format("\033[{};{}H{:<8}", 16u, 40u, "Opr");

        ostrm << fmt::format("{}", color(Regular, Yellow));
        ostrm << fmt::format("\033[{};{}H{}", 4u, 2u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 4u, 14u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 4u, 26u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 7u, 14u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 7u, 26u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 10u, 14u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 10u, 26u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 13u, 14u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 13u, 26u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 16u, 6u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 16u, 14u, Bar);
        ostrm << fmt::format("\033[{};{}H{}", 16u, 26u, Bar);

        ostrm << fmt::format("{}", color(Regular));

        setCursorPosition();
        out().flush();
    }

    void Pdp8Terminal::loadSourceStream(std::istream &sourceCode, const std::string &title) {
        assembler.readProgram(sourceCode);
        assembler.pass1();
        std::stringstream binary;
        std::ostream nullStrm(&nullStreamBuffer);

        managedTerminals.emplace_back();

        managedTerminals.back().terminal->out() << fmt::format("\033c");
        managedTerminals.back().terminal->out() << fmt::format("\033[1;1H");
        managedTerminals.back().terminal->out() << fmt::format("\033]0;PiDP-8/I Source Listing {}\007", title);
        managedTerminals.back().terminal->out() << std::flush;

        assembler.pass2(binary, managedTerminals.back().terminal->out());

        assembler.dumpSymbols(managedTerminals.back().terminal->out());
        managedTerminals.back().terminal->out().flush();
        auto startAddress = pdp8.readBinaryFormat(binary);
        printPanel();
    }

    void Pdp8Terminal::loadPingPong() {
        assembler.clear();
        std::stringstream sourceCode(std::string{pdp8asm::PingPong});
        loadSourceStream(sourceCode, "Ping Pong");
    }

    void Pdp8Terminal::loadForth() {
        assembler.clear();
        std::stringstream sourceCode(std::string{pdp8asm::Forth});
        loadSourceStream(sourceCode, "Fourth");
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
            ostrm << fmt::format("\033[{};{}H{:<80}\n", startLine, 1, *itr);
        }
        setCursorPosition();
        out().flush();
    }

    void Pdp8Terminal::commandHelp() {
        for (auto help: CommandLineHelp) {
            commandHistory.emplace_back(help);
        }
    }

    std::optional<unsigned int> Pdp8Terminal::parseArgument(const std::string &argument) {
        try {
            char* pos;
            auto code = std::strtoul(argument.c_str(), &pos, 8);
            if (pos - argument.c_str() == argument.length())
                return code;
            return pdp8asm::generateOpCode(argument, pdp8.memory.programCounter.getProgramCounter());
        } catch (const std::invalid_argument &ia) {
            commandHistory.emplace_back(ia.what());
        } catch (const std::out_of_range &oor) {
            commandHistory.push_back(fmt::format("Error: argument out of range: {}", argument));
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
