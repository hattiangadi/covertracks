#pragma once

#include <JuceHeader.h>
#include "LLMClient.h"
#include "ClaudeClient.h"

namespace covertracks {

/**
 * @brief Manager class for LLM services
 * 
 * This class handles the creation and management of LLM clients,
 * including API key management and service configuration.
 */
class LLMManager {
public:
    /**
     * @brief Get the singleton instance of the LLM manager
     * 
     * @return LLMManager& The singleton instance
     */
    static LLMManager& getInstance();
    
    /**
     * @brief Initialize the LLM manager
     * 
     * This method initializes the LLM manager with the appropriate
     * API keys and configuration. It should be called once at startup.
     */
    void initialize();
    
    /**
     * @brief Get the default LLM client
     * 
     * @return std::shared_ptr<LLMClient> The default LLM client
     */
    std::shared_ptr<LLMClient> getDefaultClient();
    
    /**
     * @brief Get a specific LLM client by name
     * 
     * @param name The name of the client to get
     * @return std::shared_ptr<LLMClient> The requested LLM client, or nullptr if not found
     */
    std::shared_ptr<LLMClient> getClient(const juce::String& name);
    
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
    
    // The singleton instance
    static LLMManager* instance;
    
    // Map of client names to client instances
    std::map<juce::String, std::shared_ptr<LLMClient>> clients;
    
    // API keys
    juce::String claudeApiKey;
    
    // Initialization flag
    bool initialized;
};

} // namespace covertracks
