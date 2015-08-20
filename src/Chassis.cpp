/*
 * Chassis.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef SYSLOG
#include <syslog.h>
#endif

#define DEBUG_LEVEL 5
#include "PDP8.h"
#include "Chassis.h"
#include "Console.h"

using namespace ca::pdp8;

void sigintHandler(int s) {
    Chassis::instance()->stop(false);
}

void timerHandler(int s) {
	Chassis::instance()->timerHandler();
}

bool    daemonMode = true;

int32_t	DeepThought[] = {
	000000, 006133, 007000, 007200, 007404, 003072, 001072, 000177, 006134,
	007200, 001072, 007002, 000177, 003072, 006136, 003000, 001000, 007002,
	000000, 000072, 006135, 006136, 003000, 006136, 000000, 003000, 004063,
	007421, 004063, 003037, 007403, 000000, 004063, 003074, 006136, 007004,
	004063, 000176, 003071, 004063, 000175, 007450, 001174, 001071, 006005,
	006136, 007004, 007200, 001074, 006001, 005400, 000000, 006136, 003073,
	006136, 000073, 005463, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000010, 000070,
	000007, 000077, 007200, 003236, 001234, 003204, 006211, 007200, 001236,
	000377, 001233, 003636, 002236, 005206, 001204, 001376, 003204, 001204,
	000232, 007440, 005204, 006201, 007200, 006135, 001235, 006134, 006131,
	005001, 000070, 005200, 006211, 000006, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
	000000, 000000, 000010, 000177
};

int main( int argc, char ** argv ) {

#ifdef SYSLOG
    openlog( "pidp8", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "pidp8 - PDP-8/I simulator starting.");
#endif

    if (daemonMode) {
        pid_t pid, sid;

        pid = fork();
        if (pid < 0) {
            ERROR( "Error starting daemon mode %s", strerror(errno) );
            exit(1);
        }

        if (pid > 0) {
            exit(0);
        }

        umask(0);
        sid = setsid();
        if (sid < 0) {
            ERROR( "Error setting process group %s", strerror(errno) );
            exit(1);
        }

		LOG("Entering daemon mode.");

        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

	signal( SIGINT, sigintHandler );
	signal( SIGTERM, sigintHandler );

	Chassis *chassis = Chassis::instance();
	Console::instance()->run();

    return 0;
}

namespace ca
{
    namespace pdp8
    {

		Chassis * Chassis::_instance = NULL;

        Chassis::Chassis() :
                timeoutCounter(0),
                timerFreq(false)
        {
            int sxfd[2];

            if (pipe(sxfd)) {
                perror("pipe");
                exit(1);
            }

			struct sigaction sa;

			memset( &sa, 0, sizeof(sa));
			sa.sa_handler = &::timerHandler;
			sigaction( SIGALRM, &sa, NULL);

            for (int i = 0; i < DEV_MAX_COUNT; ++i)
                deviceList[i] = NULL;

			// Create and assign devices.

            deviceList[DEV_MEM] = Memory::instance();
            deviceList[DEV_CPU] = CPU::instance();
            deviceList[DEV_PANEL] = Panel::instance();
			deviceList[DEV_CONSOLE] = Console::instance(daemonMode);

			deviceList[DEV_CLK] = DK8EA::instance();

		    Panel::instance()->setSwitchFd(sxfd[1]);
		    Console::instance()->setSwitchFd(sxfd[0]);

			// Initialize devices

            for (int i = 0; i < DEV_MAX_COUNT; ++i) {
                if (deviceList[i] != NULL) {
                    deviceList[i]->initialize();
                    deviceList[i]->reset();
                }
            }

			//Memory::instance()->loadFile("/nas/pi/PiDP-8-sim/pal/DeepThought.bin");
			//Memory::instance()->loadFile("/nas/pi/PiDP-8-sim/pal/Test.bin");
			//Memory::instance()->saveFile("/nas/pi/PiDP-8-sim/pal/DeepThought.txt", 0, 0400);
			Memory::instance()->initMemory( DeepThought, 0, 0400, 0200);

			setTimerFreq(true);
        }

        Chassis::~Chassis()
        {
            // TODO Auto-generated destructor stub
        }

		void Chassis::reset() {
			for (int i = 0; i < DEV_MAX_COUNT; ++i) {
				if (deviceList[i] != NULL) {
					deviceList[i]->reset();
				}
			}
		}

		Chassis * Chassis::instance() {
			if (_instance == NULL) {
				_instance = new Chassis();
			}

			return _instance;
		}

		void Chassis::stop(bool halt) {
			LOG( "PIDP-8/I simulator exiting");
		    for (int i = 0; i < DEV_MAX_COUNT; ++i) {
		        if (deviceList[i] != NULL) {
		            deviceList[i]->stop();
		        }
		    }

			if (halt) {
				system( "/sbin/shutdown -h now" );
			}
		}

		void Chassis::setTimerFreq( bool f120 ) {
			struct itimerval timer;
			timer.it_value.tv_sec = 0;
			timer.it_value.tv_usec = (f120 ? 8333 : 10000);	// 120Hz 10000 for 100Hz

			timer.it_interval.tv_sec = 0;
			timer.it_interval.tv_usec = (f120 ? 8333 : 10000);

			timerFreq = f120;
			setitimer( ITIMER_REAL, &timer, NULL );
		}

		void Chassis::timerHandler() {
			CPU::instance()->timerTick();
			DK8EA::instance()->tick();

			++timeoutCounter;
			if (timeoutCounter > (timerFreq? 120 : 100)) {
			    timeoutCounter = 0;
			    Console::instance()->oneSecond();
			}
		}

    } /* namespace pdp8 */
} /* namespace ca */
/* vim: set ts=4 sw=4  noet autoindent : */
