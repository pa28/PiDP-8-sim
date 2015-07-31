/*
 * Device.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <string>

namespace ca
{
    namespace pdp8
    {

        class Device
        {
        public:
            Device(std::string name, std::string longName);
            virtual ~Device();

            virtual void initialize() = 0;
			virtual void reset() = 0;
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* DEVICE_H_ */
