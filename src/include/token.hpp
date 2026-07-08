#pragma once
#include <string>

enum class TokenType {
    COMMAND,
    PARAMETER,
    FLOAT,
    INT,
    BOOL,
    TEXT, 
    END_OF_FILE,
    UNKNOWN,

    PLUS,         // '+'
    MINUS,        // '-'
    STAR,         // '*'
    SLASH,        // '/'
    EQUAL,        // '='
    BANG,         // '!' 

    EQUAL_EQUAL,  // '=='
    BANG_EQUAL,   // '!='
    LESS,         // '<'
    GREATER,      // '>'

    LEFT_PAREN,   // '(' (buat if)
    RIGHT_PAREN,  // ')'
    LEFT_BRACE,   // '{' (awal BlockStatement)
    RIGHT_BRACE,  // '}' (akhir BlockStatement)
    COMMA,        // ','

    IF,           // "if"
    ELSE          // "else"
};

struct Token {
    TokenType type;
    std::string value;
};

std::string getTokenName(TokenType type);