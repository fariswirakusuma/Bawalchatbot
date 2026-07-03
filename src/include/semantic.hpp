#pragma once

#include "context.hpp" // For ContextSession, ChatMessage, etc.
#include "parser.hpp"          // For CommandNode, CommandType, and other AST structures
// #include "llama_engine.hpp"    // Included for potential future integration or type definitions,
                               // but not directly used for llama_context/model lifecycle management here.

#include <memory>      // For std::unique_ptr (if internal state requires dynamic ownership)
#include <string>
#include <vector>
#include <map>
#include <optional>    // For robust argument parsing and return types
#include <stdexcept>   // For potential parsing errors in helpers

// Original file had 'class Parser' defined here, which is a duplicate and incorrect.
// This file should define the SemanticAnalyzer class.
class SemanticAnalyzer {
public:
    SemanticAnalyzer() = default;

    // Analyzes the Abstract Syntax Tree (AST) starting from `ast_root`.
    // It performs semantic validation on each command node. If a command is
    // semantically valid, its effects are applied to the provided `ContextSession`.
    //
    // Returns true if all commands in the AST were successfully analyzed and applied
    // without any semantic errors. Returns false if one or more semantic errors
    // were detected. Detailed errors can be retrieved using `get_errors()`.
    bool analyze_and_apply(const CommandNode* ast_root, ContextSession& session);

    // Retrieves a list of accumulated semantic error messages from the last
    // `analyze_and_apply` call. The vector will be empty if no errors occurred.
    const std::vector<std::string>& get_errors() const;

private:
    std::vector<std::string> m_errors; // Stores semantic error messages.

    // Helper method to add an error message to the internal error list.
    void add_error(const std::string& message);

    // --- Command-specific validation and application methods ---
    // These methods are responsible for validating the structure and arguments
    // of a specific command type and applying its effects to the session if valid.
    // They return true on successful processing and application, false if a semantic
    // error was found for that specific command.

    bool process_set_model_command(const CommandNode& node, ContextSession& session);
    bool process_set_prompt_command(const CommandNode& node, ContextSession& session);
    bool process_load_history_command(const CommandNode& node, ContextSession& session);
    bool process_save_history_command(const CommandNode& node, ContextSession& session);
    bool process_generate_command(const CommandNode& node, ContextSession& session);
    bool process_set_param_command(const CommandNode& node, ContextSession& session); // For generic --param value commands
    bool process_exit_command(const CommandNode& node, ContextSession& session);
    // Add more command processing methods as new command types are introduced in the parser.

    // --- Utility helper methods for argument parsing ---

    // Safely converts a string to an integer. Returns std::nullopt if conversion fails.
    std::optional<int> string_to_int(const std::string& s);

    // Safely converts a string to a boolean (e.g., "true", "false", "1", "0").
    // Returns std::nullopt if conversion fails.
    std::optional<bool> string_to_bool(const std::string& s);

    // Safely converts a string to a float. Returns std::nullopt if conversion fails.
    std::optional<float> string_to_float(const std::string& s);
};
