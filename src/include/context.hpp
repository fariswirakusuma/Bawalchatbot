#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Message {
    std::string role;    
    std::string content;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Message, role, content)

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
    std::string historyFileName;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ContextSession,
    systemPrompt,
    currentTemperature,
    maxTokens,
    topK,
    topP,
    repeatPenalty,
    chatHistory,
    modelName,
    historyFileName
)

struct StructuredOutput {
    std::string rawText;          
    int32_t tokensGenerated = 0;
    float inferenceTimeMs = 0.0f;
    std::string executionStatus;  
};

