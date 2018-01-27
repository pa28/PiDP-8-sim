//
// Created by richard on 26/01/18.
//

#ifndef PIDP_PDP8_H
#define PIDP_PDP8_H

#include <cstdio>
#include <cstdint>
#include "Console.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

// Requires fmt be a string literal
#ifdef SYSLOG
#include <syslog.h>
#define debug(l,...) do{if(l<DEBUG_LEVEL) syslog(LOG_DEBUG,  __VA_ARGS__);} while(0)

#define ERROR(...) syslog(LOG_ERR,  __VA_ARGS__)

#define LOG(...) syslog(LOG_INFO,  __VA_ARGS__)

#else
#define debug(l,...) do{if(l<DEBUG_LEVEL) Console::instance()->printf(__VA_ARGS__);} while(0)
#define ERROR(...)
#define LOG(...)
#endif

namespace pdp8 {

    using register_base_t = uint16_t;
    using memory_base_t = uint16_t;

}

#endif //PIDP_PDP8_H
