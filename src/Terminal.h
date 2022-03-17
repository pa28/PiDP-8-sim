/**
 * @file terminal.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-15
 */

#pragma once

#include <ext/stdio_filebuf.h>
#include <fmt/format.h>
#include <array>
#include <iostream>
#include <sstream>
#include <string_view>
#include <cstdio>
#include <unistd.h>
#include <filesystem>
#include <memory>
#include <fcntl.h>
#include <csignal>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

namespace sim {

    using stdio_filebuf = __gnu_cxx::stdio_filebuf<char>;

    class NullStreamBuffer : public std::streambuf {
    protected:
        int overflow(int c) override { return c; }
    };

    namespace TerminalConsts {
        enum Colors : size_t {
            Regular = 0, Bold, LowIntensity, Italic, Underline, Blinking, Reverse, Background,
            Invisible,
            Black = 30, Red, Green, Yellow, Blue, Magenta, Cyan, White,
            BlackBg = 40, RedBg, GreenBg, YellowBg, BlueBg, MagentaBg, CyanBg, WhiteBg,
        };

        static constexpr std::array<std::string_view, 2> Light = {{"\u25AF", "\u25AE"}};
        static constexpr std::string_view Bar = "\u2594\u2594\u2594\u2594\u2594";

        template<typename Arg, typename...Args>
        static std::string color(Arg arg, Args...args) {
            std::stringstream r;
            r << "\033[" << arg;
            ((r << ';' << std::forward<Args>(args)), ...);
            r << 'm';
            return r.str();
        }
    }

    class TerminalPipeException : public std::runtime_error {
    public:
        explicit TerminalPipeException(const char *what_arg) : std::runtime_error(what_arg) {}
        explicit TerminalPipeException(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    class TerminalConnection {
        friend class Terminal;
    protected:
        pid_t childPid{-1};
        int terminalFd{-1};

        std::unique_ptr<stdio_filebuf> iBuffer{};
        std::unique_ptr<stdio_filebuf> oBuffer{};

    public:
        TerminalConnection() = default;

        virtual ~TerminalConnection() {
            if (terminalFd >= 0)
                close(terminalFd);
        }

        size_t write(const char* buf) const {
            return ::write(terminalFd, buf, strlen(buf));
        }
    };
    /**
     * @class TerminalPipe
     * @brief Create a set of pipes to a new process and attach stdio_filebuf to the input of one and the output
     * of the other
     */
    class TerminalPipe : public TerminalConnection {
    protected:
        std::string ptsName{};

    public:

        TerminalPipe() = default;

        ~TerminalPipe() override {
            kill(childPid, SIGKILL);
            int waitStatus;
            wait(&waitStatus);
        }

        /**
         * @brief Execute as a process the executable found at a filesystem path location.
         * @param process The file system path location
         * @throws TerminalPipeException
         */
        void open(const std::string& title);
    };

    class TerminalSocket : public TerminalConnection {
    protected:
        struct sockaddr_in sock_address{};
        struct sockaddr_in client_address{};
        socklen_t client_addr_len{};
        int socket{-1};

    public:

        TerminalSocket() = default;

        ~TerminalSocket() override {
            int waitStatus;
            wait(&waitStatus);
        }

        void open();

    };

    /**
     * @class Terminal
     * @brief Output to terminal using escape sequences.
     */
    class Terminal {
    public:

    private:
        static NullStreamBuffer nullStreamBuffer;
        std::ostream ostrm;
        std::istream istrm;

    public:

        Terminal() : ostrm(&nullStreamBuffer), istrm(&nullStreamBuffer) {}

        explicit Terminal(stdio_filebuf *outbuff) : ostrm(outbuff), istrm(&nullStreamBuffer) {}

        Terminal(stdio_filebuf *inBuff, stdio_filebuf *outBuff) : ostrm(outBuff), istrm(inBuff) {}

        explicit Terminal(TerminalConnection &terminalConnection)
        : Terminal(terminalConnection.iBuffer.get(), terminalConnection.oBuffer.get()) {}

        std::ostream &out() { return ostrm; }

        std::istream &in() { return istrm; }

        template<typename...Args>
        std::ostream &print(Args...args) {
            return ostrm << fmt::format(std::forward<Args>(args)...);
        }

        template<typename U1, typename U2>
        requires std::unsigned_integral<U1> && std::unsigned_integral<U2>
        void setCursorPosition(U1 line, U2 column) {
            print("\033[{};{}H", line, column);
        }

        void flush() {
            ostrm.flush();
        }
    };

}

