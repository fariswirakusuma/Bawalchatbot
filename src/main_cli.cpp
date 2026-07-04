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
                    } 
                    else if (command_node->commandName == "generate") {
                        generation_requested = true;
                    }
                    else if (command_node->commandName == "exit") {
                        std::cout << "Exiting CLI...\n";
                        return 0; 
                    }
                    else if (command_node->commandName == "help") {
                        std::cout << "Available Commands:\n"
                                  << "  /set_model --name <model_name> : Load a specific model.\n"
                                  << "  /set_prompt --text <prompt>    : Set the system prompt.\n"
                                  << "  /load_history --file <filename> : Load chat history from a file.\n"
                                  << "  /save_history --file <filename> : Save chat history to a file.\n"
                                  << "  /generate [text]                : Generate a response based on the current context and optional user input.\n"
                                  << "  /set_param --param <value>      : Set various parameters (e.g., temperature, top_k, etc.).\n"
                                  << "  /exit                           : Exit the CLI.\n";
                    }
                    else if (command_node->commandName == "save_history") {
                        std::cout << "Saving chat history to file...\n";
    
                        std::string filename = "";
                        if (!command_node->arguments.empty()) {
                            auto text_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get());
                            if (text_node) {
                                filename = text_node->text;
                            }
                        }
                        
                        filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end());
                        
                        session.historyFileName = filename;
                        engine.save_history(session.historyFileName, session);
                    }
                    else if (command_node->commandName == "load_history") {
                        std::cout << "Loading chat history from file...\n";
    
                        std::string filename = "";
                        
                        if (!command_node->arguments.empty()) {
                            auto text_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get());
                            if (text_node) {
                                filename = text_node->text;
                            }
                        }
                        
                        filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end());
                        
                        session.historyFileName = filename;
                        engine.load_history(session.historyFileName, session);
                    }
                    else {
                        std::cout << "Command '" << command_node->commandName << "' processed successfully.\n";
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