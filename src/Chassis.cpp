/*
 * Chassis.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DEBUG_LEVEL 5
#include "PDP8.h"
#include "Chassis.h"

using namespace ca::pdp8;

void sigintHandler(int s) {
    Chassis::instance()->stop();
}

void timerHandler(int s) {
	Chassis::instance()->timerHandler();
}

int main( int argc, char ** argv ) {
	signal( SIGINT, sigintHandler );

	Chassis *chassis = Chassis::instance();
	Console::instance()->run();

    return 0;
}

namespace ca
{
    namespace pdp8
    {

		Chassis * Chassis::_instance = NULL;

        Chassis::Chassis()
        {
            int sxfd[2];

            if (pipe(sxfd)) {
                perror("pipe");
                exit(1);
            }

			struct sigaction sa;

			memset( &sa, 0, sizeof(sa));
			sa.sa_handler = &::timerHandler;
			sigaction( SIGALRM, &sa, NULL);

            for (int i = 0; i < DEV_MAX_COUNT; ++i)
                deviceList[i] = NULL;

			// Create and assign devices.

            deviceList[DEV_MEM] = Memory::instance();
            deviceList[DEV_CPU] = CPU::instance();
            deviceList[DEV_PANEL] = Panel::instance();
			deviceList[DEV_CONSOLE] = Console::instance();

		    Panel::instance()->setSwitchFd(sxfd[1]);
		    Console::instance()->setSwitchFd(sxfd[0]);

			// Initialize devices

            for (int i = 0; i < DEV_MAX_COUNT; ++i) {
                if (deviceList[i] != NULL) {
                    deviceList[i]->initialize();
                }
            }

			Memory::instance()->loadFile("/nas/pi/PiDP-8-sim/Test.bin");

            Console::instance()->printf("PiDP-8-sim started.\n");

			setTimerFreq(false);
        }

        Chassis::~Chassis()
        {
            // TODO Auto-generated destructor stub
        }

		Chassis * Chassis::instance() {
			if (_instance == NULL) {
				_instance = new Chassis();
			}

			return _instance;
		}

		void Chassis::stop() {
		    for (int i = 0; i < DEV_MAX_COUNT; ++i) {
		        if (deviceList[i] != NULL) {
		            deviceList[i]->stop();
		        }
		    }
		}

		void Chassis::setTimerFreq( bool f120 ) {
			struct itimerval timer;
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = (f120 ? 8333 : 100000);	// 120Hz 10000 for 100Hz

			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = (f120 ? 8333 : 100000);

			setitimer( ITIMER_REAL, &timer, NULL );
		}

		void Chassis::timerHandler() {
			debug( 10, "%d\n", 0);
			CPU::instance()->timerTick();
		}

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
