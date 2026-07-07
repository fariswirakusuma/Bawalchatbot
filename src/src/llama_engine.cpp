#include "llama_engine.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include <curl/curl.h>
#include <cstdlib>
#include <sys/stat.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

LlamaEngine::LlamaEngine() : model(nullptr), ctx(nullptr) {
}

LlamaEngine::~LlamaEngine() {
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_free_model(model);
        model = nullptr;
    }
}

void LlamaEngine::add_url_model(const std::string& model_name, const std::string& model_url) {
    if (model_name.empty() || model_url.empty()) {
        std::cerr << "{\"status\":\"error\",\"message\":\"Empty parameters.\"}\n" << std::endl;
        return;
    }

    try {
        if (!fs::exists("models")) {
            fs::create_directory("models");
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "{\"status\":\"error\",\"message\":\"Failed to create directory: " << e.what() << "\"}\n" << std::endl;
        return;
    }

    std::string output_path = "models/" + model_name;
    std::string command;

    #ifdef _WIN32
        command = "curl -L -o " + output_path + " \"" + model_url + "\"";
    #else
        command = "wget -c --show-progress -O " + output_path + " \"" + model_url + "\"";
    #endif

    std::cout << "{\"status\":\"processing\",\"message\":\"Downloading model to " << output_path << "...\"}\n" << std::flush;

    int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr << "{\"status\":\"error\",\"message\":\"Download execution failed with exit code " << result << "\"}\n" << std::endl;
        return;
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

    llama_model_params model_params = llama_model_default_params();
    model = llama_load_model_from_file(full_path.c_str(), model_params);
    if (!model) {
        std::cout << "{\"status\":\"error\",\"message\":\"Gagal memuat file model\"}\n" << std::flush;
        return false;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        std::cout << "{\"status\":\"error\",\"message\":\"Gagal membuat LLaMA context\"}\n" << std::flush;
        llama_free_model(model);
        model = nullptr;
        return false;
    }

    std::cout << "{\"status\":\"success\",\"message\":\"Model berhasil dimuat\"}\n" << std::flush;
    return true;
}

void LlamaEngine::initialize_backend() {
    if (!m_backend_initialized) {
        llama_backend_init();
        m_backend_initialized = true;
    }
}

void LlamaEngine::change_parameters(const std::string& param_name, const std::string& param_value, ContextSession& session, bool is_ui_mode) {
    bool success = false;
    std::string message = "";

    try {
        if (param_name == "temperature" || param_name == "temp") {
            session.currentTemperature = std::stof(param_value);
            success = true;
            message = "Temperature updated to " + param_value;
        } else if (param_name == "top_k" || param_name == "topk") {
            session.topK = std::stoi(param_value);
            success = true;
            message = "TopK updated to " + param_value;
        } else if (param_name == "top_p" || param_name == "topp") {
            session.topP = std::stof(param_value);
            success = true;
            message = "TopP updated to " + param_value;
        } else if (param_name == "max_tokens" || param_name == "max_token") {
            session.maxTokens = std::stoi(param_value);
            success = true;
            message = "Max tokens updated to " + param_value;
        } else {
            message = "Unknown parameter: " + param_name;
        }
    } catch (const std::exception& e) {
        message = "Invalid value format for parameter " + param_name + ": " + e.what();
    }

    if (is_ui_mode) {
        if (success) {
            std::cout << "{\"status\":\"success\",\"message\":\"" << message << "\"}\n" << std::flush;
        } else {
            std::cout << "{\"status\":\"error\",\"message\":\"" << message << "\"}\n" << std::flush;
        }
    } else {
        if (success) {
            std::cout << "[SYSTEM] " << message << "\n";
        } else {
            std::cerr << "[ERROR] " << message << "\n";
        }
    }
}

void LlamaEngine::execute_generation(ContextSession& session, bool is_ui_mode) {
    if (!model) {
        if (is_ui_mode) std::cout << "{\"status\":\"error\",\"message\":\"Model belum dimuat\"}\n" << std::flush;
        else std::cerr << "Error: Model belum dimuat\n";
        return;
    }

    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        if (is_ui_mode) std::cout << "{\"status\":\"error\",\"message\":\"Gagal membuat ulang context\"}\n" << std::flush;
        else std::cerr << "Error: Gagal membuat ulang context\n";
        return;
    }

    if (session.chatHistory.empty()) {
        if (is_ui_mode) std::cout << "{\"status\":\"error\",\"message\":\"Chat history kosong\"}\n" << std::flush;
        else std::cerr << "Error: Chat history kosong\n";
        return;
    }
    std::string user_prompt = session.chatHistory.back().content;

    const llama_vocab* vocab = llama_model_get_vocab(model);

    std::vector<llama_token> tokens(user_prompt.size() + 1);
    int n_tokens = llama_tokenize(vocab, user_prompt.c_str(), user_prompt.size(), 
                                  tokens.data(), tokens.size(), 
                                  true, false);
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(vocab, user_prompt.c_str(), user_prompt.size(), 
                                  tokens.data(), tokens.size(), true, false);
    }
    tokens.resize(n_tokens);

    llama_sampler* smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(session.topK));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(session.topP, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(session.currentTemperature));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    batch.n_tokens = 0; 
    for (int i = 0; i < n_tokens; ++i) {
        batch.token[batch.n_tokens] = tokens[i];
        batch.pos[batch.n_tokens] = i;
        batch.n_seq_id[batch.n_tokens] = 1;
        batch.seq_id[batch.n_tokens][0] = 0; 
        batch.logits[batch.n_tokens] = (i == n_tokens - 1); 
        batch.n_tokens++;
    }

    if (llama_decode(ctx, batch) != 0) {
        if (is_ui_mode) std::cout << "{\"status\":\"error\",\"message\":\"Gagal melakukan decode awal\"}\n" << std::flush;
        else std::cerr << "Error: Gagal melakukan decode awal\n";
        llama_sampler_free(smpl);
        llama_batch_free(batch);
        return;
    }

    int n_cur = n_tokens;
    int n_decode_generated = 0;
    std::string ResponseOutput;
    
    if (is_ui_mode) {
        std::cout << "{\"status\":\"start\"}\n" << std::flush;
    }

    while (n_decode_generated < session.maxTokens) {
        llama_token id = llama_sampler_sample(smpl, ctx, -1);

        if (llama_token_is_eog(vocab, id)) {
            break;
        }

        char buf[128];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, false);
        if (n > 0) {
            std::string piece(buf, n);
            
            if (is_ui_mode) {
                std::string escaped_piece = "";
                for (char c : piece) {
                    if (c == '"') escaped_piece += "\\\"";
                    else if (c == '\\') escaped_piece += "\\\\";
                    else if (c == '\n') escaped_piece += "\\n";
                    else if (c == '\r') escaped_piece += "\\r";
                    else if (c == '\t') escaped_piece += "\\t";
                    else escaped_piece += c;
                }
                std::cout << "{\"status\":\"token\",\"content\":\"" << escaped_piece << "\"}\n" << std::flush;
            } else {
                // Jalur CLI murni: Keluarkan karakter mentah langsung ke layar
                std::cout << piece << std::flush;
            }
            ResponseOutput += piece;
        }

        batch.n_tokens = 0;
        batch.token[batch.n_tokens] = id;
        batch.pos[batch.n_tokens] = n_cur;
        batch.n_seq_id[batch.n_tokens] = 1;
        batch.seq_id[batch.n_tokens][0] = 0;
        batch.logits[batch.n_tokens] = true;
        batch.n_tokens++;

        n_cur++;
        n_decode_generated++;

        if (llama_decode(ctx, batch) != 0) {
            break;
        }
    }
    
    if (is_ui_mode) {
        std::cout << "{\"status\":\"end\"}\n" << std::flush;
    } else {
        std::cout << "\n";
    }

    session.chatHistory.push_back({"assistant", ResponseOutput});

    llama_sampler_free(smpl);
    llama_batch_free(batch);
}

