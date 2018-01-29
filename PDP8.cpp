//
// Created by richard on 27/01/18.
//

#include <unistd.h>
#include <sys/stat.h>
#include "PDP8.h"
#include "Chassis.h"
#include "Memory.h"
#include "CPU.h"
#include "Console.h"

using namespace hw_sim;
using namespace pdp8;

bool    daemonMode = false;

int32_t	DeepThought[] = {
        000000, 006133, 007000, 007200, 007404, 003071, 001071, 000177,
        006134, 007200, 001071, 007002, 000177, 003071, 006136, 003000,
        001000, 007002, 000000, 000071, 006135, 004061, 003000, 004061,
        007421, 004061, 007040, 003035, 007403, 000000, 004061, 003073,
        006136, 007004, 004061, 000176, 003070, 004061, 000175, 007450,
        001174, 001070, 006005, 006136, 007004, 007200, 001073, 006001,
        005400, 000000, 006136, 007000, 003072, 006136, 000072, 005461,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000010, 000070, 000007, 000077,
        007200, 003236, 001234, 003234, 006211, 007200, 001236, 000377,
        001233, 003636, 002236, 005206, 001204, 001376, 003204, 001204,
        000232, 007440, 005204, 006201, 007200, 006135, 001235, 006134,
        006131, 005001, 000070, 005200, 000211, 000006, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
        000000, 000000, 000000, 000000, 000000, 000000, 000010, 000177,
};

void sigintHandler(int) {
    hw_sim::Chassis::instance()->stop(false);
}

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

    getchar(); // Allow time to attach the debugger if needed.

    signal( SIGINT, sigintHandler );
    signal( SIGTERM, sigintHandler );

    Chassis *chassis = Chassis::instance();

    (*chassis)[DEV_MEM] = std::make_shared<Memory<MAXMEMSIZE, uint16_t, 12>>();
    (*chassis)[DEV_CPU] = std::make_shared<CPU>();
    (*chassis)[DEV_CONSOLE] = std::make_shared<Console>(false);

    chassis->reset();
    chassis->initialize();

    auto mi = Memory<MAXMEMSIZE,memory_base_t,12>::getMemory(DEV_MEM)->begin();
    for (auto d: DeepThought) {
        *mi = d;
        ++mi;
    }

    Console::getConsole()->run();

    return 0;
}

