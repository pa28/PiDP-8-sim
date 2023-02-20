/*
 * Instruction.h Created by Richard Buckley (C) 17/02/23
 */

/**
 * @file Instruction.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 17/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_INSTRUCTION_H
#define PDP8_INSTRUCTION_H

#include <string_view>
#include <array>
#include <HostInterface.h>
#include <Register.h>

namespace pdp8 {

    enum class OpCode : unsigned int {
        AND = 0, TAD = 1, ISZ = 2, DCA = 3, JMS = 4, JMP = 5, IOT = 6, OPR = 7
    };

    static constexpr std::array<std::string_view, 8> OpCodeStr{
            "AND", "TAD", "ISZ", "DCA", "JMS", "JMP", "IOT", "OPR"
    };

    class InstructionReg : public registers::Register<registers::register_t<12,0>> {
    protected:
        using op_code_t = registers::register_t<3, 0>;
        using indirect_t = registers::register_t<1, 3>;
        using zero_page_t = registers::register_t<1, 4>;
        using address_t = registers::register_t<7, 5>;
        using word_t = registers::register_t<12,0>;
        using opr_bits_t = registers::register_t<9,3>;
        using device_sel_t = registers::register_t<6,3>;
        using device_opr_t = registers::register_t<3,0>;
        using field_reg_t = registers::register_t<3,3>;

    public:
        InstructionReg() = default;

        explicit InstructionReg(unsigned int value) : registers::Register<registers::register_t<12,0>>(value) {}

        [[nodiscard]] base_type getOpCode() const { return get<op_code_t>(); }

        [[nodiscard]] bool getIndirect() const { return getOpCode() < 6 && get<indirect_t>() != 0; }

        [[nodiscard]] bool getZeroPage() const { return get<zero_page_t>() == 0; }

        [[nodiscard]] base_type getAddress() const { return get<address_t>(); }

        [[nodiscard]] const std::string_view& getOpCodeStr() const {
            auto opCode = getOpCode();
            return OpCodeStr[opCode];
        }

        [[nodiscard]] base_type getWord() const { return get<word_t>(); }

        [[nodiscard]] base_type getOprBits() const { return get<opr_bits_t>(); }

        [[nodiscard]] base_type getDeviceSel() const { return get<device_sel_t>(); }

        [[nodiscard]] base_type getDeviceOpr() const { return get<device_opr_t>(); }

        [[nodiscard]] base_type getFieldReg() const { return get<field_reg_t>(); }

        [[nodiscard]] bool isMemoryInstruction() const {
            switch (static_cast<OpCode>(getOpCode())) {
                case OpCode::IOT:
                case OpCode::OPR:
                    return false;
                default:
                    return true;
            }
        }

        [[nodiscard]] bool isIndirectInstruction() const {
            return isMemoryInstruction() && getIndirect() != 0;
        }
    };

} // pdp8

#endif //PDP8_INSTRUCTION_H
