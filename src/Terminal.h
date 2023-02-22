/**
 * @file Terminal.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-15
 */

#pragma once

#include <assembler/NullStream.h>
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
#include <vector>
#include <chrono>
#include <thread>
#include <functional>


namespace pdp8 {

    using namespace null_stream;
    // https://datatracker.ietf.org/doc/html/rfc1073
    // https://datatracker.ietf.org/doc/html/rfc1184
    // http://pcmicro.com/netfoss/telnet.html

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
        TerminalConnection(const TerminalConnection&) = delete;
        TerminalConnection(TerminalConnection&&) = default;
        TerminalConnection& operator=(const TerminalConnection&) = delete;
        TerminalConnection& operator=(TerminalConnection&&) = default;

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
        TerminalSocket(const TerminalSocket&) = delete;
        TerminalSocket(TerminalSocket&&) = default;
        TerminalSocket& operator=(const TerminalSocket&) = delete;
        TerminalSocket& operator=(TerminalSocket&&) = default;

        /**
         * @brief Destructor, wait for the client to exit.
         */
        ~TerminalSocket() override {
            int waitStatus;
            if (terminalFd >= 0)
                close(terminalFd);
            terminalFd = -1;
        }

        /**
         * @brief Open the connection, fork mate-terminal and run telnet to connect back to the listening port.
         */
        void open();

        void openServer(int listenPort);
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
        NullStreamBuffer nullStreamBuffer{};   ///< Null buffer for unused streams
        std::unique_ptr<std::ostream> ostrm;                    ///< Output stream
        std::unique_ptr<std::istream> istrm;                    ///< Input stream

        int ofd{-1};                           ///< File descriptor of the output stream
        int ifd{-1};                           ///< File descriptor of the input stream

    public:

        /**
         * @brief Default constructor
         * @details Both streams have null buffers. An object constructed with this won't communicate
         * anything.
         */
        Terminal() : ostrm(std::make_unique<std::ostream>(&nullStreamBuffer)),
                        istrm(std::make_unique<std::istream>(&nullStreamBuffer)) {}
        Terminal(const Terminal&) = delete;
        Terminal(Terminal&&) = default;
        Terminal& operator=(const Terminal&) = delete;
        Terminal& operator=(Terminal&&) = default;

        ~Terminal() = default;

        [[nodiscard]] auto getReadFd() const { return ifd; }
        [[nodiscard]] auto getWriteFd() const { return ofd; }

        virtual int selected(bool selectedRead, bool selectedWrite);

        /**
         * @brief Construct an outbound only connection using a provided stream buffer.
         * @param outbuff The output stream buffer.
         */
        explicit Terminal(stdio_filebuf *outbuff)
            : ostrm(std::make_unique<std::ostream>(outbuff)), istrm(std::make_unique<std::istream>(&nullStreamBuffer)) {}

        /**
         * @brief Construct a bi-directional connection using provided in and out stream buffers.
         * @param inBuff The input buffer.
         * @param outBuff The output buffer
         */
        Terminal(stdio_filebuf *inBuff, stdio_filebuf *outBuff)
            : ostrm(std::make_unique<std::ostream>(outBuff)), istrm(std::make_unique<std::istream>(inBuff)) {}

        /**
         * @brief Construct a bi-directional connection using buffers provided by a TerminalConnection
         * @param terminalConnection The terminal connection
         */
        explicit Terminal(TerminalConnection &terminalConnection)
                : Terminal(terminalConnection.iBuffer.get(), terminalConnection.oBuffer.get()) {
            ifd = terminalConnection.terminalFd;
            ofd = terminalConnection.terminalFd;
        }

        explicit Terminal(TerminalConnection* terminalConnection)
                : Terminal(terminalConnection->iBuffer.get(), terminalConnection->oBuffer.get()) {
            ifd = terminalConnection->terminalFd;
            ofd = terminalConnection->terminalFd;
        }

        std::ostream &out() { return *ostrm; }   ///< Get the out stream

        std::istream &in() { return *istrm; }    ///< Get the in stream

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

