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

        static constexpr std::array<std::pair<uint16_t,uint16_t>,18> RIM_LOADER = {{
                                                                                       {07756, 06014},
                                                                                       {07757, 06011},
                                                                                       {07760, 05357},
                                                                                       {07761, 06016},
                                                                                       {07762, 07106},
                                                                                       {07763, 07006},
                                                                                       {07764, 07510},
                                                                                       {07765, 05357},
                                                                                       {07766, 07006},
                                                                                       {07767, 06011},
                                                                                       {07770, 05367},
                                                                                       {07771, 06016},
                                                                                       {07772, 07420},
                                                                                       {07773, 03776},
                                                                                       {07774, 03376},
                                                                                       {07775, 05357},
                                                                                       {07776, 0},
                                                                                       {07777, 0}
                                                                               }};

        static constexpr std::array<small_register_t,3> WaitInstructions = {
                06031, // KSF
                06053, // CLSC
                06133, // CLSK
        };

        enum class CycleState {
            Interrupt, Fetch, Defer, Execute, Pause
        };

        CycleState cycle_state{CycleState::Interrupt};

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
        InstructionReg wait_instruction{0u};

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

        std::map<unsigned long, std::shared_ptr<IOTDevice>> iotDevices{};

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
