//
// Created by richard on 17/02/23.
//

/*
 * Register.h Created by Richard Buckley (C) 17/02/23
 */

/**
 * @file Register.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 17/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_REGISTER_H
#define PDP8_REGISTER_H

#include <limits>
#include <HostInterface.h>

namespace registers {
    template<class RegBase, std::size_t Width, std::size_t Offset, std::size_t SigBits = 12, bool DecBitOrder = true>
    requires ((Width+Offset) < std::numeric_limits<RegBase>::digits)
    struct RegSlice {
        using base_type = RegBase;
        static constexpr uint16_t width = Width;
        static constexpr uint16_t offset = (DecBitOrder ? SigBits - width - Offset : Offset);
        static constexpr base_type base_mask = ((1u<<Width) - 1u);
        static constexpr base_type mask = base_mask << offset;
        static constexpr base_type clear = static_cast<const unsigned short>(~mask);
        [[maybe_unused]] static constexpr std::size_t max = std::numeric_limits<RegBase>::max();
        [[maybe_unused]] static constexpr std::size_t digits = std::numeric_limits<RegBase>::digits;
    };

    template<std::size_t Width, std::size_t Offset, std::size_t SigBits = 12>
    using register_t = registers::RegSlice<pdp8::fast_register_t, Width, Offset, SigBits>;

    template<class Slice>
    struct Register {
        using base_type = Slice::base_type;
        base_type value{};

        static constexpr uint16_t width = Slice::width;
        static constexpr uint16_t offset = Slice::offset;
        static constexpr Slice::base_type base_mask = Slice::base_mask;
        static constexpr Slice::base_type mask = Slice::mask;
        static constexpr Slice::base_type clear = Slice::clear;

        Register() = default;
        Register(const Register&) = default;
        Register(Register&&)  noexcept = default;
        Register& operator=(const Register&) = default;
        Register& operator=(Register&&) noexcept = default;

        explicit Register(const Slice::base_type v) : value((v & Slice::mask) << Slice::offset) {}

        explicit Register(const unsigned int v) : value((v & Slice::mask) << Slice::offset) {}

        template<class Access = Slice>
        Slice::base_type get() const {
            return (value & Access::mask) >> Access::offset;
        }

        template<class Access = Slice>
        Register<Slice> set(Slice::base_type s) {
            value = static_cast<unsigned short>((value & Access::clear) | ((s & Access::base_mask) << Access::offset));
            return *this;
        }
    };
}


#endif //PDP8_REGISTER_H
