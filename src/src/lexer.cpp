#include "lexer.hpp"
#include <iostream>

Lexer::Lexer(const std::string& src) : source(src), pos(0), peekBuffer(std::nullopt) {
    keywordMap["/generate"] = TokenType::COMMAND;
    keywordMap["/system"]   = TokenType::COMMAND;
}
Token Lexer::peekToken() {
    if (!peekBuffer) {
        peekBuffer = scanToken();
    }
    return *peekBuffer;
}

Token Lexer::getNextToken() {
    if (peekBuffer) {
        Token token = *peekBuffer;
        peekBuffer.reset();
        return token;
    }
    return scanToken();
}

Token Lexer::scanToken() {
    while (pos < source.length() && std::isspace(source[pos])) {
        pos++;
    }

    if (pos >= source.length()) {
        return Token{TokenType::END_OF_FILE, ""};
    }

    std::size_t start = pos;

    if (source[pos] == '/') {
        while (pos < source.length() && !std::isspace(source[pos])) {
            pos++;
        }
        return Token{TokenType::COMMAND, std::string(source.substr(start, pos - start))};
    }
    if (pos + 1 < source.length() && source[pos] == '-' && source[pos + 1] == '-') {
        while (pos < source.length() && !std::isspace(source[pos])) {
            pos++;
        }
        return Token{TokenType::PARAMETER, std::string(source.substr(start, pos - start))};
    }
    while (pos < source.length() && !std::isspace(source[pos])) {
        pos++;
    }
    return Token{TokenType::TEXT, std::string(source.substr(start, pos - start))};
}

void Lexer::print_all_tokens() {
    Token tok = getNextToken();
    while (tok.type != TokenType::END_OF_FILE) {
        std::cout << "Token: [" << getTokenName(tok.type) 
                  << "] -> Value: " << tok.value << std::endl;
        tok = getNextToken();
    }
}

