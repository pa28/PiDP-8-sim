/*
 * Chassis.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include "Chassis.h"

int main( int argc, char ** argv ) {
    return 0;
}

namespace ca
{
    namespace pdp8
    {

        Chassis::Chassis()
        {
            for (int i = 0; i < DEV_MAX_COUNT; ++i)
                deviceList[i] = NULL;

            deviceList[DEV_CPU] = new CPU();
            deviceList[DEV_MEM] = new Memory();

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

    } /* namespace pdp8 */
} /* namespace ca */
