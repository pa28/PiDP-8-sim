/*
 * Chassis.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include <unistd.h>
#include "Chassis.h"

using namespace ca::pdp8;

int main( int argc, char ** argv ) {
	Panel *panel = Panel::instance();
	Console *console = Console::instance();
	
	console->initialize();
	panel->initialize();
	panel->testLeds(true);
	panel->setSwitchFd(console->getSwitchFd());

	sleep(30);

	panel->stop();
	console->stop();

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
