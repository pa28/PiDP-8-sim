/**
 * @file Assembler.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-14
 */

#pragma once

#include <map>
#include <array>
#include <exception>
#include <utility>
#include "src/hardware.h"
#include "src/core_memory.h"

namespace asmbl {

    class AssemblerException : public std::runtime_error {
    public:
        AssemblerException() = delete;
        explicit AssemblerException(const std::string& what_arg) : std::runtime_error(what_arg) {}
        explicit AssemblerException(const char* what_arg) : std::runtime_error(what_arg) {}
        AssemblerException(const AssemblerException& other) noexcept = default;
    };

    /**
     * @class Assembler
     * @brief A simple PDP-8 assembler. Reads source code from an istream and outputs BIN format to ostream.
     */
    class Assembler {
    public:
        using word_t = uint16_t;

        /**
         * @brief Methods for combining multiple Instructions in one instruction.
         */
        enum CombinationType {
            Memory,    ///< Replace any pre-existing value (memory access operations).
            Flag,       ///< Logical Or with Memory value (Indirect Flag)
            Mask,       ///< Logical And with Memory value (Page Zero Flag).
            Gr,         ///< Group agnostic microcode Or with any Gr1, Gr2, Gr3.
            Gr1,        ///< Group 1 microcode
            Gr2,        ///< Group 2 microcode
            Gr3,        ///< Group 3 microcode
        };

        enum SymbolSatus {
            Undefined,  ///< An undefined symbol
            Defined,    ///< A defined symbol
        };

        struct Instruction {
            word_t opCode;
            std::string_view mnemonic;
            CombinationType orCombination;
        };

        struct Symbol {
            word_t value;
            std::string symbol;
            SymbolSatus status;

            Symbol() = default;
            ~Symbol() = default;
            Symbol(word_t value, std::string symbol, SymbolSatus status)
            : value(value), symbol(std::move(symbol)), status(status) {}
            Symbol(const Symbol&) = default;
            Symbol(Symbol&&) = default;
            Symbol& operator=(const Symbol &) = default;
            Symbol& operator=(Symbol &&) = default;
        };

        static constexpr std::array<Instruction, 48> InstructionSet =
                {{
                         // Instruction flags
                         {00400, "I", Flag},
                         {07577, "Z", Mask},
                         // Memory access functions, default to current page addressing
                         {00000, "AND", Memory},
                         {01000, "TAD", Memory},
                         {02000, "ISZ", Memory},
                         {03000, "DCA", Memory},
                         {04000, "JMS", Memory},
                         {05000, "JMP", Memory},
                         // Group agnostic Microcode
                         {07000, "NOP", Gr},
                         {07200, "CLA", Gr},
                         // Group 1 Microcode
                         {07100, "CLL", Gr1},
                         {07040, "CMA", Gr1},
                         {07020, "CML", Gr1},
                         {07001, "IAC", Gr1},
                         {07010, "RAR", Gr1},
                         {07004, "RAL", Gr1},
                         {07012, "RTR", Gr1},
                         {07006, "RTL", Gr1},
                         {07002, "BSW", Gr1},
                         // Group 2 Microcode OR group
                         {07500, "SMA", Gr2},
                         {07440, "SZA", Gr2},
                         {07420, "SNZ", Gr2},
                         // Group 2 Microcode AND group
                         {07510, "SPA", Gr2},
                         {07450, "SNA", Gr2},
                         {07430, "SZL", Gr2},
                         // Group 2 Privileged Microcode
                         {07404, "OSR", Gr2},
                         {07402, "HLT", Gr2},
                         // Microcode Macros -- Commonly combined microcode
                         {07041, "TCA", Gr1},
                         {07540, "SLE", Gr2}, // Skip if accumulator less than or equal to zero
                         {07550, "SGZ", Gr2}, // Skip if accumulator greater than zero
                         // Group 3 Microcode
                         {07501, "MQA", Gr3}, // Or Multiplier Quotient with Accumulator
                         {07421, "MQL", Gr3}, // Multiplier Quotient Load
                         // Memory management
                         {06201, "CDF", Memory},
                         {06202, "CIF", Memory},
                         {06214, "RDF", Memory},
                         {06224, "RIF", Memory},
                         {06234, "RIB", Memory},
                         {06244, "RMF", Memory},
                         // High speed paper tape input
                         {06010, "RPE", Memory},
                         {06011, "RSF", Memory},
                         {06012, "RRB", Memory},
                         {06014, "RFC", Memory},
                         {06016, "RBC", Memory}, // Read buffer and continue
                         // High speed paper tap output
                         {06020, "PCE", Memory},
                         {06021, "PSF", Memory},
                         {06022, "PCF", Memory},
                         {06024, "PPC", Memory},
                         {06026, "PLS", Memory},
                 }};

    protected:
        std::map<std::string_view, Instruction> instructionMap{};
        std::map<std::string, Symbol> symbolTable{};

        enum class TokenClass {
            UNKNOWN, PC_TOKEN, WORD_ALLOCATION, ASSIGNMENT, OP_CODE, NUMBER, LITERAL, ADD, SUB, LABEL_CREATE
        };

        struct AssemblerToken {
            TokenClass tokenClass{TokenClass::UNKNOWN};
            std::string literal{};
            AssemblerToken() = default;
            ~AssemblerToken() = default;
            AssemblerToken(TokenClass tokenClass, std::string& literal) : tokenClass(tokenClass), literal(literal) {}
            AssemblerToken(const AssemblerToken &) = default;
            AssemblerToken(AssemblerToken &&) = default;
            AssemblerToken& operator=(const AssemblerToken&) = default;
            AssemblerToken& operator=(AssemblerToken&&) = default;
        };

        std::optional<sim::register_type> get_token_value(const AssemblerToken &token);

        using TokenList = std::vector<AssemblerToken>;
        void classify_tokens(TokenList& tokens);

        void generate_code(word_t pc, TokenList::iterator first, TokenList::iterator last, TokenList::iterator label,
                           std::ostream &list, std::ostream &bin);

    public:

        Assembler();

        void clear() {
            symbolTable.clear();
        }

        static TokenList parse_tokens(std::istream &src);

        void pass1(std::istream &src);

        void pass2(std::istream &src, std::ostream &list, std::ostream &bin);

        void dump_symbols(std::ostream &strm);
    };
}

