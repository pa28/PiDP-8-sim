//
// Created by richard on 06/08/18.
//

#ifndef CPU_SIM_PDP8_H
#define CPU_SIM_PDP8_H

#include <concepts>
#include "hardware.h"

namespace pdp8 {

    struct output_policy : sim::register_output_policy<8,4,'0',false>{};

    struct cpu_store_policy : sim::register_store_policy<true,true>{};

    struct memory_store_policy : sim::register_store_policy<true,false>{};

    using register_t = sim::hw_register<16,output_policy,cpu_store_policy>;
    using memory_t = sim::hw_register<16,output_policy,memory_store_policy>;

    using base_type = register_t::base_type;
    using memory_type = memory_t::base_type;

    using device_bus_t = int32_t;

    template <size_t width, size_t offset>
    struct pdp8_off {
        static_assert(offset < width, "PDP8 Offsets must be less than register width.");
        static constexpr size_t value = width - offset - 1;
    };

    template <size_t width, size_t offset>
    inline constexpr size_t pdp8_off_v = pdp8_off<width,offset>::value;


    class chassis;

    inline constexpr base_type OP_KSF =         06031;                        /* for idle */
    inline constexpr base_type OP_CLSC =        06053;                        /* for idle */

    inline constexpr uint8_t INT_V_START =    0;                               /* enable start */
    inline constexpr uint8_t INT_V_LPT =      (INT_V_START+0);                 /* line printer */
    inline constexpr uint8_t INT_V_PTP =      (INT_V_START+1);                 /* tape punch */
    inline constexpr uint8_t INT_V_PTR =      (INT_V_START+2);                 /* tape reader */
    inline constexpr uint8_t INT_V_TTO =      (INT_V_START+3);                 /* terminal */
    inline constexpr uint8_t INT_V_TTI =      (INT_V_START+4);                 /* keyboard */
    inline constexpr uint8_t INT_V_CLK =      (INT_V_START+5);                 /* clock */
    inline constexpr uint8_t INT_V_TTO1 =     (INT_V_START+6);                 /* tto1 */
    inline constexpr uint8_t INT_V_TTO2 =     (INT_V_START+7);                 /* tto2 */
    inline constexpr uint8_t INT_V_TTO3 =     (INT_V_START+8);                 /* tto3 */
    inline constexpr uint8_t INT_V_TTO4 =     (INT_V_START+9);                 /* tto4 */
    inline constexpr uint8_t INT_V_TTI1 =     (INT_V_START+10);                /* tti1 */
    inline constexpr uint8_t INT_V_TTI2 =     (INT_V_START+11);                /* tti2 */
    inline constexpr uint8_t INT_V_TTI3 =     (INT_V_START+12);                /* tti3 */
    inline constexpr uint8_t INT_V_TTI4 =     (INT_V_START+13);                /* tti4 */
    inline constexpr uint8_t INT_V_DIRECT =   (INT_V_START+14);                /* direct start */
    inline constexpr uint8_t INT_V_RX =       (INT_V_DIRECT+0);                /* RX8E */
    inline constexpr uint8_t INT_V_RK =       (INT_V_DIRECT+1);                /* RK8E */
    inline constexpr uint8_t INT_V_RF =       (INT_V_DIRECT+2);                /* RF08 */
    inline constexpr uint8_t INT_V_DF =       (INT_V_DIRECT+3);                /* DF32 */
    inline constexpr uint8_t INT_V_MT =       (INT_V_DIRECT+4);                /* TM8E */
    inline constexpr uint8_t INT_V_DTA =      (INT_V_DIRECT+5);                /* TC08 */
    inline constexpr uint8_t INT_V_RL =       (INT_V_DIRECT+6);                /* RL8A */
    inline constexpr uint8_t INT_V_CT =       (INT_V_DIRECT+7);                /* TA8E int */
    inline constexpr uint8_t INT_V_PWR =      (INT_V_DIRECT+8);                /* power int */
    inline constexpr uint8_t INT_V_UF =       (INT_V_DIRECT+9);                /* user int */
    inline constexpr uint8_t INT_V_TSC =      (INT_V_DIRECT+10);               /* TSC8-75 int */
    inline constexpr uint8_t INT_V_FPP =      (INT_V_DIRECT+11);               /* FPP8 */
    inline constexpr uint8_t INT_V_OVHD =     (INT_V_DIRECT+12);               /* overhead start */
    inline constexpr uint8_t INT_V_NO_ION_PENDING = (INT_V_OVHD+0);            /* ion pending */
    inline constexpr uint8_t INT_V_NO_CIF_PENDING = (INT_V_OVHD+1);            /* cif pending */
    inline constexpr uint8_t INT_V_ION =      (INT_V_OVHD+2);                  /* interrupts on */

    inline constexpr int32_t INT_LPT =        (1 << INT_V_LPT);
    inline constexpr int32_t INT_PTP =        (1 << INT_V_PTP);
    inline constexpr int32_t INT_PTR =        (1 << INT_V_PTR);
    inline constexpr int32_t INT_TTO =        (1 << INT_V_TTO);
    inline constexpr int32_t INT_TTI =        (1 << INT_V_TTI);
    inline constexpr int32_t INT_CLK =        (1 << INT_V_CLK);
    inline constexpr int32_t INT_TTO1 =       (1 << INT_V_TTO1);
    inline constexpr int32_t INT_TTO2 =       (1 << INT_V_TTO2);
    inline constexpr int32_t INT_TTO3 =       (1 << INT_V_TTO3);
    inline constexpr int32_t INT_TTO4 =       (1 << INT_V_TTO4);
    inline constexpr int32_t INT_TTI1 =       (1 << INT_V_TTI1);
    inline constexpr int32_t INT_TTI2 =       (1 << INT_V_TTI2);
    inline constexpr int32_t INT_TTI3 =       (1 << INT_V_TTI3);
    inline constexpr int32_t INT_TTI4 =       (1 << INT_V_TTI4);

} // namespace pdp8

#endif //CPU_SIM_PDP8_H
