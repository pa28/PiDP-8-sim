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

#include <memory>
#include <HostInterface.h>
#include <Memory.h>
#include <Instruction.h>
#include <Accumulator.h>
#include <atomic>
#include <IOTDevice.h>
#include <Terminal.h>
#include <map>

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

        bool idle_flag{false};
        bool interrupt_enable{false};
        bool interrupt_request{false};
        bool error_flag{false};
        bool interrupt_deferred{false};
        int interrupt_delayed{0};
//        bool short_jmp_flag{false};
        bool halt_flag{false};
        bool run_flag{false};
        bool greater_than_flag{false};

        small_register_t switch_register{0};

        PDP8() {
            memory.programCounter.clear();
            memory.programCounter.setProgramCounter(0200);
            memory.fieldRegister.setDataField(0);
            memory.fieldRegister.setInstField(0);
            memory.fieldRegister.setInstBuffer(0);
        }

        PDP8(const PDP8&) = delete;
        PDP8(PDP8&&) = default;
        PDP8& operator = (const PDP8&) = delete;
        PDP8& operator = (PDP8&&) = default;

        Memory memory{};
        InstructionReg instructionReg{};
        Accumulator accumulator{};
        MulQuotient mulQuotient{};
        OperatorSwitchRegister opSxReg{};
        StepCounter stepCounter{};
        TerminalManager terminalManager{};

        std::map<unsigned int, std::shared_ptr<IOTDevice>> iotDevices{};

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
