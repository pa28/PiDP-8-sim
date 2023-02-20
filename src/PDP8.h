//
// Created by richard on 17/02/23.
//

/*
 * PDP8.h Created by Richard Buckley (C) 17/02/23
 */

/**
 * @file PDP8.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 17/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_PDP8_H
#define PDP8_PDP8_H

#include <vector>
#include <HostInterface.h>
#include <Memory.h>
#include <Instruction.h>
#include <Accumulator.h>
#include <atomic>
#include <IOTDevice.h>

namespace pdp8 {

    /**
     * @class PDP8
     */
    class PDP8 {
    public:

        [[maybe_unused]] static constexpr uint16_t RIM_LOADER_START = 07756;
        [[maybe_unused]] static constexpr std::array<uint16_t, 18> RIM_LOADER =
                {
                        06014, 06011, 05357, 06016, 07106, 07006,
                        07510, 05357, 07006, 06011, 05367, 06016,
                        07420, 03776, 03376, 05357, 0, 0
                };

        static constexpr std::array<small_register_t,2> WaitInstructions = {
                06031, // KSF
                06053, // CLSC
        };

        enum class CycleState {
            Interrupt, Fetch, Defer, Execute, Pause
        };

        CycleState cycle_state{CycleState::Fetch};

        std::atomic_bool idle_flag{false};
        std::atomic_bool interrupt_enable{false};
        std::atomic_bool interrupt_request{false};
        std::atomic_bool error_flag{false};
        std::atomic_bool interrupt_deferred{false};
        std::atomic_int interrupt_delayed{0};
        std::atomic_bool short_jmp_flag{false};
        std::atomic_bool halt_flag{false};
        std::atomic_bool run_flag{false};
        std::atomic_bool greater_than_flag{false};

        small_register_t switch_register{0};

        PDP8() {
            memory.programCounter.clear();
            memory.programCounter.setProgramCounter(0200);
            memory.fieldRegister.setDataField(0);
            memory.fieldRegister.setInstField(0);
            memory.fieldRegister.setInstBuffer(0);
        }

        Memory memory{};
        InstructionReg instructionReg{};
        Accumulator accumulator{};
        MulQuotient mulQuotient{};
        OperatorSwitchRegister opSxReg{};
        StepCounter stepCounter{};

        std::vector<IOTDevice> iotDevices{};

        void instructionCycle();

        void instructionStep();

        void decodeInstruction() const;

        /**
         * @brief Fetch the next instruction to execute.
         */
        bool fetch();

        /**
         * @brief Defer execution to perform indirect addressing.
         */
        void defer();

        /**
         * @brief Execute the instruction.
         */
        void execute();

        void execute_iot();

        void execute_opr();

        bool readBinaryFormat(std::istream& iStream);

        void rimLoader();

        [[maybe_unused]] void reset();
    };

} // pdp8

#endif //PDP8_PDP8_H
