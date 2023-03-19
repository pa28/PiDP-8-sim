//
// Created by richard on 18/03/23.
//

/*
 * DK8_EA.cpp Created by Richard Buckley (C) 18/03/23
 */

/**
 * @file DK8_EA.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 18/03/23
 */

#include "DK8_EA.h"
#include <chrono>

using namespace std::chrono_literals;

namespace pdp8 {

    void DK8_EA::operation(PDP8 &pdp8, unsigned int, unsigned int opCode) {
        switch (opCode) {
            case 1: // CLEI
                enable_interrupt = true;
                break;
            case 2: // CLDI
                enable_interrupt = false;
                break;
            case 3: // CLSK
                if (getClockFlag())
                    ++pdp8.memory.programCounter;
                setClockFlag(false);
                break;
            default:
                break;
        }
    }

    bool DK8_EA::getInterruptRequest() {
        return getClockFlag() && enable_interrupt;
    }

    bool DK8_EA::getClockFlag() {
        return clock_flag;
    }

    void DK8_EA::setClockFlag(bool flag) {
        clock_flag = flag;
    }

    DK8_EA::DK8_EA(bool runClock) {
        if (runClock)
            clock_thread = std::jthread([this](std::stop_token stopToken){
                while (!stopToken.stop_requested()) {
                    std::this_thread::sleep_for(16667us);
                    this->setClockFlag(true);
                }
            });
    }
}