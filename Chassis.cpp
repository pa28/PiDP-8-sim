/*
 * Chassis.cpp
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

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#ifdef SYSLOG
#include <syslog.h>
#endif

#define DEBUG_LEVEL 5
#include "Chassis.h"

void timerHandler(int) {
    hw_sim::Chassis::instance()->timerHandler();
}

namespace hw_sim
{

    Chassis * Chassis::_instance = nullptr;

    Chassis::Chassis() :
            timeoutCounter(0),
            timerFreq(false)
    {
        int sxfd[2];

        if (pipe(sxfd)) {
            perror("pipe");
            exit(1);
        }

        struct sigaction sa{};

        memset( &sa, 0, sizeof(sa));
        sa.sa_handler = &::timerHandler;
        sigaction( SIGALRM, &sa, nullptr);

        // Create and assign devices.

//        deviceList[DEV_MEM] = Memory::instance();
//        deviceList[DEV_CPU] = CPU::instance();
//        deviceList[DEV_PANEL] = Panel::instance();
//        deviceList[DEV_CONSOLE] = Console::instance(daemonMode);
//
//        deviceList[DEV_CLK] = DK8EA::instance();
//
//        Panel::instance()->setSwitchFd(sxfd[1]);
//        Console::instance()->setSwitchFd(sxfd[0]);

        // Initialize devices

        for (auto device: *this) {
            device.second->initialize();
            device.second->reset();
        }

        setTimerFreq(true);
    }

    void Chassis::reset() {
        for (auto device: *this) {
            device.second->reset();
        }
    }

    Chassis * Chassis::instance() {
        if (_instance == nullptr) {
            _instance = new Chassis();
        }

        return _instance;
    }

    void Chassis::stop(bool halt) {
//        LOG("PIDP-8/I simulator exiting");
        for (auto device: *this) {
            device.second->stop();
        }

        if (halt) {
            system( "/sbin/shutdown -h now" );
        }
    }

    void Chassis::setTimerFreq( bool f120 ) {
        struct itimerval timer{};
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = (f120 ? 8333 : 10000);	// 120Hz 10000 for 100Hz

        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = (f120 ? 8333 : 10000);

        timerFreq = f120;
        setitimer( ITIMER_REAL, &timer, nullptr );
    }

    void Chassis::timerHandler() {
        for (auto device: *this) {
            device.second->tick(timerFreq ? 120 : 100);
        }
    }

} /* namespace hw_sim */
