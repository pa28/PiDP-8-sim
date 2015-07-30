/*
 * Chassis.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef CHASSIS_H_
#define CHASSIS_H_

/* Non-IOT device numbers */

#define DEV_CPU         0100                            /* cpu */
#define DEV_MEM         0101                            /* core memmory */
#define DEV_CONSOLE     0102                            /* simulator console */
#define DEV_PANEL       0103                            /* front pannel */
#define DEV_MAX_COUNT   0104                            /* maximum number of devices */

/* Standard device numbers */

#define DEV_PTR         001                             /* paper tape reader */
#define DEV_PTP         002                             /* paper tape punch */
#define DEV_TTI         003                             /* console input */
#define DEV_TTO         004                             /* console output */
#define DEV_CLK         013                             /* clock */
#define DEV_TSC         036
#define DEV_KJ8         040                             /* extra terminals */
#define DEV_FPP         055                             /* floating point */
#define DEV_DF          060                             /* DF32 */
#define DEV_RF          060                             /* RF08 */
#define DEV_RL          060                             /* RL8A */
#define DEV_LPT         066                             /* line printer */
#define DEV_MT          070                             /* TM8E */
#define DEV_CT          070                             /* TA8E */
#define DEV_RK          074                             /* RK8E */
#define DEV_RX          075                             /* RX8E/RX28 */
#define DEV_DTA         076                             /* TC08 */
#define DEV_TD8E        077                             /* TD8E */

#include "CPU.h"
#include "Memory.h"
#include "Console.h"
#include "Panel.h"

namespace ca
{
    namespace pdp8
    {

        class Chassis
        {
            Chassis();

        public:
            virtual ~Chassis();

			static Chassis * instance();

        protected:
			static	Chassis * _instance;
            Device*    deviceList[DEV_MAX_COUNT];


        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* CHASSIS_H_ */
