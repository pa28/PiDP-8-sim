//
// Created by richard on 18/02/23.
//

/*
 * Accumulator.h Created by Richard Buckley (C) 18/02/23
 */

/**
 * @file Accumulator.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 18/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_ACCUMULATOR_H
#define PDP8_ACCUMULATOR_H

#include <Register.h>

namespace pdp8 {

/**
 * @class Accumulator
 */
    class Accumulator : public registers::Register<registers::register_t<13,0,13>> {
    public:
        using accumulator_t = registers::register_t<12,1,13>;
        using link_t = registers::register_t<1,0,13>;
        using arithmetic_t = registers::register_t<13,0,13>;
        using lsb_t = registers::register_t<1,12,13>;
        using msb_t = registers::register_t<1,1,13>;
        using lowerNibble_t = registers::register_t<6,7,13>;
        using upperNibble_t = registers::register_t<6,1,13>;
        using asciiChar_t = registers::register_t<8,4,12>;

        [[nodiscard]] base_type getAscii() const {
            return get<asciiChar_t>();
        }

        void setAscii(base_type value) {
            set<asciiChar_t>(value);
        }

        [[nodiscard]] base_type getMostSig() const {
            return get<msb_t>();
        }

        [[nodiscard]] base_type getUpperNibble() const {
            return get<upperNibble_t>();
        }

        [[nodiscard]] base_type getLowerNibble() const {
            return get<lowerNibble_t>();
        }

        [[nodiscard]] base_type getArithmetic() const {
            return get<arithmetic_t>();
        }

        void setArithmetic(base_type value) {
            set<arithmetic_t>(value);
        }

        [[nodiscard]] base_type getLeastSig() const {
            return get<lsb_t>();
        }

        void setLeastSig(base_type value) {
            set<lsb_t>(value);
        }

        [[nodiscard]] base_type getAcc() const {
            return get<accumulator_t>();
        }

        void setAcc(base_type value) {
            set<accumulator_t>(value);
        }

        [[nodiscard]] base_type getLink() const {
            return get<link_t>();
        }

        void setLink(base_type value) {
            set<link_t>(value);
        }

        void andOp(base_type arg) {
            setAcc(getAcc() & arg);
        }

        void addOp(base_type arg) {
            value = value + arg;
        }
    };

    class MulQuotient : public registers::Register<registers::register_t<12,0,12>> {
    public:
        using word_t = registers::register_t<12,0>;

        void setWord(base_type value) {
            set<word_t>(value);
        }

        [[nodiscard]] base_type getWord() const {
            return get<word_t>();
        }
    };

    class OperatorSwitchRegister : public registers::Register<registers::register_t<12,0,12>> {

    };

    class StepCounter : public registers::Register<registers::register_t<5,0,5>> {

    };
} // pdp8

#endif //PDP8_ACCUMULATOR_H
