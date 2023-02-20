//
// Created by richard on 18/02/23.
//

/*
 * HostInterface.h Created by Richard Buckley (C) 18/02/23
 */

/**
 * @file HostInterface.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 18/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_HOSTINTERFACE_H
#define PDP8_HOSTINTERFACE_H

#include <cstdint>

namespace pdp8 {
    using fast_register_t = uint_fast16_t;
    using small_register_t = uint_least16_t;

    static constexpr std::size_t NumberOfFields = 8;


}

#endif //PDP8_HOSTINTERFACE_H
