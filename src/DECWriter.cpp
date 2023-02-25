//
// Created by richard on 20/02/23.
//

/*
 * DECWriter.cpp Created by Richard Buckley (C) 20/02/23
 */

/**
 * @file DECWriter.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 20/02/23
 */

#include "DECWriter.h"
#include <stdexcept>
#include <fmt/format.h>
#include <thread>
#include <PDP8.h>

namespace pdp8 {
    void DECWriter::operation(PDP8 &pdp8, unsigned int device, unsigned int opCode) {
        if (device == keyboardDevice) {
            switch (opCode) {
                case 0: // KCF
                    keyboardFlag = false;
                    performInputOutput(pdp8);
                    break;
                case 1: // KSF
                    if (keyboardFlag)
                        ++pdp8.memory.programCounter;
                    break;
                case 4: // KRS
                    pdp8.accumulator.setAcc(pdp8.accumulator.getAcc() | (keyboardBuffer & 0377));
                    break;
                case 5: // KIE
                    interruptEnable = (pdp8.accumulator.getAcc() & 01) == 01;
                    break;
                case 6: // KRB
                    keyboardFlag = false;
                    pdp8.accumulator.setAcc(keyboardBuffer & 0377);
                    performInputOutput(pdp8);
                    break;
                default:
                    throw std::invalid_argument(fmt::format("DECWriter keyboard sent opCode{}", opCode));
            }
        } else if (device == printerDevice) {
            switch (opCode) {
                case 0: // TFL
                    printerFlag = true;
                    break;
                case 1: // TSF
                    if (printerFlag)
                        ++pdp8.memory.programCounter;
                    break;
                case 2: // TCF
                    printerFlag = false;
                    performInputOutput(pdp8);
                    break;
                case 4: // TPC
                    printerBuffer = pdp8.accumulator.getAscii();
                    break;
                case 5: // TSK
                    if (printerFlag || keyboardFlag)
                        ++pdp8.memory.programCounter;
                    break;
                case 6: // TLS
                    printerFlag = false;
                    printerBuffer = pdp8.accumulator.getAscii();
                    performInputOutput(pdp8);
                    break;
                default:
                    throw std::invalid_argument(fmt::format("DECWriter printer sent opCode{}", opCode));
            }
        } else
            throw std::invalid_argument(fmt::format("DECWriter addressed as {} but configured: keyboard {}, printer {}.",
                                        device, keyboardDevice, printerDevice));
    }

    bool DECWriter::getInterruptRequest() {
        return printerFlag || keyboardFlag;
    }

    void DECWriter::nextChar() {
        if (!terminal->inputLineBuffer.empty()) {
            if (!keyboardFlag) {
                auto c = terminal->inputLineBuffer[0];
                keyboardBuffer = static_cast<unsigned int>((u_char) c);
                terminal->inputLineBuffer = terminal->inputLineBuffer.substr(1);
                keyboardFlag = true;
            }
        }
    }


    void DECWriter::performInputOutput(PDP8 &pdp8) {
//        std::chrono::microseconds delay{100000};
        if (!terminal) {
            terminal = std::make_shared<PopupTerminal>();
            terminal->telnetTerminal = std::make_unique<DECWriterTerminal>(terminal->terminalSocket);
            terminal->inputWaiting = [this]() -> void {
                nextChar();
            };


            pdp8.terminalManager.push_back(terminal);
            terminal->setCharacterMode();
            terminal->negotiateAboutWindowSize();
            terminal->parseInput();

            terminal->out() << fmt::format("\033c"); terminal->out().flush();
            terminal->out() << fmt::format("\033[1;1H"); terminal->out().flush();
            terminal->out() << fmt::format("\033]0;DECWriter\007"); terminal->out().flush();
        }

        if (!printerFlag) {
            terminal->out().put(static_cast<char>(printerBuffer & 0xFF));
            terminal->out().flush();
            printerFlag = true;
        }

        if (!keyboardFlag) {
            nextChar();
        }
    }

    int DECWriterTerminal::selected(bool selectedRead, bool selectedWrite) {
        if (selectedRead) {
            parseInput();
            inputBufferChanged();
        }
        return 0;
    }

    void DECWriterTerminal::inputBufferChanged() {
        if (!inputLineBuffer.empty()) {
            if (inputWaiting) {
                inputWaiting();
            }
        }
    }
} // pdp8