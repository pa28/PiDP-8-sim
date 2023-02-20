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

namespace pdp8 {

/**
 * @class DECWriterKeyBoard
 */
    class DECWriterKeyBoard : public IOTDevice {
    public:
        DECWriterKeyBoard() : IOTDevice(3) {}
        ~DECWriterKeyBoard() override = default;

        void operation(PDP8 &pdp8, unsigned int opCode) override;
    };

/**
 * @class DECWriterKeyBoard
 */
    class DECWriterPrinter : public IOTDevice {
    public:
        DECWriterPrinter() : IOTDevice(4) {}
        ~DECWriterPrinter() override = default;

        void operation(PDP8 &pdp8, unsigned int opCode) override;
    };

} // pdp8

#endif //PDP8_DECWRITER_H
