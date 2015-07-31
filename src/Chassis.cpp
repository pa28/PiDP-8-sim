/*
 * Chassis.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "Chassis.h"

using namespace ca::pdp8;

void sigintHandler(int s) {
    Chassis::instance()->stop();
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

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
