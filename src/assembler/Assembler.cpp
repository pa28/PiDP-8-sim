/*
 * Assembler.cpp Created by Richard Buckley (C) 18/02/23
 */

/**
 * @file Assembler.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 18/02/23
 */

#include <fmt/format.h>
#include "Assembler.h"

namespace pdp8asm {
    std::tuple<TokenClass, std::string, size_t, size_t> TokenParser::nextToken() {
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
                return {(*foundToken)->tokenClass, literalValue, textLine, textChar};
            } else if (failedOnCount > 1) {
                throw std::runtime_error("Ambiguous tokens");
            }
            literalValue.push_back(static_cast<char>(character));
            character = input.get();
            if (character == '\n') {
                ++textLine;
                textChar = 0;
            } else {
                ++textChar;
            }
        }
        if (literalValue.empty())
            return {TokenClass::END_OF_FILE, literalValue, textLine, textChar};
        return {tokenList.front()->tokenClass, literalValue, textLine, textChar};
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
            symbolTable.emplace(std::string{symbol.symbol}, Symbol(symbol.value, Defined));
        }
    }

    void Assembler::readProgram(std::istream &istream) {
        clear();
        pdp8asm::TokenParser tokenParser{istream};
        pdp8asm::TokenClass tokenClass = pdp8asm::UNKNOWN;
        do {
            std::string literalValue;
            std::size_t textLine, textChar;
            std::tie(tokenClass, literalValue, textLine, textChar) = tokenParser.nextToken();
            auto literalValueUpper = sToUpper(literalValue);
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
                    literalValueUpper = sToUpper(literalValue);
                    if (opCode = instructionTable.find(literalValueUpper); opCode != instructionTable.end()) {
                        tokenClass = OP_CODE;
                    } else if (auto label = symbolTable.find(literalValue); label != symbolTable.end()) {
                            tokenClass = LABEL;
                        } else {
                            symbolTable.emplace(literalValue, Symbol{0, Undefined});
                        }
                    }
                }
                if (tokenClass != WHITE_SPACE)
                    program.emplace_back(tokenClass, literalValue, textLine, textChar);
            } while (tokenClass != pdp8asm::END_OF_FILE);

            for (auto first = program.begin(); first != program.end(); ++first) {
                auto next = first + 1;
                if (next != program.end()) {
                    if ((first->tokenClass == LITERAL && next->tokenClass == LABEL_DEFINE) ||
                        next->tokenClass == LABEL_ASSIGN) {
                        undefinedLabel(first->literal);
                        first->tokenClass = LABEL;
                        ++first;
                    }
                }
            }

            for (auto &token: program) {
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
        Assembler::parseLine(std::ostream &binary, std::ostream &listing, Program::iterator first,
                             Program::iterator last) {
            // Skip comment lines
            BinaryInputFormatter binaryInputFormatter(binary);

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
                auto [value, itr] = evaluateExpression(first + 2, last);
                setLabelValue(first->literal, value);
                first = itr;
            } else if (first->tokenClass == LABEL && (first + 1)->tokenClass == LABEL_DEFINE) {
                setLabelValue(first->literal, programCounter);
                ++first;
                ++first;
            } else if (first->tokenClass == LOCATION) {
                std::tie(programCounter, first) = evaluateExpression(first + 1, last);
                if (assemblerPass == PASS_TWO)
                    binaryInputFormatter.write(programCounter);
            }

            if (isValue(first->tokenClass)) {
                std::tie(codeValue, first) = evaluateExpression(first, last);
            } else {
                if (first->tokenClass == OP_CODE) {
                    std::tie(codeValue, first) = evaluateOpCode(first, last);
                }
            }

            while (!isEndOfCodeLine(first->tokenClass))
                ++first;

            if (first->tokenClass == COMMENT)
                ++first;
            if (isEndOfLine(first->tokenClass)) {
                generateListing(listing, startOfLine, first);
                if (assemblerPass == PASS_TWO) {
                    if (codeValue) {
                        binaryInputFormatter.write(programCounter, codeValue.value());
                        ++programCounter;
                    }
                } else if (codeValue) {
                    ++programCounter;
                }
                codeValue = std::nullopt;
                ++first;
            }
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
                        if (auto op = instructionTableFind(first->literal); op != instructionTable.end()) {
//                    if (auto op = instructionTable.find(sToUpper(first->literal)); op != instructionTable.end()) {
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
                                case CombinationType::Gr2Or:
                                    if (restrict == CombinationType::Gr || restrict == CombinationType::Gr2 ||
                                        restrict == CombinationType::Gr2Or) {
                                        code |= op->second.opCode;
                                        restrict = CombinationType::Gr2Or;
                                    } else
                                        throw std::invalid_argument("Invalid microcode combination:");
                                    break;
                                case CombinationType::Gr2And:
                                    if (restrict == CombinationType::Gr || restrict == CombinationType::Gr2 ||
                                        restrict == CombinationType::Gr2And) {
                                        code |= op->second.opCode;
                                        restrict = CombinationType::Gr2And;
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
                                case CombinationType::Iot:
                                    if (restrict == CombinationType::Gr) {
                                        code = op->second.opCode;
                                        restrict = CombinationType::Iot;
                                    } else
                                        throw std::invalid_argument("IOT instructions do not combine:");
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
            auto p0 = literal.find_first_of("0123456789aAbBcCdDeEfF");
            auto p1 = literal.find_last_of("0123456789aAbBcCdDeEfF");
            auto convert = literal.substr(p0, (p1 + 1) - p0);
            switch (radix) {
                case Radix::OCTAL:
                    return static_cast<word_t>(stoul(convert, nullptr, 8));
                case Radix::DECIMAL:
                    return static_cast<word_t>(stoul(convert, nullptr, 10));
                case Radix::AUTOMATIC:
                    return static_cast<word_t>(stoul(convert, nullptr, 0));
            }
            return 0;
        }

        word_t Assembler::convertLabel(const std::string &literal) const {
            if (auto symbol = symbolTable.find(literal); symbol != symbolTable.end()) {
                if (symbol->second.status == Defined)
                    return symbol->second.value;
            } else if (assemblerPass == PASS_TWO)
                throw std::invalid_argument(fmt::format("Undefined symbol: '{}'", literal));
            return 0;
        }

        std::tuple<word_t, Assembler::Program::iterator>
        Assembler::evaluateExpression(Program::iterator first, Program::iterator last) const {
            std::optional<word_t> left{}, right{};
            TokenClass opCode = UNKNOWN;
            try {
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
            } catch (std::invalid_argument &e) {
                throw std::invalid_argument(fmt::format("{}, line: {} char: {}", e.what(),
                                                        first->textLine, first->textChar - first->literal.size()));
            }
        }

        [[maybe_unused]] void Assembler::dumpSymbols(std::ostream &strm) {
            strm << fmt::format("\n{:^22}\n", "Symbol Table");
            for (auto &symbol: symbolTable) {
                if (symbol.second.status == Defined)
                    strm << fmt::format("{:04o}  {:<21}\n", symbol.second.value, symbol.first);
                else
                    strm << fmt::format("Undef {:<21}\n", symbol.second.value, symbol.first);
            }
        }

        void BinaryInputFormatter::write(word_t address, word_t data) {
            write(address);
            binary << static_cast<char>((data & 07700) >> 6) << static_cast<char>(data & 077);
        }

        void BinaryInputFormatter::write(word_t address) {
            if (!programCounter || programCounter.value() != address) {
                programCounter = address;
                binary << static_cast<char>(((address & 07700) >> 6) | 0100) << static_cast<char>(address & 077);
            }
        }

    } // pdp8asm
