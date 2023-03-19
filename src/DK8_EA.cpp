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
#include "Memory.h"

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
                if (clock_flag)
                    ++pdp8.memory.programCounter;
                setClockFlag(false);
                break;
            default:
                break;
        }
    }

    bool DK8_EA::getInterruptRequest() {
        return clock_flag && enable_interrupt;
    }
}