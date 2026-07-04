![Version](https://img.shields.io/badge/Release-v0.1.0-blue)
![C++](https://img.shields.io/badge/C++-17-orange)
![Python](https://img.shields.io/badge/Python-3.8+-yellow)

# Bawalchatbot


## Description :
Bawalchatbot is an AI-Chatbot Assistant CLI application built in C++17. It utilizes a custom-built Compiler Front-End (Lexer and Parser) to interpret structured slash-commands, and integrates with `llama.cpp` as the core Llama Engine to perform local text generation using GGUF-formatted Quantized LLMs.

## Features
* **Custom Abstract Syntax Tree (AST) Parser:** Built-in Lexer and Parser capable of handling commands, variables, and parameters.
* **Local Inference Engine:** Direct integration with `llama.cpp` utilizing modern `llama_sampler` and tokenization pipelines.
* **Isolated Sesi Context:** Automatically manages and recreates `llama_context` per generation to prevent KV cache token position conflicts.
* **Session Persistence:** Built-in support to save and load complete chat histories to and from the local filesystem.

---

## Command Architecture (CLI)

The application interprets user input through a structured command parser. Commands accept arguments (positional values) and parameters (key-value pairs).

### Available Commands 

| Command | Type | Description | Example |
| :--- | :--- | :--- | :--- |
| `/set_model` | Arguments | Loads a specific GGUF model file into memory. | `/set_model qwen-1_5b.gguf` |
| `/generate` | Arguments | Triggers text generation using the currently loaded model. | `/generate Tell me a story` |
| `/save_history` | Arguments | Saves the active session chat history to a file. | `/save_history session1` |
| `/load_history` | Arguments | Loads a previously saved chat history from a file. | `/load_history session1` |
| `/exit` | None | Safely destroys contexts and exits the CLI application. | `/exit` |

---

## Technical Specifications & Data Structures

### Context Session Configuration
The engine keeps track of the conversation state and generation hyperparameters using the `ContextSession` struct:

```cpp
struct Message {
    std::string role;    
    std::string content;
};

struct ContextSession {
    std::string systemPrompt;
    float currentTemperature = 0.7f;
    int maxTokens = 256;
    int topK = 40;
    float topP = 0.95f;
    float repeatPenalty = 1.1f;
    bool shouldExit = false;
    std::vector<Message> chatHistory;
    std::string modelName; 
};
```
## Installation & Compilation
### Prerequisites
* C++17 compliant compiler (GCC 9+, Clang 10+, or MSVC 2019+)
* CMake (Version 3.16 or higher)
* GGUF Model Files (placed inside a models/ directory)


## Build & Automation Management

This project uses `task` (Taskfile.dev) as its task runner and automation tool. You can inspect and trigger all available workflows from the root directory.

### Listing Available Commands
To view the comprehensive list of automation tasks alongside their descriptions directly in your terminal, execute:
```bash
task help
```
Essential Task Instructions
* Compile Backend Core: `task default` or `task build_backend`

* Launch Terminal CLI: `task run_cli`

* Download Default LLM Model: `task install_llama`

* Purge Build Artifacts: `task clean`

* Automated Agent Workflows: `task prompt_debug` or `task prompt_debug_gemini`

