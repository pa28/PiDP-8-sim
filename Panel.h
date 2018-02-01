/*
 * Panel.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_PANNEL_H
#define PIDP_PANNEL_H


#include "PDP8.h"
#include "Device.h"
#include "Thread.h"
#include "CPU.h"
#include "Memory.h"

#include <cstdio>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h> // extra


//#define BCM2708_PERI_BASE       0x3f000000
//#define GPIO_BASE               (BCM2708_PERI_BASE + 0x200000)    // GPIO controller

#define BLOCK_SIZE      (4*1024)

// IO Acces
struct bcm2835_peripheral {
    unsigned long addr_p;
    int mem_fd;
    void *map;
    volatile unsigned int *addr;
};

//struct bcm2835_peripheral gpio = {GPIO_BASE};

namespace pdp8
{

    /*******************************************************************************************************************
        PiDP8 Front Panel Driver.
        This is a singleton class that maintains a thread which multiplexes simulator status information onto
        a bank of LEDs for display, and scans a bank of swtiches for input.
    ********************************************************************************************************************/

    class Panel: public Device, Thread
    {
    public:
        Panel();

        virtual ~Panel();

        void initialize();
        void reset();

        virtual void * run();
        virtual void tick(int);

        void stop() { driveLeds = false; }
        void setSwitchFd(int fd) { if (switchFd >= 0) close(switchFd); switchFd = fd; }

        void testLeds(bool ledsOn);		// Turn all LEDs on (if true) or off (if false)

    protected:
        bool    driveLeds;
        int		switchFd;				// Switch status is written here when it changes.
        int		ledBlink;				// A counter to blink LEDs for special status

        pthread_mutex_t		accessMutex;

        void getLeds();

    };

} /* namespace pdp8 */



#endif //PIDP_PANNEL_H
