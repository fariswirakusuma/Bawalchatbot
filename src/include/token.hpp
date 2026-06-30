#pragma once
#include <string>

enum class TokenType {
    COMMAND,
    PARAMETER,
    TEXT,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
};

std::string getTokenName(TokenType type);