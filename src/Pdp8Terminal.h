/**
 * @file Pdp8Terminal.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-18
 */

#pragma once

#include "src/Terminal.h"

namespace sim {

    /**
     * @class Pdp8Terminal
     * @brief A terminal for the
     */
    class Pdp8Terminal : public TelnetTerminal {
    public:

    protected:

    public:
        Pdp8Terminal() = default;

        ~Pdp8Terminal() override = default;

        explicit Pdp8Terminal(TerminalSocket &connection) : TelnetTerminal(connection) {}

        void console();
    };
}

