/**
 * @file Assembler.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-14
 */

#include <cctype>
#include <iostream>
#include <fmt/format.h>
#include "Assembler.h"

namespace pdp8asm {

    std::tuple<TokenClass, std::string> TokenParser::nextToken() {
        literalValue.clear();
        for (auto &token: tokenList) {
            token->clear();
        }

        int character = input.get();
        while (input && character != EOF) {
            int failedOnCount = 0, passingCount = 0;
            int passingScore = 0;
            for (auto &token: tokenList) {
                if (token->passingCount >= passingScore)
                    switch (token->parse(character)) {
                        case PASSING:
                            token->trackPassingCount();
                            ++passingCount;
                            break;
                        case FAILED_ON:
                            ++failedOnCount;
                        default:
                            break;
                    }
                else
                    break;
                passingScore = std::max(passingScore, token->passingCount);
            }

            std::sort(tokenList.begin(), tokenList.end(),
                      [](std::unique_ptr<TokenType> &t0, std::unique_ptr<TokenType> &t1) -> bool {
                          return t0->passingCount > t1->passingCount;
                      });

            if (failedOnCount == 1) {
                input.putback(static_cast<char>(character));
                auto foundToken = std::find_if(tokenList.begin(), tokenList.end(),
                                               [](const std::unique_ptr<TokenType> &t) {
                                                   return t->tokenState == FAILED_ON;
                                               });
                return {(*foundToken)->tokenClass, literalValue};
            } else if (failedOnCount > 1) {
                throw std::runtime_error("Ambiguous tokens");
            }
            literalValue.push_back(static_cast<char>(character));
            character = input.get();
        }
        if (literalValue.empty())
            return {TokenClass::END_OF_FILE, literalValue};
        return {tokenList.front()->tokenClass, literalValue};
    }

    Assembler::Assembler() {
        clear();
        for (auto &instruction: InstructionSet) {
            instructionTable.emplace(std::string{instruction.mnemonic}, instruction);
        }
    }

    void Assembler::clear() {
        symbolTable.clear();
        program.clear();
        for (auto &symbol: PRE_DEFINED_SYMBOLS) {
            symbolTable.emplace(std::string{symbol.symbol}, Symbol(symbol.value , Defined));
        }
    }

    void Assembler::readProgram(std::istream &istream) {
        clear();
        pdp8asm::TokenParser tokenParser{istream};
        pdp8asm::TokenClass tokenClass = pdp8asm::UNKNOWN;
        do {
            std::string literalValue;
            std::tie(tokenClass, literalValue) = tokenParser.nextToken();
            if (tokenClass == LITERAL) {
                if (literalValue == "OCTAL") {
                    tokenClass = OCTAL;
                } else if (literalValue == "DECIMAL") {
                    tokenClass = DECIMAL;
                } else if (literalValue == "AUTOMATIC") {
                    tokenClass = AUTOMATIC;
                } else if (auto opCode = instructionTable.find(literalValue); opCode != instructionTable.end()) {
                    tokenClass = OP_CODE;
                } else {
                    std::string upperCase{};
                    auto back = std::back_insert_iterator<std::string>(upperCase);
                    std::transform(literalValue.begin(), literalValue.end(), back,
                                   [](unsigned char c) { return std::toupper(c); });
                    if (opCode = instructionTable.find(upperCase); opCode != instructionTable.end()) {
                        tokenClass = OP_CODE;
                    } else if (auto label = symbolTable.find(literalValue); label != symbolTable.end()) {
                        tokenClass = LABEL;
                    } else {
                        symbolTable.emplace(literalValue, Symbol{0, Undefined});
                    }
                }
            }
            if (tokenClass != WHITE_SPACE)
                program.emplace_back(tokenClass, literalValue);
        } while (tokenClass != pdp8asm::END_OF_FILE);

        for (auto first = program.begin(); first != program.end(); ++first) {
            auto next = first + 1;
            if (next != program.end()) {
                if (first->tokenClass == LITERAL && next->tokenClass == LABEL_DEFINE ||
                    next->tokenClass == LABEL_ASSIGN) {
                    undefinedLabel(first->literal);
                    first->tokenClass = LABEL;
                    ++first;
                }
            }
        }

        for (auto & token : program ) {
            if (token.tokenClass == LITERAL) {
                if (auto label = symbolTable.find(token.literal); label != symbolTable.end())
                    token.tokenClass = LABEL;
            }
        }
    }

    bool Assembler::pass1() {
        assemblerPass = PASS_ONE;
        if (program.back().tokenClass != END_OF_FILE)
            throw std::invalid_argument("Program does not terminate with End of File.");

        std::ostream nullStream(&nullStreamBuffer);
        auto first = program.begin();
        while (first != program.end()) {
            first = parseLine(nullStream, nullStream, first, program.end());
        }
        return true;
    }

