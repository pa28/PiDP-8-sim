/*
 * Console.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
   Portions of this program are based substantially on work by Robert M Supnik
   The license for Mr Supnik's work follows:
   Copyright (c) 1993-2013, Robert M Supnik
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.
 */


#include <cstdio>
#include <cstdarg>
#include <unistd.h>
#include <ncurses.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cerrno>

#include "PDP8.h"
#include "Console.h"

using namespace std;
using namespace hw_sim;

namespace pdp8
{
    Console * Console::_instance = nullptr;

    Console::Console(bool headless) :
            Device("CONS", "Console"),
            runConsole(false),
            stopMode(false),
            switchPipe(-1),
            stopCount(0),
            tickCount(0),
            consoleTerm(nullptr)
    {
        if (!headless) {
            consoleTerm = new VirtualPanel();
        }
        pthread_mutex_init( &mutex, nullptr );
    }

    Console::~Console()
    {
        pthread_mutex_destroy( &mutex );
    }

    int Console::printf( const char * format, ... ) {
        if (consoleTerm == nullptr) {
            return 0;
        }

        int n = -1;
        try {
            va_list args;
            va_start (args, format);
            consoleTerm->vconf(format, args);
            va_end (args);
        } catch (LockException &le ) {
            consoleTerm->printw( le.what() );
        }
        return n;
    }

    void Console::initialize() {
        runConsole = true;
    }

    void Console::reset() {

    }

    std::shared_ptr<Console> Console::getConsole() {
        std::shared_ptr<Console> console;
        auto devItr = Chassis::instance()->find(DEV_CONSOLE);
        if (devItr != Chassis::instance()->end()) {
            console = std::dynamic_pointer_cast<Console>(devItr->second);
        }

        return console;
    }


    int Console::run() {
        fd_set	rd_set{}, wr_set{};

        auto cpu = CPU::getCPU();
        if (cpu == nullptr) {
            return ENODEV;
        }

        uint32_t switchstatus[SWITCHSTATUS_COUNT] = { 0 };

        if (consoleTerm)
            consoleTerm->updatePanel(switchstatus);

        while (runConsole) {
            FD_ZERO(&rd_set);
            FD_ZERO(&wr_set);

            int	n = 0;

            if (consoleTerm != nullptr) {
                FD_SET(consoleTerm->fdOfInput(), &rd_set); n = consoleTerm->fdOfInput() + 1;
            }

            if (switchPipe >= 0) {
                FD_SET(switchPipe, &rd_set);
                n = max(n,switchPipe + 1);
            }

            int s;

#ifdef HAS_PSELECT
            s = pselect( n, &rd_set, nullptr, nullptr, nullptr, nullptr);
#else
            s = select( n, &rd_set, nullptr, nullptr, nullptr);
#endif

            if (s < 0) {
                switch(errno) {
                    case EBADF:	    // invalid FD
                        break;
                    case EINTR:     // Signal was caught
                        break;
                    case EINVAL:	// invalid n or timeout
                        break;
                    case ENOMEM:	// no memory
                        break;
                    default:
                        break;
                }
            } else if (s > 0) {
                // one or more fd ready
                if (consoleTerm != nullptr) {
                    if (FD_ISSET(consoleTerm->fdOfInput(), &rd_set)) {
                        consoleTerm->processStdin();
                    }
                } else if (FD_ISSET(switchPipe, &rd_set)) {
                    // Panel
                    uint32_t switchReport[2];

                    ssize_t sxn = read(switchPipe, switchReport, sizeof(switchReport));
                    if (sxn == sizeof(switchReport)) {
                        // handle switches
                        switchReport[1] ^= 07777;

                        switch (switchReport[0]) {
                            case 0: // Switch register
                                switchstatus[0] = switchReport[1];
                                debug(1, "SR: %04o\n", switchstatus[0]);
                                if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                cpu->OSR = switchstatus[0];
                                break;
                            case 1: // DF and IF
                                switchstatus[1] = switchReport[1];
                                debug(1, "DF: %1o  IF: %1o\n", ((switchstatus[1] >> 9) & 07), ((switchstatus[1] >> 6) & 07) );
                                if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                break;
                            case 2: // Command switches
                                switchstatus[2] = switchReport[1];
                                debug(1, "Cmd: %02o  SS: %1o\n", (switchstatus[2] >> 6) & 077, (switchstatus[2] >> 4) & 03 );
                                break;
                            default:
                                break;
                        }

                        debug(1, "Sx: %04o %04o %04o\n", switchstatus[0],  switchstatus[2], switchstatus[2]);

                        cpu->setStepping(static_cast<CPUStepping>((switchstatus[2] >> 4) & 03));

                        if (cpu->getStepping() == PanelCommand) {
                            switch ((switchstatus[2] >> 6) & 077) {
                                case PanelNoCmd:
                                    stopMode = false;
                                    stopCount = 8;
                                    break;
                                case PanelStart:
                                    break;
                                case PanelLoadAdr:
                                    break;
                                case PanelDeposit:
                                    break;
                                case PanelExamine:
                                    break;
                                case PanelContinue:
                                    break;
                                case PanelStop:
                                    stopMode = true;
                                    stopCount = 8;
                                    break;
                                default:
                                    break;
                            }
                        } else {
                            switch ((switchstatus[2] >> 6) & 077) {
                                case PanelStart:
                                    debug(1, "%s\n", "PanelStart");
                                    Chassis::instance()->reset();
                                    cpu->setCondition(CPURunning);
                                    cpu->cpuContinue();
                                    break;
                                case PanelLoadAdr:
                                    cpu->PC = switchstatus[0];
                                    cpu->DF = ((switchstatus[1] >> 9) & 07);
                                    cpu->IF = ((switchstatus[1] >> 6) & 07);
                                    if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                    break;
                                case PanelDeposit:
                                    cpu->setMA(cpu->IF, cpu->PC);
                                    cpu->M[cpu->MA] = switchstatus[0];
                                    ++(cpu->PC);
                                    cpu->setState(DepositState);
                                    if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                    break;
                                case PanelExamine:
                                    cpu->setMA(cpu->IF, cpu->PC);
                                    switchReport[1] = cpu->M[cpu->MA];
                                    ++(cpu->PC);
                                    cpu->setState(ExamineState);
                                    if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                    break;
                                case PanelContinue:
                                    debug(1, "%s\n", "PanelContinue");
                                    cpu->setCondition(CPURunning);
                                    cpu->cpuContinue();
                                    if (consoleTerm) consoleTerm->updatePanel( switchstatus );
                                    break;
                                case PanelStop:
                                    debug(1, "%s\n", "PanelStop");
                                    cpu->cpuStop();
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
//#endif

            //getch();
            //runConsole = false;
        }
        printf("Console exiting.\n");
        delete consoleTerm;
        return 0;
    }

    void Console::tick(int ticksPerSecond) {
        if (tickCount == 0) {
            tickCount = ticksPerSecond;
            oneSecond();
        } else {
            --tickCount;
        }
    }

    void Console::oneSecond() {
        // timeout
        debug(10, "Timeout mode %d, count %d\n", stopMode, stopCount );
        if (stopMode) {
            --stopCount;
            if (stopCount < 0) {
                Chassis::instance()->stop();
            }
        }
    }
} /* namespace pdp8 */
