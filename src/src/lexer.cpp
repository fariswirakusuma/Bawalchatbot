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

    char c = source[pos];
    std::size_t start = pos;
    if (c == '/') {
        if (pos + 1 >= source.length() || std::isspace(source[pos + 1])) {
            pos++;
            return Token{TokenType::SLASH, "/"};
        }
        while (pos < source.length() && !std::isspace(source[pos])) {
            pos++;
        }
        return Token{TokenType::COMMAND, std::string(source.substr(start, pos - start))};
    }
    if (c == '-') {
        if (pos + 1 < source.length() && (std::isalpha(source[pos + 1]) || source[pos + 1] == '-')) {
            while (pos < source.length() && !std::isspace(source[pos])) {
                pos++;
            }
            return Token{TokenType::PARAMETER, std::string(source.substr(start, pos - start))};
        }
    }

    switch (c) {
        case '+': pos++; return Token{TokenType::PLUS, "+"};
        case '-': pos++; return Token{TokenType::MINUS, "-"}; 
        case '*': pos++; return Token{TokenType::STAR, "*"};
        case ',': pos++; return Token{TokenType::COMMA, ","};
        case '(': pos++; return Token{TokenType::LEFT_PAREN, "("};
        case ')': pos++; return Token{TokenType::RIGHT_PAREN, ")"};
        case '{': pos++; return Token{TokenType::LEFT_BRACE, "{"};
        case '}': pos++; return Token{TokenType::RIGHT_BRACE, "}"};
        
        case '=':
            pos++;
            if (pos < source.length() && source[pos] == '=') {
                pos++;
                return Token{TokenType::EQUAL_EQUAL, "=="};
            }
            return Token{TokenType::EQUAL, "="};
            
        case '!':
            pos++;
            if (pos < source.length() && source[pos] == '=') {
                pos++;
                return Token{TokenType::BANG_EQUAL, "!="};
            }
            return Token{TokenType::BANG, "!"};
            
        case '<': pos++; return Token{TokenType::LESS, "<"};
        case '>': pos++; return Token{TokenType::GREATER, ">"};
    }

    if (std::isdigit(c)) {
        bool is_float = false;
        while (pos < source.length() && (std::isdigit(source[pos]) || source[pos] == '.')) {
            if (source[pos] == '.') {
                if (is_float) break; 
                is_float = true;
            }
            pos++;
        }
        std::string num_str = std::string(source.substr(start, pos - start));
        if (is_float) {
            return Token{TokenType::FLOAT, num_str};
        }
        return Token{TokenType::INT, num_str};
    }

    if (std::isalnum(c) || c == '_') {
        while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
            pos++;
        }
        std::string text_str = std::string(source.substr(start, pos - start));

        if (text_str == "if")     return Token{TokenType::IF, text_str};
        if (text_str == "else")   return Token{TokenType::ELSE, text_str};
        if (text_str == "true" || text_str == "false") return Token{TokenType::BOOL, text_str};

        return Token{TokenType::TEXT, text_str};
    }

    pos++;
    return Token{TokenType::UNKNOWN, std::string(1, c)};
}

void Lexer::print_all_tokens() {
    Token tok = getNextToken();
    while (tok.type != TokenType::END_OF_FILE) {
        std::cout << "Token: [" << getTokenName(tok.type) 
                  << "] -> Value: " << tok.value << std::endl;
        tok = getNextToken();
    }
}

