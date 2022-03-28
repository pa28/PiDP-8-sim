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
            symbolTable.emplace(std::string{symbol.symbol}, symbol);
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
                if (auto opCode = instructionTable.find(literalValue); opCode != instructionTable.end()) {
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
                        symbolTable.emplace(literalValue, Symbol{0, literalValue, Undefined});
                    }
                }
            }
            if (tokenClass != WHITE_SPACE)
                program.emplace_back(tokenClass, literalValue);
        } while (tokenClass != pdp8asm::END_OF_FILE);

        for (auto first = program.begin(); first != program.end(); ++first) {
            auto next = first + 1;
            if (next != program.end()) {
                if (first->tokenClass == LITERAL && next->tokenClass == LABEL_DEFINE || next->tokenClass == LABEL_ASSIGN) {
                    first->tokenClass = LABEL;
                    ++first;
                }
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
        // Parse lines that begin with LITERALS
        if (first->tokenClass == LITERAL) {
            if (first->tokenClass == LITERAL && first->literal == "OCTAL") {
                radix = Radix::OCTAL;
                ++first;
            }
        } else if (first->tokenClass == LABEL && (first+1)->tokenClass == LABEL_ASSIGN) {
            auto[value, itr] = evaluateExpression(first+2, last);
            setLabelValue(first->literal, value);
            first = itr;
        } else if (first->tokenClass == LABEL && (first+1)->tokenClass == LABEL_DEFINE) {
            setLabelValue(first->literal, programCounter);
            incrementProgramCounter = true;
            ++first;
            ++first;
        } else if (first->tokenClass == LOCATION) {
            std::tie(programCounter, first) = evaluateExpression(first+1, last);
        } else if (!isEndOfLine(first->tokenClass) && first->tokenClass != COMMENT) {
            incrementProgramCounter = true;
        }

        if (incrementProgramCounter) {
            if (assemblerPass == PASS_TWO) {

            }
            while (!isEndOfLine(first->tokenClass))
                ++first;
            ++programCounter;
        }

        if (first->tokenClass == COMMENT)
            ++first;
        if (isEndOfLine(first->tokenClass))
            ++first;
        return first;
    }

    void Assembler::setLabelValue(const std::string& literal, word_t value) {
        if (auto symbol = symbolTable.find(literal); symbol != symbolTable.end()) {
            symbol->second.value = value;
            symbol->second.status = Defined;
        } else {
            symbolTable.emplace(literal, Symbol{value, literal, Defined});
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
                    return {left.value(),first};
                throw std::invalid_argument("Bad expression.");
            }
        }
        return {0,first};
    }
}
