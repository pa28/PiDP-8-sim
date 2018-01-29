//
// Created by richard on 26/01/18.
//

#ifndef PIDP_PDP8_H
#define PIDP_PDP8_H

#include <cstdio>
#include <cstdint>

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

// Requires fmt be a string literal
#ifdef SYSLOG
#include <syslog.h>
#define debug(l,...) do{if(l<DEBUG_LEVEL) syslog(LOG_DEBUG,  __VA_ARGS__);} while(false)

#define ERROR(...) syslog(LOG_ERR,  __VA_ARGS__)

#define LOG(...) syslog(LOG_INFO,  __VA_ARGS__)

#else
#define debug(l,...) do{if(l<DEBUG_LEVEL) Console::getConsole()->printf(__VA_ARGS__);} while(false)
#define ERROR(...)
#define LOG(...)
#endif

namespace pdp8 {

/* Non-IOT device numbers */

    constexpr int32_t DEV_MEM =       0100;                           /* core memmory */
    constexpr int32_t DEV_CPU =       0101;                           /* cpu */
    constexpr int32_t DEV_PANEL =     0102;                           /* front pannel */
    constexpr int32_t DEV_CONSOLE =   0103;                           /* simulator console */
    constexpr int32_t DEV_MAX_COUNT = 0104;                           /* maximum number of devices */

/* Standard device numbers */

    constexpr int32_t DEV_PTR =       001;                            /* paper tape reader */
    constexpr int32_t DEV_PTP =       002;                            /* paper tape punch */
    constexpr int32_t DEV_TTI =       003;                            /* console input */
    constexpr int32_t DEV_TTO =       004;                            /* console output */
    constexpr int32_t DEV_CLK =       013;                            /* clock */
    constexpr int32_t DEV_TSC =       036;
    constexpr int32_t DEV_KJ8 =       040;                            /* extra terminals */
    constexpr int32_t DEV_FPP =       055;                            /* floating point */
    constexpr int32_t DEV_DF =        060;                            /* DF32 */
    constexpr int32_t DEV_RF =        060;                            /* RF08 */
    constexpr int32_t DEV_RL =        060;                            /* RL8A */
    constexpr int32_t DEV_LPT =       066;                            /* line printer */
    constexpr int32_t DEV_MT =        070;                            /* TM8E */
    constexpr int32_t DEV_CT =        070;                            /* TA8E */
    constexpr int32_t DEV_RK =        074;                            /* RK8E */
    constexpr int32_t DEV_RX =        075;                            /* RX8E/RX28 */
    constexpr int32_t DEV_DTA =       076;                            /* TC08 */
    constexpr int32_t DEV_TD8E =      077;                            /* TD8E */

    using register_base_t = uint16_t;
    using memory_base_t = uint16_t;

    constexpr register_base_t WORD_SIZE = 12;
}


#endif //PIDP_PDP8_H
