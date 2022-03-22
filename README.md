# PiDP-8-sim
A PDP-8 simulator for the Raspberry Pi aimed at running efficiently on the A+ model and working with the PiDP8 kit.

## Updating

It has been quite a while since I've worked on this project, and there
have been a lot of changes to C++ compilers and programming techniques.
Now that I have time to work on this again, it is time to refactor
the code to employ modern techniques.

## Goals

* Implement as many C++20 features as I can in the code.
* Continue to focus on running the simulation efficiently (without consuming Raspberry Pi CPU resources unnecessarily).
* Focus on work-alike rather than faithful simulation. 
* Provide a usable implementation of a fairly basic PDP-8 much like I used in middle school (a PDP-8/E 4K).
* Provide a front panel implementation that can be used on my Linux development machine from within CLion.
  * This is working pretty much the way I want, but not using the method I originally planned. 
* Provide a simple PAL III compliant assembler that can be used to assemble short bits of code up to substantial programs.
  * This is also working though not with complete coverage of all PDP-8 instructions or as a command.
  * It can be used to assemble code for use in deposit commands for example.

### Console Commands

#### Number Radix
All numbers entered into the console are decoded and encoded in octal.

#### Load Address ```l[ ]<d><i><number>```
Places ```<d><i><number>``` in the Data Field, Instruction Field and Program Counter.
White space between the ```l``` and the ```<d><i><number>``` is optional.  The address will be displayed in the
Program Counter and Data and Instruction fields. If fewer than 5 or 6 octal digits are provided the data / instruction
fields will be set to zero.

#### Deposit Value ```d[ ]<number>```
Places ```<number>``` in the address specified in the Instruction Field - Program Counter. The Memory Address and
Memory Buffer registers are updated with the field address and deposited value. The Program Counter is incremented.

#### Deposit Assembler ```d[ ]<assember code>```
*This Feature is not fully operational yet*

The Deposit command may also use assembler instruction semantics:
* ```d cla cll```
* ```d jmp 200```

If the assembler has a symbol table loaded, by using the ```PING PONG``` command for example, the Deposit command
may take advantage of those symbols:
* ```d jmp Initialize```

#### Examine Value ```e```
Retrieves the content of the address specified in the instruction Field - Program Counter. The Memory Address and
Memory Buffer registers are updated with the field address and retrieved value. The value is also displayed in
octal in the console buffer area.

#### Instruction Cycle ```c```
The CPU will execute one (the next) machine cycle (Fetch, Execute or Defer) at the address specified in the
Program Counter and update the panel. 

#### Instruction Step ```s```
The CPU will complete the current instruction, if the Instruction Cycle command has left an instruction partially
executed, or step through a full instruction.

#### Continue ```C```
Places the CPU in run mode, the RUN flag is set to true. The CPU will run until a Halt instruction (HLT) is executed
or the Stop command is used. The panel updates at the end of each instruction. The CPU is run at a speed determined
by the console input timeout which is 10 milliseconds.

#### Stop ```S```
Places the CPU in halt mode, the RUN flag is set to false. The CPU will complete the current instruction, update the
panel and stop.

#### Sample Program - Ping Pong ```PING PONG```
Assembles and loads the sample program coded into the software in ```TestPrograms.h``` into core memory.

#### Read In Memory - RIM Loader ```RIM```
Pokes the binary values that make up the RIM loader into high memory 7756 - 7777 in the field indicated by the
instruction field register.

#### Repeatable Instructions
Three of the commands are repeatable by pressing the Enter key: Examine, Cycle and Step. If the Enter key is held
down the command will be repeated at the key repeat rate.

### Running a built-in program
![Console Running](https://github.com/pa28/PiDP-8-sim/blob/main/images/Screenshot%20at%202022-03-20%2017-29-13.png)

### Deposit OP Code
![Deposit Op Code](https://github.com/pa28/PiDP-8-sim/blob/main/images/Screenshot%20at%202022-03-20%2020-04-09.png)

### Assembler

Assembling this source code:
```
/ Simple Ping-Pong Assembler
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
```

Generates a BIN loader format stream and this listing. The final ```*Initialize``` causes the program start address
to be encoded to the file at the end which allows the BIN file loader included in the CPU to have the program ready
to run as soon as loading is complete.
```
/ Simple Ping-Pong Assembler                                
                               OCTAL                           
0174                           *0174                            
                        First= .                               
0174  7776         CycleCount, 0 - 2                           / 0 - Number of times to cycle before halting
0175  0017        Accumulator, 17                              / Initial value and temp store for the ACC
0176  7770          SemiCycle, 0 - 10                          / 0 - Semi cycle initial count
0177  0000            Counter, 0                               / Semi cycle counter
0200                           *0200                            
0200  7300         Initialize, CLA CLL                         / Clear ACC
0201  1176          OuterLoop, TAD SemiCycle                   / Init semi cycle count
0202  3177                     DCA Counter                     
0203  1175                     TAD Accumulator                 / Accumulator display
0204  7004              Loop1, RAL                             
0205  2177                     ISZ Counter                     / Increment count
0206  5204                     JMP Loop1                       / First semi cycle loop ends
0207  3175                     DCA Accumulator                 / Save ACC
0210  1176                     TAD SemiCycle                   / Init semi cycle count
0211  3177                     DCA Counter                     
0212  1175                     TAD Accumulator                 / Accumulator Display
0213  7010              Loop2, RAR                             
0214  2177                     ISZ Counter                     / Increment count
0215  5213                     JMP Loop2                       / Second semi cycle loop ends
0216  3175                     DCA Accumulator                 / Save Acc
/               ISZ CycleCount          / Uncomment to cycle for a bit and end.                                
0217  5201                     JMP OuterLoop                   / Outer loop ends.
0220  7402                     HLT                             
                   BufferSize= 10                              
                       Buffer= .                               
0231                           *. + BufferSize                  
                    BufferEnd= .                               
0200                           *Initialize                      

     Symbol Table     
0175  Accumulator          
0221  Buffer               
0231  BufferEnd            
0010  BufferSize           
0177  Counter              
0174  CycleCount           
0174  First                
0200  Initialize           
0204  Loop1                
0213  Loop2                
0201  OuterLoop            
0176  SemiCycle            
        
```