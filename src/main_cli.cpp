#include "token.hpp"
#include "lexer.hpp"
#include "parser.hpp"
// #include "semantic_analyzer.hpp"
#include "llama_engine.hpp"
#include "chat_structures.hpp"
#include <iostream>

int main() {
    std::string input = "/generate --temp=0.5 Teks prompt ini";
    Lexer lexer(input);

    Token tok = lexer.getNextToken();
    while (tok.type != TokenType::END_OF_FILE) {
        // Menggunakan getTokenName untuk logging visual
        std::cout << "Token: [" << getTokenName(tok.type) << "] -> Value: " << tok.value << std::endl;
        tok = lexer.getNextToken();
    }

    return 0;
}