bool LlamaEngine::load_history(const std::string& filename, ContextSession& session) {\
    std::string directory_name = "history";
    std::string full_path = directory_name + "/" + filename;
    
    try {
        std::ifstream file(full_path);
        if (!file.is_open()) {
            std::cerr << "Gagal membuka file riwayat.\n";
            return false;
        }
        nlohmann::json j;
        file >> j;

        bool current_exit_status = session.shouldExit;
        session = j.get<ContextSession>();
        session.shouldExit = current_exit_status;

        file.close();
        return true;
    } 
    catch (const std::exception& e) {
        std::cerr << "JSON Deserialize Error: " << e.what() << "\n";
        return false;
    }
}

bool LlamaEngine::save_history(const std::string& filename, const ContextSession& session){
    std::string directory_name = "history";
    std::string full_path = directory_name + "/" + filename;
    try {

        if (!std::filesystem::exists(directory_name)) {
            std::filesystem::create_directories(directory_name);
        }

        std::ofstream file(full_path);
        if (!file.is_open()) {
            std::cerr << "Gagal membuka file di jalur: " << full_path << "\n";
            return false;
        }
        nlohmann::json j = session; 
        file << j.dump(4);          
        
        file.flush(); 
        file.close();
        return true; 
    } 
    catch (const std::exception& e) {
        std::cerr << "JSON Serialize Error: " << e.what() << "\n";
        return false;
    }
}
