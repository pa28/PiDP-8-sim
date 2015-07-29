/*
 * CPU.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef CPU_H_
#define CPU_H_

#include "Device.h"

namespace ca
{
    namespace pdp8
    {

        class CPU: public Device
        {
        public:
            CPU();
            virtual ~CPU();
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CPU_H_ */
