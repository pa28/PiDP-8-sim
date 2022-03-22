/**
 * @file Pdp8Terminal.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#pragma once

#include "src/Terminal.h"
#include "src/cpu.h"
#include "assembler/Assembler.h"

namespace sim {

    /**
     * @class Pdp8Terminal
     * @brief A terminal for the
     */
    class Pdp8Terminal : public TelnetTerminal {
    public:

    protected:

        PDP8I cpu{};

        asmbl::Assembler assembler{};

        std::string lastCommand{};

        bool runConsole{true};

        void inputBufferChanged() override;

        void inputBufferReady() override;

        void windowSizeChanged() override {
            TelnetTerminal::windowSizeChanged();
            inputLine = termHeight;
            print("\033[2J");
            printPanelSilk();
            printPanel();
            inputBufferChanged();
        }

        std::vector<std::string> commandHistory{};

        std::vector<TelnetTerminalSet> managedTerminals{};

        void printCommandHistory();

        void commandHelp();

        std::optional<register_type> parseArgument(const std::string &argument);

        static constexpr std::array<std::string_view, 6> CommandLineHelp =
                {{
                         "l <octal> -- Load Address.            d <octal> -- Deposit at address.",
                         "e -- Examine at address, repeats.     c -- CPU single cycle, repeats.",
                         "s -- CPU single instruction, repeats. ?|h -- Print this help.",
                         "C -- Continue from current address.   S -- Stop execution.",
                         "PING PONG -- Assemble and load built in program.",
                         "quit -- Exit the program."
                 }};

        template<size_t width, size_t offset, typename U1, typename U2>
        requires std::unsigned_integral<U2> && std::unsigned_integral<U2>
        std::tuple<size_t, size_t> printPanelField(U1 line, U2 col, slice_value<width, offset> slice) {
            using namespace TerminalConsts;
            for (auto idx = slice.WIDTH; idx > 0; --idx) {
                setCursorPosition(line, col);
                auto lightIdx = slice() & (1 << (idx - 1)) ? 1 : 0;
                print("{} ", Light[lightIdx]);
                col += 2;
            }
            return {line, col};
        }

        template<typename U1, typename U2>
        requires std::unsigned_integral<U1> && std::unsigned_integral<U2>
        void printPanelFlag(U1 line, U2 col, bool flag) {
            using namespace TerminalConsts;
            setCursorPosition(line, col);
            print("{}", Light[flag ? 1 : 0]);
        }

        struct SelectAllResult {
            int listIndex{-1};
            int readFd{-1}, writeFd{-1};
            bool selectRead{false};
            bool selectWrite{false};

            SelectAllResult() = default;
            SelectAllResult(int idx, int read, int write, bool readSel, bool writeSel)
                : listIndex(idx), readFd(read), writeFd(write), selectRead(readSel), selectWrite(writeSel) {}
        };

        std::tuple<std::chrono::microseconds, std::vector<SelectAllResult>>
        selectOnAll(std::chrono::microseconds timeoutUs);

        void printPanelSilk();

        void printPanel();

        void loadPingPong();

        void loadSourceStream(std::istream &sourceCode, const std::string &title);

    public:
        Pdp8Terminal() = default;

        ~Pdp8Terminal() override = default;

        explicit Pdp8Terminal(TerminalSocket &connection) : TelnetTerminal(connection) {}

        void console();

        int selected(bool selectedRead, bool selectedWrite) override;
    };
}

