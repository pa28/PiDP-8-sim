/*
 * IOTDevice.h Created by Richard Buckley (C) 20/02/23
 */

/**
 * @file IOTDevice.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 20/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_IOTDEVICE_H
#define PDP8_IOTDEVICE_H

namespace pdp8 {

    class PDP8;

    /**
     * @class IOTDevice
     */
    class IOTDevice {
    public:
        IOTDevice() = default;

        IOTDevice(const IOTDevice&) = delete;
        IOTDevice(IOTDevice&&) = default;
        IOTDevice& operator=(const IOTDevice&) = delete;
        IOTDevice& operator=(IOTDevice&&) = default;

        virtual ~IOTDevice() = default;

        virtual void operation(PDP8 &pdp8, unsigned int device, unsigned int opCode) = 0;

        virtual bool getInterruptRequest() = 0;

        virtual bool getServiceRequest() = 0;

        virtual void setServiceRequest() = 0;
    };

} // pdp8

#endif //PDP8_IOTDEVICE_H
