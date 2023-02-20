//
// Created by richard on 17/02/23.
//

/*
 * Memory.h Created by Richard Buckley (C) 17/02/23
 */

/**
 * @file Memory.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 17/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_MEMORY_H
#define PDP8_MEMORY_H

#include <fmt/format.h>
#include <HostInterface.h>
#include <Register.h>
#include <array>
#include <istream>

namespace pdp8 {

    /**
     * @brief The type of a core memory location.
     */
    template<std::size_t Width, std::size_t Offset, std::size_t SigBits = 16>
    using memory_t = registers::RegSlice<pdp8::small_register_t, Width, Offset, SigBits, false>;

    /**
     * @brief The type of memory address.
     */
    template<std::size_t Width, std::size_t Offset, std::size_t SigBits = 16>
    using address_t = registers::RegSlice<pdp8::fast_register_t, Width, Offset, SigBits, false>;

    /**
     * @brief A 12 bit address that uniquely locates a memory location in a field.
     */
    using page_word_address_t = address_t<12,0>;

    /**
     * @brief A 7 bit address that uniquely locates a memory location in a page.
     */
    using page_address_t = address_t<5,7>;

    /**
     * @brief A 5 bit address that uniquely locates a memory page in a field.
     */
    using word_address_t = address_t<7,0>;

    /**
     * @class ProgramCounter
     * @brief The ProgramCounter keeps track of the current location of execution in a field.
     */
    class ProgramCounter : public registers::Register<address_t<12,0>> {
    public:
        using program_counter_t = address_t<12,0>;

        void clear() {
            setProgramCounter(0);
        }

        void setProgramCounter(base_type pc) {
            set<program_counter_t>(pc);
        }

        [[nodiscard]] base_type getProgramCounter() const {
            return get<program_counter_t>();
        }

        ProgramCounter& operator++() {
            set<program_counter_t>(get<program_counter_t>() + 1);
            return *this;
        }
    };

    /**
     * @class FiledRegister
     * @brief The FieldRegister holds the current data and instruction field addresses.
     */
    class FieldRegister : public registers::Register<address_t<12,0>> {
    public:
        using data_field_t = address_t<3,3>;
        using inst_field_t = address_t<3,6>;
        using inst_buff_t = address_t<3,0>;

        void setDataField(base_type field) {
            set<data_field_t>(field);
        }

        void setInstField(base_type field) {
            set<inst_field_t>(field);
        }

        void setInstBuffer(base_type field) {
            set<inst_buff_t>(field);
        }

        [[maybe_unused]] [[nodiscard]] base_type getDataField() const {
            return get<data_field_t>();
        }

        [[nodiscard]] base_type getInstField() const {
            return get<inst_field_t>();
        }

        [[nodiscard]] base_type getInstBuffer() const {
            return get<inst_buff_t>();
        }
    };

    /**
     * @class MemoryAddress
     * @brief The MemoryAddress is used to compose the memory address for a fetch or deposit operation.
     */
    class MemoryAddress : public registers::Register<address_t<15,0>> {
    public:
        using field_address_t = address_t<3,12>;
        using page_t = memory_t<5,0>;
        using addr_t = memory_t<7,5>;

        void setWordAddress(base_type value) {
            set<word_address_t>(value);
        }

        void setPageAddress(base_type value) {
            set<page_address_t>(value);
        }

        void setPageWordAddress(base_type value) {
            set<page_word_address_t>(value);
        }

        void setFieldAddress(base_type value) {
            set<field_address_t>(value);
        }

        [[nodiscard]] base_type getFieldAddress() const {
            return get<field_address_t>();
        }

        [[nodiscard]] base_type getPageWordAddress() const {
            return get<page_word_address_t>();
        }
    };

    /**
     * @class MemoryBuffer
     * @brief The MemoryBuffer holds values fetched from memory and assembles values for depositing in memory.
     */
    class MemoryBuffer : public registers::Register<memory_t<15,0>> {
    public:
        using word_t = memory_t<12,0>;
        using init_t = memory_t<1,12>;
        using prog_t = memory_t<1,13>;

        [[nodiscard]] base_type getData() const {
            return get<word_t>();
        }

        [[maybe_unused]] bool getProgrammed() {
            return get<prog_t>();
        }

        bool getInitialized() {
            return get<init_t>();
        }

        void setData(base_type data) {
            set<word_t>(data);
        }

        void setInit(bool initialized) {
            set<init_t>(initialized ? 1u : 0u);
        }

        void setProgrammed(bool programmed) {
            set<prog_t>(programmed ? 1u : 0u);
        }
    };

    /**
     * @class Memory
     * @brief Contains the available core memory and provides access to it.
     */
    class Memory {
    protected:
        std::array<small_register_t, NumberOfFields * 4096> core{};

    public:

        using base_type = MemoryBuffer::base_type;
        MemoryBuffer memoryBuffer{};
        ProgramCounter programCounter{};
        FieldRegister fieldRegister{};
        MemoryAddress memoryAddress{};

        void write(base_type field, base_type address, base_type data, bool programmed = false) {
            if (field < NumberOfFields) {
                memoryAddress.setFieldAddress(field);
                memoryAddress.setPageWordAddress(address);
                memoryBuffer.setData(data);
                memoryBuffer.setInit(true);
                memoryBuffer.setProgrammed(programmed);
                core[memoryAddress.value] = memoryBuffer.value;
            }
        }

        void write() {
            memoryBuffer.setInit(true);
            core[memoryAddress.value] = memoryBuffer.value;
        }

        void deposit(base_type data) {
            write(fieldRegister.getInstField(), programCounter.getProgramCounter(), data, true);
            ++programCounter;
        }

        MemoryBuffer read() {
            memoryBuffer.value = core[memoryAddress.value];
            return memoryBuffer;
        }

        MemoryBuffer read(base_type field, base_type address) {
            memoryBuffer.value = 0u;
            if (field < NumberOfFields) {
                memoryAddress.setFieldAddress(field);
                memoryAddress.setPageWordAddress(address);
                read();
            }
            return memoryBuffer;
        }

        MemoryBuffer examine() {
            auto pc = read(fieldRegister.getInstField(), programCounter.getProgramCounter());
            ++programCounter;
            return pc;
        }

        void decodeAddress(const std::string_view& type) const {
            fmt::print("{} {:1o} {:04o} {:04o}\n", type, memoryAddress.getFieldAddress(),
                       memoryAddress.getPageWordAddress(), memoryBuffer.getData());
        }
    };

} // pdp8

#endif //PDP8_MEMORY_H
