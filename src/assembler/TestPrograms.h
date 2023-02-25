/**
 * @file TestPrograms.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-17
 */

#pragma once

#include <string_view>

namespace pdp8asm {

    static constexpr std::string_view DecWriter =
            R"(
                OCTAL
*177
Prompt,         041
                TAD Prompt
Print,          TLS
Loop,           KSF
                JMP Loop
                KRB
                JMP Print
*200
)";

    static constexpr std::string_view PingPong =
            R"(/ Simple Ping-Pong Assembler
                OCTAL
*0174
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
                JMP OuterLoop           / Outer loop ends.
BufferSize      = 10
Buffer,
                *.+BufferSize-1
BufferEnd,
*Initialize
)";

    static constexpr std::string_view Forth = R"(/ A simple Forth implementation
ProgramCounter  = _AutoIndex0
*100
/
/ Locations in the top 0100 (64) bytes of page 0 are accessible from anywhere in the field.
/
/
/ Temporary working storage
PushData,       0
Scratch0,       0
Scratch1,       0
SaveCode,       0
/
/ Constants
CodeMask,       07770
/
/ Stack pointers
PCStackPtr,     07777
StackPtr,       06777
/
/ Static Code call vector
CallBase,       CallVector
CallVector,     MachineCode
ReturnVector,   Return
                LiteralNumber
                Swap
                Over
                0
                0
                0
/
/ Constants for static codes to use as Forth words.
/
FMachineCode    = 0
FReturn         = 1
FLiteralNumber  = 2
FSwap           = 3
FOver           = 4
/
/ Push the program counter onto the PC stack.
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
/ Pop the program counter from the PC stack.
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
/ Push a numeric value onto the number stack.
/
Push,           0
                DCA PushData
                CLA CMA
                TAD StackPtr
                DCA StackPtr
                TAD PushData
                DCA I StackPtr
                JMP I Push
/
/ Pop a numeric value from the number stack.
/
Pop,            0
                CLA
                TAD I StackPtr
                ISZ StackPtr
                JMP I Pop
                HLT
/
/ Trampolines for jumps and subroutine calls
/
_Clear,         Clear                   / Subroutine Clear
_Return,        Return                  / Jump to Return
/
/
/
*200
                JMS I _Clear
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
/ Machine code passes control to machine instructions in the word body. Must be followed by
/ JMP Return for code on the same page, or JMP I ReturnVector
/
MachineCode,    JMP I ProgramCounter    / Jump to machine code, must be terminated by JMP Return
/
/ Return function at the end of a word body
/
Return,         JMS PopPC
                JMP ForthOpCode
/
/ Push the next word in the body on the argument stack.
/
LiteralNumber,  CLA CLL
                TAD I ProgramCounter
                JMS Push
                JMP ForthOpCode
/
/ Compute a pointer to next on stack and store in Scratch0. Leaves AC == 0
/
NextOnStack,    0
                CLA CML RAL
                TAD StackPtr
                DCA Scratch0
                JMP I NextOnStack
/
Swap,           JMS NextOnStack
                TAD I NextOnStack
                DCA Scratch1
                TAD I StackPtr
                DCA I Scratch0
                TAD Scratch1
                DCA I StackPtr
                JMP ForthOpCode
/
Over,           JMS NextOnStack
                TAD I Scratch0
                JMS Push
                JMP ForthOpCode

*300
/
/ Clear the abstract machine
/
InitPCStackPtr, 07000
InitialPC,      ForthCode-1
/
Clear,          0
                CLA CLL
                DCA StackPtr            / Set the stack pointer to 0
                TAD InitPCStackPtr
                DCA PCStackPtr          / SEt the program counter stack pointer
                DCA ProgramCounter
                JMS PushPC              / Put a zero PC guard on the top of the stack
                TAD InitialPC           / Set starting point for Forth
                DCA ProgramCounter
                JMP I Clear
/
/ Duplicate the top of stack
/
Dup,            FMachineCode
                CLA
                TAD I StackPtr
                JMS Push
                JMP I _Return
/
/ Dup2 is a Forth word made up of static codes.
/ Forth words are use 12 bit addresses so page numbers are not important.
/
Dup2,           FOver
                FOver
                FReturn
/
ForthCode,      2
                1
                2
                2


*200
)";
}
