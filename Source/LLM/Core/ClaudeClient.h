// Source/LLM/Core/ClaudeClient.h
#pragma once

#include "LLMClient.h"
#include <JuceHeader.h>

namespace covertracks {

/**
 * @brief Client for communicating with Claude AI
 * 
 * This class implements the LLMClient interface for Anthropic's Claude AI.
 */
class ClaudeClient : public LLMClient, private Thread {
public:
    /**
     * @brief Construct a new Claude Client
     * 
     * @param apiKey The API key for Claude
     * @param model The model to use (default: claude-3-opus-20240229)
     */
    ClaudeClient(const juce::String& apiKey, const juce::String& model = "claude-3-opus-20240229");
    
    /**
     * @brief Create a clone of this client
     * 
     * @return LLMClient* A new instance of the same client type
     */
    LLMClient* clone() const override;
    
    /**
     * @brief Destroy the Claude Client
     */
    ~ClaudeClient() override;
    
    // LLMClient interface
    void sendMessage(const String& message, ResponseCallback callback) override;
    void sendMessageWithHistory(const String& message, 
                              const Array<std::pair<String, String>>& history,
                              ResponseCallback callback) override;
    bool isConnected() const override;
    String getServiceName() const override;
    
private:
    // Thread interface
    void run() override;
    
    // Helper methods
    var createRequestBody(const String& message, 
                          const Array<std::pair<String, String>>& history = {});
    
    // Member variables
    String apiKey;
    String model;
    bool connected;
    
    // Message queue for thread
    struct MessageRequest {
        String message;
        Array<std::pair<String, String>> history;
        ResponseCallback callback;
    };
    
    Array<MessageRequest> messageQueue;
    CriticalSection queueLock;
    WaitableEvent waitEvent;
    
    // HTTP utilities
    CriticalSection httpLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClaudeClient)
};

} // namespace covertracks