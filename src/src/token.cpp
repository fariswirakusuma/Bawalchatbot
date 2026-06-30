#include "token.hpp"


std::string getTokenName(TokenType type) {
    switch (type) {
        case TokenType::COMMAND:     return "COMMAND";
        case TokenType::PARAMETER:   return "PARAMETER";
        case TokenType::TEXT:        return "TEXT";
        case TokenType::END_OF_FILE: return "EOF";
        default:                     return "UNKNOWN";
    }
}
