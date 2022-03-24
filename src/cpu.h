/**
 * @file cpu.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-11
 */

#pragma once

#include <array>
#include <vector>
#include <atomic>
#include <iostream>
#include <fmt/format.h>
#include <variant>
#include "hardware.h"
#include "CoreMemory.h"
#include "Terminal.h"

namespace sim {

    [[maybe_unused]] static constexpr uint16_t HELP_LOADER_START = 0027;
    [[maybe_unused]] static constexpr std::array<uint16_t, 10> HELP_LOADER =
            {
                    06031, 05027, 06036, 07450, 05027, 07012, 07010, 03007, 02036, 05027
            };

    [[maybe_unused]] static constexpr uint16_t RIM_LOADER_START = 07756;
    [[maybe_unused]] static constexpr std::array<uint16_t, 18> RIM_LOADER =
            {
                    06014, 06011, 05357, 06016, 07106, 07006,
                    07510, 05357, 07006, 06011, 05367, 06016,
                    07420, 03776, 03376, 05357, 0, 0
            };

    [[maybe_unused]] static constexpr register_type OP_KSF = 06031;       /* for idle */
    [[maybe_unused]] static constexpr register_type OP_CLSC = 06053;      /* for idle */

    /**
     * @class cpu
     * @brief
     */
    class PDP8I {
    public:

        enum class Instruction : unsigned int {
            AND = 0, TAD, ISZ, DCA, JMS, JMP, IOT, OPR
        };

        enum class CycleState {
            Interrupt, Fetch, Defer, Execute, Pause
        };

        register_index<13, 0> arithmetic{};         ///< Access for arithmetic
        register_index<1, 12> linkIndex{};          ///< Access the link value
        register_index<1, 0> leastSignificant{};    ///< access the least significant bit
        register_index<1, 11> mostSignificant{};    ///< access the most significant bit
        register_index<12, 0> wordIndex{};          ///< Access a 12 bit word.
        register_index<6, 6> upperNibble{};
        register_index<6, 0> lowerNibble{};
        register_index<1, 15> memorySet{};
        register_index<1, 8> indirectIndex{};       ///< Access the indirect flag.
        register_index<1, 7> memPageIndex{};        ///< Access the memory page flag.
        register_index<6, 3> deviceSelect{};        ///< IOT device selection bits.
        register_index<3, 0> deviceOpr{};           ///< IOT device operation.

        register_index<5, 7> page_index{};           ///< Page address access
        register_index<7, 0> addr_index{};           ///< Address in page access
        register_index<9, 0> opr_bits{};             ///< The argument flags to OPR instruction

        register_index<3, 0> data_field{};           ///< Access the memory field from the field register
        register_index<3, 3> instruction_field{};    ///< Access the instruction field from the field register
        register_index<3, 9> instruction_index{};    ///< Access the memory instruction address.
        register_index<5, 0> step_counter_index{};   ///< Access the value of the step counter.

    protected:

        CycleState cycle_state{CycleState::Fetch};

        register_value link_accumulator{};          ///< Store the link and accumulator values
        register_value program_counter{};           ///< Store the program counter
        register_value memory_buffer{};             ///< Memory buffer register
        register_value cpma{};                      ///< Central Processor Memory Address
        register_value field_register{};            ///< Storage for instruction and data fields
        register_value instruction_buffer{};        ///< Holds a new instruction field until next jump.
        register_value switch_register{};           ///< Holds the value read from the switch register
        register_value interrupt_buffer{};
        register_value field_buffer{};
        register_value multiplier_quotient{};       ///< The multiplier quotient in the EAE
        register_value step_counter{};              ///< The step counter in the EAE

        Instruction instruction_register{};         ///< The instruction register

        bool interrupt_enable{false};
        bool interrupt_deferred{false};             // deferred until a JMP or JSR
        int interrupt_delayed{0u};                  // delayed until after next instruction
        bool greater_than_flag{false};
        bool short_jmp_flag{false};                 // set to true if idling on a short jump

        std::atomic_bool interrupt_request;         // An interrupt has been raised.
        std::atomic_bool run_flag;                  // CPU thread function runs while this is true
        std::atomic_bool halt_flag;                 // CPU halted by HLT or other difficulty
        std::atomic_bool idle_flag;                 // CPU has detected an idle condition
        std::atomic_bool error_flag;                // CPU has thrown an exception
        std::atomic_bool step_flag;                 // Allow the CPU to run one step if the halt_flag is set
        std::atomic_bool single_step_flag;          // The CPU will progress through a single time slice

        register_index<1, 11> bit0;
        register_index<1, 10> bit1;
        register_index<1, 9> bit2;
        register_index<1, 8> bit3;
        register_index<1, 7> bit4;
        register_index<1, 6> bit5;
        register_index<1, 5> bit6;
        register_index<1, 4> bit7;
        register_index<1, 3> bit8;
        register_index<1, 2> bit9;
        register_index<1, 1> bit10;
        register_index<1, 0> bit11;

        register_index<3, 15> tri0;
        register_index<3, 12> tri1;
        register_index<3, 9> tri2;
        register_index<3, 6> tri3;
        register_index<3, 3> tri4;
        register_index<3, 0> tri5;

        CoreMemory coreMemory;

    public:
        ~PDP8I() = default;

        PDP8I() : PDP8I(1u) {}

        template<typename U>
        requires std::unsigned_integral<U>
        explicit PDP8I(U memoryFields) : coreMemory(memoryFields) {

        }

