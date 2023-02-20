/*
 * Assembler.h Created by Richard Buckley (C) 18/02/23
 */

/**
 * @file Assembler.h
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 18/02/23
 * @brief 
 * @details
 */

#ifndef PDP8_ASSEMBLER_H
#define PDP8_ASSEMBLER_H


#include <map>
#include <array>
#include <utility>
#include <vector>
#include <memory>
#include <sstream>
#include <exception>
#include <utility>
#include <optional>
#include <cctype>
#include <fmt/format.h>
#include "NullStream.h"

//#include "src/hardware.h"
//#include "src/CoreMemory.h"

// https://www.toptal.com/c-plus-plus/creating-programming-language-in-c- https://github.com/jakisa/stork/tree/part01
// https://www.toptal.com/c-plus-plus/creating-an-expression-parser-in-c-
namespace pdp8asm {

    using word_t = uint16_t;

    inline std::string sToUpper(const std::string& s) {
        std::string out{};
        for (auto c : s){
            out += static_cast<char>(toupper(c));
        }
        return out;
    }

    /**
     * @class BinaryInputFormatter
     * @brief Convert address data pairs into a stream of bytes formatted to the DEC BIN tape format.
     */
    class BinaryInputFormatter {
    protected:
        std::ostream& binary;
        std::optional<word_t>   programCounter{};

    public:

        BinaryInputFormatter() = delete;

        ~BinaryInputFormatter() = default;

        explicit BinaryInputFormatter(std::ostream& strm) : binary(strm) {}

        void write(word_t address, word_t data);

        void write(word_t address);
    };

    enum TokenClass {
        UNKNOWN, END_OF_FILE, WHITE_SPACE, END_OF_LINE, COMMENT, LITERAL, LABEL, OP_CODE, NUMBER, LABEL_DEFINE,
        LABEL_ASSIGN, LOCATION, PROGRAM_COUNTER, ADDITION, SUBTRACTION, END_OF_INSTRUCTION,
        OCTAL, DECIMAL, AUTOMATIC
    };

