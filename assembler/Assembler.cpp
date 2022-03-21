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
                /**
                 * Process pseudo commands.
                 */
                for (auto &token: tokens)
                    switch (token.tokenClass) {
                        case TokenClass::OCTAL:
                            setNumberRadix(Radix::OCTAL);
                            break;
                        case TokenClass::DECIMAL:
                            setNumberRadix(Radix::DECIMAL);
                            break;
                        case TokenClass::AUTOMATIC:
                            setNumberRadix(Radix::AUTOMATIC);
                            break;
                        default:
                            break;
                    }
                auto first = tokens.begin();
                auto last = tokens.end();
                switch (first->tokenClass) {
                    case TokenClass::LOCATION:
                        pc = generate_code(first + 1, last, pc);
                        break;
                    case TokenClass::LITERAL:
                        if (tokens.size() >= 2) {
                            if (tokens[1].tokenClass == TokenClass::LABEL_CREATE) {
                                if (auto symbol = symbolTable.find(tokens[0].literal); symbol != symbolTable.end()) {
                                    if (symbol->second.status == Undefined) {
                                        symbol->second.status = Defined;
                                        symbol->second.value = pc;
                                    } else {
                                        symbol->second.status = ReDefined;
                                        symbol->second.value = pc;
                                    }
                                } else {
                                    symbolTable.emplace(tokens[0].literal, Symbol(pc, tokens[0].literal, Defined));
                                }
                                ++pc;
                            } else if (tokens[1].tokenClass == TokenClass::ASSIGNMENT) {
                                if (auto symbol = symbolTable.find(tokens[0].literal); symbol != symbolTable.end()) {
                                    if (symbol->second.status != Undefined) {
                                        symbol->second.status = ReDefined;
                                        symbol->second.value = pc;
                                    }
                                } else {
                                    symbolTable.emplace(tokens[0].literal, Symbol(pc, tokens[0].literal, Undefined));
                                }
                            }
                        }
                        break;
                    case TokenClass::OP_CODE:
                        ++pc;
                        break;
                    case TokenClass::COMMENT:
                    case TokenClass::OCTAL:
                    case TokenClass::DECIMAL:
                    case TokenClass::AUTOMATIC:
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
                strm << fmt::format(" at {:04o}", pc);
                throw AssemblerException(strm.str());
            }
        }
    }

    void Assembler::pass2(std::istream &src, std::ostream &bin, std::ostream &list) {
        sim::register_type pc = 0u, code = 0u;
        for (auto tokens = parse_tokens(src); !tokens.empty(); tokens = parse_tokens(src)) {
            try {
                classify_tokens(tokens);
                /**
                 * Process pseudo commands.
                 */
                for (auto &token: tokens)
                    switch (token.tokenClass) {
                        case TokenClass::OCTAL:
                            setNumberRadix(Radix::OCTAL);
                            break;
                        case TokenClass::DECIMAL:
                            setNumberRadix(Radix::DECIMAL);
                            break;
                        case TokenClass::AUTOMATIC:
                            setNumberRadix(Radix::AUTOMATIC);
                            break;
                        default:
                            break;
                    }
                auto first = tokens.begin();
                auto last = tokens.end();
                switch ((first++)->tokenClass) {
                    case TokenClass::LOCATION: {
                        pc = generate_code(first, last, pc);
                        bin << static_cast<char>(((pc & 07700) >> 6) | 0100) << static_cast<char>(pc & 077);
                        listing(list, tokens, pc, code);
                    }
                        break;
                    case TokenClass::LITERAL:
                        if (tokens.size() >= 2) {
                            if (tokens[1].tokenClass == TokenClass::LABEL_CREATE) {
                                if (auto symbol = symbolTable.find(tokens[0].literal); symbol != symbolTable.end()) {
                                    if (symbol->second.status == Undefined) {
                                        symbol->second.status = Defined;
                                        symbol->second.value = pc;
                                    }
                                } else {
                                    symbolTable.emplace(tokens[0].literal, Symbol(pc, tokens[0].literal, Defined));
                                }

                                if (tokens.size() >= 3 && first->tokenClass == TokenClass::LABEL_CREATE) {
                                    code = generate_code(++first, tokens.end(), pc);
                                    bin << static_cast<char>((code & 07700) >> 6) << static_cast<char>(code & 077);
                                    listing(list, tokens, pc, code);
                                    ++pc;
                                } else {
                                    listing(list, tokens, pc, code);
                                }
                            } else if (tokens[1].tokenClass == TokenClass::ASSIGNMENT) {
                                ++first;
                                if (auto symbol = symbolTable.find(tokens[0].literal); symbol != symbolTable.end()) {
                                    if (symbol->second.status != ReDefined) {
                                        symbol->second.status = Defined;
                                        symbol->second.value = generate_code(first, last, pc);
                                    }
                                } else {
                                    auto value = generate_code(first, last, pc);
                                    symbolTable.emplace(tokens[0].literal, Symbol(value, tokens[0].literal, Defined));
                                }
                                listing(list, tokens, pc, code);
                            }
                        }
                        break;
                    case TokenClass::OP_CODE:
                        code = generate_code(tokens.begin(), tokens.end(), pc);
                        bin << static_cast<char>((code & 07700) >> 6) << static_cast<char>(code & 077);
                        listing(list, tokens, pc, code);
                        ++pc;
                        break;
                    case TokenClass::COMMENT:
                        listing(list, tokens, pc, code);
                        break;
                    case TokenClass::OCTAL:
                    case TokenClass::DECIMAL:
                    case TokenClass::AUTOMATIC:
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

                while ((c = src.get()) != -1) {
                    if (c == '\n')
                        break;
                    buffer.push_back(static_cast<char>(c));
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
                } else if (c == '=' || c == '.' || c == '+' || c == '-' || c == ',' || c == ':' || c == '*') {
                    if (!buffer.empty()) {
                        tok.emplace_back(TokenClass::UNKNOWN, buffer);
                        buffer.clear();
                    }
                    buffer.push_back(static_cast<char>(c));
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
                        case ',':
                            tok.emplace_back(TokenClass::LABEL_CREATE, buffer);
                            break;
                        case '*':
                            tok.emplace_back(TokenClass::LOCATION, buffer);
                            break;
                        default:
                            break;
                    }
                    buffer.clear();
                } else if (!buffer.empty()) {
                    if (std::isalpha(buffer.at(0))) {
                        if (std::isalnum(c)) {
                            buffer.push_back(static_cast<char>(c));
                        } else {
                            tok.emplace_back(TokenClass::UNKNOWN, buffer);
                            buffer.clear();
                            buffer.push_back(static_cast<char>(c));
                        }
                    } else if (std::isdigit(buffer.at(0))) {
                        if (std::isdigit(c) || std::isxdigit(c) || (buffer.length() == 1 && std::toupper(c) == 'X')) {
                            buffer.push_back(static_cast<char>(c));
                        } else {
                            tok.emplace_back(TokenClass::UNKNOWN, buffer);
                            buffer.clear();
                            buffer.push_back(static_cast<char>(c));
                        }
                    }
                } else {
                    buffer.push_back(static_cast<char>(c));
                }
            }
        }

        return tok;
    }

    std::optional<sim::register_type> Assembler::get_token_value(const AssemblerToken &token, sim::register_type pc) {
        switch (token.tokenClass) {
            case TokenClass::PC_TOKEN:
                return pc;
            case TokenClass::NUMBER:
                try {
                    switch (numberRadix) {
                        case Radix::OCTAL:
                            return stoul(token.literal, nullptr, 8);
                        case Radix::DECIMAL:
                            return stoul(token.literal, nullptr, 10);
                        case Radix::AUTOMATIC:
                        default:
                            return stoul(token.literal, nullptr, 0);
                    }
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
                if (token.literal == "OCTAL")
                    token.tokenClass = TokenClass::OCTAL;
                else if (token.literal == "DECIMAL")
                    token.tokenClass = TokenClass::DECIMAL;
                else if (token.literal == "AUTOMATIC")
                    token.tokenClass = TokenClass::AUTOMATIC;
                else if (token.literal == "=")
                    token.tokenClass = TokenClass::ASSIGNMENT;
                else if (token.literal == ".")
                    token.tokenClass = TokenClass::PC_TOKEN;
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
                else {
                    if (symbolTable.find(token.literal) != symbolTable.end()) {
                        token.tokenClass = TokenClass::LITERAL;
                    } else {
                        std::string uppercase;
                        std::transform(token.literal.begin(), token.literal.end(),
                                       std::back_insert_iterator<std::string>(uppercase), ::toupper);
                        if (instructionMap.find(uppercase) != instructionMap.end()) {
                            token.literal = uppercase;
                            token.tokenClass = TokenClass::OP_CODE;
                        } else {
                            token.tokenClass = TokenClass::LITERAL;
                        }
                    }
                }
            }
        }
    }

    sim::register_type
    Assembler::generate_code(TokenList::iterator first, TokenList::iterator last, sim::register_type pc) {
        word_t code = 0u;
        word_t arg = 0u;
        bool opCode = false;
        bool memoryOpr = false;
        bool finished = false;
        CombinationType restrict{CombinationType::Gr};

        for (auto itr = first; itr != last && !finished; ++itr) {
            switch (itr->tokenClass) {
                case TokenClass::NUMBER:
                case TokenClass::LITERAL:
                case TokenClass::PC_TOKEN:
                    std::tie(itr, arg) = evaluate_expression(itr, last, pc);
                    finished = itr == last;
                    break;
                case TokenClass::OP_CODE:
                    if (auto op = instructionMap.find(itr->literal); op != instructionMap.end()) {
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
                    }
                    break;
                default:
                    break;
            }
        }

        if (opCode) {
            if (memoryOpr)
                code |= arg & 0377;
        } else {
            code = arg;
        }
        return code;
    }

    void Assembler::dump_symbols(std::ostream &strm) {
        strm << fmt::format("\n{:^22}\n", "Symbol Table");
        for (auto &symbol: symbolTable) {
            if (symbol.second.status == Defined)
                strm << fmt::format("{:04o}  {:<21}\n", symbol.second.value, symbol.second.symbol);
            else
                strm << fmt::format("Undef {:<21}\n", symbol.second.value, symbol.second.symbol);
        }
    }

    void
    Assembler::listing(std::ostream &list, const TokenList &tokens, sim::register_type pc, sim::register_type code) {

        /**
         * Listing for set location commands:
         *              *0200
         *    Start,    *0200
         */
        auto listLocation =
                [&list, &tokens, pc]() {
                    list << fmt::format("{:04o}{:9}", pc, "");
                    auto itr = tokens.begin();
                    if (itr->tokenClass == TokenClass::LITERAL) {
                        if ((itr + 1)->tokenClass == TokenClass::LABEL_CREATE)
                            list << fmt::format("{:>16}{} ", (itr)->literal, (itr + 1)->literal);
                        ++itr;
                    } else {
                        list << fmt::format("{:>18}{}", "", itr->literal);
                    }
                    return itr;
                };

        /**
         * Listing for label assignment:
         *   Label = .
         *   Label = 0222;
         */
        auto listAssignment = [&list, &tokens]() {
            list << fmt::format("{:13}", "");
            auto itr = tokens.begin();
            if (itr->tokenClass == TokenClass::LITERAL) {
                if ((itr + 1)->tokenClass == TokenClass::ASSIGNMENT)
                    list << fmt::format("{:>16}{} ", (itr)->literal, (itr + 1)->literal);
                ++itr;
            } else {
                list << fmt::format("{:>18}{}", "", itr->literal);
            }
            return itr;
        };

        /**
         * Listing for label create:
         *   Start, CLA CLL
         *   Value, 0-010
         */
        auto listLabelCreate = [&list, &tokens, pc, code]() {
            list << fmt::format("{:04o}  {:04o}   ", pc, code);
            auto itr = tokens.begin();
            if (itr->tokenClass == TokenClass::LITERAL) {
                if ((itr + 1)->tokenClass == TokenClass::LABEL_CREATE)
                    list << fmt::format("{:>16}{} ", (itr)->literal, (itr + 1)->literal);
                ++itr;
            } else {
                list << fmt::format("{:>18}{}", "", itr->literal);
            }
            return itr;
        };

        /**
         * Step through the token list looking for the defining token, then process it.
         */
        auto itr = tokens.begin();
        bool finished = false;
        std::stringstream strm;
        for (; itr != tokens.end() && !finished; ++itr) {
            finished = true;
            switch (itr->tokenClass) {
                case TokenClass::LOCATION:
                    itr = listLocation();
                    break;
                case TokenClass::ASSIGNMENT:
                    itr = listAssignment();
                    break;
                case TokenClass::LABEL_CREATE:
                    itr = listLabelCreate();
                    break;
                case TokenClass::OP_CODE:
                    list << fmt::format("{:04o}  {:04o}   {:18}{} ", pc, code, "", itr->literal);
                    break;
                case TokenClass::COMMENT:
                    list << fmt::format("/{}", itr->literal);
                    break;
                default:
                    finished = false;
                    break;
            }
        }

        /**
         * List any remaining tokens in the operation section of the instruction.
         */
        while (itr != tokens.end() && itr->tokenClass != TokenClass::COMMENT) {
            strm << fmt::format("{} ", itr->literal);
            ++itr;
        }

        /**
         * Complete the listing by printing a comment, if one exists, and the end of line.
         */
        list << fmt::format("{:<32}", strm.str());
        if (itr != tokens.end() && itr->tokenClass == TokenClass::COMMENT)
            list << fmt::format("/{}", itr->literal);
        list << '\n';
    }

    std::optional<Assembler::word_t> Assembler::find_symbol(const std::string &symbol) {
        if (auto entry = symbolTable.find(symbol); entry != symbolTable.end()) {
            return entry->second.value;
        }
        return std::nullopt;
    }

    std::optional<Assembler::word_t> Assembler::parse_command(const std::string &command, sim::register_type pc) {
        std::stringstream strm(command);
        auto tokens = parse_tokens(strm);
        classify_tokens(tokens);
        return generate_code(tokens.begin(), tokens.end(), pc);
    }

    std::tuple<std::vector<Assembler::AssemblerToken>::iterator, sim::register_type>
    Assembler::evaluate_expression(TokenList::iterator first, TokenList::iterator last, sim::register_type pc) {
        std::optional<int> left;
        std::optional<int> right;

        TokenClass op = TokenClass::UNKNOWN;
        auto itr = first;
        for (; itr != last; ++itr) {
            switch (itr->tokenClass) {
                case TokenClass::LITERAL:
                case TokenClass::NUMBER:
                case TokenClass::PC_TOKEN:
                    if (auto value = get_token_value(*itr, pc); value) {
                        if (left)
                            right = value;
                        else
                            left = value;
                    }

                    if (left && right && op != TokenClass::UNKNOWN) {
                        if (op == TokenClass::ADD)
                            left = left.value() + right.value();
                        else
                            left = left.value() - right.value();
                        op = TokenClass::UNKNOWN;
                        right = std::nullopt;
                    }
                    break;
                case TokenClass::ADD:
                case TokenClass::SUB:
                    op = itr->tokenClass;
                    break;
                case TokenClass::COMMENT:
                    break;
                default:
                    throw AssemblerException(fmt::format("Token not allowed in expression: {}", itr->literal));
            }
        }
        if (left && right)
            throw AssemblerException(fmt::format("Incomplete expression."));

        if (left.value() < 0)
            return {itr, ((010000 + left.value()) & 07777)};
        else
            return {itr, (left.value() & 07777)};
    }
}
