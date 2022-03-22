#include <cstdio>
#include <csignal>
#include <sys/wait.h>
#include <sys/resource.h>
#include <chrono>
#include <thread>
#include <fmt/format.h>
#include "src/cpu.h"
#include "src/Pdp8Terminal.h"


using namespace sim;

static constexpr uint16_t TEST_START = 0200u;
static constexpr std::array<uint16_t, 3> TEST_PROGRAM =
        {
                01202, 05200, 04000
        };


int main() {

    signal(SIGCHLD, SIG_IGN);

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
    Pdp8Terminal terminal(terminalSocket);
    terminal.console();


#endif
#endif
}
