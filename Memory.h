/*
 * Memory.h
 *
 *  Created on: Jul 28, 2015
 *      Author: H Richard Buckley
 */

#ifndef PIDP_MEMORY_H
#define PIDP_MEMORY_H


#include <cstdio>
#include <cstdint>
#include <stack>
#include <utility>
#include <exception>
#include <array>
#include <memory>
#include <cmath>
#include "Registers.h"
#include "Device.h"

namespace hw_sim {

    constexpr size_t MAXMEMSIZE = 8 * 4096;


    class MemoryOutOfRange : public std::out_of_range {
    public:
        MemoryOutOfRange() : std::out_of_range("Memory access out of range") {}
    };

    enum MemoryFlag {
        MemFlagClear = 0,
        MemFlagInitialized = 1,
        MemFlagBreakExecute = 2,
        MemFlagBreakRead = 4,
        MemFlagBreakWrite = 8,
        MemFlagBreak = MemFlagBreakExecute | MemFlagBreakRead | MemFlagBreakWrite,
    };

    /**
     * An stream inserter for a MemoryFlag
     * @tparam Ct The character type of the stream
     * @tparam Tt The character traits type of the stream
     */
    template<class Ct, class Tt>
    std::basic_ostream<Ct, Tt> &operator<<(std::basic_ostream<Ct, Tt> &os, MemoryFlag f) {
        return os << setw(1)
                  << setfill(' ')
                  << (f & MemFlagInitialized ? 'I' : '-')
                  << (f & MemFlagBreakExecute ? 'X' : '-')
                  << (f & MemFlagBreakRead ? 'R' : '-')
                  << (f & MemFlagBreakWrite ? 'W' : '-');
    };

    constexpr size_t MemoryFlagWidth = 4;

    template<typename Base, size_t Width>
    class MemoryCell {
    public:
        MemoryCell() {
            clear();
        }

        constexpr static size_t width() { return Width; }

        using base_type = Base;

        MemoryCell &operator=(Base v) {
            static_assert(sizeof(Base) * 8 >= Width + MemoryFlagWidth);
            m = (m & ~hw_sim::RegisterMask<Width>::value)
                | (v & hw_sim::RegisterMask<Width>::value);
            return *this;
        }

        template <typename B2, class RT2>
        MemoryCell<Base,Width> & operator = (ScalarRegister<B2, RT2> &r) {
            return operator=(r());
        };

        MemoryCell<Base,Width> & operator ++ () {
            return operator=(operator()()+1);
        };

        operator Base() const {
            static_assert(sizeof(Base) * 8 >= Width + MemoryFlagWidth);
            return static_cast<Base>(m & hw_sim::RegisterMask<Width>::value);
        }

        Base operator()() const {
            static_assert(sizeof(Base) * 8 >= Width + MemoryFlagWidth);
            return static_cast<Base>(*this);
        }

        bool operator==(MemoryCell &other) {
            static_assert(sizeof(Base) * 8 >= Width + MemoryFlagWidth);
            return static_cast<Base>(*this) == static_cast<Base>(other);
        }

        MemoryFlag flags() const {
            return static_cast<MemoryFlag>((m >> Width) & hw_sim::RegisterMask<MemoryFlagWidth>::value);
        }

        void setFlag(MemoryFlag f) {
            m |= f << Width;
        }

        void clear() {
            static_assert(sizeof(Base) * 8 >= Width + MemoryFlagWidth);
            m = MemFlagClear << Width;
        }

    protected:
        Base m;
    };



