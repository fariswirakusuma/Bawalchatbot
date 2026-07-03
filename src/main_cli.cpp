#include "token.hpp"
#include "lexer.hpp"
#include "parser.hpp" // Now includes Parser class declaration
#include "semantic.hpp" // Include SemanticAnalyzer
// #include "llama_engine.hpp" // Included for potential future integration or type definitions,
                            // but not directly used for llama_context/model lifecycle management here.
#include "context.hpp" // For ContextSession
#include <iostream>
#include <vector> // For std::vector
#include <string> // For std::string
#include <memory> // For std::unique_ptr

int main() {
    // Example input string. You can change this to test different commands.
    std::string input = "/generate --temp=0.5 --top_p=0.9 Teks prompt ini";
    // std::string input = "/set model my_model.gguf";
    // std::string input = "/save history chat_log.txt";
    // std::string input = "my_var = 'Hello World'"; // Example for variable declaration (if parser supports it)
    // std::string input = "/set prompt $my_var"; // Example for variable access (if parser supports it)

    Lexer lexer(input);
    Parser parser(lexer); // Instantiate Parser with the lexer
    SemanticAnalyzer semanticAnalyzer;
    ContextSession session; // Initialize a session for semantic analysis

    std::cout << "Parsing input: \"" << input << "\"" << std::endl;

    // Parse the input to build the AST
    std::unique_ptr<ProgramNode> program_ast = parser.parse();

    // Check for parsing errors
    if (parser.has_errors()) {
        std::cerr << "Parsing Errors:" << std::endl;
        for (const auto& error : parser.get_errors()) {
            std::cerr << "- " << error << std::endl;
        }
        return 1; // Indicate failure
    }

    if (!program_ast || program_ast->children.empty()) {
        std::cout << "No commands or statements parsed." << std::endl;
        return 0; // Or 1, depending on whether empty input is considered an error
    }

    std::cout << "\n--- Abstract Syntax Tree (AST) ---" << std::endl;
    program_ast->printAST_tree(std::cout, "", true);
    std::cout << "----------------------------------\n" << std::endl;

    bool all_commands_applied_successfully = true;
    for (const auto& child_node : program_ast->children) {
        // The semantic analyzer's analyze_and_apply expects a CommandNode.
        // If the parser can produce other top-level nodes (like VariableDeclarationNode),
        // we need to handle them appropriately or extend SemanticAnalyzer.
        if (auto command_node = dynamic_cast<CommandNode*>(child_node.get())) {
            std::cout << "Attempting semantic analysis for command: " << command_node->commandName << std::endl;
            if (!semanticAnalyzer.analyze_and_apply(command_node, session)) {
                all_commands_applied_successfully = false;
                std::cerr << "Semantic Errors for command '" << command_node->commandName << "':" << std::endl;
                for (const auto& error : semanticAnalyzer.get_errors()) {
                    std::cerr << "- " << error << std::endl;
                }
                // In a real application, you might want to stop processing after the first semantic error.
                // For this example, we continue to report all errors.
            } else {
                std::cout << "Command '" << command_node->commandName << "' applied successfully to session." << std::endl;
            }
        } else if (auto var_decl_node = dynamic_cast<VariableDeclarationNode*>(child_node.get())) {
            // Problem: SemanticAnalyzer currently lacks specific methods to process VariableDeclarationNode.
            // Perbaikan: Ini adalah tempat untuk memperluas SemanticAnalyzer atau ContextSession
            // untuk menyimpan variabel yang dideklarasikan. Untuk saat ini, hanya log.
            std::cout << "Info: Variable declaration found: '" << var_decl_node->variableName << "'. Semantic processing not fully implemented for variables." << std::endl;
            // Example: session.set_variable(var_decl_node->variableName, evaluate(var_decl_node->value_node));
        } else {
            std::cerr << "Error: Unexpected top-level AST node type encountered during semantic analysis." << std::endl;
            all_commands_applied_successfully = false;
        }
    }

    if (all_commands_applied_successfully) {
        std::cout << "\nAll commands and statements processed successfully!" << std::endl;
        // You can inspect the session state here if needed
        // std::cout << "Current model in session: " << session.get_model_name() << std::endl;
        // std::cout << "Current prompt in session: " << session.get_current_prompt() << std::endl;
    } else {
        std::cerr << "\nOne or more commands/statements failed semantic analysis." << std::endl;
        return 1; // Indicate failure
    }

    return 0; // Indicate success
}
