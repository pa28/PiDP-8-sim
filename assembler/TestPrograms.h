/**
 * @file TestPrograms.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-17
 */

#pragma once

#include <string_view>

namespace asmbl {

    static constexpr std::string_view PingPong = R"(/ Simple Ping-Pong Assembler
                OCTAL
*0174
First           = .
CycleCount,     0-2                     / 0 - Number of times to cycle before halting
Accumulator,    17                      / Initial value and temp store for the ACC
SemiCycle,      0-10                    / 0 - Semi cycle initial count
Counter,        0                       / Semi cycle counter
*0200
Initialize,     CLA CLL                 / Clear ACC
OuterLoop,      TAD SemiCycle           / Init semi cycle count
                DCA Counter
                TAD Accumulator         / Accumulator display
Loop1,          RAL
                ISZ Counter             / Increment count
                JMP Loop1               / First semi cycle loop ends
                DCA Accumulator         / Save ACC
                TAD SemiCycle           / Init semi cycle count
                DCA Counter
                TAD Accumulator         / Accumulator Display
Loop2,          RAR
                ISZ Counter             / Increment count
                JMP Loop2               / Second semi cycle loop ends
                DCA Accumulator         / Save Acc
/               ISZ CycleCount          / Uncomment to cycle for a bit and end.
                JMP OuterLoop           / Outer loop ends.
                HLT
BufferSize      = 10
Buffer          = .
                *.+BufferSize
BufferEnd       = .
*Initialize
)";

    static constexpr std::string_view Forth = R"(/ A simple Forth implementation
*100
Push,           0
                JMP I PushIndirect
Pop,            0
                JMP I PopIndirect
PushIndirect,   PushBody
PopIndirect,    PopBody
/
*200
                CLA
                TAD Argument
                JMS Push
                JMS Pop
                JMS Pop
                HLT
/
Argument,       5
/

*300
/
PushBody,       DCA PushData
                CLA CMA                 / -1
                TAD ArgStackPtr
                DCA ArgStackPtr
                TAD PushData
                DCA I ArgStackPtr
                JMP I Push
/
PopBody,        CLA CLL
                TAD I ArgStackPtr
                ISZ ArgStackPtr
                JMP I Pop
                HLT

ArgStackPtr,    7777
PushData,       0
*200
)";
}
