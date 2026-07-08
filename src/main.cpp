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

void process_commands(LlamaEngine& engine, SemanticAnalyzer& semanticAnalyzer, ContextSession& session, const std::string& input, bool is_ui_mode) {
    Lexer lexer(input); 
    Parser parser(lexer); 
    std::unique_ptr<ProgramNode> program_ast = parser.parse(); 
    if (parser.has_errors()) { 
        if (is_ui_mode) {
            nlohmann::json response;
            response["status"] = "error";
            response["message"] = "❌ Parsing error. Check command syntax.";
            std::cout << response.dump() << "\n" << std::flush;
        } else {
            std::cerr << "[ERROR] Parsing error. Check command syntax.\n";
        }
        return; 
    }

    if (!program_ast || program_ast->children.empty()) return;

    bool all_commands_applied_successfully = true; 
    bool model_load_requested = false; 
    bool generation_requested = false; 

    for (const auto& child_node : program_ast->children) { 
        if (auto command_node = dynamic_cast<CommandNode*>(child_node.get())) { 
            if (!semanticAnalyzer.analyze_and_apply(command_node, session)) { 
                all_commands_applied_successfully = false; 
                if (is_ui_mode) {
                    nlohmann::json response;
                    response["status"] = "error";
                    response["message"] = "❌ Semantic error in command: " + command_node->commandName;
                    std::cout << response.dump() << "\n" << std::flush;
                } else {
                    std::cerr << "Semantic Errors for command '" << command_node->commandName << "':\n";
                    for (const auto& error : semanticAnalyzer.get_errors()) { 
                        std::cerr << "- " << error << "\n";
                    }
                }
            } else {
                if (command_node->commandName == "set_model") {
                    model_load_requested = true;
                    for (const auto& paramNodePtr : command_node->parameters) {
                        const ParameterNode* pNode = paramNodePtr.get();
                        if (pNode->key == "name") {
                            session.modelName = pNode->value;
                            if (is_ui_mode) {
                                nlohmann::json response;
                                response["status"] = "success";
                                response["message"] = "✅ Model name set to: " + pNode->value + ". Use /generate to load.";
                                std::cout << response.dump() << "\n" << std::flush;
                            }
                            else std::cout << "[SYSTEM] Model name set to: \"" << pNode->value << "\". Use /generate to load the model.\n";
                        }
                    }
                }
                else if (command_node->commandName == "add_url_model") {
                    if (command_node->arguments.size() >= 2) {
                        auto name_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get());
                        auto url_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[1].get());

                        if (name_node && url_node) {
                            std::string model_name = name_node->text;
                            std::string model_url = url_node->text;

                            model_name.erase(std::remove(model_name.begin(), model_name.end(), '\"'), model_name.end());
                            model_url.erase(std::remove(model_url.begin(), model_url.end(), '\"'), model_url.end());

                            session.modelName = model_name;
                            engine.add_url_model(model_name, model_url);
                            
                            if (is_ui_mode) {
                                nlohmann::json response;
                                response["status"] = "success";
                                response["message"] = "✅ Model " + model_name + " has been downloaded and loaded.";
                                std::cout << response.dump() << "\n" << std::flush;
                            } else {
                                std::cout << "[SYSTEM] Model " << model_name << " has been downloaded and loaded.\n";
                            }
                        } else {
                            if (is_ui_mode) {
                                nlohmann::json response;
                                response["status"] = "error";
                                response["message"] = "❌ Arguments extraction failed.";
                                std::cout << response.dump() << "\n" << std::flush;
                            } else {
                                std::cerr << "[ERROR] Arguments extraction failed.\n";
                            }
                        }
                    } else {
                        if (is_ui_mode) {
                            nlohmann::json response;
                            response["status"] = "error";
                            response["message"] = "❌ Missing required arguments.";
                            std::cout << response.dump() << "\n" << std::flush;
                        } else {
                            std::cerr << "[ERROR] Missing required arguments.\n";
                        }
                    }
                }
                else if (command_node->commandName == "generate") {
                    generation_requested = true; 
                }
                else if (command_node->commandName == "exit") {
                    if (!is_ui_mode) std::cout << "Exiting CLI...\n";
                    session.shouldExit = true;
                    return; 
                }
                else if (command_node->commandName == "help") {
                    if (is_ui_mode) {
                        std::string help_msg = "Commands:\n/set_model --name <model>\n/set_prompt --text <prompt>\n/load_history --file <file>\n/save_history --file <file>\n/generate [text]\n/set_param --param <val>\n/show_params : Show current session parameters.\n/exit";
                        nlohmann::json response;
                        response["status"] = "info";
                        response["message"] = help_msg;
                        std::cout << response.dump() << "\n" << std::flush;
                    } else {
                        std::cout << "Available Commands:\n"
                                  << "  /help                              : Show this help message.\n"
                                  << "  /add_url_model --name <model_name> : Add a specific model from a URL.\n"
                                  << "  /set_model --name <model_name>     : Load a specific model.\n"
                                  << "  /set_prompt --text <prompt>        : Set the system prompt.\n"
                                  << "  /load_history --file <filename>    : Load chat history from a file.\n"
                                  << "  /save_history --file <filename>    : Save chat history to a file.\n"
                                  << "  /generate [text]                   : Generate a response based on context.\n"
                                  << "  /set_param --param <value>         : Set parameters (temp, top_k, top_p, max_token).\n"
                                  << "  /show_params                       : Show current session parameters.\n"
                                  << "  /exit                              : Exit the application.\n" << std::flush;
                    }
                }
                else if (command_node->commandName == "show_params") {
                    if (is_ui_mode) {
                        nlohmann::json response;
                        response["status"] = "info";
                        
                        nlohmann::json data;
                        data["modelName"] = session.modelName;
                        data["systemPrompt"] = session.systemPrompt;
                        data["currentTemperature"] = session.currentTemperature;
                        data["maxTokens"] = session.maxTokens;
                        data["topK"] = session.topK;
                        data["topP"] = session.topP;
                        data["repeatPenalty"] = session.repeatPenalty;
                        data["historyFileName"] = session.historyFileName;

                        response["data"] = data;
                        std::cout << response.dump() << "\n" << std::flush; 
                    } else {
                        std::cout << "[SYSTEM] Current Session Parameters:\n"
                                  << "  Model Name: " << session.modelName << "\n"
                                  << "  System Prompt: " << session.systemPrompt << "\n"
                                  << "  Temperature: " << session.currentTemperature << "\n"
                                  << "  Max Tokens: " << session.maxTokens << "\n"
                                  << "  Top K: " << session.topK << "\n"
                                  << "  Top P: " << session.topP << "\n"
                                  << "  Repeat Penalty: " << session.repeatPenalty << "\n"
                                  << "  History File Name: " << session.historyFileName
                                  << "\n" << std::flush;
                    }
                }
                else if (command_node->commandName == "set_prompt") {
                    for (const auto& paramNodePtr : command_node->parameters) {
                        const ParameterNode* pNode = paramNodePtr.get();
                        if (pNode->key == "text") {
                            session.systemPrompt = pNode->value;
                            if (is_ui_mode){
                                nlohmann::json response;
                                response["status"] = "success";
                                response["message"] = "✅ System prompt updated.";
                                std::cout << response.dump() << "\n" << std::flush;
                            }
                            else std::cout << "[SYSTEM] System prompt updated to: \"" << pNode->value << "\"\n";
                        }
                    }
                }
                else if (command_node->commandName == "set_param") {
                    for (const auto& paramNodePtr : command_node->parameters) {
                        const ParameterNode* pNode = paramNodePtr.get();
                        engine.change_parameters(pNode->key, pNode->value, session, is_ui_mode);
                    }
                }
                else if (command_node->commandName == "save_history") { 
                    std::string filename = ""; 
                    if (!command_node->arguments.empty()) { 
                        auto text_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get()); 
                        if (text_node) filename = text_node->text; 
                    }
                    filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end()); 
                    session.historyFileName = filename; 
                    
                    if (engine.save_history(session.historyFileName, session)) {
                        if (is_ui_mode) {
                            nlohmann::json response;
                            response["status"] = "success";
                            response["message"] = "✅ History saved: " + filename;
                            std::cout << response.dump() << "\n" << std::flush;
                        } else {
                            std::cout << "[SUCCESS] Saving chat history to file: " << filename << "\n";
                        }
                    } else {
                        if (is_ui_mode) {
                            nlohmann::json response;
                            response["status"] = "error";
                            response["message"] = "❌ Failed to save history: " + filename;
                            std::cout << response.dump() << "\n" << std::flush;
                        } else {
                            std::cerr << "[ERROR] Failed to save chat history to file: " << filename << "\n";
                        }
                    }
                }
                else if (command_node->commandName == "load_history") { 
                    std::string filename = ""; 
                    if (!command_node->arguments.empty()) { 
                        auto text_node = dynamic_cast<LiteralTextNode*>(command_node->arguments[0].get()); 
                        if (text_node) filename = text_node->text; 
                    }
                    filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end()); 
                    session.historyFileName = filename; 
                    
                    if (engine.load_history(session.historyFileName, session)) {
                        if (is_ui_mode) {
                            nlohmann::json response;
                            response["status"] = "success";
                            response["message"] = "✅ History loaded: " + filename;
                            std::cout << response.dump() << "\n" << std::flush;
                        } else {
                            std::cout << "[SUCCESS] Loading chat history from file: " << filename << "\n";
                            std::cout << "--------------------------------------------------------\n";
                            for (const auto& msg : session.chatHistory) {
                                if (msg.role == "user") {
                                    std::cout << "\n> " << msg.content << "\n";
                                } else if (msg.role == "assistant") {
                                    std::cout << "assistant:\n" << msg.content << "\n";
                                }
                            }
                            std::cout << "--------------------------------------------------------\n";
                        }
                    } else {
                        if (is_ui_mode) {
                            nlohmann::json response;
                            response["status"] = "error";
                            response["message"] = "❌ Failed to load history: " + filename;
                            std::cout << response.dump() << "\n" << std::flush;
                        } else {
                            std::cerr << "[ERROR] Failed to load chat history from file: " << filename << "\n";
                        }
                    }
                }
            }
        }
    }

    if (all_commands_applied_successfully) { 
        if (model_load_requested) engine.load_model(session.modelName); 
        if (generation_requested) engine.execute_generation(session, is_ui_mode); 
    }
}

void run_ui_mode(LlamaEngine& engine, SemanticAnalyzer& semanticAnalyzer, ContextSession& session) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    std::cout << std::unitbuf; 

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input.empty()) continue; 
        process_commands(engine, semanticAnalyzer, session, input, true);
        if (session.shouldExit) break;
    }
}
void run_cli_mode(LlamaEngine& engine, SemanticAnalyzer& semanticAnalyzer, ContextSession& session) {
    std::cout << "=== BawalChatbot CLI Interactive Mode ===\n";
    std::cout << "Type /help to see available commands or /exit to quit.\n";
    
    std::string input;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        process_commands(engine, semanticAnalyzer, session, input, false);
        if (session.shouldExit) break;
    }
}

int main(int argc, char* argv[]) {
    LlamaEngine engine;
    engine.initialize_backend(); 

    SemanticAnalyzer semanticAnalyzer; 
    ContextSession session; 

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