//
// Created by richard on 20/02/23.
//

/*
 * DECWriter.h Created by Richard Buckley (C) 20/02/23
 */

/**
 * @file DECWriter.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 20/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_DECWRITER_H
#define PDP8_DECWRITER_H

#include <IOTDevice.h>
#include <Terminal.h>
#include <functional>

namespace pdp8 {

    class DECWriterTerminal : public TelnetTerminal {
        friend class DECWriter;

    protected:
        std::function<void()> inputWaiting{};

    public:
        DECWriterTerminal() : TelnetTerminal() {}
        DECWriterTerminal(const DECWriterTerminal&) = delete;
        DECWriterTerminal(DECWriterTerminal&&) = default;
        DECWriterTerminal& operator=(const DECWriterTerminal&) = delete;
        DECWriterTerminal& operator=(DECWriterTerminal&&) = default;

        explicit DECWriterTerminal(TerminalConnection &terminalConnection) : TelnetTerminal(terminalConnection) {
        }

        int selected(bool selectedRead, bool selectedWrite) override;
        void inputBufferChanged() override;
    };

    /**
     * @class DECWriter
     */
    class DECWriter : public IOTDevice {
    public:
        std::shared_ptr<PopupTerminal> terminal{};
        TerminalSocket terminalSocket{};

        unsigned int keyboardDevice{3};
        unsigned int printerDevice{4};

        unsigned int keyboardBuffer{};
        unsigned int printerBuffer{};

        bool interruptEnable{false};
        bool printerFlag{true};
        bool keyboardFlag{false};

        DECWriter() = default;
        DECWriter(unsigned int keyDev, unsigned int prnDev) : DECWriter() {
            keyboardDevice = keyDev;
            printerDevice = prnDev;
        }

        ~DECWriter() override = default;

        void operation(PDP8 &pdp8, unsigned int device, unsigned int opCode) override;

        bool getInterruptRequest() override;

        void performInputOutput(PDP8 &pdp8);

        void nextChar();
    };

} // pdp8

#endif //PDP8_DECWRITER_H
