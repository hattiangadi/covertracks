#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <string>

namespace covertracks {

/**
 * @brief Abstract interface for LLM communication
 * 
 * This class defines a common interface for communication with
 * various Language Learning Models (LLMs).
 */
class LLMClient {
public:
    virtual ~LLMClient() = default;
    
    /**
     * @brief Create a clone of this client
     * 
     * @return LLMClient* A new instance of the same client type
     */
    virtual LLMClient* clone() const = 0;
    
    /**
     * @brief Callback for LLM responses
     * 
     * @param success Whether the request was successful
     * @param response The response from the LLM
     */
    using ResponseCallback = std::function<void(bool success, const String& response)>;
    
    /**
     * @brief Send a message to the LLM
     * 
     * @param message The message to send
     * @param callback Callback to be invoked when a response is received
     */
    virtual void sendMessage(const String& message, ResponseCallback callback) = 0;
    
    /**
     * @brief Send a message with conversation history to the LLM
     * 
     * @param message The latest message to send
     * @param history Previous conversation turns
     * @param callback Callback to be invoked when a response is received
     */
    virtual void sendMessageWithHistory(const String& message, 
                                      const Array<std::pair<String, String>>& history,
                                      ResponseCallback callback) = 0;
    
    /**
     * @brief Check if the client is connected to the LLM service
     * 
     * @return true if connected
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Get the name of the LLM service
     * 
     * @return String The name of the LLM service
     */
    virtual String getServiceName() const = 0;
};

} // namespace covertracks
