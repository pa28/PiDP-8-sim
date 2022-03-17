#include <iostream>
#include <chrono>
#include <thread>
#include <fmt/format.h>
#include "src/cpu.h"
#include "src/Terminal.h"
#include "assembler/Assembler.h"


using namespace sim;

static constexpr uint16_t TEST_START = 0200u;
static constexpr std::array<uint16_t, 3> TEST_PROGRAM =
        {
                01202, 05200, 04000
        };

int main() {

#if 0
    TerminalSocket terminalSocket;
    terminalSocket.open();
    terminalSocket.write("\033[1;1H");
    terminalSocket.write("\033[2J");
    terminalSocket.write("\033]0;PiDP-8/8 Console\007");
    terminalSocket.write("\033[0;33mThis appears in the terminal window.\n");
    terminalSocket.write("\033[2;2H\u25AF  \u25AE");

    std::chrono::milliseconds delay(10000);
    std::this_thread::sleep_for(delay);
#endif
#if 1
#if 0
    std::ofstream bin;
    bin.open("program.bin");

    std::string source = ".=    030; VAR1: 010; VAR2: SUBRTN;.=  0200; START: JMP I SUBRTN; CMA IAC; SUBRTN: NOP; NOP; JMP START;";
    std:
    std::stringstream src{source};
    asmbl::Assembler assembler;

    stdio_filebuf filebuf(1, std::ios::out);
    Terminal terminal(&filebuf);

    {
        using namespace sim::TerminalConsts;
        terminal.print("{}{} {}{}\n", color(Regular, Yellow), Light[0], Light[1], color(Regular));
    }

    assembler.pass1(src);
    src.clear();
    src.str(source);
    assembler.pass2(src, terminal.out(), bin);
    bin.close();
    assembler.dump_symbols(terminal.out());

    terminal.setCursorPosition(5u,0u);
#else
    TerminalSocket terminalSocket;
    terminalSocket.open();
    terminalSocket.write("\033[1;1H");
    terminalSocket.write("\033[2J");
    terminalSocket.write("\033]0;PiDP-8/I Console\007");

    TestCPU cpu{};

    cpu.reset();
    cpu.loadAddress(0174u);
    cpu.deposit(07776u); // 0174
    cpu.deposit(00017u); // 0175
    cpu.deposit(07770u); // 0176
    cpu.deposit(07770u); // 0177
    cpu.deposit(07200u); // 0200 CLA
    cpu.deposit(01176u); // 0201 TAD 0176
    cpu.deposit(03177u); // 0202 DCA 0177
    cpu.deposit(01175u); // 0203 TAD 0175
    cpu.deposit(07004u); // 0204 RAl
    cpu.deposit(02177u); // 0205 ISZ 0177
    cpu.deposit(05204u); // 0206 JMP 0204
    cpu.deposit(03175u); // 0207 DCA 0175
    cpu.deposit(01176u); // 0210 TAD 0176
    cpu.deposit(03177u); // 0211 DCA 0177
    cpu.deposit(01175u); // 0212 TAD 0175
    cpu.deposit(07010u); // 0213 RA4
    cpu.deposit(02177u); // 0214 ISZ 0177
    cpu.deposit(05213u); // 0215 JMP 0213
    cpu.deposit(03175u); // 0216 DCA 0175
//    cpu.deposit(02174u); // 2017 ISZ 0174
    cpu.deposit(05201u); // 0220 JMP 0201
    cpu.deposit(07402u); // 0221 HLT
    cpu.loadAddress(0200u);
    cpu.loadAddress(0200u);
    cpu.reset();

    std::chrono::milliseconds timespan(100);
    Terminal terminal(terminalSocket);
    cpu.printPanelSilk(terminal);
    cpu.setRunFlag(true);
    while (!cpu.getHaltFlag()) {
        cpu.instruction_cycle();
        cpu.printPanel(terminal);
        if (cpu.getCycleStat() == sim::PDP8I::CycleState::Interrupt) {
            terminalSocket.write("\033[1;1H");
            terminalSocket.write("\033[2J");
            cpu.printPanelSilk(terminal);
        } else
            std::this_thread::sleep_for(timespan);
    }
    cpu.setRunFlag(false);
    cpu.printPanel(terminal);

#endif
#endif
}
