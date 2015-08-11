/*
 * Chassis.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: richard
 */

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
    Chassis::instance()->stop();
}

void timerHandler(int s) {
	Chassis::instance()->timerHandler();
}

bool    daemonMode = true;

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

			Memory::instance()->loadFile("/nas/pi/PiDP-8-sim/pal/DeepThought.bin");
			//Memory::instance()->loadFile("/nas/pi/PiDP-8-sim/pal/Test.bin");

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

		void Chassis::stop() {
			LOG( "PIDP-8/I simulator exiting");
		    for (int i = 0; i < DEV_MAX_COUNT; ++i) {
		        if (deviceList[i] != NULL) {
		            deviceList[i]->stop();
		        }
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
