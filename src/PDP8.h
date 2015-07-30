

#ifndef _PDP8_H_
#define _PDP8_H_

#include <stdio.h>

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

// Requires fmt be a string literal
#define debug(l,fmt,...) do{if(l<DEBUG_LEVEL) fprintf(stderr, "%s: " fmt, __PRETTY_FUNCTION__, __VA_ARGS__)} while(0)

#endif	// _PDP8_H_