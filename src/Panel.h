/*
 * Panel.h
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#ifndef PANEL_H_
#define PANEL_H_

#include "Device.h"
#include "Thread.h"

#include <stdio.h>

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

namespace ca
{
    namespace pdp8
    {

        class Panel: public Device, Thread
        {
        public:
            Panel();
            virtual ~Panel();

            virtual int run();

        protected:
            bool    driveLeds;
        };

    } /* namespace pdp8 */
} /* namespace ca */

#endif /* PANEL_H_ */
