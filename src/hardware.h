//
// Created by richard on 02/08/18.
//

#pragma once


#include <cstddef>
#include <type_traits>
#include <limits>
#include <iostream>
#include <iomanip>

namespace sim {

    using register_type = uint_fast16_t;

    struct register_value;

    template<size_t width, size_t offset>
    struct slice_value {
        static_assert(width + offset <= std::numeric_limits<register_type>::digits, "Slice out of range.");
        static constexpr register_type base_mask = ((1u<<width) - 1u);
        static constexpr register_type mask = base_mask << offset;
        static constexpr register_type clear = ~mask;
        static constexpr size_t WIDTH = width;
        [[maybe_unused]] static constexpr size_t OFFSET = offset;

        register_value& registerRef;

        slice_value() = default;
        ~slice_value() = default;

        explicit slice_value(register_value& registerRef) : registerRef(registerRef) {}

        template<typename U>
        requires std::unsigned_integral<U>
        slice_value& operator=(U value) noexcept;

        register_type operator()() const noexcept;

        register_type operator++() noexcept;

        register_type operator--() noexcept;
    };

    template<size_t width, size_t offset>
    struct register_index {
        static_assert(width + offset <= std::numeric_limits<register_type>::digits, "Slice out of range.");
        static constexpr register_type mask = ((1u<<width) - 1u) << offset;
        static constexpr register_type clear = ~mask;
    };

    struct register_value {
        register_type value{};

        register_value() = default;
        ~register_value() = default;

        template<typename U>
        requires std::unsigned_integral<U>
        explicit register_value(U init) : value(init) {}

        template<size_t width, size_t offset>
        slice_value<width,offset> operator[](register_index<width,offset> index) {
            return slice_value<width,offset>(*this);
        }
    };

    template<size_t width, size_t offset>
    template<typename U>
    requires std::unsigned_integral<U>
    slice_value<width,offset> &slice_value<width, offset>::operator=(U value) noexcept {
        registerRef.value = (registerRef.value & clear) | ((value << offset) & mask);
        return *this;
    }

    template<size_t width, size_t offset>
    register_type slice_value<width, offset>::operator()() const noexcept {
        return (registerRef.value & mask) >> offset;
    }

    template<size_t width, size_t offset>
    register_type slice_value<width, offset>::operator++() noexcept {
        auto r = (operator()() + 1) & base_mask;
        operator=(r);
        return r;
    }

    template<size_t width, size_t offset>
    register_type slice_value<width, offset>::operator--() noexcept {
        auto r = (operator()() - 1) & base_mask;
        operator=(r);
        return r;
    }

} // namespace sim

