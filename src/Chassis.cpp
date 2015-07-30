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
	Panel::instance()->stop();
	Console::instance()->stop();
}

int main( int argc, char ** argv ) {
	int sxfd[2];

	if (pipe(sxfd)) {
		perror("pipe");
		exit(1);
	}

	signal( SIGINT, sigintHandler );
	
	Panel *panel = Panel::instance();
	Console *console = Console::instance();
	
	panel->setSwitchFd(sxfd[1]);
	console->setSwitchFd(sxfd[0]);

	console->initialize();
	panel->initialize();
	panel->testLeds(true);
	sleep(5);
	panel->testLeds(false);

	sleep(30);

    return 0;
}

namespace ca
{
    namespace pdp8
    {
		
		Chassis * Chassis::_instance = NULL;

        Chassis::Chassis()
        {
            for (int i = 0; i < DEV_MAX_COUNT; ++i)
                deviceList[i] = NULL;

			// Create and assign devices.
			
            deviceList[DEV_CPU] = CPU::instance();
            deviceList[DEV_MEM] = Memory::instance();
			deviceList[DEV_CONSOLE] = Console::instance();
			deviceList[DEV_PANEL] = Panel::instance();

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

    } /* namespace pdp8 */
} /* namespace ca */
