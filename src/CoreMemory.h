/**
 * @file Memory.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-10
 */

#pragma once

#include <vector>
#include "hardware.h"

namespace sim {

    /**
     * @class core_memory_field
     */
    class core_memory_field : public std::array<uint_least16_t,4096> {

    };

    /**
     * @clase core_memory
     */
    class CoreMemory : public std::vector<core_memory_field> {
    protected:
        size_t fields;

    public:
        CoreMemory() = delete;
        ~CoreMemory() = default;

        explicit CoreMemory(size_t field_count) : std::vector<core_memory_field>() {
            std::size_t lo = 1, hi = 8;
            fields = std::clamp(field_count, lo, hi);
            reserve(fields);
            for (size_t i = 0; i < fields; ++i) {
                emplace_back();
            }
        }

        void writeCore(size_t field, size_t address, size_t data) {
            if (field < fields) {
                size_t writeAddress = address & 07777u;
                uint_least16_t writeData = (data & 07777u) | 0x8000u;
                at(field)[writeAddress] = writeData;
            }
        }

        uint_least16_t readCore(size_t field, size_t address) {
            if (field < fields) {
                size_t readAddress = address & 07777u;
                return at(field)[readAddress] & 07777u;
            }
            return 0u;
        }
    };

}

