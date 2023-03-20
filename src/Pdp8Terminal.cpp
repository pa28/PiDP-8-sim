/**
 * @file Pdp8Terminal.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#include <chrono>
#include <thread>
#include <assembler/NullStream.h>
#include "Pdp8Terminal.h"

namespace pdp8 {

    void Pdp8Terminal::initialize() {
        *oStrm << fmt::format("\033c"); oStrm->flush();
        *oStrm << fmt::format("\033[1;1H"); oStrm->flush();
        *oStrm << fmt::format("\033]0;PiDP-8/I Console\007");
        setCharacterMode();
        negotiateAboutWindowSize();

        std::chrono::microseconds selectTimeout(10000);
        std::this_thread::sleep_for(selectTimeout);

        parseInput();
        commandHelp();
        inputBufferChanged();
        initialized = true;
    }

    void Pdp8Terminal::console() {
        if (!initialized)
            initialize();

        setCursorPosition();

        if (pdp8.get_run_flag()) {
            pdp8.instructionStep();
            printPanel();
        }
    }

    int Pdp8Terminal::selected(bool selectedRead, bool ) {
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
                pdp8.terminalManager.closeAll();
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
            } else if (command == "DECW") {
                decWriter();
                commandHistory.emplace_back("Load DECWriter");
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
                        pdp8.memory.deposit(static_cast<Memory::base_type>(code.value()));
                    }
                    printPanel();
                }
                    break;
                case 'E': {
                    if (auto pc = parseArgument(command.substr(1)); pc) {
                        auto word = pdp8.memory.read(static_cast<Memory::base_type>(pdp8.memory.fieldRegister.getInstField()),
                                                     static_cast<Memory::base_type>(pc.value())).getData();
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
                    pdp8.set_step_flag(true);;
                    commandHistory.push_back(fmt::format("1 Cycle @ {:04o}", pdp8.memory.programCounter.getProgramCounter()));
                    while (pdp8.get_step_flag()) {
                        pdp8.instructionStep();
                        printPanel();
                    }
                    lastCommand = command;
                    break;
                case 's':
                    pdp8.set_instruction_flag(true);
                    commandHistory.push_back(fmt::format("1 Instruction @ {:04o}", pdp8.memory.programCounter.getProgramCounter()));
                    while (pdp8.get_instruction_flag()) {
                        pdp8.instructionStep();
                        printPanel();
                    }
                    lastCommand = command;
                    break;
                case 'C':
                    pdp8.set_run_flag(true);
                    break;
                case 'S':
                    pdp8.set_run_flag(false);
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
        *oStrm << fmt::format("{}", color(Regular, Yellow));

        size_t line = 3, column = 2;
        std::tie(line, column) = printPanelField(line, column, 3, static_cast<uint>(pdp8.memory.fieldRegister.getDataField()));
        std::tie(line, column) = printPanelField(line, column, 3, static_cast<uint>(pdp8.memory.fieldRegister.getInstField()));
        std::tie(line, column) = printPanelField(line, column, 12, static_cast<uint>(pdp8.memory.programCounter.getProgramCounter()));
        std::tie(line, column) = printPanelField(line + 3u, 14u, 12, static_cast<uint>(pdp8.memory.memoryAddress.getPageWordAddress()));
        std::tie(line, column) = printPanelField(line + 3u, 14u, 12, static_cast<uint>(pdp8.memory.memoryBuffer.getData()));
        std::tie(line, column) = printPanelField(line + 3u, 12u, 13, static_cast<uint>(pdp8.accumulator.getArithmetic()));
        std::tie(line, column) = printPanelField(line + 3u, 2u, 5, static_cast<uint>(pdp8.stepCounter.value));
        std::tie(line, column) = printPanelField(line, 14u, 12, static_cast<uint>(pdp8.mulQuotient.getWord()));

        printPanelFlag(2u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::AND);
        printPanelFlag(4u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::TAD);
        printPanelFlag(6u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::ISZ);
        printPanelFlag(8u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::DCA);
        printPanelFlag(10u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::JMS);
        printPanelFlag(12u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::JMP);
        printPanelFlag(14u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::IOT);
        printPanelFlag(16u, 44u, static_cast<OpCode>(pdp8.instructionReg.getOpCode()) == OpCode::OPR);
        *oStrm << fmt::format("  Managed terms: {:02}", pdp8.terminalManager.size());

        printPanelFlag(2u, 56u, pdp8.cycle_state == PDP8::CycleState::Fetch ||
                                pdp8.cycle_state == PDP8::CycleState::Interrupt);
        printPanelFlag(4u, 56u, pdp8.cycle_state == PDP8::CycleState::Execute);
        printPanelFlag(6u, 56u, pdp8.cycle_state == PDP8::CycleState::Defer);

        printPanelFlag(2u, 66u, pdp8.interrupt_enable);
        printPanelFlag(4u, 66u, false);
        printPanelFlag(6u, 66u, pdp8.get_run_flag());

        *oStrm << fmt::format("{}", color(Regular));
        setCursorPosition();

        out().flush();
    }

    void Pdp8Terminal::printPanelSilk() {
        using namespace TerminalConsts;
        *oStrm << fmt::format("\033[{};{}H{:^6}{:^6}{:^24}", 2u, 2u, "Data", "Inst", "Program Counter");
        *oStrm << fmt::format("\033[{};{}H{:^24}", 5u, 14u, "Memory Address");
        *oStrm << fmt::format("\033[{};{}H{:^24}", 8u, 14u, "Memory Buffer");
        *oStrm << fmt::format("\033[{};{}H{:^24}", 11u, 14u, "Link Accumulator");
        *oStrm << fmt::format("\033[{};{}H{:^10}", 14u, 2u, "Step Cnt");
        *oStrm << fmt::format("\033[{};{}H{:^24}", 14u, 14u, "Multiplier Quotient");

        *oStrm << fmt::format("\033[{};{}H{:<8}{:<12}{:<6}", 2u, 40u, "And", "Fetch", "Ion");
        *oStrm << fmt::format("\033[{};{}H{:<8}{:<12}{:<6}", 4u, 40u, "Tad", "Execute", "Pause");
        *oStrm << fmt::format("\033[{};{}H{:<8}{:<12}{:<6}", 6u, 40u, "Isz", "Defer", "Run");
        *oStrm << fmt::format("\033[{};{}H{:<8}{:<12}", 8u, 40u, "Dca", "Wrd Cnt");
        *oStrm << fmt::format("\033[{};{}H{:<8}{:<12}", 10u, 40u, "Jms", "Cur Adr");
        *oStrm << fmt::format("\033[{};{}H{:<8}{:<12}", 12u, 40u, "Jmp", "Break");
        *oStrm << fmt::format("\033[{};{}H{:<8}", 14u, 40u, "Iot");
        *oStrm << fmt::format("\033[{};{}H{:<8}", 16u, 40u, "Opr");

        *oStrm << fmt::format("{}", color(Regular, Yellow));
        *oStrm << fmt::format("\033[{};{}H{}", 4u, 2u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 4u, 14u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 4u, 26u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 7u, 14u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 7u, 26u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 10u, 14u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 10u, 26u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 13u, 14u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 13u, 26u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 16u, 6u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 16u, 14u, Bar);
        *oStrm << fmt::format("\033[{};{}H{}", 16u, 26u, Bar);

        *oStrm << fmt::format("{}", color(Regular));

        setCursorPosition();
        out().flush();
    }

    void Pdp8Terminal::loadSourceStream(std::istream &sourceCode, const std::string &title) {
        assembler.readProgram(sourceCode);
        assembler.pass1();
        std::stringstream binary;

        auto terminal = std::make_shared<TelnetTerminal>();
        pdp8.terminalManager.terminalQueue = terminal;

        terminal->out() << fmt::format("\033c");
        terminal->out() << fmt::format("\033[1;1H");
        terminal->out() << fmt::format("\033]0;PiDP-8/I Source Listing {}\007", title);
        terminal->out() << std::flush;

        assembler.pass2(binary, terminal->out());

        assembler.dumpSymbols(terminal->out());
        terminal->out().flush();
        pdp8.readBinaryFormat(binary);
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

    void Pdp8Terminal::decWriter() {
        assembler.clear();
        std::stringstream sourceCode(std::string{pdp8asm::DecWriter});
        loadSourceStream(sourceCode, "DECWriter");
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
            *oStrm << fmt::format("\033[{};{}H{:<80}\n", startLine, 1, *itr);
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
            if (static_cast<size_t>(pos - argument.c_str()) == argument.length())
                return code;
            return pdp8asm::generateOpCode(argument, static_cast<unsigned int>(pdp8.memory.programCounter.getProgramCounter()));
        } catch (const std::invalid_argument &ia) {
            commandHistory.emplace_back(ia.what());
        } catch (const std::out_of_range &oor) {
            commandHistory.push_back(fmt::format("Error: argument out of range: {}", argument));
        }
        return std::nullopt;
    }
}
