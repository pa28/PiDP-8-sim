

#ifndef _PDP8_H_
#define _PDP8_H_

#include <stdio.h>
#include "Console.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

// Requires fmt be a string literal
#ifdef SYSLOG
#define debug(l,fmt,...) do{if(l<DEBUG_LEVEL) syslog(LOG_DEBUG, "%s: " fmt, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)

#define ERROR(fmt,...) syslog(LOG_ERR, "%s: " fmt, __PRETTY_FUNCTION__, __VA_ARGS__)

#define LOG(fmt,...) syslog(LOG_INFO, fmt, __VA_ARGS__)

#else
#define debug(l,fmt,...) do{if(l<DEBUG_LEVEL) Console::instance()->printf( "%s: " fmt, __PRETTY_FUNCTION__, __VA_ARGS__);} while(0)
#define ERROR(fmt,...)
#define LOG(fmt,...)
#endif

#endif	// _PDP8_H_
