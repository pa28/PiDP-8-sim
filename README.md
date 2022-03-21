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

### Running a built in program
![Console Running](https://github.com/pa28/PiDP-8-sim/blob/main/images/Screenshot%20at%202022-03-20%2017-29-13.png)

### Deposit OP Code
![Deposit Op Code](https://github.com/pa28/PiDP-8-sim/blob/main/images/Screenshot%20at%202022-03-20%2020-04-09.png)
