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
                CLA CMA                 / -1
                TAD PCStackPtr          / Decrement PC StackPointer
                DCA PCStackPtr
                TAD ProgramCounter      / Put the current PC on the stack
                DCA I PCStackPtr
                TAD PushData            / Restore the AC
                JMP I PushPC
/
PopPC,
                DCA PushData            / Save the AC
                TAD I PCStackPtr        / Pull the pas PC off the stack
                SNA                     / Skip if not zero
                JMP PopPcErr            / A Zero PC is an error
                DCA ProgramCounter      / Put the past PC in the register
                ISZ PCStackPtr          / Adjust the stack pointer
                TAD PushData            / Restore the AC
                JMP I PopPC
PopPcErr,       HLT

/
*200
                CLA CLL
                DCA ProgramCounter      / Put a zero PC guard on the top of the stack
                JMS PushPC
                TAD InitialPC           / Initialize the Program Counter
                DCA ProgramCounter
/
/ Start of the Forth word processing loop.
/
ForthOpCode,    TAD I ProgramCounter    / Fetch the first Forth word
                DCA SaveCode            / Save the word for future use
                TAD SaveCode            / Get the code back
                AND CodeMask            / Mask off low order bits to find out if it is a static code.
                SNA                     / Skip if not a static code.
                JMP StaticCode          / Jump to process static codes.
                CLA CLL
                JMS PushPC              / Push the current Program Counter
                TAD SaveCode            / Get the word back
                DCA ProgramCounter      / Put it in the Program Counter
                JMP ForthOpCode         / Loop
/
/ Handle static codes.
/
StaticCode,     TAD CallBase            / Get the call vector base address
                TAD SaveCode            / Add the save code
                DCA SaveCode            / Put this back in a storage area
                TAD I SaveCode          / Load the process address from the vector
                DCA SaveCode            / Save that
                JMP I SaveCode          / Jump to the appropriate process function
/
/ Machine code passes control to machine instructions in the word body
/
MachineCode,    JMP I ProgramCounter
/
/ Return function at the end of a word body
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
