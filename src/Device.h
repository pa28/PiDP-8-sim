/*
 * Device.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <string>
#include <stdint.h>

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
			virtual void stop() = 0;

			virtual int32_t dispatch(int32_t IR, int32_t dat) { return 0; }
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* DEVICE_H_ */
