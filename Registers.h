//
// Created by richard on 26/01/18.
//

#ifndef PIDP_8_SIM_REGISTERS_H
#define PIDP_8_SIM_REGISTERS_H

#include <iostream>
#include <array>
#include <iomanip>
#include <cmath>

namespace hw_sim
{
    /**
     * @brief A bit mask of a specified width.
     * @tparam W the number of bist in the mask
     */
    template <size_t W>
    struct RegisterMask
    {
        constexpr static size_t value = (RegisterMask<W-1>::value << 1) | 1;
    };

    template <>
    struct RegisterMask<1>
    {
        constexpr static size_t value = 1;
    };


    /**
     * @brief Type encoding for register specifications
     * @tparam Radix the display and entry radix of the register
     * @tparam Width the number of bits in the register
     * @tparam Offset the offset (leftshift) of the LSB of the register in the base type
     */
    template <size_t Radix, size_t Width, size_t Offset>
    struct RegisterType
    {
        constexpr static size_t radix = Radix;
        constexpr static size_t width = Width;
        constexpr static size_t offset = Offset;
    };


    /**
     * @brief An accessor object to get and set register values
     * @tparam Array The type of the register array
     * @tparam Id The index of the register storage in the array
     * @tparam RegType The type of the register
     */
    template <class Array, size_t Id, class RegType>
    struct RegisterAccessor
    {
        constexpr static size_t count = Array::size();
        constexpr static size_t id = Id;

        Array &arraySet;

        explicit RegisterAccessor(Array &as) : arraySet(as) {}

        RegisterAccessor&operator = (const typename Array::value_type&& t)
        {
            static_assert(std::tuple_size<Array>::value > id, "Register ID out of bounds.");

            arraySet[id] = (arraySet[id] & ~(RegisterMask<RegType::width>::value << RegType::offset)) |
                    (t & RegisterMask<RegType::width>::value) << RegType::offset;

            return *this;
        }

        template <size_t oId, class oRegT>
        RegisterAccessor &operator = (const RegisterAccessor<Array, oId, oRegT> & other) noexcept
        {
            operator=(other());
            return *this;
        };

        template <size_t oId, class oRegT>
        typename Array::value_type operator + (const RegisterAccessor<Array, oId, oRegT> &other) noexcept
        {
            return operator()() + other();
        };

        template <size_t oId, class oRegT>
        typename Array::value_type operator - (const RegisterAccessor<Array, oId, oRegT> &other) noexcept
        {
            return operator()() - other();
        };

        explicit operator typename Array::value_type()
        {
            return operator()();
        }

        typename Array::value_type operator() () const
        {
            static_assert(std::tuple_size<Array>::value > id, "Register ID out of bounds.");

            return (arraySet[id] >> RegType::offset) & RegisterMask<RegType::width>::value;
        }
    };

    /**
     * An stream inserter for a register
     * @tparam Ct The character type of the stream
     * @tparam Tt The character traits type of the stream
     * @tparam Array The type of the array
     * @tparam Id The index of the register storage in the array
     * @tparam RegType The register type
     * @param os The stream object
     * @param r The register object
     * @return The stream object
     */
    template <class Ct, class Tt, class Array, size_t Id, class RegType>
    std::basic_ostream<Ct,Tt> &operator << (std::basic_ostream<Ct,Tt> & os, RegisterAccessor<Array, Id, RegType> & r)
    {
        constexpr auto w = static_cast<int>(RegType::width / (int) log2(RegType::radix)) +
                           (static_cast<int>(RegType::width % (int) log2(RegType::radix)) ? 1 : 0);
        return os << std::setbase(static_cast<int>(RegType::radix))
                  << std::setw(w)
                  << std::setfill('0')
                  << r();
    };
}

#endif //PIDP_8_SIM_REGISTERS_H
