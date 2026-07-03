#include "llama_engine.hpp"


LlamaEngine::LlamaEngine() : model(nullptr), ctx(nullptr) {
    // Constructor body can be empty or used for initialization if needed
}

LlamaEngine::~LlamaEngine() {
    // Destructor: Clean up resources if necessary
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_free_model(model);
        model = nullptr;
    }
}
bool LlamaEngine::load_model(const std::string& model_filename) {
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_free_model(model);
        model = nullptr;
    }

    std::string full_path = "models/" + model_filename;
    std::cout << "[LlamaEngine] Loading model from: " << full_path << "...\n";

    llama_model_params model_params = llama_model_default_params();
    
    model = llama_load_model_from_file(full_path.c_str(), model_params);
    if (!model) {
        std::cerr << "[LlamaEngine] Error: Gagal memuat file model " << full_path << "\n";
        return false;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        std::cerr << "[LlamaEngine] Error: Gagal membuat LLaMA context.\n";
        llama_free_model(model);
        model = nullptr;
        return false;
    }

    std::cout << "[LlamaEngine] Model berhasil dimuat ke memori!\n";
    return true;
}

void LlamaEngine::initialize_backend() {
    if (!m_backend_initialized) {
        llama_backend_init();
        m_backend_initialized = true;
    }
}

void LlamaEngine::execute_generation(const ContextSession& session) {
    if (!ctx) {
        std::cerr << "Context is not initialized. Load a model first." << std::endl;
        return;
    }
    if (!model) {
        std::cerr << "Model is not loaded. Load a model first." << std::endl;
        return;
    }
    std::cout << "Generating text with model: " << session.modelName << std::endl;
    std::cout << "System Prompt: " << session.systemPrompt << std::endl;
    std::cout << "Temperature: " << session.currentTemperature << ", Max Tokens: " << session.maxTokens << std::endl;
}