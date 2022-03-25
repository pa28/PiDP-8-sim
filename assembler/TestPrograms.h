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
ProgramCounter  = _AutoIndex0
*100
PushData,       0
PCStackPtr,     07777
CodeMask,       07770
SaveCode,       0
CallBase,       CallVector
CallVector,     MachineCode
                Return
                0
                0
                0
                0
                0
                0
/
PushPC,         0
                DCA PushData            / Save the AC
                CLA CMA
                TAD PCStackPtr
                DCA PCStackPtr
                TAD ProgramCounter
                DCA I PCStackPtr
                TAD PushData           / Restore the AC
                JMP I PushPC
/
PopPC,
                DCA PushData            / Save the AC
                TAD I PCStackPtr
                SNA
                JMP PopPcErr
                DCA ProgramCounter
                ISZ PCStackPtr
                TAD PushData            / Restore the AC
                JMP I PopPC
PopPcErr,       HLT

/
*200
                CLA CLL
                DCA ProgramCounter
                JMS PushPC
                TAD InitialPC
                DCA ProgramCounter
/
ForthOpCode,    TAD I ProgramCounter
                DCA SaveCode
                TAD SaveCode
                AND CodeMask
                SNA
                JMP Assembly
                CLA CLL
                JMS PushPC
                TAD SaveCode
                DCA ProgramCounter
                JMP ForthOpCode
Assembly,       TAD CallBase
                TAD SaveCode
                DCA SaveCode
                TAD I SaveCode
                DCA SaveCode
                JMP I SaveCode
MachineCode,    JMP I ProgramCounter
/
Return,         JMS PopPC
                JMP ForthOpCode
/
fn1,            1
fn2,            0
                JMP Return
/
InitialPC,      ForthCode-1
/
ForthCode,      fn2-1
                fn2-1

*200
)";
}
