#include "LLMClient.h"
#include "ClaudeClient.h"
#include "MockLLMClient.h"
#include <iostream>

namespace coverband {

std::unique_ptr<LLMClient> LLMClientFactory::createClient(const std::string& type, const json& config)
{
    std::cout << "Creating LLM client of type: " << type << std::endl;

    if (type == "claude" || type == "anthropic") {
        // Check for API key
        if (!config.contains("api_key") || !config["api_key"].is_string()) {
            // Try to get API key from environment variable
            const char* apiKey = std::getenv("CLAUDE_API_KEY");
            if (apiKey == nullptr || strlen(apiKey) == 0) {
                std::cerr << "Error: API key is required for Claude client. "
                          << "Either provide it in config or set CLAUDE_API_KEY environment variable." 
                          << std::endl;
                return nullptr;
            }
            
            // Create a copy of the config and add the API key
            json configWithKey = config;
            configWithKey["api_key"] = apiKey;
            return std::make_unique<coverband::ClaudeClient>(apiKey, configWithKey);
        } else {
            // Use API key from config
            std::string apiKey = config["api_key"];
            return std::make_unique<coverband::ClaudeClient>(apiKey, config);
        }
    }
    else if (type == "mock") {
        // Mock client for testing
        return std::make_unique<coverband::MockLLMClient>();
    }
    else {
        std::cerr << "Unknown LLM client type: " << type << std::endl;
        return nullptr;
    }
}

} // namespace coverband