        /**
         * @brief Perform a fetch cycle
         */
        void fetch();

        /**
         * @briefPerform defer cycle
         */
        void defer();

        /**
         * @brief Execute the instruction
         */
        void execute();

        /**
         * @brief Step CPU through one instruction cycle.
         */
        void instruction_cycle(bool skipInterrupt = true);

        /**
         * @brief Step CPU through one instruction.
         * @details If Cycle State is not Interrupt, cycles to Interrupt, if Cycle State is Interrupt, cycles to next
         * Interrupt state.
         */
        void instruction_step();

        void reset();

        [[nodiscard]] auto fieldRegister() const { return field_register; }

        [[nodiscard]] auto PC() const { return program_counter; }

        [[nodiscard]] auto MA() const { return cpma; }

        [[nodiscard]] auto MB() const { return memory_buffer; }

        [[nodiscard]] auto LAC() const { return link_accumulator; }

        [[nodiscard]] auto MQ() const { return multiplier_quotient; }

        [[nodiscard]] auto SC() const { return step_counter; }

        [[nodiscard]] auto InstReg() const { return instruction_register; }

        [[nodiscard]] bool runFlag() const { return run_flag; }

        [[nodiscard]] auto cycleState() const { return cycle_state; }

        [[nodiscard]] auto interruptEnable() const { return interrupt_enable; }

        void run() { run_flag = true; }

        void stop() { run_flag = false; }

        /**
         * @brief Load the address register
         */
        void loadAddress(register_type address) {
            register_value input(address);
            program_counter[wordIndex] = input[wordIndex]();
            field_register[data_field] = input[tri0]();
            field_register[instruction_field] = input[tri1]();
        }

        /**
         * @brief Deposit value in memory
         */
        void deposit(register_type value);

        /**
         * @brief Examine value in memory
         */
        register_type examine();

        register_type examineAt(register_type address);

        /**
         * @brief Read from core memory
         */
        register_value readCore(register_type field, register_type address);

        /**
         * @brief Read a word from the core memory field specified as the current instruction field.
         * @return
         */
        register_value readInstructionCore() {
            return readCore(field_register[instruction_field](), cpma[wordIndex]());
        }

        /**
         * @brief Read a word from the core memory field specified as the current data field.
         * @return
         */
        register_value readDataCore() {
            return readCore(field_register[data_field](), cpma[wordIndex]());
        }

        /**
         * @brief Write to core memory
         */
        void writeCore(register_type field, register_type address, register_type data);

        /**
         * @brief Write a word to the core memory field specified as the current instruction field.
         * @return
         */
        void writeInstructionCore(register_type data) {
            writeCore(field_register[instruction_field](), cpma[wordIndex](), data);
        }

        /**
         * @brief Write a word to the core memory field specified as the current data field.
         * @return
         */
        void writeDataCore(register_type data) {
            writeCore(field_register[data_field](), cpma[wordIndex](), data);
        }

        /**
         * @brief Read Binary Format (BIN) file into memory field.
         * @details The memory field is set in the field_register[instruction_field]
         * @param istrm The input stream with BIN (Binary Format) data.
         * @return true if the file had at least one address and one word in it.
         */
        [[maybe_unused]] bool readBinaryFormat(std::istream &istrm);

        /**
         * @brief Write Binary Format (BIN) file from memory field.
         * @param ostrm The stream to write the BIN file on.
         * @param first
         * @param last
         * @return
         */
        [[maybe_unused]] bool writeBinaryFormat(std::ostream &ostrm, register_type first, register_type last);

        /**
         * @brief Push the RIM loader into high memory in the current instruction field.
         */
        [[maybe_unused]] void rimLoader();

        /**
         * @brief Execute OPR instructions.
         */
        void execute_opr();

        /**
         * @brief Execute IOT instructions.
         */
        void execute_iot();
    };

    class TestCPU;

    using test_result = std::variant<bool, uint16_t>;

    enum CompareCriteria {
        Less, LessEq, Equal, GreaterEq, Greater, NotEqual
    };

    static constexpr std::array<std::string_view, 6> CompareCriteriaText =
            {{
                     "<", "<=", "==", ">=", ">", "!="
             }};

    struct SingleInstructionTest {
        uint16_t instruction;
        std::string_view name;

        test_result (*test_function)(TestCPU &);

        test_result expected;
        CompareCriteria criteria;
        bool resetCPU;
    };

    class TestCPU : public PDP8I {
    public:
        /**
         * @brief Load a test program (in binary form) into core memory
         * @tparam length The length of the program.
         * @param start The starting address of the program.
         * @param code The code.
         */
        template<size_t length>
        [[maybe_unused]] void loadTestProgram(uint16_t start, std::array<uint16_t, length> code);

        template<typename Title, size_t length>
        requires std::is_convertible_v<Title, std::string_view>
        std::pair<int, int> singleInstructionTests(Title title, std::array<SingleInstructionTest, length> tests);

        std::pair<int, int> testTadInstructions_1();

        std::pair<int, int> testAndInstructions_1();

        std::pair<int, int> testOprInstructions_1();

        std::pair<int, int> testAutoIndexMemory();

        std::pair<int, int> testIszInstructions();

        [[maybe_unused]] void setAccumulator(uint16_t value) {
            link_accumulator[wordIndex] = value;
        }

        [[maybe_unused]] [[nodiscard]] bool getHaltFlag() const {
            return halt_flag;
        }
    };
}