    bool Assembler::pass2(std::ostream &binary, std::ostream &listing) {
        assemblerPass = PASS_TWO;

        auto first = program.begin();
        while (first != program.end()) {
            first = parseLine(binary, listing, first, program.end());
        }
        return true;
    }

    Assembler::Program::iterator
    Assembler::parseLine(std::ostream &binary, std::ostream &listing, Program::iterator first, Program::iterator last) {
        // Skip comment lines
        bool incrementProgramCounter = false;
        auto startOfLine = first;
        // Parse lines that begin with LITERALS
        if (first->tokenClass == OCTAL) {
            radix = Radix::OCTAL;
            ++first;
        } else if (first->tokenClass == DECIMAL) {
            radix = Radix::DECIMAL;
            ++first;
        } else if (first->tokenClass == AUTOMATIC) {
            radix = Radix::AUTOMATIC;
            ++first;
        } else if (first->tokenClass == LABEL && (first + 1)->tokenClass == LABEL_ASSIGN) {
            auto[value, itr] = evaluateExpression(first + 2, last);
            setLabelValue(first->literal, value);
            first = itr;
        } else if (first->tokenClass == LABEL && (first + 1)->tokenClass == LABEL_DEFINE) {
            setLabelValue(first->literal, programCounter);
            incrementProgramCounter = true;
            ++first;
            ++first;
        } else if (first->tokenClass == LOCATION) {
            std::tie(programCounter, first) = evaluateExpression(first + 1, last);
        } else if (!isEndOfLine(first->tokenClass) && first->tokenClass != COMMENT) {
            incrementProgramCounter = true;
        }

        if (incrementProgramCounter) {
            if (assemblerPass == PASS_TWO) {
                if (isValue(first->tokenClass)) {
                    std::tie(codeValue, first) = evaluateExpression(first, last);
                } else {
                    if (first->tokenClass == OP_CODE) {
                        std::tie(codeValue, first) = evaluateOpCode(first, last);
                    }
                }
            }
            while (!isEndOfCodeLine(first->tokenClass))
                ++first;
        }

        if (first->tokenClass == COMMENT)
            ++first;
        if (isEndOfLine(first->tokenClass)) {
            generateListing(listing, startOfLine, first);
            codeValue = std::nullopt;
            if (incrementProgramCounter && assemblerPass == PASS_TWO) {
                ++programCounter;
            }
            ++first;
        } else
            throw std::invalid_argument("Line did not end where expected.");
        return first;
    }

    void Assembler::generateListing(std::ostream &listing, Assembler::Program::iterator first,
                                    Assembler::Program::iterator last) {
        if (codeValue) {
            listing << fmt::format("{:04o}  {:04o}  ", programCounter, codeValue.value());
        } else if (first->tokenClass == LABEL && (first + 1)->tokenClass == LABEL_DEFINE) {
            listing << fmt::format("{:04o}        ", programCounter);
        } else
            listing << fmt::format("{:12}", "");

        if (first->tokenClass == LABEL) {
            if (auto next = first + 1; next->tokenClass == LABEL_DEFINE || next->tokenClass == LABEL_ASSIGN) {
                listing << fmt::format("{:>18}{} ", first->literal, next->literal);
                first = next;
            } else {
                listing << fmt::format("{:>18}  ", first->literal);
            }
            ++first;
        } else if (first->tokenClass == COMMENT) {
            listing << first->literal;
            ++first;
        } else {
            listing << fmt::format("{:>18}  ", "");
        }

        std::stringstream strm{};
        for (; first != last && first->tokenClass != COMMENT; ++first) {
            strm << first->literal << ' ';
        }

        listing << fmt::format("{:<32}", strm.str());
        if (first->tokenClass == COMMENT) {
            listing << first->literal;
            ++first;
        }
        listing << '\n';
    }

