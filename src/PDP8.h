

#ifndef _PDP8_H_
#define _PDP8_H_

#include <stdio.h>
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
#define debug(l,...) do{if(l<DEBUG_LEVEL) Console::instance()->printf( "%s: " fmt, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
#define ERROR(...)
#define LOG(...)
#endif

#endif	// _PDP8_H_