    /**
     * An stream inserter for a memory cell
     * @tparam Ct The character type of the stream
     * @tparam Tt The character traits type of the stream
     * @tparam Base The cell base type
     * @param Width The cell width
     * @return The stream object
     */
    template<class Ct, class Tt, typename Base, size_t Width>
    std::basic_ostream<Ct, Tt> &operator<<(std::basic_ostream<Ct, Tt> &os, const MemoryCell<Base, Width> &m) {
        double bits = 0;
        int radix = 0;
        auto basefield = os.flags() & std::ios_base::basefield;
        switch (basefield) {
            case std::_S_oct:
                bits = 3;
                radix = 8;
                break;
            case std::_S_hex:
                bits = 4;
                radix = 16;
                break;
            default:
                bits = 3.322;
                radix = 10;
        }

        double intPart;
        double fracPart = modf(Width / bits, &intPart);

        auto w = static_cast<int>(intPart) + (fracPart != 0 ? 1 : 0);

        return os << std::setbase(radix)
                  << std::setw(w)
                  << std::setfill('0')
                  << m();
    };

    template<size_t Size, typename Base, size_t Width>
    class Memory : public Device
    {
    public:
        Memory() : Device("MEM", "Core Memory"), ma(), mb(), m() {}

        virtual ~Memory() = default;

        virtual void initialize() {}
        virtual void reset() {}
        virtual void stop() {}
        virtual void tick(int ticksPerSecond) {}

        constexpr static size_t size() { return Size; }

        constexpr static size_t width() { return Width; }

        using base_type = Base;

        MemoryCell<Base, Width> &operator[](size_t memoryAddress) {
            return at(memoryAddress);
        }

        MemoryCell<Base, Width> &at(size_t memoryAddress) {
            if (memoryAddress < size()) {
                ma = begin() + memoryAddress;
                mb = *ma;
                return *ma;
            }
            throw MemoryOutOfRange{};
        }

        template <typename B2, class RT2>
        MemoryCell<Base, Width> &operator[](ScalarRegister<B2,RT2> &ma) {
            return at(ma());
        };

        template <typename B2, class RT2>
        MemoryCell<Base, Width> &at(ScalarRegister<B2,RT2> &ma) {
            return at(ma());
        }

        MemoryCell<Base, Width> &MB() { return mb; }

        auto MA() { return ma; }

        auto begin() { return m.begin(); }

        auto end() { return m.end(); }

        auto cbegin() const { return m.cbegin(); }

        auto cend() const { return m.cend(); }

        auto rbegin() { return m.rbegin(); }

        auto rend() { return m.rend(); }

        auto crbegin() const { return m.crbegin(); }

        auto crend() const { return m.crend(); }

        void clear() {
            for (auto i = begin(); i != end(); ++i)
                i->clear();
        }

        static std::shared_ptr<Memory<Size,Base,Width>> getMemory(int32_t dev) {
            std::shared_ptr<Memory<Size,Base,Width>> memory;
            auto devItr = Chassis::instance()->find(dev);
            if (devItr != Chassis::instance()->end()) {
                memory = std::dynamic_pointer_cast<Memory<Size,Base,Width>>(devItr->second);
            }

            return memory;
        }

    protected:
        typename std::array<MemoryCell<Base, Width>, Size>::iterator ma;
        MemoryCell<Base, Width> mb;

        std::array<MemoryCell<Base, Width>, Size> m;
    };

    /**
     * @brief Some sytactic surgar to make the use of a shared pointer to memory easier
     * @tparam Size The number of memory cells
     * @tparam Base The base type of memory
     * @tparam Width The number of bits in a memory cell
     */
    template <size_t Size, typename Base, size_t Width>
    struct MemoryHelper
    {
        explicit MemoryHelper() : memory() {}

        void set(std::shared_ptr<Memory<Size,Base,Width>> & m) {
            memory = m;
        };

        MemoryCell<Base, Width> &operator [] (size_t ma) {
            return memory->at(ma);
        }

        template <typename B2, class RT2>
        MemoryCell<Base, Width> &operator [] (ScalarRegister<B2,RT2> & ma) {
            return memory->at(ma());
        }

        std::shared_ptr<Memory<Size,Base,Width>> memory;
    };


} /* namespace pdp8 */


#endif //PIDP_MEMORY_H
