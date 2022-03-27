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
    }

    bool Assembler::pass1() {
        if (program.back().tokenClass != END_OF_FILE)
            throw std::invalid_argument("Program does not terminate with End of File.");
        word_t programCounter = 0;

        auto itr = program.begin();
        while (itr != program.end()) {
            switch (itr->tokenClass) {
                case LOCATION: {
                    ++itr;
                    std::tie(programCounter,itr) = evaluateExpression(itr, program.end());
                }
                    break;
                default:
                    break;
            }
            ++itr;
        }
        return true;
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
                ++first;
            } else if (left && !right && !isOperator(opCode) && isOperator(first->tokenClass)) {
                opCode = first->tokenClass;
                ++first;
            } else if (left && !right && isOperator(opCode) && isValue(first->tokenClass)) {
                if (first->tokenClass == NUMBER)
                    right = convertNumber(first->literal);
                else if (first->tokenClass == LABEL)
                    right = convertLabel(first->literal);
                if (opCode == ADDITION)
                    left = left.value() + right.value();
                else
                    left = left.value() - right.value();
                right = std::nullopt;
                opCode = UNKNOWN;
                ++first;
            } else if (left && !right && !isOperator(opCode)) {
                if (left)
                    return {left.value(),first};
                throw std::invalid_argument("Bad expression.");
            }
        }
        return {0,first};
    }
}
