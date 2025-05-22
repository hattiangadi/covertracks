#include "LLMManager.h"
#include <iostream>

namespace covertracks {

LLMManager& LLMManager::getInstance()
{
    static LLMManager instance;
    return instance;
}

LLMManager::LLMManager()
{
    // Create log directory
    createLogDirectory();
    
    // Load API keys
    loadApiKeys();
    
    // Create the default client
    defaultClient = std::make_unique<ClaudeClient>(claudeApiKey);
}

void LLMManager::loadApiKeys()
{
    const char* envKey = std::getenv("CLAUDE_API_KEY");
    if (envKey != nullptr) {
        claudeApiKey = juce::String(envKey);
    } else {
        claudeApiKey = juce::SystemStats::getEnvironmentVariable("CLAUDE_API_KEY", "");
    }
    
    // For development purposes, use a hardcoded key if the environment variable is not set
    if (claudeApiKey.isEmpty()) {
        // This is just for development and should be removed in production
        claudeApiKey = "sk-ant-api03-_QjNPG-Uj9jTKQZGbU_fXw-EiPUzYEDnVVUZvR9-uxFEL6_dDYzxrjh9IQVS-9BwIQ-5HtIgAAAAGGADIwAQ";
        
        std::cout << "[LLMManager] Using hardcoded API key for development" << std::endl;
    } else {
        std::cout << "[LLMManager] Using API key from environment variable" << std::endl;
    }
    
    std::cout << "[LLMManager] API key length: " << claudeApiKey.length() << std::endl;
    if (!claudeApiKey.isEmpty()) {
        std::cout << "[LLMManager] First few characters: " << claudeApiKey.substring(0, 8) << "..." << std::endl;
    }
}

void LLMManager::createLogDirectory()
{
    // Create the log directory in the user's documents folder
    logDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("Covertracks")
                    .getChildFile("Logs")
                    .getChildFile("LLM");
    
    if (!logDirectory.exists()) {
        logDirectory.createDirectory();
    }
    
    std::cout << "[LLMManager] Log directory: " << logDirectory.getFullPathName() << std::endl;
}

LLMClient* LLMManager::getDefaultClient()
{
    return defaultClient.get();
}

LLMClient* LLMManager::getClient(const juce::String& clientName)
{
    if (clientName == "claude") {
        return defaultClient.get();
    }
    
    return nullptr;
}

const juce::String& LLMManager::getClaudeApiKey() const
{
    return claudeApiKey;
}

void LLMManager::setClaudeApiKey(const juce::String& apiKey)
{
    claudeApiKey = apiKey;
    
    // Update the default client
    defaultClient = std::make_unique<ClaudeClient>(claudeApiKey);
}

void LLMManager::logInteraction(const juce::String& prompt, 
                               const juce::String& response, 
                               const juce::String& clientName,
                               bool success)
{
    // Create a timestamp
    juce::Time now = juce::Time::getCurrentTime();
    juce::String timestamp = now.formatted("%Y-%m-%d_%H-%M-%S");
    
    // Create the log file
    juce::File logFile = logDirectory.getChildFile(clientName + "_" + timestamp + ".log");
    
    // Create the log content
    juce::String logContent = "Timestamp: " + now.toString(true, true) + "\n";
    logContent += "Client: " + clientName + "\n";
    logContent += "Success: " + juce::String(success ? "Yes" : "No") + "\n\n";
    logContent += "Prompt:\n" + prompt + "\n\n";
    logContent += "Response:\n" + response + "\n";
    
    // Write the log file
    logFile.replaceWithText(logContent);
    
    std::cout << "[LLMManager] Logged interaction to " << logFile.getFullPathName() << std::endl;
}

} // namespace covertracks
