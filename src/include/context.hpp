#pragma once

#include <string>
#include <vector>

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

struct StructuredOutput {
    std::string rawText;          
    int32_t tokensGenerated = 0;
    float inferenceTimeMs = 0.0f;
    std::string executionStatus;  
};

