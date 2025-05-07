#pragma once

#include <JuceHeader.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <functional>

using json = nlohmann::json;

namespace coverband {

/**
 * @brief Abstract interface for LLM clients
 * 
 * This class defines the interface for all LLM clients, whether they're
 * local inference engines or remote API services.
 */
class LLMClient {
public:
    virtual ~LLMClient() = default;

    /**
     * @brief Generate a response from the LLM based on a prompt
     * 
     * @param prompt The text prompt to send to the LLM
     * @return json The generated response as a JSON object
     */
    virtual json generateResponse(const std::string& prompt) = 0;
    
    /**
     * @brief Generate a response asynchronously
     * 
     * @param prompt The text prompt to send to the LLM
     * @param callback Function to call with the response
     */
    virtual void generateResponseAsync(const std::string& prompt, 
                                      std::function<void(const json&)> callback) = 0;

    /**
     * @brief Update the LLM client's configuration
     * 
     * @param updatedConfig JSON object with configuration updates
     * @return true If the configuration was updated successfully
     * @return false If the configuration update failed or is unsupported
     */
    virtual bool updateConfiguration(const json& /*updatedConfig*/) { return false; }

    /**
     * @brief Set the GBNF grammar to use for generations (if supported)
     * 
     * @param grammarString The GBNF grammar string
     * @return true If the grammar was set successfully (or conceptually accepted)
     * @return false If setting the grammar failed or is unsupported
     */
    virtual bool setGrammar(const std::string& /*grammarString*/) { return false; }

    /**
     * @brief Check if the LLM client is ready
     * 
     * @return true If the client is initialized and ready to generate responses
     * @return false If the client is not ready
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Get the name of the LLM model being used
     * 
     * @return std::string The model name
     */
    virtual std::string getModelName() const = 0;
};

/**
 * @brief Factory for creating LLM clients
 */
class LLMClientFactory {
public:
    /**
     * @brief Create an LLM client of the specified type
     * 
     * @param type The type of LLM client ("local", "anthropic", "openai")
     * @param config Configuration parameters as JSON
     * @return std::unique_ptr<LLMClient> The created LLM client
     */
    static std::unique_ptr<LLMClient> createClient(const std::string& type, const json& config);
};

} // namespace coverband