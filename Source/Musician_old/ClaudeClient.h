#pragma once

#include "LLMClient.h"
#include "PatternProcessor.h"
#include "ShorthandProcessor.h"
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <juce_core/juce_core.h>

namespace covertracks {

/**
 * @brief Claude API client implementation of the LLMClient interface
 * 
 * This class provides a connection to the Claude API for generating
 * musical patterns, leveraging Claude 3.7 Sonnet's capabilities.
 */
class ClaudeClient : public LLMClient {
public:
    /**
     * @brief Construct a new Claude Client
     * 
     * @param apiKey The Claude API key
     * @param config Configuration options as JSON
     */
    ClaudeClient(const std::string& apiKey, const json& config = json::object());
    
    /**
     * @brief Destroy the Claude Client
     */
    ~ClaudeClient() override;
    
    /**
     * @brief Generate a response from Claude based on a prompt
     * 
     * @param prompt The text prompt to send to Claude
     * @return json The generated response as a JSON object
     */
    json generateResponse(const std::string& prompt) override;
    
    /**
     * @brief Generate a response asynchronously
     * 
     * @param prompt The text prompt to send to Claude
     * @param callback Function to call with the response
     */
    void generateResponseAsync(const std::string& prompt, 
                             std::function<void(const json&)> callback) override;
    
    /**
     * @brief Update the client's configuration
     * 
     * @param updatedConfig JSON object with configuration updates
     * @return true If the configuration was updated successfully
     * @return false If the configuration update failed
     */
    bool updateConfiguration(const json& updatedConfig) override;
    
    /**
     * @brief Check if the client is ready to generate responses
     * 
     * @return true If the client is initialized and has a valid API key
     * @return false If the client is not ready
     */
    bool isReady() const override;
    
    /**
     * @brief Get the model name being used
     * 
     * @return std::string The model name
     */
    std::string getModelName() const override;
    
private:
    /**
     * @brief Make an API request to Claude
     * 
     * @param prompt The prompt to send
     * @param systemPrompt Optional system prompt
     * @return json The raw API response
     */
    json makeApiRequest(const std::string& prompt, const std::string& systemPrompt = "");
    
    /**
     * @brief Estimate token count for a message
     * 
     * @param text The message text
     * @return int Approximate token count
     */
    int estimateTokenCount(const std::string& text) const;
    
    // API key for authentication
    std::string apiKey;
    
    // Base URL for the Claude API
    juce::URL apiBaseUrl;
    
    // Configuration parameters
    json config;
    
    // Model name
    std::string modelName;
    
    // Token usage tracking
    struct TokenUsage {
        int promptTokens = 0;
        int completionTokens = 0;
        int totalTokens = 0;
    };
    
    // Total token usage since initialization
    std::atomic<int> totalTokensUsed;
    
    // Mutex for thread safety
    mutable std::mutex mutex;
    
    // Initialization flag
    std::atomic<bool> isInitialized;
    
    // In-memory cache for storing previous responses
    std::map<std::string, json> responseCache;
};

} // namespace covertracks