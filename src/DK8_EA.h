/*
 * DK8_EA.h Created by Richard Buckley (C) 18/03/23
 */

/**
 * @file DK8_EA.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 18/03/23
 * @brief 
 * @details
 */

#ifndef PDP8_DK8_EA_H
#define PDP8_DK8_EA_H

#include <IOTDevice.h>
#include <PDP8.h>
#include <SysClock.h>
#include <thread>
#include <mutex>

namespace pdp8 {
    /**
     * @class DK8_EA
     * @brief DK8-EA Real Time Clock (Line Frequency)
     */
    class DK8_EA : public IOTDevice {
    protected:
        std::atomic_bool clock_flag{false};
        std::jthread clock_thread;

    public:
        bool enable_interrupt{false};

        DK8_EA();

        DK8_EA(const DK8_EA&) = delete;
        DK8_EA(DK8_EA&&) = delete;
        DK8_EA& operator=(const DK8_EA&) = delete;
        DK8_EA& operator=(DK8_EA&&) = delete;

        ~DK8_EA() override = default;

        void operation(PDP8 &pdp8, unsigned int device, unsigned int opCode) override;

        bool getInterruptRequest() override;

        bool getClockFlag();

        void setClockFlag(bool flag);
    };
}


#endif //PDP8_DK8_EA_H
