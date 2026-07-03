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

int main() {
    LlamaEngine engine;
    engine.initialize_backend(); 

    SemanticAnalyzer semanticAnalyzer;
    ContextSession session; 

    std::cout << "CLI Started. Type /exit to quit.\n";

    while (true) {
        std::cout << "\n> ";
        std::string input;
        
        if (!std::getline(std::cin, input) || input.empty()) {
            continue; 
        }

        Lexer lexer(input);
        Parser parser(lexer); 
        
        std::unique_ptr<ProgramNode> program_ast = parser.parse();

        if (parser.has_errors()) {
            std::cerr << "Parsing Errors:\n";
            for (const auto& error : parser.get_errors()) {
                std::cerr << "- " << error << "\n";
            }
            continue; 
        }

        if (!program_ast || program_ast->children.empty()) {
            continue;
        }

        bool all_commands_applied_successfully = true;
        bool model_load_requested = false;
        bool generation_requested = false;

        for (const auto& child_node : program_ast->children) {
            if (auto command_node = dynamic_cast<CommandNode*>(child_node.get())) {
                if (!semanticAnalyzer.analyze_and_apply(command_node, session)) {
                    all_commands_applied_successfully = false;
                    std::cerr << "Semantic Errors for command '" << command_node->commandName << "':\n";
                    for (const auto& error : semanticAnalyzer.get_errors()) {
                        std::cerr << "- " << error << "\n";
                    }
                } else {
                    if (command_node->commandName == "set_model") {
                        model_load_requested = true;
                    } else if (command_node->commandName == "generate") {
                        generation_requested = true;
                    }
                }
            } else if (auto var_decl_node = dynamic_cast<VariableDeclarationNode*>(child_node.get())) {
                std::cout << "Info: Variable declaration processing not fully implemented.\n";
            }
        }

        if (all_commands_applied_successfully) {
            if (session.shouldExit) {
                break;
            }

            if (model_load_requested) {
                engine.load_model(session.modelName);
            }

            if (generation_requested) {
                engine.execute_generation(session);
            }
        }
    }
    
    return 0; 
}