    std::tuple<word_t, Assembler::Program::iterator>
    Assembler::evaluateOpCode(Assembler::Program::iterator first, Assembler::Program::iterator last) {
        if (first->tokenClass != OP_CODE)
            throw std::invalid_argument("Called without OpCode.");

        word_t code = 0u;
        word_t arg = 0u;
        bool opCode = false;
        bool memoryOpr = false;
        bool finished = false;
        bool zeroFlag = false;
        CombinationType restrict{CombinationType::Gr};

        for (; first != last; ++first) {
            switch (first->tokenClass) {
                case TokenClass::END_OF_FILE:
                case TokenClass::END_OF_LINE:
                case TokenClass::COMMENT:
                    finished = true;
                    break;
                case TokenClass::NUMBER:
                case TokenClass::LITERAL:
                case TokenClass::LABEL:
                case TokenClass::PROGRAM_COUNTER:
                    std::tie(arg, first) = evaluateExpression(first, last);
                    finished = first == last;
                    break;
                case TokenClass::OP_CODE:
                    if (auto op = instructionTable.find(first->literal); op != instructionTable.end()) {
                        opCode = true;
                        switch (op->second.orCombination) {
                            case CombinationType::Memory:
                                code = op->second.opCode;
                                memoryOpr = true;
                                break;
                            case CombinationType::Flag:
                            case CombinationType::Gr:
                                code |= op->second.opCode;
                                break;
                            case CombinationType::Mask:
                                if (memoryOpr && op->second.opCode == 07577)
                                    zeroFlag = true;
                                else
                                    code &= op->second.opCode;
                                break;
                            case CombinationType::Gr1:
                                if (restrict == CombinationType::Gr || restrict == CombinationType::Gr1) {
                                    code |= op->second.opCode;
                                    restrict = CombinationType::Gr1;
                                } else
                                    throw std::invalid_argument("Invalid microcode combination:");
                                break;
                            case CombinationType::Gr2:
                                if (restrict == CombinationType::Gr || restrict == CombinationType::Gr2) {
                                    code |= op->second.opCode;
                                    restrict = CombinationType::Gr2;
                                } else
                                    throw std::invalid_argument("Invalid microcode combination:");
                                break;
                            case CombinationType::Gr3:
                                if (restrict == CombinationType::Gr || restrict == CombinationType::Gr3) {
                                    code |= op->second.opCode;
                                    restrict = CombinationType::Gr3;
                                } else
                                    throw std::invalid_argument("Invalid microcode combination:");
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
            finished |= isEndOfCodeLine(first->tokenClass);
            if (finished)
                break;
        }

        if (opCode) {
            if (memoryOpr) {
                code |= arg & 0177;
                if (arg > 0177) {
                    if ((arg & 07300) != (programCounter & 07300)) {
                        throw std::invalid_argument(fmt::format("Memory location out of range {:04o}", arg));
                    }
                    code |= 0200;       // Current page flag;
                }
                if (zeroFlag)
                    code &= 07577;      // Zero flag forced by 'Z' token in source.
            }
        } else {
            code = arg;
        }
        return {code, first};
    }

    void Assembler::setLabelValue(const std::string &literal, word_t value) {
        if (auto symbol = symbolTable.find(literal); symbol != symbolTable.end()) {
            symbol->second.value = value;
            symbol->second.status = Defined;
        } else {
            symbolTable.emplace(literal, Symbol{value, Defined});
        }
    }

    void Assembler::undefinedLabel(const std::string &literal) {
        if (auto symbol = symbolTable.find(literal); symbol != symbolTable.end()) {
            symbolTable.emplace(literal, Symbol{0, Undefined});
        }
    }

    word_t Assembler::convertNumber(const std::string &literal) const {
        switch (radix) {
            case Radix::OCTAL:
                return stoul(literal, nullptr, 8);
            case Radix::DECIMAL:
                return stoul(literal, nullptr, 10);
            case Radix::AUTOMATIC:
                return stoul(literal, nullptr, 0);
        }
        return 0;
    }

    word_t Assembler::convertLabel(const std::string &literal) const {
        if (auto symbol = symbolTable.find(literal); symbol != symbolTable.end()) {
            if (symbol->second.status == Defined)
                return symbol->second.value;
        }
        throw std::invalid_argument("Undefined label.");
    }

    std::tuple<word_t, Assembler::Program::iterator>
    Assembler::evaluateExpression(Program::iterator first, Program::iterator last) const {
        std::optional<word_t> left{}, right{};
        TokenClass opCode = UNKNOWN;
        while (first != last) {
            if (!left && !right && !isOperator(opCode) && isValue(first->tokenClass)) {
                if (first->tokenClass == NUMBER)
                    left = convertNumber(first->literal);
                else if (first->tokenClass == LABEL)
                    left = convertLabel(first->literal);
                else if (first->tokenClass == PROGRAM_COUNTER)
                    left = programCounter;
                ++first;
            } else if (left && !right && !isOperator(opCode) && isOperator(first->tokenClass)) {
                opCode = first->tokenClass;
                ++first;
            } else if (left && !right && isOperator(opCode) && isValue(first->tokenClass)) {
                if (first->tokenClass == NUMBER)
                    right = convertNumber(first->literal);
                else if (first->tokenClass == LABEL)
                    right = convertLabel(first->literal);
                else if (first->tokenClass == PROGRAM_COUNTER)
                    left = programCounter;
                if (opCode == ADDITION)
                    left = left.value() + right.value();
                else
                    left = left.value() - right.value();
                right = std::nullopt;
                opCode = UNKNOWN;
                ++first;
            } else if (left && !right && !isOperator(opCode)) {
                if (left.value() > 07777)
                    left = left.value() & 07777;
                if (left)
                    return {left.value(), first};
                throw std::invalid_argument("Bad expression.");
            }
        }
        return {0, first};
    }
}
