#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "parser.hpp"// For CommandNode, CommandType, and other AST structures
#include "context.hpp" // For ContextSession, ChatMessage, etc.
#include "llama.h"    // Included for potential future integration or type definitions,


class LlamaEngine {
public:
    LlamaEngine();
    ~LlamaEngine();
    // Dipanggil sekali saat aplikasi CLI atau UI pertama kali menyala
    void initialize_backend();

    // Dipanggil saat SemanticAnalyzer sukses memproses perintah '/set_model'
    bool load_model(const std::string& model_path);
    bool load_history(const std::string& filename, ContextSession& session);
    bool save_history(const std::string& filename, const ContextSession& session);
    bool add_url_model(const std::string& model_name, const std::string& model_url);


    // Dipanggil saat SemanticAnalyzer sukses memproses perintah '/generate'
    void execute_generation(ContextSession& session);

private:
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    bool m_backend_initialized = false;
};