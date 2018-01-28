/*
 * Chassis.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_CHASSIS_H
#define PIDP_CHASSIS_H


#include <csignal>
#include <map>
#include <memory>
#include <sys/time.h>

#include "Device.h"
//#include "CPU.h"
//#include "Memory.h"
//#include "Console.h"
//#include "Panel.h"
//#include "DK8EA.h"

namespace hw_sim
{
    class Chassis : public std::map<int32_t, std::shared_ptr<Device>>
    {
        Chassis();

    public:

        static 		Chassis * instance();

        void    	stop(bool halt=true);
        void		timerHandler();
        void		reset();

        void		setTimerFreq( bool f120 = true );

    protected:
        static		Chassis * _instance;
        int         timeoutCounter;
        bool        timerFreq;
    };

} /* namespace hw_sim */


#endif //PIDP_CHASSIS_H