    /**
     * @brief Return true if tokenClass represents a value.
     * @param tokenClass
     * @return bool
     */
    inline bool isValue(TokenClass tokenClass) {
        switch (tokenClass) {
            case NUMBER:
            case LABEL:
            case PROGRAM_COUNTER:
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Return true if tokenClass represents an arithmetic operation
     * @param tokenClass
     * @return bool
     */
    inline bool isOperator(TokenClass tokenClass) {
        switch (tokenClass) {
            case ADDITION:
            case SUBTRACTION:
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Determine if token is end of a line
     * @param tokenClass
     * @return boolean
     */
    inline bool isEndOfLine(TokenClass tokenClass) {
        return tokenClass == END_OF_LINE || tokenClass == END_OF_FILE;
    }

    /**
     * @brief Determine if token is the end of the code line.
     * @param tokenClass
     * @return boolean
     */
    inline bool isEndOfCodeLine(TokenClass tokenClass) {
        return tokenClass == END_OF_LINE || tokenClass == END_OF_FILE || tokenClass == COMMENT;
    }

    /**
     * @brief The state of token parsing.
     */
    enum TokenState {
        UNDETERMINED,   ///< No data yes.
        FAILED,         ///< Character stream failed to match token syntax.
        PASSING,        ///< So far the character stream has matched token syntax.
        FAILED_ON       ///< The character stream failed to match token syntax on the current character.
    };

    /**
     * @brief An abstraction of a token.
     */
    struct TokenType {
        TokenState tokenState{UNDETERMINED};
        TokenClass tokenClass{UNKNOWN};
        int passingCount{};

        TokenType() = default;

        virtual ~TokenType() = default;

        virtual void clear() {
            tokenState = UNDETERMINED;
            passingCount = 0;
        }

        virtual TokenState parse(int character) = 0;

        void trackPassingCount() {
            if (tokenState == PASSING)
                ++passingCount;
        }

        bool operator>(const TokenType &other) const noexcept {
            return passingCount > other.passingCount;
        }
    };

    /**
     * @brief Comment token
     */
    struct CommentToken : public TokenType {
        CommentToken() : TokenType() {
            tokenClass = COMMENT;
        }

        ~CommentToken() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    if (character == '/') {
                        tokenState = PASSING;
                        ++passingCount;
                    }
                    break;
                case PASSING:
                    if (character == '\n' || character == '\r')
                        tokenState = FAILED_ON;
                    break;
                case FAILED:
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief A label token
     */
    struct LabelAssignment : public TokenType {
        LabelAssignment() : TokenType() {
            tokenClass = LABEL_ASSIGN;
        }

        ~LabelAssignment() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == '=' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief The label definition token
     */
    struct LabelDefinition : public TokenType {
        LabelDefinition() : TokenType() {
            tokenClass = LABEL_DEFINE;
        }

        ~LabelDefinition() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == ',' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief Set current program counter token
     */
    struct Location : public TokenType {
        Location() : TokenType() {
            tokenClass = LOCATION;
        }

        ~Location() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == '*' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief Get current program counter token
     */
    struct ProgramCounter : public TokenType {
        ProgramCounter() : TokenType() {
            tokenClass = PROGRAM_COUNTER;
        }

        ~ProgramCounter() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == '.' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief Marker to end an instruction before the end of the line or a comment. Additional
     * instructions may follow.
     */
    struct EndOfInstruction : public TokenType {
        EndOfInstruction() : TokenType() {
            tokenClass = END_OF_INSTRUCTION;
        }

        ~EndOfInstruction() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == ';' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief Addition operator
     */
    struct Addition : public TokenType {
        Addition() : TokenType() {
            tokenClass = ADDITION;
        }

        ~Addition() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == '+' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief Subtraction operator.
     */
    struct Subtraction : public TokenType {
        Subtraction() : TokenType() {
            tokenClass = SUBTRACTION;
        }

        ~Subtraction() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (character == '-' ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = FAILED_ON;
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief End of an input line.
     */
    struct EndOfLine : public TokenType {
        EndOfLine() : TokenType() {
            tokenClass = END_OF_LINE;
        }

        ~EndOfLine() override = default;

        TokenState parse(int character) override {
            auto testCharacter = [character]() {
                return character == '\n' || character == '\r';
            };
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (testCharacter() ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = (testCharacter() ? PASSING : FAILED_ON);
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief White space on a line. This is ignored except to separate tokens which may otherwise run together.
     */
    struct WhiteSpace : public TokenType {
        WhiteSpace() : TokenType() {
            tokenClass = WHITE_SPACE;
        }

        ~WhiteSpace() override = default;

        TokenState parse(int character) override {
            auto testCharacter = [character]() {
                return std::isspace(character) && character != '\n' && character != '\r';
            };
            switch (tokenState) {
                case UNDETERMINED:
                    tokenState = (testCharacter() ? PASSING : FAILED);
                    break;
                case PASSING:
                    tokenState = (testCharacter() ? PASSING : FAILED_ON);
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                    break;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief A string literal. These should all be convertable to Labels.
     */
    struct Literal : public TokenType {
        Literal() : TokenType() {
            tokenClass = LITERAL;
        }

        ~Literal() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    if (std::isalpha(character) || character == '_')
                        tokenState = PASSING;
                    else
                        tokenState = FAILED;
                    break;
                case PASSING:
                    if (!std::isalnum(character) && character != '_')
                        tokenState = FAILED_ON;
                    break;
                case FAILED:
                case FAILED_ON:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @struct Number
     * @brief Syntax for decoding a number in the source stream.
     */
    struct Number : public TokenType {
        enum class InternalState {
            UNKNOWN, DECIMAL, HEX, OCT_PREFIX
        };
        InternalState internalState{InternalState::UNKNOWN};

        Number() : TokenType() {
            tokenClass = NUMBER;
        }

        ~Number() override = default;

        TokenState parse(int character) override {
            switch (tokenState) {
                case UNDETERMINED:
                    if (std::isdigit(character)) {
                        tokenState = PASSING;
                        if (character == '0')
                            internalState = InternalState::OCT_PREFIX;
                        else
                            internalState = InternalState::DECIMAL;
                    }
                    break;
                case PASSING:
                    switch (internalState) {
                        case InternalState::OCT_PREFIX:
                            if (character == 'x' || character == 'X')
                                internalState = InternalState::HEX;
                            else {
                                if (!std::isdigit(character))
                                    tokenState = FAILED_ON;
                                else
                                    internalState = InternalState::DECIMAL;
                            }
                            break;
                        case InternalState::DECIMAL:
                            if (!std::isdigit(character))
                                tokenState = FAILED_ON;
                            break;
                        case InternalState::HEX:
                            if (!std::isxdigit(character))
                                tokenState = FAILED_ON;
                            break;
                        default:
                            break;
                    }
                    break;
                case FAILED_ON:
                    tokenState = FAILED;
                case FAILED:
                    break;
            }
            return tokenState;
        }
    };

    /**
     * @brief A class that can apply token parsers to an input stream converting it to a token stream.
     * @details Tokens are matched by a set of structures which embody each different token syntax.
     */
    struct TokenParser {
        using TokenList = std::vector<std::unique_ptr<TokenType>>;
        size_t textLine{};
        size_t textChar{};

        std::istream &input;
        std::string literalValue{};
        TokenList tokenList{};

        TokenParser() = delete;

        explicit TokenParser(std::istream &istream) : input(istream) {
            textLine = 1;
            textChar = 1;
            tokenList.push_back(std::make_unique<CommentToken>());
            tokenList.push_back(std::make_unique<LabelAssignment>());
            tokenList.push_back(std::make_unique<LabelDefinition>());
            tokenList.push_back(std::make_unique<Location>());
            tokenList.push_back(std::make_unique<ProgramCounter>());
            tokenList.push_back(std::make_unique<Addition>());
            tokenList.push_back(std::make_unique<Subtraction>());
            tokenList.push_back(std::make_unique<Literal>());
            tokenList.push_back(std::make_unique<Number>());
            tokenList.push_back(std::make_unique<EndOfLine>());
            tokenList.push_back(std::make_unique<WhiteSpace>());
            tokenList.push_back(std::make_unique<EndOfInstruction>());
        }

        /**
         * @brief Convert a stream of characters into the next token.
         * @return A tuple containing a token class value and the string literal that matched token syntax.
         * @throws std::runtime_error When ambiguous tokens are encountered.
         */
        std::tuple<TokenClass, std::string, size_t, size_t> nextToken();
    };

    enum SymbolStatus {
        Undefined,  ///< An undefined symbol
        Defined,    ///< A defined symbol
    };

    struct PreDefinedSymbol {
        word_t value{};
        std::string_view symbol{};
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
        SymbolStatus status{Undefined};

        Symbol() = default;

        ~Symbol() = default;

        Symbol(word_t value, SymbolStatus status)
                : value(value), status(status) {}

        Symbol(const Symbol &) = default;

        Symbol(Symbol &&) = default;

        Symbol &operator=(const Symbol &) = default;

        Symbol &operator=(Symbol &&) = default;

    };

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
        Gr2Or,      ///< Group 2 OR microcode
        Gr2And,     ///< Group 2 AND microcode
        Gr3,        ///< Group 3 microcode
    };

    struct Instruction {
        word_t opCode;
        std::string_view mnemonic;
        CombinationType orCombination;
    };

    static constexpr std::array<Instruction, 59> InstructionSet =
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
                     {07041, "CIA", Gr1},
                     {07010, "RAR", Gr1},
                     {07004, "RAL", Gr1},
                     {07012, "RTR", Gr1},
                     {07006, "RTL", Gr1},
                     {07002, "BSW", Gr1},
                     // Group 2 Microcode OR group
                     {07500, "SMA", Gr2Or},
                     {07440, "SZA", Gr2Or},
                     {07420, "SNZ", Gr2Or},
                     // Group 2 Microcode AND group
                     {07510, "SPA", Gr2And},
                     {07450, "SNA", Gr2And},
                     {07430, "SZL", Gr2And},
                     // Group 2 Privileged Microcode
                     {07404, "OSR", Gr2},
                     {07402, "HLT", Gr2},
                     // Microcode Macros -- Commonly combined microcode
                     {07041, "TCA", Gr1},
                     {07540, "SLE", Gr2}, // Skip if accumulator less than or equal to zero
                     {07550, "SGZ", Gr2}, // Skip if accumulator greater than zero
                     // Group 3 Microcode
                     {07621, "CAM", Gr3}, // Clear Accumulator and Multiplier Quotient
                     {07501, "MQA", Gr3}, // Or Multiplier Quotient with Accumulator
                     {07421, "MQL", Gr3}, // Multiplier Quotient Load
                     {07521, "SWP", Gr3}, // Swap MQ and AC
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
                     // Realtime clock - homebrew
                     /*
                      * Flag register bits
                      *
                      * X X X m b f X X S M B F
                      *
                      * X - Not assigned
                      * F - When set to 1 the DONE flag is raised when the fundamental clock signal (50 or 60 HZ) occurs.
                      * B - When set to 1 the DONE flag is raised when the interval counter reaches 0.
                      * M - When set to 1 the DONE flag is raised when the multiplier counter reaches 0.
                      * f - Indicates that the DONE flag was raised by the fundamental clock signal.
                      * b - Indicates that the DONE flag was raised by the interval counter underflow.
                      * m - Indicates that the DONE flag was raised by the multiplier counter underflow.
                      * S - When S is 0 the counters act as cascaded binary deciders. Underflow occurs when
                      * the counter contains 0. Counters are reloaded from respective buffers on underflow.
                      * When S is 1 the counters act in series
                      */
//                     {06130, "CLSF", Memory}, // Clock set Flags - AC > Clock flags
//                     {06131, "CLEI", Memory}, // Clock enable interrupt
//                     {06132, "CLDI", Memory}, // Clock disable interrupt
//                     {06133, "CLSC", Memory}, // Skip on Clock Done flag and clear Done.
//                     {06134, "CLSI", Memory}, // Set counter interval
//                     {06135, "CLSM", Memory}, // Set counter multiplier
//                     {06136, "RAND", Memory}, // Random number generator
//                     {06137, "CLRF", Memory}  // Read and clear Flags.
                     /*
                      * DK8-EA, DK8-EC
                      */
//                      {06131, "CLEI", Memory}, // Enable clock interrupt.
//                      {06132, "CLED", Memory}, // Disable clock interrupt.
//                      {06133, "CLSK", Memory}, // Skip on clock interrupt and clear flag.
                     /*
                      * DK8-EP, DK8-ES
                      */
                     {06130, "CLZE", Memory}, // Clock set Flags - AC > Clock flags
                     {06131, "CLSK", Memory}, // Skip on clock interrupt.
                     {06132, "CLOE", Memory}, // AC to Clock enable register.
                     {06133, "CLAB", Memory}, // AC to Clock buffer register.
                     {06134, "CLEN", Memory}, // Clock enable register to AC.
                     {06135, "CLSA", Memory}, // Clock status to AC.
                     {06136, "CLBA", Memory}, // Clock buffer to AC.
                     {06137, "CLCA", Memory}  // Clock counter to AC.
                     /* CLZE 6130 Clear enable register per AC. Clears enable bits corresponding to bits set in AC
                      * CLSK 6131 Skip on clock interrupt.
                      * CLOE 6132 Transfer "1"s from the AC to clock enabling register.
                      *           Bit 0 enable overflow
                      *           Bit 1,2 mode control
                      *           00 Counter runs at selected rate overflows at 4096 counts.
                      *           01 Counter runs at selected rate, overflow causes the buffer register
                      *           to transfer to the counter.
                      *           10 Counter runs at selected rate, external trigger cause buffer to transfer
                      *           to the counter.
                      *           11 Counter runs at selected rate, external trigger cause the counter to transfer
                      *           to the buffer. The counter will continue to run from 0.
                      *           Bit 3,4,5 Count rate
                      *           000 Stop
                      *           001 External source
                      *           010 100 Hz
                      *           011 1 kHz
                      *           100 10 kHz
                      *           101 100 kHz
                      *           110 1 MHz
                      *           111 Stop
                      *           Bit 6 When set 1 overflow causes external pulse.
                      *           Bit 7 When set 1 the counter is inhibited from counting.
                      *           Bit 8 When set 1 External signal causes interrupt.
                      *           Bit 9,10,11 Enable external triggers
                      *           100 Input 4
                      *           010 Input 2
                      *           001 Input 1
                      *
                      * CLAB 6133 Transfer AC to the clock then buffers and counter.
                      * CLEN 6134 Transfer enable register to AC see CLOE
                      *
                      * CLSA 6135 Transfer clock status to AC then cleared from status
                      * AC Bits     Functions
                      * 0           Overflow
                      * 1-8         Unused
                      * 9           Input 4
                      * 10          Input 2
                      * 11          Input 1
                      *
                      * CLBA 6136 Transfer clock buffer to the AC
                      * CLca 6137 Transfer clock counter through buffer to the AC
                      */
             }};

    static constexpr std::array<PreDefinedSymbol, 8> PRE_DEFINED_SYMBOLS =
            {{
                     {0010, "_AutoIndex0"},
                     {0011, "_AutoIndex1"},
                     {0012, "_AutoIndex2"},
                     {0013, "_AutoIndex3"},
                     {0014, "_AutoIndex4"},
                     {0015, "_AutoIndex5"},
                     {0016, "_AutoIndex6"},
                     {0017, "_AutoIndex7"},
             }};

    /**
     * @brief The number conversion radix currently in use.
     */
    enum class Radix {
        OCTAL, DECIMAL, AUTOMATIC
    };

    /**
     * @brief An assembler token
     * @details Holds the token class and its literal value.
     */
    struct AssemblerToken {
        TokenClass tokenClass{TokenClass::UNKNOWN};
        std::string literal{};
        size_t textLine{};
        size_t textChar{};

        AssemblerToken() = default;

        ~AssemblerToken() = default;

        AssemblerToken(TokenClass tokenClass, std::string &literal, size_t line, size_t textChar)
                : tokenClass(tokenClass), literal(literal), textLine(line), textChar(textChar) {}

        AssemblerToken(const AssemblerToken &) = default;

        AssemblerToken(AssemblerToken &&) = default;

        AssemblerToken &operator=(const AssemblerToken &) = default;

        AssemblerToken &operator=(AssemblerToken &&) = default;
    };

    /**
     * @brief The assembler.
     */
    struct Assembler {
        using Program = std::vector<AssemblerToken>;
        std::map<std::string, Symbol> symbolTable;
        std::map<std::string, Instruction> instructionTable;
        Radix radix{Radix::OCTAL};
        Program program{};
        word_t programCounter{0};
        std::optional<word_t> codeValue{};
        null_stream::NullStreamBuffer nullStreamBuffer{};

        enum AssemblerPass { PASS_ZERO, PASS_ONE, PASS_TWO } assemblerPass{PASS_ZERO};

        Assembler();

        auto instructionTableFind(const std::string& str) {
            if (auto op = instructionTable.find(str); op != instructionTable.end())
                return op;
            return instructionTable.find(sToUpper(str));
        }
        /**
         * @brief Clear the assembler. This deletes the current program and all symbols it defined.
         */
        void clear();

        /**
         * @brief Read a program from the specified input stream, covert to tokens and save.
         * @param istream
         */
        void readProgram(std::istream &istream);

        /**
         * @brief Pars a single line of assembly code.
         * @param binary The output stream for the binary program in BIN format, only used in pass 2.
         * @param listing The output stream for the program listing, only used in pass 2.
         * @param first The first token in the line.
         * @param last The end of the program.
         * @return The next token after the line.
         * @throws std::invalid_argument Line did not end where expected.
         */
        Assembler::Program::iterator
        parseLine(std::ostream &binary, std::ostream &listing, Program::iterator first, Program::iterator last);

        /**
         * @brief Set a label value, define the label if necessary.
         * @param literal The name of the label.
         * @param value The value to assign.
         */
        void setLabelValue(const std::string& literal, word_t value);

        /**
         * @brief Make an undefined label entry in the symbol table, if one does not already exist.
         * @details These will be used to help resolve forward label references.
         * @param literal The name of the label.
         */
        void undefinedLabel(const std::string& literal);

        /**
         * @brief Perform the first assembly pass.
         * @details This ensures the program can define all labels used in the program.
         * @return true on success.
         * @throws std::invalid_argument Program does not end with END_OF_FILE token.
         */
        bool pass1();

        /**
         * @brief Perform the second assembly pass.
         * @details This pass generates the binary program and a listing.
         * @param binary Output stream for the binary program in BIN format.
         * @param listing Output stream for the program listing.
         * @return true on success.
         */
        bool pass2(std::ostream& binary, std::ostream& listing);

        /**
         * @brief Generate a code listing of a line of assembly to an output stream
         * @param listing The output stream
         * @param first The first token on the line
         * @param last The end of the line.
         */
        void generateListing(std::ostream& listing, Program::iterator first, Program::iterator last);

        /**
         * @brief Convert a number from a string.
         * @param literal The string representation.
         * @return The number
         * @throws std::invalid_argument
         * @throws std::out_of_range
         */
        [[nodiscard]] word_t convertNumber(const std::string& literal) const;

        /**
         * @brief Looks up a liable to get its defined value.
         * @param literal
         * @return The label value.
         * @throws std::invalid_argument Undefined label
         */
        [[nodiscard]] word_t convertLabel(const std::string& literal) const;

        /**
         * @brief Evaluates an expression.
         * @param first The iterator where the expression starts.
         * @param last The limit of the expression, the expression may end before this point.
         * (may be the end of the program).
         * @return A tuple with the expression value and the next token in the program after the expression.
         * @throws std::invalid_argument Bad expression
         * @throws std::invalid_argument Invalid microcode combination.
         * @throws std::invalid_argument Memory location out of range (addressing crosses page boundary).
         */
        [[nodiscard]] std::tuple<word_t, Program::iterator>
        evaluateExpression(Program::iterator first, Program::iterator last) const;

        /**
         * @brief Evaluate the op code portion of an instruction.
         * @param first The first op code.
         * @param last The end of the program.
         * @return a tuple containing the binary op code and then next token to process.
         * @throws std::invalid_argument Called without OpCode.
         */
        [[nodiscard]] std::tuple<word_t, Program::iterator>
        evaluateOpCode(Program::iterator first, Program::iterator last);

        [[maybe_unused]] void dumpSymbols(std::ostream &strm);
    };

    /**
     * @brief Use the assembler to convert an opcode to its binary value.
     * @tparam String A string like object type
     * @param opCodeStr The string with one or more opcodes.
     * @return std::optional with the binary op code if it could be translated.
     */
    template<class String>
    unsigned int generateOpCode(String opCodeStr) {
        null_stream::NullStreamBuffer nullStreamBuffer{};
        std::ostream nullStream(&nullStreamBuffer);

        std::string codeString{opCodeStr};
        std::stringstream strm{codeString};
        Assembler assembler;
        assembler.readProgram(strm);
        assembler.pass1();
        assembler.assemblerPass = Assembler::PASS_TWO;
        auto first = assembler.program.begin();
        auto last = assembler.program.end();
        std::optional<word_t> codeValue{};
        while (first != last) {
            if (first->tokenClass == OP_CODE) {
                std::tie(codeValue, first) = assembler.evaluateOpCode(first, last);
                if (codeValue)
                    return codeValue.value();
            }
        }
        throw std::invalid_argument(fmt::format("Can't convert {} to instruction code.", opCodeStr));
    }
}



#endif //PDP8_ASSEMBLER_H
