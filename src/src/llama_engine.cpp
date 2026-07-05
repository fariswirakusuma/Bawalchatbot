#include "llama_engine.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

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

bool LlamaEngine::load_model(const std::string& model_filename) {
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_free_model(model);
        model = nullptr;
    }

    std::string full_path = "model/" + model_filename;

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

void LlamaEngine::execute_generation(ContextSession& session) {
    if (!model) {
        std::cout << "{\"status\":\"error\",\"message\":\"Model belum dimuat\"}\n" << std::flush;
        return;
    }

    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        std::cout << "{\"status\":\"error\",\"message\":\"Gagal membuat ulang context\"}\n" << std::flush;
        return;
    }

    if (session.chatHistory.empty()) {
        std::cout << "{\"status\":\"error\",\"message\":\"Chat history kosong\"}\n" << std::flush;
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
        std::cout << "{\"status\":\"error\",\"message\":\"Gagal melakukan decode awal\"}\n" << std::flush;
        llama_sampler_free(smpl);
        llama_batch_free(batch);
        return;
    }

    int n_cur = n_tokens;
    int n_decode_generated = 0;
    std::string ResponseOutput;
    
    std::cout << "{\"status\":\"start\"}\n" << std::flush;

    while (n_decode_generated < session.maxTokens) {
        llama_token id = llama_sampler_sample(smpl, ctx, -1);

        if (llama_token_is_eog(vocab, id)) {
            break;
        }

        char buf[128];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, false);
        if (n > 0) {
            std::string piece(buf, n);
            
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
    
    std::cout << "{\"status\":\"end\"}\n" << std::flush;
    session.chatHistory.push_back({"assistant", ResponseOutput});

    llama_sampler_free(smpl);
    llama_batch_free(batch);
}

bool LlamaEngine::load_history(const std::string& filename, ContextSession& session) {
    if (filename.empty()) {
        return false;
    }
    std::string full_path = "history/" + filename;
    if (!fs::exists(full_path)) {
        return false;
    }
    std::ifstream infile(full_path);
    if (!infile.is_open()) {
        return false;
    }   
    session.chatHistory.clear();
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            Message msg;
            msg.role = line.substr(0, colon_pos);
            msg.content = line.substr(colon_pos + 1);
            session.chatHistory.push_back(msg);
        }
    }
    return true;
}

bool LlamaEngine::save_history(const std::string& filename, const ContextSession& session) {
    if (filename.empty()) {
        return false;
    }
    std::string folder_path = "history";
    if (!fs::exists(folder_path)) {
        fs::create_directories(folder_path);
    }
    std::string full_path = folder_path + "/" + filename;
    std::ofstream outfile(full_path);
    if (!outfile.is_open()) {
        return false;
    }   
    for (const auto& msg : session.chatHistory) {
        outfile << msg.role << ":" << msg.content << "\n";
    }
    return true;
}