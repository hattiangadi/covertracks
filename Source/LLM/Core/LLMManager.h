#pragma once

#include <JuceHeader.h>
#include "LLMClient.h"
#include "ClaudeClient.h"

namespace covertracks {

/**
 * @brief Manages LLM clients and API keys
 */
class LLMManager
{
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return LLMManager& The singleton instance
     */
    static LLMManager& getInstance();
    
    /**
     * @brief Get the default LLM client
     * 
     * @return LLMClient* The default LLM client
     */
    LLMClient* getDefaultClient();
    
    /**
     * @brief Get a specific LLM client by name
     * 
     * @param clientName The name of the client to get
     * @return LLMClient* The LLM client, or nullptr if not found
     */
    LLMClient* getClient(const juce::String& clientName);
    
    /**
     * @brief Get the Claude API key
     * 
     * @return const juce::String& The Claude API key
     */
    const juce::String& getClaudeApiKey() const;
    
    /**
     * @brief Log an LLM interaction to a file
     * 
     * @param prompt The prompt sent to the LLM
     * @param response The response from the LLM
     * @param clientName The name of the client used
     * @param success Whether the interaction was successful
     */
    void logInteraction(const juce::String& prompt, 
                      const juce::String& response, 
                      const juce::String& clientName,
                      bool success);
    
    /**
     * @brief Set a custom API key for Claude
     * 
     * @param apiKey The API key to set
     */
    void setClaudeApiKey(const juce::String& apiKey);
    
private:
    /**
     * @brief Construct a new LLM Manager
     * 
     * Private constructor to enforce singleton pattern
     */
    LLMManager();
    
    /**
     * @brief Load API keys from environment variables or configuration
     */
    void loadApiKeys();
    
    /**
     * @brief Create the log directory if it doesn't exist
     */
    void createLogDirectory();
    
    // API keys
    juce::String claudeApiKey;
    
    // LLM clients
    std::unique_ptr<LLMClient> defaultClient;
    std::map<juce::String, std::unique_ptr<LLMClient>> clients;
    
    // Logging
    juce::File logDirectory;
};

} // namespace covertracks
