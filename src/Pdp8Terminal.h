/**
 * @file Pdp8Terminal.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#pragma once

#include "Terminal.h"
#include "PDP8.h"
#include "assembler/Assembler.h"
#include "assembler/TestPrograms.h"
#include <fmt/format.h>

namespace pdp8 {

    /**
     * @class Pdp8Terminal
     * @brief A terminal for the
     */
    class Pdp8Terminal : public TelnetTerminal {
    public:

    protected:

        PDP8 pdp8{};

        pdp8asm::Assembler assembler{};

        std::string lastCommand{};

        bool runConsole{true};

        void inputBufferChanged() override;

        void inputBufferReady() override;

        void windowSizeChanged() override {
            TelnetTerminal::windowSizeChanged();
            inputLine = termHeight;
            ostrm << fmt::format("\033[2J");
            printPanelSilk();
            printPanel();
            inputBufferChanged();
        }

        std::vector<std::string> commandHistory{};

        std::vector<TelnetTerminalSet> managedTerminals{};

        void printCommandHistory();

        void commandHelp();

        std::optional<unsigned int> parseArgument(const std::string &argument);

        static constexpr std::array<std::string_view, 6> CommandLineHelp =
                {{
                         "l <octal> -- Load Address.            d <octal> -- Deposit at address.",
                         "e -- Examine at address, repeats.     c -- CPU single cycle, repeats.",
                         "s -- CPU single instruction, repeats. ?|h -- Print this help.",
                         "C -- Continue from current address.   S -- Stop execution.",
                         "PING PONG -- Assemble and load built in program.",
                         "quit -- Exit the program."
                 }};

        std::tuple<size_t, size_t> printPanelField(size_t line, size_t col, size_t width, uint value) {
            using namespace TerminalConsts;
            for (auto idx = width; idx > 0; --idx) {
                setCursorPosition(line, col);
                auto lightIdx = value & (1 << (idx - 1)) ? 1 : 0;
                ostrm << fmt::format("{} ", Light[lightIdx]);
                col += 2;
            }
            return {line, col};
        }

        template<typename U1, typename U2>
        requires std::unsigned_integral<U1> && std::unsigned_integral<U2>
        void printPanelFlag(U1 line, U2 col, bool flag) {
            using namespace TerminalConsts;
            setCursorPosition(line, col);
            ostrm << fmt::format("{} ", Light[flag ? 1 : 0]);
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

        void loadForth();

        void loadSourceStream(std::istream &sourceCode, const std::string &title);

    public:
        Pdp8Terminal() = default;

        ~Pdp8Terminal() override = default;

        explicit Pdp8Terminal(TerminalSocket &connection) : TelnetTerminal(connection) {}

        void console();

        int selected(bool selectedRead, bool selectedWrite) override;
    };
}

