/*
 * CPU.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include "CPU.h"

namespace ca
{
    namespace pdp8
    {

		CPU * CPU::_instance = NULL;

        CPU::CPU() :
                Device("CPU", "CPU"),
                PC(0), IF(0), DF(0), LAC(0), MQ(0), SC(0)
        {
            // TODO Auto-generated constructor stub

        }

        CPU::~CPU()
        {
            // TODO Auto-generated destructor stub
        }

		CPU * CPU::instance() {
			if (_instance == NULL) {
				_instance = new CPU();
			}

			return _instance;
		}

    } /* namespace pdp8 */
} /* namespace ca */
