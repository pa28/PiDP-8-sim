/**
 * @file TestPrograms.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-17
 */

#pragma once

#include <string_view>

namespace asmbl {

    static constexpr std::string_view PingPong = R"(# Simple Ping-Pong Accumulator
            .   = 0174;
CycleCount:     07776;                  # 0 - Number of times to cycle before halting
Accumulator:    00017;                  # Initial value and temp store for the ACC
SeminCycle:     07770;                  # 0 - Semi cycle intiial count
Counter:        07770;                  # Semi cycle counter
Initialize:     CLA;                    # Clear ACC
OuterLoop:      TAD SemiCycle;          # Init semi cycle count
                DCA Counter;
                TAD Accumulator;        # Accumulator display
Loop1:          RAL;
                ISZ Counter;            # Increment count
                JMP Loop1;              # First semi cycle loop ends
                DCA Accumulator;        # Save ACC
                TAD SemiCycle;          # Init semi cycle count
                DCA Counter;
                TAD Accumulator;        # Accumulator Display
Loop2:          RAR;
                ISZ Counter;            # Increment count
                JMP Loop2;              # Second semi cycle loop ends
                DCA Accumulator;        # Save Acc
#               ISZ CycleCount;         # Uncomment to cycle for a bin and end.
                JMP OuterLoop;          # Outer loop ends.
                HLT;
)";
}
