#include "semantic.hpp"

#include <algorithm> // For std::transform, std::tolower
#include <cctype>    // For ::tolower

// --- SemanticAnalyzer Implementation ---

void SemanticAnalyzer::add_error(const std::string& message) {
    m_errors.push_back(message);
}

const std::vector<std::string>& SemanticAnalyzer::get_errors() const {
    return m_errors;
}

std::optional<int> SemanticAnalyzer::string_to_int(const std::string& s) {
    try {
        size_t pos;
        int val = std::stoi(s, &pos);
        if (pos == s.length()) { 
            return val;
        }
    } catch (const std::invalid_argument&) {
        // Not an integer
    } catch (const std::out_of_range&) {
        // Value out of range for int
    }
    return std::nullopt;
}

std::optional<bool> SemanticAnalyzer::string_to_bool(const std::string& s) {
    std::string lower_s = s;
    std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(), ::tolower);

    if (lower_s == "true" || lower_s == "1") {
        return true;
    }
    if (lower_s == "false" || lower_s == "0") {
        return false;
    }
    return std::nullopt;
}

std::optional<float> SemanticAnalyzer::string_to_float(const std::string& s) {
    try {
        size_t pos;
        float val = std::stof(s, &pos);
        if (pos == s.length()) { // Ensure the entire string was consumed
            return val;
        }
    } catch (const std::invalid_argument&) {
        // Not a float
    } catch (const std::out_of_range&) {
        // Value out of range for float
    }
    return std::nullopt;
}

bool SemanticAnalyzer::analyze_and_apply(const CommandNode* ast_root, ContextSession& session) {
    m_errors.clear(); // Clear errors from previous runs

    if (!ast_root) {
        add_error("AST root is null. No command to analyze.");
        return false;
    }

    bool success = true;
    switch (ast_root->cmdType) { // Use cmdType from CommandNode
        case CommandType::SetModel:
            success = process_set_model_command(*ast_root, session);
            break;
        case CommandType::AddUrlModel:
            success = process_add_url_model_command(*ast_root, session);
            break;
        case CommandType::SetPrompt:
            success = process_set_prompt_command(*ast_root, session);
            break;
        case CommandType::LoadHistory:
            success = process_load_history_command(*ast_root, session);
            break;
        case CommandType::SaveHistory:
            success = process_save_history_command(*ast_root, session);
            break;
        case CommandType::Generate:
            success = process_generate_command(*ast_root, session);
            break;
        case CommandType::ShowParams:
            // ShowParams command doesn't modify session, but we can validate its structure if needed
            success = true; // Assuming show_params command is always valid for now
            break;
        case CommandType::SetParam:
            success = process_set_param_command(*ast_root, session);
            break;
        case CommandType::Help:
            success = true; // Assuming help command is always valid for now
            break;
        case CommandType::Exit:
            success = process_exit_command(*ast_root, session);
            break;
        case CommandType::Unknown:
        default:
            add_error("Unknown or unsupported command type encountered.");
            success = false;
            break;
    }

    return success;
}

bool SemanticAnalyzer::process_set_model_command(const CommandNode& node, ContextSession& session) {
    if (node.arguments.empty() && node.parameters.empty()) {
        add_error("Semantic Error: 'set_model' command requires a model name.");
        return false;
    }

    if (!node.arguments.empty()) {
        if (node.arguments[0]->type == NodeType::LiteralText) {
            const LiteralTextNode* modelNameNode = static_cast<const LiteralTextNode*>(node.arguments[0].get());
            session.modelName = modelNameNode->text;
            return true;
        } else {
            add_error("Semantic Error: 'set_model' command expects a literal text for the model name.");
            return false;
        }
    }
    else if (!node.parameters.empty()) {
        const ParameterNode* modelParam = node.parameters[0].get();
        if (modelParam->key == "name" && !modelParam->value.empty()) {
            session.modelName = modelParam->value;
            return true;
        } else {
            add_error("Semantic Error: 'set_model' command requires a valid model name parameter (e.g., --name <model_name>).");
            return false;
        }
    }
    return false;
}

