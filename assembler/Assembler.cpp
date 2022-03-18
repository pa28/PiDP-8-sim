/**
 * @file Assembler.cpp
 * @author Richard Buckley <richard.buckley@ieee.org>
 * @version 1.0
 * @date 2022-03-14
 */

#include <cctype>
#include <fmt/format.h>
#include "Assembler.h"

namespace asmbl {

    Assembler::Assembler() {
        for (auto &instruction: InstructionSet) {
            instructionMap.emplace(instruction.mnemonic, instruction);
        }
    }

    void Assembler::pass1(std::istream &src) {
        sim::register_type pc = 0u;
        for (auto tokens = parse_tokens(src); !tokens.empty(); tokens = parse_tokens(src)) {
            try {
                classify_tokens(tokens);
                switch (tokens.front().tokenClass) {
                    case TokenClass::PC_TOKEN:
                        if (tokens.size() >= 3 && tokens[1].tokenClass == TokenClass::ASSIGNMENT) {
                            if (tokens[2].tokenClass == TokenClass::NUMBER ||
                                tokens[2].tokenClass == TokenClass::LITERAL) {
                                if (auto value = get_token_value(tokens[2]); value)
                                    pc = value.value();
                                else
                                    throw AssemblerException("PC assignment needs defined value.");
                            }
                        }
                        break;
                    case TokenClass::LITERAL:
                        if (tokens.size() >= 3 && tokens[1].tokenClass == TokenClass::LABEL_CREATE) {
                            switch (tokens[2].tokenClass) {
                                case TokenClass::WORD_ALLOCATION:
                                case TokenClass::OP_CODE:
                                case TokenClass::LITERAL:
                                case TokenClass::NUMBER:
                                    if (auto symbol = symbolTable.find(tokens[0].literal); symbol !=
                                                                                           symbolTable.end()) {
                                        symbol->second.value = pc;
                                        symbol->second.status = Defined;
                                    } else {
                                        symbolTable.emplace(tokens[0].literal, Symbol(pc, tokens[0].literal, Defined));
                                    }
                                    ++pc;
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case TokenClass::OP_CODE:
                        ++pc;
                        break;
                    case TokenClass::COMMENT:
                        break;
                    default:
                        throw AssemblerException("Malformed instruction: ");
                }
            } catch (const AssemblerException &ae) {
                std::stringstream strm{};
                strm << ae.what();
                for (auto &token: tokens) {
                    strm << fmt::format(" {}", token.literal);
                }
                throw AssemblerException(strm.str());
            }
        }

        if (auto undef = std::find_if(symbolTable.begin(), symbolTable.end(),
                                      [](const std::pair<std::string, Symbol> &pair) {
                                          return pair.second.status == Undefined;
                                      }); undef != symbolTable.end()) {
            throw AssemblerException("Undefined symbol: " + undef->second.symbol);
        }
    }

    void Assembler::pass2(std::istream &src, std::ostream &bin, std::ostream &list) {
        sim::register_type pc = 0u, code = 0u;
        for (auto tokens = parse_tokens(src); !tokens.empty(); tokens = parse_tokens(src)) {
            try {
                classify_tokens(tokens);
                switch (tokens.front().tokenClass) {
                    case TokenClass::PC_TOKEN:
                        if (tokens.size() >= 3 && tokens[1].tokenClass == TokenClass::ASSIGNMENT) {
                            if (tokens[2].tokenClass == TokenClass::NUMBER ||
                                tokens[2].tokenClass == TokenClass::LITERAL) {
                                if (auto value = get_token_value(tokens[2]); value) {
                                    pc = value.value();
                                    bin << static_cast<char>(0100 | ((pc & 07700) >> 6)) << static_cast<char>(pc & 077);
                                    listing(list, tokens, pc, code);
                                } else
                                    throw AssemblerException("PC assignment needs defined value.");
                            }
                        }
                        break;
                    case TokenClass::LITERAL:
                        if (tokens.size() >= 3 && tokens[1].tokenClass == TokenClass::LABEL_CREATE) {
                            code = generate_code(pc, tokens.begin() + 2, tokens.end(), tokens.begin(), bin);
                            listing(list, tokens, pc, code);
                            ++pc;
                        }
                        break;
                    case TokenClass::OP_CODE:
                        code = generate_code(pc, tokens.begin(), tokens.end(), tokens.end(), bin);
                        listing(list, tokens, pc, code);
                        ++pc;
                        break;
                    case TokenClass::COMMENT:
                        listing(list, tokens, pc, code);
                        break;
                    default:
                        throw AssemblerException("Malformed instruction: ");
                }
            } catch (const AssemblerException &ae) {
                std::stringstream strm{ae.what()};
                for (auto &token: tokens) {
                    strm << fmt::format(" {}", token.literal);
                }
                throw AssemblerException(strm.str());
            }
        }
    }

    std::vector<Assembler::AssemblerToken> Assembler::parse_tokens(std::istream &src) {
        TokenList tok{};
        std::string buffer;
        while (!src.eof()) {
            auto c = src.get();
            if (c == '/') {                             // Comment, ignore rest of instruction
                if (!buffer.empty()) {
                    tok.emplace_back(TokenClass::UNKNOWN, buffer);
                    buffer.clear();
                }

                while (!src.eof()) {
                    c = src.get();
                    if (c == '\n')
                        break;
                    buffer.push_back(c);
                }

                tok.emplace_back(TokenClass::COMMENT, buffer);

                if (!tok.empty())
                    return tok;
            } else {
                if (std::isblank(c)) {
                    if (!buffer.empty()) {   // Blanks separate tokens
                        tok.emplace_back(TokenClass::UNKNOWN, buffer);
                        buffer.clear();
                    } else
                        continue;
                } else if (c == '\n' || c == ';') {   // Other white space or ';' ends instruction
                    if (!buffer.empty()) {
                        tok.emplace_back(TokenClass::UNKNOWN, buffer);
                        buffer.clear();
                    }
                    if (!tok.empty())
                        return tok;
                } else if (c == '=' || c == '.' || c == '+' || c == '-' || c == '@' || c == ':') {
                    if (!buffer.empty()) {
                        tok.emplace_back(TokenClass::UNKNOWN, buffer);
                        buffer.clear();
                    }
                    buffer.push_back(c);
                    switch (c) {
                        case '=':
                            tok.emplace_back(TokenClass::ASSIGNMENT, buffer);
                            break;
                        case '.':
                            tok.emplace_back(TokenClass::PC_TOKEN, buffer);
                            break;
                        case '+':
                            tok.emplace_back(TokenClass::ADD, buffer);
                            break;
                        case '-':
                            tok.emplace_back(TokenClass::SUB, buffer);
                            break;
                        case '@':
                            tok.emplace_back(TokenClass::WORD_ALLOCATION, buffer);
                            break;
                        case ',':
                            tok.emplace_back(TokenClass::LABEL_CREATE, buffer);
                            break;
                        default:
                            break;
                    }
                    buffer.clear();
                } else if (!buffer.empty()) {
                    if (std::isalpha(buffer.at(0))) {
                        if (std::isalnum(c)) {
                            buffer.push_back(c);
                        } else {
                            tok.emplace_back(TokenClass::UNKNOWN, buffer);
                            buffer.clear();
                            buffer.push_back(c);
                        }
                    } else if (std::isdigit(buffer.at(0))) {
                        if (std::isdigit(c) || std::isxdigit(c) || (buffer.length() == 1 && std::toupper(c) == 'X')) {
                            buffer.push_back(c);
                        } else {
                            tok.emplace_back(TokenClass::UNKNOWN, buffer);
                            buffer.clear();
                            buffer.push_back(c);
                        }
                    }
                } else {
                    buffer.push_back(c);
                }
            }
        }
        return tok;
    }

    std::optional<sim::register_type> Assembler::get_token_value(const AssemblerToken &token) {
        switch (token.tokenClass) {
            case TokenClass::NUMBER:
                try {
                    return stoul(token.literal, nullptr, 0);
                } catch (const std::invalid_argument &ia) {
                    throw AssemblerException("Invalid numeric argument: ");
                } catch (const std::out_of_range &range) {
                    throw AssemblerException("Numeric argument out of range: ");
                }
                break;
            case TokenClass::LITERAL:
                if (auto symbol = symbolTable.find(token.literal); symbol != symbolTable.end()) {
                    if (symbol->second.status == Undefined)
                        return std::nullopt;
                    return symbol->second.value;
                } else {
                    if (auto op = instructionMap.find(token.literal); op != instructionMap.end()) {
                        return op->second.opCode;
                    }
                }
                break;
            default:
                return std::nullopt;
        }
        return std::nullopt;
    }

    void Assembler::classify_tokens(std::vector<Assembler::AssemblerToken> &tokens) {
        std::vector<TokenClass> tokenClass{};

        for (auto &token: tokens) {
            if (token.tokenClass == TokenClass::UNKNOWN) {
                if (token.literal == "=")
                    token.tokenClass = TokenClass::ASSIGNMENT;
                else if (token.literal == ".")
                    token.tokenClass = TokenClass::PC_TOKEN;
                else if (token.literal == "@")
                    token.tokenClass = TokenClass::WORD_ALLOCATION;
                else if (token.literal == "+")
                    token.tokenClass = TokenClass::ADD;
                else if (token.literal == "-")
                    token.tokenClass = TokenClass::SUB;
                else if (token.literal == ",")
                    token.tokenClass = TokenClass::LABEL_CREATE;
                else if (std::isdigit(token.literal.at(0)))
                    token.tokenClass = TokenClass::NUMBER;
                else if (instructionMap.find(token.literal) != instructionMap.end())
                    token.tokenClass = TokenClass::OP_CODE;
                else token.tokenClass = TokenClass::LITERAL;
            }
        }
    }

    sim::register_type
    Assembler::generate_code(word_t pc, TokenList::iterator first, TokenList::iterator last, TokenList::iterator label,
                             std::ostream &bin) {
        word_t code = 0u;
        bool memoryOpr = false;
        CombinationType restrict{CombinationType::Gr};

        for (auto itr = first; itr != last; ++itr) {
            switch (itr->tokenClass) {
                case TokenClass::NUMBER:
                    if (auto value = get_token_value(*itr); value)
                        code |= value.value();
                    else
                        throw AssemblerException("Invalid number " + itr->literal);
                    break;
                case TokenClass::OP_CODE:
                    if (auto op = instructionMap.find(itr->literal); op != instructionMap.end())
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
                                code &= op->second.opCode;
                                break;
                            case CombinationType::Gr1:
                                if (restrict == CombinationType::Gr || restrict == CombinationType::Gr1) {
                                    code |= op->second.opCode;
                                    restrict = CombinationType::Gr1;
                                } else
                                    throw AssemblerException("Invalid microcode combination:");
                                break;
                            case CombinationType::Gr2:
                                if (restrict == CombinationType::Gr || restrict == CombinationType::Gr2) {
                                    code |= op->second.opCode;
                                    restrict = CombinationType::Gr2;
                                } else
                                    throw AssemblerException("Invalid microcode combination:");
                                break;
                            case CombinationType::Gr3:
                                if (restrict == CombinationType::Gr || restrict == CombinationType::Gr3) {
                                    code |= op->second.opCode;
                                    restrict = CombinationType::Gr3;
                                } else
                                    throw AssemblerException("Invalid microcode combination:");
                                break;
                        }
                    break;
                case TokenClass::LITERAL:
                    if (auto symbol = symbolTable.find(itr->literal); symbol != symbolTable.end()) {
                        if (symbol->second.status == Defined) {
                            if (memoryOpr) {
                                code |= symbol->second.value & 0177;
                                if (symbol->second.value > 0177)
                                    code |= 0200;
                            }
                            code |= memoryOpr ? (symbol->second.value & 0177) : symbol->second.value;
                        } else
                            throw AssemblerException("Undefined symbol " + itr->literal);
                    } else
                        throw AssemblerException("Symbol not found " + itr->literal);
                    break;
                default:
                    break;
            }
        }

        bin << static_cast<char>((code & 07700) >> 6) << static_cast<char>(code & 077);
        return code;
    }

    void Assembler::dump_symbols(std::ostream &strm) {
        for (auto &symbol: symbolTable) {
            if (symbol.second.status == Defined)
                strm << fmt::format("{:04o}  {:<21}\n", symbol.second.value, symbol.second.symbol);
            else
                strm << fmt::format("Undef {:<21}\n", symbol.second.value, symbol.second.symbol);
        }
    }

    void
    Assembler::listing(std::ostream &list, const TokenList &tokens, sim::register_type pc, sim::register_type code) {
        if (tokens.begin()->tokenClass == TokenClass::COMMENT) {
            list << fmt::format("{:>14}{:<72}\n", "/", tokens.begin()->literal);
        } else {
            auto itr = tokens.begin();
            list << fmt::format("{:04o}  {:04o}   ", pc, code);
            while (itr != tokens.end()) {
                if (itr->tokenClass == TokenClass::LITERAL) {
                    list << fmt::format(" {:>16}, ", itr->literal);
                    ++itr;
                } else {
                    list << fmt::format(" {:>16}  ", "");
                }
                std::stringstream instruction;
                while (itr != tokens.end() && itr->tokenClass != TokenClass::COMMENT) {
                    switch (itr->tokenClass) {
                        case TokenClass::OP_CODE:
                        case TokenClass::NUMBER:
                        case TokenClass::LITERAL:
                        case TokenClass::PC_TOKEN:
                        case TokenClass::ASSIGNMENT:
                            instruction << fmt::format("{} ", itr->literal);
                            break;
                        default:
                            break;
                    }
                    ++itr;
                }
                list << fmt::format("{:<32}", instruction.str());
                if (itr != tokens.end() && itr->tokenClass == TokenClass::COMMENT) {
                    list << fmt::format("/ {}", itr->literal);
                    ++itr;
                }
                list << '\n';
            }
        }
    }
}
