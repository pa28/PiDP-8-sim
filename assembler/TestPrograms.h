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
Push,           0
                JMP I _Push_
_Push_,         _Push
/
Pop,            0
                JMP I _Pop_
_Pop_,          _Pop
/
PushPC,         0
                JMP I _PushPC_
_PushPC_,       _PushPC
/
PopPC,          0
                JMP I _PopPC_
_PopPC_,        _PopPC
/
ToAssembler,    0
                JMP I ProgramCounter
/
*200
                CLA CLL
                DCA ProgramCounter
                JMS PushPC
                TAD InitialPC
                DCA ProgramCounter
                JMP ThreadOp
InitialPC,      ForthStart-1
/
/
_Push,          DCA PushData
                CLA CMA
                TAD ArgStackPtr
                DCA ArgStackPtr
                TAD PushData
                DCA I ArgStackPtr
                JMP I Push
/
_Pop,           CLA CLL
                TAD I ArgStackPtr
                ISZ ArgStackPtr
                JMP I Pop
                HLT
/
_PushPC,        DCA PushData            / Save the AC
                CLA CMA
                TAD PCStackPtr
                DCA PCStackPtr
                TAD ProgramCounter
                DCA I PCStackPtr
                TAD PushData           / Restore the AC
                JMP I PushPC
/
_PopPC,         DCA PushData            / Save the AC
                TAD I PCStackPtr
                SNA
                JMP PopPcErr
                DCA ProgramCounter
                ISZ PCStackPtr
                TAD PushData            / Restore the AC
                JMP I PopPC
PopPcErr,       HLT
/
ThreadOp,       TAD I ProgramCounter    / Increment PC and load next Forth binary word to AC
                SZA                     / Zero indicates in-line assembly call.
                JMP ThreadCall          / Jump to threaded call
                JMS ToAssembler         / Call the in-line assembly
                JMP ThreadReturn        / Return from the word
/ This is a threaded call, so push the program counter on the stack an go to the new one.
/
ThreadCall,     JMS PushPC              / Push the ProgramCounter
                DCA ProgramCounter      / Call next word
                JMP ThreadOp
ThreadReturn,   JMS PopPC
                JMP ThreadOp
/
/
PCStackPtr,     6777
ArgStackPtr,    7777
PushData,       0
/
/
ForthPop,       0
                JMS Pop
                JMP I ToAssembler
/
ForthPush,      0
                JMS Push
                JMP I ToAssembler
/
ForthDup,       ForthPop-1
                ForthPush-1
                ForthPush-1
                ForthReturn-1
/
ForthReturn,    0
                JMS PopPC
                JMP I ToAssembler
/
ForthJmp,       0
                JMS PopPC
_ForthJmp,      TAD I ProgramCounter
                DCA ProgramCounter
                JMP ThreadOp
/
ForthIf,        0
                JMS PopPC
                JMS Pop
                SNA
                JMP _ForthJmp
                ISZ ProgramCounter
                JMP ThreadOp
/
ForthTrue,      0
                CLA CMA
                JMS Push
                JMP I ToAssembler
/
ForthFalse,     0
                CLA
                JMS Push
                JMP I ToAssembler
/
ForthStart,     ForthTrue-1
                ForthIf-1
                ForthStart-1
                ForthJmp-1
                ForthStart-1

*200
)";
}