bool SemanticAnalyzer::process_set_prompt_command(const CommandNode& node, ContextSession& session) {
    if (node.arguments.empty() && node.parameters.empty()) {
        add_error("Semantic Error: 'set_prompt' command requires a prompt string.");
        return false;
    }
    std::string combined_prompt = "";
    if (!node.arguments.empty()) {
        for (const auto& argNodePtr : node.arguments) {
            if (argNodePtr->type == NodeType::LiteralText) {
                const LiteralTextNode* textNode = static_cast<const LiteralTextNode*>(argNodePtr.get());
                if (!combined_prompt.empty()) combined_prompt += " ";
                combined_prompt += textNode->text;
            } else {
                add_error("Semantic Error: 'set_prompt' command expects literal text arguments.");
                return false;
            }
        }
        session.systemPrompt = combined_prompt;
        return true;
    }
    else if (!node.parameters.empty()) {
        const ParameterNode* promptParam = node.parameters[0].get();
        if (promptParam->key == "text" && !promptParam->value.empty()) {
            session.systemPrompt = promptParam->value;
            return true;
        } else {
            add_error("Semantic Error: 'set_prompt' command requires a valid prompt string parameter (e.g., --text \"prompt\").");
            return false;
        }
    }
    return false;
}

bool SemanticAnalyzer::process_add_url_model_command(const CommandNode& node, ContextSession& session) {
    if (node.arguments.size() < 2) {
        add_error("Semantic Error: 'add_url_model' command requires two arguments: <model_name> and <model_url>.");
        return false;
    }

    std::string model_name;
    std::string model_url;
    if (node.arguments[0]->type == NodeType::LiteralText) {
        const LiteralTextNode* nameNode = static_cast<const LiteralTextNode*>(node.arguments[0].get());
        model_name = nameNode->text;
    } else {
        add_error("Semantic Error: 'add_url_model' command expects a literal text for the model name at argument 1.");
        return false;
    }

    if (node.arguments[1]->type == NodeType::LiteralText) {
        const LiteralTextNode* urlNode = static_cast<const LiteralTextNode*>(node.arguments[1].get());
        model_url = urlNode->text;
    } else {
        add_error("Semantic Error: 'add_url_model' command expects a literal text for the model URL at argument 2.");
        return false;
    }
    if (!model_name.empty() && !model_url.empty()) {
        model_name.erase(std::remove(model_name.begin(), model_name.end(), '\"'), model_name.end());
        model_url.erase(std::remove(model_url.begin(), model_url.end(), '\"'), model_url.end());

        session.modelName = model_name; 
        return true;
    }

    return false;
}

bool SemanticAnalyzer::process_load_history_command(const CommandNode& node, ContextSession& session) {
    if (node.arguments.empty() && node.parameters.empty()) {
        add_error("Semantic Error: 'load_history' command requires a filename.");
        return false;
    }

    std::string filename;
    if (!node.arguments.empty()) {
        if (node.arguments[0]->type == NodeType::LiteralText) {
            const LiteralTextNode* fileNode = static_cast<const LiteralTextNode*>(node.arguments[0].get());
            filename = fileNode->text;
        } else {
            add_error("Semantic Error: 'load_history' command expects a literal text for the filename.");
            return false;
        }
    } else if (!node.parameters.empty()) {
        const ParameterNode* fileParam = node.parameters[0].get();
        if (fileParam->key == "file" && !fileParam->value.empty()) {
            filename = fileParam->value;
        } else {
            add_error("Semantic Error: 'load_history' command requires a valid filename parameter (e.g., --file <filename>).");
            return false;
        }
    }

    if (!filename.empty()) {
        return true;
    }
    return false;
}

