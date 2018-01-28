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

//#include "CPU.h"
//#include "Memory.h"
//#include "Console.h"
//#include "Panel.h"
//#include "DK8EA.h"

namespace hw_sim
{
    class Device
    {
    public:
        Device(std::string & name, std::string & longName) : name(name), longName(longName) {}
        Device(const char * name, const char * longName) : name(name), longName(longName) {}

        virtual ~Device() = default;
        virtual void initialize() = 0;
        virtual void reset() = 0;
        virtual void stop() = 0;
        virtual void tick(int ticksPerSecond) = 0;

        virtual int32_t dispatch(int32_t IR, int32_t dat) { return 0; }

    protected:
        std::string		name, longName;
    };


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

} /* namespace pdp8 */


#endif //PIDP_CHASSIS_H
