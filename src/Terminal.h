/**
 * @file Terminal.h
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

    // https://datatracker.ietf.org/doc/html/rfc1073
    // https://datatracker.ietf.org/doc/html/rfc1184

    using stdio_filebuf = __gnu_cxx::stdio_filebuf<char>;   ///< Used to create NullStreamBuffer.

    /**
     * @class NullStreamBuffer
     * @brief A Stream buffer that doesnt buffer data.
     * @details A NullStreamBuffer may be used where you have to pass a std::istream or std::ostream but
     * don't want any output. Create the required stream with a NullStreamBuffer and pass it wehre required.
     */
    class NullStreamBuffer : public std::streambuf {
    protected:
        int overflow(int c) override { return c; }
    };

    /**
     * @namespace TerminalConsts
     * @brief Useful value for terminals.
     */
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

    /**
     * @class TerminalConnectionException
     * @brief Thrown by TerminalConnection and derivatives on errors.
     */
    class TerminalConnectionException : public std::runtime_error {
    public:
        explicit TerminalConnectionException(const char *what_arg) : std::runtime_error(what_arg) {}

        explicit TerminalConnectionException(const std::string &what_arg) : std::runtime_error(what_arg) {}
    };

    /**
     * @brief TerminalConnection base class. This provides a standard interface to an open connection.
     */
    class TerminalConnection {
        friend class Terminal;

    protected:
        pid_t childPid{-1};         ///< The terminal process pid.
        int terminalFd{-1};         ///< The terminal file descriptor. Closed on destruction if open.

        std::unique_ptr<stdio_filebuf> iBuffer{};
        std::unique_ptr<stdio_filebuf> oBuffer{};

    public:
        TerminalConnection() = default;

        virtual ~TerminalConnection() {
            if (terminalFd >= 0)
                close(terminalFd);
        }

        /**
         * @brief Write a string to the output file descriptor.
         * @param buf
         * @return
         */
        size_t write(const char *buf) const {
            return ::write(terminalFd, buf, strlen(buf));
        }
    };

    /**
     * @class TerminalPipe -- Deprecated due to poor support.
     * @brief Use /dev/ptmx to set up a connection to a virtual terminal that supports adopting an open
     * /dev/ptmx file descriptor. XTerm supports this.
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
        void open(const std::string &title);
    };

    /**
     * @class TerminalSocket
     * @brief Launch a TerminalConnection to a virtual terminal and run telnet to connect back to this
     * process using TCP/IP and the loop back device and a system assigned server port number.
     */
    class TerminalSocket : public TerminalConnection {
    protected:
        struct sockaddr_in sock_address{};      ///< Address of the this program
        struct sockaddr_in client_address{};    ///< Address of the telnet client
        socklen_t client_addr_len{};            ///< Client address structure size.
        int socket{-1};                         ///< The listen socket file descriptor

    public:

        TerminalSocket() = default;             ///< Constructor

        /**
         * @brief Destructor, wait for the client to exit.
         */
        ~TerminalSocket() override {
            int waitStatus;
            if (terminalFd >= 0)
                close(terminalFd);
            terminalFd = -1;
            wait(&waitStatus);
        }

        /**
         * @brief Open the connection.
         */
        void open();

    };

    /**
     * @class Terminal
     * @brief Output to terminal using escape sequences.
     */
    class Terminal {
    public:
        enum SelectStatus {
            Timeout, Data
        };

    protected:
        static NullStreamBuffer nullStreamBuffer;   ///< Null buffer for unused streams
        std::ostream ostrm;                         ///< Output stream
        std::istream istrm;                         ///< Input stream

        int ofd{-1};                                ///< File descriptor of the output stream
        int ifd{-1};                                ///< File descriptor of the input stream

    public:

        /**
         * @brief Default constructor
         * @details Both streams have null buffers. An object constructed with this won't communicate
         * anything.
         */
        Terminal() : ostrm(&nullStreamBuffer), istrm(&nullStreamBuffer) {}

        ~Terminal() = default;

        /**
         * @brief Construct an outbound only connection using a provided stream buffer.
         * @param outbuff The output stream buffer.
         */
        explicit Terminal(stdio_filebuf *outbuff) : ostrm(outbuff), istrm(&nullStreamBuffer) {}

        /**
         * @brief Construct a bi-directional connection using provided in and out stream buffers.
         * @param inBuff The input buffer.
         * @param outBuff The output buffer
         */
        Terminal(stdio_filebuf *inBuff, stdio_filebuf *outBuff) : ostrm(outBuff), istrm(inBuff) {}

        /**
         * @brief Construct a bi-directional connection using buffers provided by a TerminalConnection
         * @param terminalConnection The terminal connection
         */
        explicit Terminal(TerminalConnection &terminalConnection)
                : Terminal(terminalConnection.iBuffer.get(), terminalConnection.oBuffer.get()) {
            ifd = terminalConnection.terminalFd;
            ofd = terminalConnection.terminalFd;
        }

        std::ostream &out() { return ostrm; }   ///< Get the out stream

        std::istream &in() { return istrm; }    ///< Get the in stream

        /**
         * @brief Use the format library to format output to the out stream.
         * @tparam Args Argument template type
         * @param args Arguments to format
         * @return the ostream.
         */
        template<typename...Args>
        std::ostream &print(Args...args) {
            return ostrm << fmt::format(std::forward<Args>(args)...);
        }

        /**
         * @brief Flush the output stream.
         */
        void flush() {
            ostrm.flush();
        }

        std::tuple<Terminal::SelectStatus, Terminal::SelectStatus, unsigned int>
        select(bool selRead, bool selWrite, unsigned int timeoutUs);
    };

    class TelnetTerminal : public Terminal {
    public:
        static constexpr int IAC = 255;
        static constexpr int DO = 253;
        static constexpr int WILL = 251;
        static constexpr int WONT = 252;
        static constexpr int SB = 250;
        static constexpr int SE = 240;

        // Negotiate About Window Size
        // Server suggests:
        // Server sends: IAC DO NAWS
        // Client replies: IAC WILL NAWS
        // Client sends: IAC SB NAWS <16 bit value nbo><16 bit value nbo> SE
        static constexpr int NAWS = 037; // 31

        // Enter character mode
        // Server sends: IAC WONT LINEMONDE
        // Server sends: IAC WILL ECHO
        static constexpr int LINEMODE = 34;
        static constexpr int ECHO = 1;
        static constexpr int SUPPRESS_GO_AHEAD = 3;

    protected:
        std::string inputLineBuffer{};              ///< Buffer to read input from the user
        unsigned int inputLine{1u};                 ///< The line the input buffer is on
        unsigned int inputColumn{1u};               ///< The Column the input cursor is at.
        unsigned int termHeight{};                  ///< The height of the terminal.
        unsigned int termWidth{};                   ///< The width of the terminal.
        bool termWindowSizeChange{};                ///< True when the terminal size has changed.

        bool echoMode{false};
        bool suppressGoAhead{false};
        bool naws{false};

    public:
        TelnetTerminal() = default;

        virtual ~TelnetTerminal() = default;

        explicit TelnetTerminal(TerminalConnection &connection) : Terminal(connection) {}

    protected:
        void setCharacterMode();

        void negotiateAboutWindowSize();

        /**
         * @brief Set the cursor position.
         * @tparam U1 The type of line parameter.
         * @tparam U2 The type of column parameter.
         * @param line The line, starts at 1.
         * @param column The column, starts at 1.
         */
        template<typename U1, typename U2>
        requires std::unsigned_integral<U1> && std::unsigned_integral<U2>
        void setCursorPosition(U1 line, U2 column) {
            print("\033[{};{}H", line, column);
        }

        void setCursorPosition() {
            print("\033[{};{}H", inputLine, inputColumn);
        }

        void parseInput();

        void parseIacBuffer(const std::vector<int>& buffer);

        virtual void windowSizeChanged() {}

        virtual void inputBufferReady() {
            inputLineBuffer.clear();
            inputBufferChanged();
        }

        virtual void inputBufferChanged() {
            setCursorPosition(inputLine, 1u);
            print("\033[0K> {}", inputLineBuffer);
            inputColumn = inputLineBuffer.size() + 3;
            setCursorPosition();
            out().flush();
        }
    };
}