bool SemanticAnalyzer::process_save_history_command(const CommandNode& node, ContextSession& session) {
    if (node.arguments.empty() && node.parameters.empty()) {
        add_error("Semantic Error: 'save_history' command requires a filename.");
        return false;
    }

    std::string filename;
    if (!node.arguments.empty()) {
        if (node.arguments[0]->type == NodeType::LiteralText) {
            const LiteralTextNode* fileNode = static_cast<const LiteralTextNode*>(node.arguments[0].get());
            filename = fileNode->text;
        } else {
            add_error("Semantic Error: 'save_history' command expects a literal text for the filename.");
            return false;
        }
    } else if (!node.parameters.empty()) {
        const ParameterNode* fileParam = node.parameters[0].get();
        if (fileParam->key == "file" && !fileParam->value.empty()) {
            filename = fileParam->value;
        } else {
            add_error("Semantic Error: 'save_history' command requires a valid filename parameter (e.g., --file <filename>).");
            return false;
        }
    }

    if (!filename.empty()) {
        return true;
    }
    return false;
}

bool SemanticAnalyzer::process_generate_command(const CommandNode& node, ContextSession& session) {
    bool success = true;
    std::string combined_user_prompt = "";

    for (const auto& argNodePtr : node.arguments) {
        if (argNodePtr->type == NodeType::LiteralText) {
            const LiteralTextNode* textNode = static_cast<const LiteralTextNode*>(argNodePtr.get());
            if (!combined_user_prompt.empty()) {
                combined_user_prompt += " ";
            }
            combined_user_prompt += textNode->text;
        } else {
            add_error("Semantic Error: 'generate' command received a non-literal argument.");
            success = false;
        }
    }

    for (const auto& paramNodePtr : node.parameters) {
        const ParameterNode* pNode = paramNodePtr.get();
        if (pNode->key == "prompt") {
            if (!combined_user_prompt.empty()) {
                add_error("Semantic Error: 'generate' command received multiple prompt arguments.");
                success = false;
            }
            combined_user_prompt = pNode->value;
        } else {
            add_error("Semantic Warning: Perintah 'generate' tidak menerima parameter tambahan di luar --prompt di versi ini.");
        }
    }

    if (!combined_user_prompt.empty() && success) {
        session.chatHistory.push_back({"user", combined_user_prompt});
    }

    return success;
}

bool SemanticAnalyzer::process_set_param_command(const CommandNode& node, ContextSession& session) {
    bool success = true;

    for (const auto& paramNodePtr : node.parameters) {
        const ParameterNode* pNode = paramNodePtr.get();

        if (pNode->key == "max_tokens") {
            if (auto val = string_to_int(pNode->value)) {
                session.maxTokens = val.value();
            } else {
                add_error("Semantic Error: Invalid value for --max_tokens: '" + pNode->value + "'. Expected an integer.");
                success = false;
            }
        } else if (pNode->key == "temp") {
            if (auto val = string_to_float(pNode->value)) {
                session.currentTemperature = val.value();
            } else {
                add_error("Semantic Error: Invalid value for --temp: '" + pNode->value + "'. Expected a float.");
                success = false;
            }
        } else if (pNode->key == "top_k") {
            if (auto val = string_to_int(pNode->value)) {
                session.topK = val.value();
            } else {
                add_error("Semantic Error: Invalid value for --top_k: '" + pNode->value + "'. Expected an integer.");
                success = false;
            }
        } else if (pNode->key == "top_p") {
            if (auto val = string_to_float(pNode->value)) {
                session.topP = val.value();
            } else {
                add_error("Semantic Error: Invalid value for --top_p: '" + pNode->value + "'. Expected a float.");
                success = false;
            }
        } else if (pNode->key == "repeat_penalty") {
            if (auto val = string_to_float(pNode->value)) {
                session.repeatPenalty = val.value();
            } else {
                add_error("Semantic Error: Invalid value for --repeat_penalty: '" + pNode->value + "'. Expected a float.");
                success = false;
            }
        } else if (pNode->key == "system") {
            session.systemPrompt = pNode->value;
        } else {
            add_error("Semantic Warning: Unknown parameter for 'set_param' command: --" + pNode->key);
        }
    }

    return success;
}

bool SemanticAnalyzer::process_exit_command(const CommandNode& node, ContextSession& session) {
    if (!node.parameters.empty()) {
        add_error("Semantic Warning: 'exit' command does not take any parameters. Ignoring them.");
    }
    session.shouldExit = true;
    return true;
}