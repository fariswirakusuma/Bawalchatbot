#include "token.hpp"


std::string getTokenName(TokenType type) {
    switch (type) {
        case TokenType::COMMAND:     return "COMMAND";
        case TokenType::PARAMETER:   return "PARAMETER";
        case TokenType::TEXT:        return "TEXT";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::BANG:         return "BANG";
        case TokenType::EQUAL:        return "EQUAL";
        case TokenType::PLUS:         return "PLUS";
        case TokenType::MINUS:        return "MINUS";
        case TokenType::STAR:         return "STAR";
        case TokenType::SLASH:        return "SLASH";
        case TokenType::EQUAL_EQUAL:  return "EQUAL_EQUAL";
        case TokenType::BANG_EQUAL:   return "BANG_EQUAL";
        case TokenType::LESS:         return "LESS";
        case TokenType::GREATER:      return "GREATER";
        case TokenType::LEFT_PAREN:   return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN:  return "RIGHT_PAREN";
        case TokenType::LEFT_BRACE:   return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE:  return "RIGHT_BRACE";
        case TokenType::COMMA:        return "COMMA";
        case TokenType::IF:           return "IF";
        case TokenType::ELSE:         return "ELSE";

        default:                     return "UNKNOWN";
    }
}
