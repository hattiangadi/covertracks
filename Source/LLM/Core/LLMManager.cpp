#include "LLMManager.h"
#include <iostream>

namespace covertracks {

// Initialize the singleton instance
LLMManager* LLMManager::instance = nullptr;

LLMManager& LLMManager::getInstance()
{
    if (instance == nullptr) {
        instance = new LLMManager();
    }
    return *instance;
}

LLMManager::LLMManager()
    : initialized(false)
{
    // Nothing to do here
}

void LLMManager::initialize()
{
    if (initialized) {
        return;
    }
    
    // Load API keys
    loadApiKeys();
    
    // Create the default Claude client
    if (!claudeApiKey.isEmpty() && claudeApiKey != "YOUR_API_KEY") {
        auto claudeClient = std::make_shared<ClaudeClient>(claudeApiKey, "claude-3-7-sonnet-20250219");
        clients["claude"] = claudeClient;
        std::cout << "[LLMManager] Created Claude client with API key" << std::endl;
    } else {
        std::cout << "[LLMManager] WARNING: No valid Claude API key found" << std::endl;
    }
    
    initialized = true;
}

std::shared_ptr<LLMClient> LLMManager::getDefaultClient()
{
    if (!initialized) {
        initialize();
    }
    
    // Try to get the Claude client first
    auto it = clients.find("claude");
    if (it != clients.end()) {
        return it->second;
    }
    
    // If no clients are available, return nullptr
    return nullptr;
}

std::shared_ptr<LLMClient> LLMManager::getClient(const juce::String& name)
{
    if (!initialized) {
        initialize();
    }
    
    auto it = clients.find(name);
    if (it != clients.end()) {
        return it->second;
    }
    
    return nullptr;
}

void LLMManager::loadApiKeys()
{
    // Try to get the Claude API key from environment variables
    const char* envKey = std::getenv("CLAUDE_API_KEY");
    if (envKey != nullptr) {
        claudeApiKey = juce::String(envKey);
        std::cout << "[LLMManager] Found CLAUDE_API_KEY environment variable" << std::endl;
    } else {
        // Try JUCE's method as a fallback
        claudeApiKey = juce::SystemStats::getEnvironmentVariable("CLAUDE_API_KEY", "");
        
        if (!claudeApiKey.isEmpty()) {
            std::cout << "[LLMManager] Found CLAUDE_API_KEY using JUCE method" << std::endl;
        } else {
            std::cout << "[LLMManager] WARNING: CLAUDE_API_KEY environment variable not found" << std::endl;
            
            // For development purposes, use a hardcoded key
            // In a production environment, you would want to handle this differently
            claudeApiKey = "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";
            std::cout << "[LLMManager] Using hardcoded API key for development" << std::endl;
        }
    }
    
    // Debug output (securely)
    if (!claudeApiKey.isEmpty()) {
        std::cout << "[LLMManager] API key length: " << claudeApiKey.length() << std::endl;
        std::cout << "[LLMManager] First few characters: " << claudeApiKey.substring(0, 8) << "..." << std::endl;
    }
}

} // namespace covertracks
