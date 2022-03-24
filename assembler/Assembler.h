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
#include "src/CoreMemory.h"

namespace asmbl {

    class AssemblerException : public std::runtime_error {
    public:
        AssemblerException() = delete;

        explicit AssemblerException(const std::string &what_arg) : std::runtime_error(what_arg) {}

        explicit AssemblerException(const char *what_arg) : std::runtime_error(what_arg) {}

        AssemblerException(const AssemblerException &other) noexcept = default;
    };

    class AssemblerMemoryOutOfRange : public std::runtime_error {
    public:
        AssemblerMemoryOutOfRange() = delete;

        explicit AssemblerMemoryOutOfRange(const std::string &what_arg) : std::runtime_error(what_arg) {}

        explicit AssemblerMemoryOutOfRange(const char *what_arg) : std::runtime_error(what_arg) {}

        AssemblerMemoryOutOfRange(const AssemblerMemoryOutOfRange &other) noexcept = default;
    };

    class AssemblerSymbolNotFound : public AssemblerException {
    public:
        AssemblerSymbolNotFound() = delete;

        explicit AssemblerSymbolNotFound(const std::string &what_arg) : AssemblerException(what_arg) {}

        explicit AssemblerSymbolNotFound(const char *what_arg) : AssemblerException(what_arg) {}

        AssemblerSymbolNotFound(const AssemblerSymbolNotFound &other) noexcept = default;
    };

    class AssemblerSymbolNotDefined : public AssemblerException {
    public:
        AssemblerSymbolNotDefined() = delete;

        explicit AssemblerSymbolNotDefined(const std::string &what_arg) : AssemblerException(what_arg) {}

        explicit AssemblerSymbolNotDefined(const char *what_arg) : AssemblerException(what_arg) {}

        AssemblerSymbolNotDefined(const AssemblerSymbolNotDefined &other) noexcept = default;
    };

    class AssemblerNumberInvalidArg : public AssemblerException {
    public:
        AssemblerNumberInvalidArg() = delete;

        explicit AssemblerNumberInvalidArg(const std::string &what_arg) : AssemblerException(what_arg) {}

        explicit AssemblerNumberInvalidArg(const char *what_arg) : AssemblerException(what_arg) {}

        AssemblerNumberInvalidArg(const AssemblerNumberInvalidArg &other) noexcept = default;
    };

    class AssemblerNumberRangeError : public AssemblerException {
    public:
        AssemblerNumberRangeError() = delete;

        explicit AssemblerNumberRangeError(const std::string &what_arg) : AssemblerException(what_arg) {}

        explicit AssemblerNumberRangeError(const char *what_arg) : AssemblerException(what_arg) {}

        AssemblerNumberRangeError(const AssemblerNumberRangeError &other) noexcept = default;
    };

    class AssemblerAbort : public std::runtime_error {
    public:
        AssemblerAbort() = delete;

        explicit AssemblerAbort(const std::string &what_arg) : std::runtime_error(what_arg) {}

        explicit AssemblerAbort(const char *what_arg) : std::runtime_error(what_arg) {}

        AssemblerAbort(const AssemblerAbort &other) noexcept = default;
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

        enum SymbolStatus {
            Undefined,  ///< An undefined symbol
            ReDefined,  ///< Symbol has been redefined, redefinition location stored in value.
            Defined,    ///< A defined symbol
        };

        struct Instruction {
            word_t opCode;
            std::string_view mnemonic;
            CombinationType orCombination;
        };

        /**
         * @struct Symbol
         * @brief The information required for an assembler symbol.
         * @details By implementation Symbols are case sensitive. However in mose PDP-8 assemblers symbols and
         * op-codes can be used interchangeably (syntactically speaking). It is desirable to allow users to
         * treat op-codes as if they are case insensitive. The way this is handled is that a token is first tested
         * for membership in the op-code list, which is all upper case. If it not found it is searched for
         * membership in the symbol table. If it is not found the all uppercase version of the token is
         * searched for in the op-code list and classified an op code if found.
         */
        struct Symbol {
            word_t value{};
            std::string symbol{};
            SymbolStatus status{Undefined};

            Symbol() = default;

            ~Symbol() = default;

            Symbol(word_t value, std::string symbol, SymbolStatus status)
                    : value(value), symbol(std::move(symbol)), status(status) {}

            Symbol(const Symbol &) = default;

            Symbol(Symbol &&) = default;

            Symbol &operator=(const Symbol &) = default;

            Symbol &operator=(Symbol &&) = default;
        };

        struct PreDefinedSymbol {
            word_t value{};
            std::string_view symbol{};
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

        static constexpr std::array<PreDefinedSymbol,8> PRE_DEFINED_SYMBOLS =
                {{
                         { 0010, "_AutoIndex0" },
                         { 0011, "_AutoIndex1" },
                         { 0012, "_AutoIndex2" },
                         { 0013, "_AutoIndex3" },
                         { 0014, "_AutoIndex4" },
                         { 0015, "_AutoIndex5" },
                         { 0016, "_AutoIndex6" },
                         { 0017, "_AutoIndex7" },
                }};

        enum class Radix { OCTAL, DECIMAL, AUTOMATIC };

    protected:
        std::map<std::string_view, Instruction> instructionMap{};
        std::map<std::string, Symbol> symbolTable{};
        Radix numberRadix{Radix::OCTAL};

        enum class TokenClass {
            UNKNOWN,
            LOCATION,
            PC_TOKEN,
            ASSIGNMENT,
            OP_CODE,
            NUMBER,
            LITERAL,
            ADD,
            SUB,
            LABEL_CREATE,
            COMMENT,
            OCTAL,
            DECIMAL,
            AUTOMATIC,
        };

        struct AssemblerToken {
            TokenClass tokenClass{TokenClass::UNKNOWN};
            std::string literal{};

            AssemblerToken() = default;

            ~AssemblerToken() = default;

            AssemblerToken(TokenClass tokenClass, std::string &literal) : tokenClass(tokenClass), literal(literal) {}

            AssemblerToken(const AssemblerToken &) = default;

            AssemblerToken(AssemblerToken &&) = default;

            AssemblerToken &operator=(const AssemblerToken &) = default;

            AssemblerToken &operator=(AssemblerToken &&) = default;
        };

        std::optional<sim::register_type> get_token_value(const AssemblerToken &token, sim::register_type pc);

        using TokenList = std::vector<AssemblerToken>;

        [[nodiscard]] sim::register_type
        generate_code(TokenList::iterator first, TokenList::iterator last, sim::register_type pc);

        [[nodiscard]] std::tuple<TokenList::iterator, sim::register_type>
        evaluate_expression(TokenList::iterator first, TokenList::iterator last, sim::register_type pc);

    public:

        Assembler();

        void clear() {
            symbolTable.clear();

            for (auto &symbol: PRE_DEFINED_SYMBOLS) {
                symbolTable[std::string(symbol.symbol)] = Symbol{symbol.value, std::string(symbol.symbol), Defined};
            }

        }

        static TokenList parse_tokens(std::istream &src);

        void classify_tokens(TokenList &tokens);

        void pass1(std::istream &src);

        void pass2(std::istream &src, std::ostream &bin, std::ostream &list);

        static void
        listing(std::ostream &list, const TokenList &tokens, sim::register_type pc, sim::register_type code);

        void dump_symbols(std::ostream &strm);

        void setNumberRadix(Radix value) { numberRadix = value; }

        std::optional<Assembler::word_t> find_symbol(const std::string &symbol);

        std::optional<Assembler::word_t> parse_command(const std::string &command, sim::register_type pc);
    };
}

