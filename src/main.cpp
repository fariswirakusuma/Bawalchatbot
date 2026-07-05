#include "token.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "llama_engine.hpp" 
#include "context.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

// Jalur khusus untuk komunikasi pipa data dengan Electron (main_ui)
void run_ui_mode(LlamaEngine& engine, SemanticAnalyzer& semanticAnalyzer, ContextSession& session) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    std::cout << std::unitbuf; 

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input.empty()) continue; 

        Lexer lexer(input); 
        Parser parser(lexer); 
        std::unique_ptr<ProgramNode> program_ast = parser.parse(); 

        if (parser.has_errors()) { 
            for (const auto& error : parser.get_errors()) { 
                std::cerr << "PARSING_ERROR: " << error << "\n";
            }
            continue; 
        }

        if (!program_ast || program_ast->children.empty()) continue;

        bool all_commands_applied_successfully = true; 
        bool model_load_requested = false; 
        bool generation_requested = false; 

        for (const auto& child_node : program_ast->children) { 
            if (auto command_node = dynamic_cast<CommandNode*>(child_node.get())) { 
                if (!semanticAnalyzer.analyze_and_apply(command_node, session)) { 
                    all_commands_applied_successfully = false; 
                    for (const auto& error : semanticAnalyzer.get_errors()) { 
                        std::cerr << "SEMANTIC_ERROR: " << error << "\n";
                    }
                } else {
                    if (command_node->commandName == "set_model") model_load_requested = true; 
                    else if (command_node->commandName == "generate") generation_requested = true; 
                    else if (command_node->commandName == "exit") return; 
                    else if (command_node->commandName == "save_history") { 
                        std::string filename = ""; 
                        if (!command_node->arguments.empty()) { 
                            auto text_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get()); 
                            if (text_node) filename = text_node->text; 
                        }
                        filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end()); 
                        session.historyFileName = filename; 
                        engine.save_history(session.historyFileName, session); 
                    }
                    else if (command_node->commandName == "load_history") { 
                        std::string filename = ""; 
                        if (!command_node->arguments.empty()) { 
                            auto text_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get()); 
                            if (text_node) filename = text_node->text; 
                        }
                        filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end()); 
                        session.historyFileName = filename; 
                        engine.load_history(session.historyFileName, session); 
                    }
                }
            }
        }

        if (all_commands_applied_successfully) { 
            if (session.shouldExit) break;
            if (model_load_requested) engine.load_model(session.modelName); 
            if (generation_requested) engine.execute_generation(session); 
        }
    }
}

// Jalur khusus untuk interaksi teks langsung di terminal (main_cli)
void run_cli_mode(LlamaEngine& engine, SemanticAnalyzer& semanticAnalyzer, ContextSession& session) {
    std::cout << "=== BawalChatbot CLI Interactive Mode ===\n";
    std::string input;
    
    // Tempatkan / salin isi logika perulangan dari berkas main_cli.cpp lama Anda ke sini
    while (true) {
        std::cout << "\nInput Command > ";
        if (!std::getline(std::cin, input)) break;
        if (input == "/exit") break;

        // Implementasi parsing dan inferensi interaktif untuk terminal Anda
        std::cout << "CLI Processing: " << input << "\n";
    }
}

int main(int argc, char* argv[]) {
    LlamaEngine engine;
    engine.initialize_backend(); 

    SemanticAnalyzer semanticAnalyzer; 
    ContextSession session; 

    // Deteksi argumen --cli dari input sistem
    bool is_cli_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--cli") {
            is_cli_mode = true;
            break;
        }
    }

    if (is_cli_mode) {
        run_cli_mode(engine, semanticAnalyzer, session);
    } else {
        run_ui_mode(engine, semanticAnalyzer, session);
    }

    return 0; 
}