        std::tuple<Terminal::SelectStatus, Terminal::SelectStatus, unsigned int>
        select(bool selRead, bool selWrite, unsigned int timeoutUs);
    };

    class TelnetTerminal : public Terminal {
    public:
        static constexpr int IAC = 255;     // 377
        static constexpr int DO = 253;      // 375
        static constexpr int WILL = 251;    // 373
        static constexpr int WONT = 252;    // 374
        static constexpr int SB = 250;      // 372
        static constexpr int SE = 240;      // 360

        // Negotiate About Window Size
        // Server suggests:
        // Server sends: IAC DO NAWS
        // Client replies: IAC WILL NAWS
        // Client sends: IAC SB NAWS <16 bit value nbo><16 bit value nbo> SE
        static constexpr int NAWS = 037; // 31

        static constexpr int ECHO = 1;
        static constexpr int SUPPRESS_GO_AHEAD = 3;

        std::function<bool()> timerTick{};

        bool disconnected{false};

    protected:
        std::string inputLineBuffer{};              ///< Buffer to read input from the user
        unsigned int inputLine{1u};                 ///< The line the input buffer is on
        unsigned int inputColumn{1u};               ///< The Column the input cursor is at.
        unsigned int termHeight{};                  ///< The height of the terminal.
        unsigned int termWidth{};                   ///< The width of the terminal.

        bool echoMode{false};
        bool naws{false};
        bool suppressGoAhead{false};

    public:
        TelnetTerminal() = default;
        TelnetTerminal(const TelnetTerminal&) = delete;
        TelnetTerminal(TelnetTerminal&&) = default;
        TelnetTerminal& operator=(const TelnetTerminal&) = delete;
        TelnetTerminal& operator=(TelnetTerminal&&) = default;

        virtual ~TelnetTerminal() = default;

        explicit TelnetTerminal(TerminalConnection &connection) : Terminal(connection) {}

        explicit TelnetTerminal(TerminalConnection *connection) : Terminal(connection) {}

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
            *ostrm << fmt::format("\033[{};{}H", line, column);
        }

        void setCursorPosition() {
            *ostrm << fmt::format("\033[{};{}H", inputLine, inputColumn);
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
            (*ostrm) << fmt::format("\033[0K> {}", inputLineBuffer);
            inputColumn = static_cast<unsigned int>(inputLineBuffer.size() + 3u);
            setCursorPosition();
            out().flush();
        }
    };

    /**
     * @class TerminalManager
     * @brief Manages a collection of TelnetTerminals.
     * @details The list of active terminals is scanned for activity using select(2). Terminals that need service
     * are have their selected(bool selectRead, selectWrite) method called. Terminals that have been disconnected
     * are marked for removal from the list. Finally each unmarked terminal which has set a timerTick callback will
     * be called on the timerTick method. This provides an opportunity for the terminal to do internal processing
     * which should be kept to much less than the selectTimeout period. Any terminal which returns false will be
     * marked for removal. Finally all terminals marked for removal are removed.
     */
    class TerminalManager : public std::vector<std::unique_ptr<TelnetTerminal>> {
    public:

    protected:

        std::chrono::microseconds selectTimeout{1};
        struct SelectAllResult {
            int listIndex{-1};
            int readFd{-1}, writeFd{-1};
            bool selectRead{false};
            bool selectWrite{false};

            SelectAllResult() = default;
            SelectAllResult(int idx, int read, int write, bool readSel, bool writeSel)
                    : listIndex(idx), readFd(read), writeFd(write), selectRead(readSel), selectWrite(writeSel) {}
        };

        /**
         * @brief Perform select(2) on all terminals file descriptors, waiting for the provided timeout.
         * @param timeoutUs The wait timeout duration.
         * @return A tuple with the time remaining from the timeout and a list of terminals which need service.
         */
        std::tuple<std::chrono::microseconds, std::vector<SelectAllResult>>
        selectOnAll(std::chrono::microseconds timeoutUs);

    public:

        /**
         * @brief Service the list of terminals. This should be called regularly in the application event loop
         * on the main thread. The method will consume at least selectTimeout microseconds which can be used to
         * set the tempo of the program.
         */
        void serviceTerminals();

        /**
         * @brief Find a managed terminal of the given type starting at first.
         * @tparam Terminal The type of terminal to look for.
         * @param first The location in the list to start searching.
         * @return A tuple with the an iterator to the found terminal and an iterator to the next on the list.
         * Either or both of these may be end().
         */
        template<class Terminal>
                requires std::derived_from<Terminal, TelnetTerminal>
        auto findTerminal(iterator first) {
            auto found = std::find_if(first, end(), [](const iterator itr) {
               return std::dynamic_pointer_cast<Terminal>(*itr);
            });

            if (found != end())
                return std::make_tuple(found, found+1);
            return std::make_tuple(found, end());
        }
    };

}

