#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include "chat_structures.hpp"
#include "token.hpp"

using namespace std;

class Lexer {
private:
    string source;
    size_t pos;
    unordered_map<string, TokenType> keywordMap;
    optional<Token> peekBuffer;

    Token scanToken();
public:
    Lexer(const string& src);
    Token peekToken();
    Token getNextToken();
    void print_all_tokens();
};