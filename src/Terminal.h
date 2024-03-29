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
#include <utility>
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

    public:
        std::unique_ptr<pid_t> childPid{};         ///< The terminal process pid.
        std::unique_ptr<int> terminalFd{};         ///< The terminal file descriptor. Closed on destruction if open.
        std::unique_ptr<int> outFd{};              ///< The output file descriptor.
        std::unique_ptr<int> inFd{};               ///< The input file descriptor.

        std::unique_ptr<stdio_filebuf> iStrmBuf{};
        std::unique_ptr<stdio_filebuf> oStrmBuf{};

        TerminalConnection() = default;
        TerminalConnection(const TerminalConnection&) = delete;
        TerminalConnection(TerminalConnection&& other) = default;

        TerminalConnection& operator=(const TerminalConnection&) = delete;
        TerminalConnection& operator=(TerminalConnection&&) = default;

        virtual ~TerminalConnection() {
            if (terminalFd && *terminalFd >= 0)
                close(*terminalFd);

            if (outFd && *outFd >= 0)
                close(*outFd);

            if (inFd && *inFd >= 0)
                close(*inFd);
        }

        virtual void open() {}
    };

    /**
     * @class TerminalSocket
     * @brief Launch a TerminalConnection to a virtual terminal and run telnet to connect back to this
     * process using TCP/IP and the loop back device and a system assigned server port number.
     */
    class TerminalSocket : public TerminalConnection {
    protected:
        std::unique_ptr<struct sockaddr_in> sock_address{};      ///< Address of the this program
        std::unique_ptr<struct sockaddr_in> client_address{};    ///< Address of the telnet client
        std::unique_ptr<socklen_t> client_addr_len{};            ///< Client address structure size.
        std::unique_ptr<int> socket{};                           ///< The listen socket file descriptor

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
            if (terminalFd && *terminalFd >= 0)
                close(*terminalFd);
        }

        /**
         * @brief Open the connection, fork mate-terminal and run telnet to connect back to the listening port.
         */
        void open() override;

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

        std::shared_ptr<TerminalConnection> connection;
        std::unique_ptr<std::ostream> oStrm;                    ///< Output stream
        std::unique_ptr<std::istream> iStrm;                    ///< Input stream

        /**
         * @brief Default constructor
         * @details Both streams have null buffers. An object constructed with this won't communicate
         * anything.
         */
        Terminal() {
            connection = std::make_shared<TerminalConnection>();
            oStrm = std::make_unique<std::ostream>(connection->oStrmBuf.get());
            iStrm = std::make_unique<std::istream>(connection->iStrmBuf.get());
        }

        explicit Terminal(std::shared_ptr<TerminalConnection> terminalConnection) {
            connection = std::move(terminalConnection);
        }

        Terminal(const Terminal&) = delete;
        Terminal(Terminal&&)  noexcept = default;
        Terminal& operator=(const Terminal&) = delete;
        Terminal& operator=(Terminal&&) = default;

        virtual ~Terminal() = default;

        [[nodiscard]] auto getReadFd() const { return *connection->inFd; }
        [[nodiscard]] auto getWriteFd() const { return *connection->outFd; }

        void setReadFd(int fd) {
            connection->inFd = std::make_unique<int>(fd);
        }

        void setWriteFd(int fd) {
            connection->outFd = std::make_unique<int>(fd);
        }

        virtual int selected(bool selectedRead, bool selectedWrite);

        /**
         * @brief Test if the output stream exists.
         * @return True if it exists
         */
        [[nodiscard]] bool outExists() const { return oStrm.operator bool(); }

        /**
         * @brief Test if the input stream exists.
         * @return True if it exists
         */
        [[nodiscard]] bool inExists() const { return iStrm.operator bool(); }

        /**
         * @brief Access the output stream.
         * @return A std::ostream&
         */
        std::ostream &out() {
            return *oStrm; }   ///< Get the out stream

        /**
         * @brief Access the input stream.
         * @return A std::istream&
         */
        std::istream &in() { return *iStrm; }    ///< Get the in stream

        /**
         * @brief Use the format library to format output to the out stream.
         * @tparam Args Argument template type
         * @param args Arguments to format
         * @return the ostream.
         */
        template<typename...Args>
        std::ostream &print(Args...args) {
            return oStrm << fmt::format(std::forward<Args>(args)...);
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
        std::function<void()> disconnectCallback{};

        bool disconnected{false};

        std::string inputLineBuffer{};              ///< Buffer to read input from the user
        unsigned int inputLine{1u};                 ///< The line the input buffer is on
        unsigned int inputColumn{1u};               ///< The Column the input cursor is at.
        unsigned int termHeight{};                  ///< The height of the terminal.
        unsigned int termWidth{};                   ///< The width of the terminal.

        bool echoMode{false};
        bool naws{false};
        bool suppressGoAhead{false};

    public:
        TelnetTerminal() : Terminal(std::dynamic_pointer_cast<TerminalConnection>(std::make_shared<TerminalSocket>())) {
            connection->open();
            oStrm = std::make_unique<std::ostream>(connection->oStrmBuf.get());
            iStrm = std::make_unique<std::istream>(connection->iStrmBuf.get());
        }
        TelnetTerminal(const TelnetTerminal&) = delete;
        TelnetTerminal(TelnetTerminal&&) = default;
        TelnetTerminal& operator=(const TelnetTerminal&) = delete;
        TelnetTerminal& operator=(TelnetTerminal&&) = default;

        ~TelnetTerminal() override = default;

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
            *oStrm << fmt::format("\033[{};{}H", line, column);
        }

        void setCursorPosition() {
            *oStrm << fmt::format("\033[{};{}H", inputLine, inputColumn);
        }

        void parseInput(bool charModeProcessing = false);

        void parseIacBuffer(const std::vector<int>& buffer);

        virtual void windowSizeChanged() {}

        virtual void inputBufferReady() {
            inputLineBuffer.clear();
            inputBufferChanged();
        }

        virtual void inputBufferChanged() {
            setCursorPosition(inputLine, 1u);
            (*oStrm) << fmt::format("\033[0K> {}", inputLineBuffer);
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
    class TerminalManager : public std::vector<std::shared_ptr<TelnetTerminal>> {
    public:
        std::shared_ptr<TelnetTerminal> terminalQueue{};

    protected:
        bool service{true};

        std::chrono::microseconds selectTimeout{1000};
        struct SelectAllResult {
            unsigned long listIndex{static_cast<unsigned long>(-1)};
            int readFd{-1}, writeFd{-1};
            bool selectRead{false};
            bool selectWrite{false};
            bool selectExcept{false};

            SelectAllResult() = default;

            SelectAllResult(int idx, int read, int write, bool readSel, bool writeSel, bool exceptSel)
                    : listIndex(idx), readFd(read), writeFd(write), selectRead(readSel), selectWrite(writeSel),
                      selectExcept(exceptSel) {}
        };

        /**
         * @brief Perform select(2) on all terminals file descriptors, waiting for the provided timeout.
         * @param timeoutUs The wait timeout duration.
         * @return A tuple with the time remaining from the timeout and a list of terminals which need service.
         */
        std::tuple<std::chrono::microseconds, std::vector<SelectAllResult>>
        selectOnAll(std::chrono::microseconds timeoutUs);

    public:
        void closeAll() {
            service = false;
        }

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
            for (auto found = first; found != end(); ++found) {
                if( auto ptr = std::dynamic_pointer_cast<Terminal>(*found); ptr)
                    return std::make_tuple(found, found+1);
            }
            return std::make_tuple(end(), end());
        }
    };
}

