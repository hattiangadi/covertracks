// Source/LLM/Core/ClaudeClient.cpp
#include "ClaudeClient.h"

namespace covertracks {

ClaudeClient::ClaudeClient(const String& apiKeyParam, const String& modelParam)
    : Thread("ClaudeClientThread"),
      apiKey(apiKeyParam),
      model(modelParam),
      connected(false),
      waitEvent(true) // Auto reset
{
    startThread();
    
    // Test connection on startup
    connected = !apiKey.isEmpty();
    
    std::cout << "Claude Client initialized with model: " << model << std::endl;
}

ClaudeClient::~ClaudeClient()
{
    // No need to cancel downloads with the direct approach
    
    // Signal the thread to exit and wait for it
    signalThreadShouldExit();
    waitEvent.signal();
    stopThread(5000);
}

void ClaudeClient::sendMessage(const String& message, ResponseCallback callback)
{
    // Just call the history version with an empty history
    sendMessageWithHistory(message, {}, std::move(callback));
}

void ClaudeClient::sendMessageWithHistory(const String& message, 
                                        const Array<std::pair<String, String>>& history,
                                        ResponseCallback callback)
{
    // Add message to the queue
    MessageRequest request;
    request.message = message;
    request.history = history;
    request.callback = std::move(callback);
    
    {
        const ScopedLock sl(queueLock);
        messageQueue.add(std::move(request));
    }
    
    // Notify the thread
    waitEvent.signal();
}

bool ClaudeClient::isConnected() const
{
    return connected;
}

String ClaudeClient::getServiceName() const
{
    return "Claude AI (" + model + ")";
}

void ClaudeClient::run()
{
    while (!threadShouldExit())
    {
        // Wait for a message to process
        waitEvent.wait(-1);
        
        if (threadShouldExit())
            break;
        
        // Process all messages in the queue
        while (!threadShouldExit())
        {
            MessageRequest request;
            
            {
                const ScopedLock sl(queueLock);
                if (messageQueue.isEmpty())
                    break;
                    
                request = std::move(messageQueue.getReference(0));
                messageQueue.remove(0);
            }
            
            // Process the message
            var requestBody = createRequestBody(request.message, request.history);
            
            // Convert to JSON
            String requestJson = JSON::toString(requestBody);
            
            // Create URL for the Anthropic API with explicit scheme and host
            URL url;
            url = URL("https://api.anthropic.com/v1/messages");
            
            // Set up headers with special attention to the API key format
            juce::String headersString;
            headersString += "Content-Type: application/json\r\n";
            headersString += "Accept: application/json\r\n";
            headersString += "anthropic-version: 2023-06-01\r\n";
            headersString += "x-api-key: " + apiKey + "\r\n";
            
            // Debug API key (securely)
            std::cout << "[ClaudeClient] API key length: " << apiKey.length() << std::endl;
            if (apiKey.isEmpty()) {
                std::cout << "[ClaudeClient] WARNING: Empty API key!" << std::endl;
            } else if (apiKey == "YOUR_API_KEY") {
                std::cout << "[ClaudeClient] WARNING: Using placeholder API key!" << std::endl;
            } else {
                std::cout << "[ClaudeClient] Using API key starting with: " << apiKey.substring(0, 4) << "..." << std::endl;
            }
            
            // Debug output
            std::cout << "[ClaudeClient] Sending request to: " << url.toString(false) << std::endl;
            std::cout << "[ClaudeClient] Request headers: " << headersString << std::endl;
            std::cout << "[ClaudeClient] Request body: " << requestJson << std::endl;
            
            // Make the HTTP request using the URL class directly
            
            // Variables for tracking response status
            int statusCode = 0;
            StringPairArray responseHeaders;
            
            // Set up the stream options and store the result
            URL::InputStreamOptions options(URL::ParameterHandling::inPostData);
            auto optionsWithHeaders = options.withExtraHeaders(headersString);
            auto optionsWithTimeout = optionsWithHeaders.withConnectionTimeoutMs(30000);
            auto optionsWithStatusCode = optionsWithTimeout.withStatusCode(&statusCode);
            auto finalOptions = optionsWithStatusCode.withResponseHeaders(&responseHeaders);
                  
            // Create a stream for the response with the properly configured options
            std::unique_ptr<InputStream> stream = url.withPOSTData(requestJson).createInputStream(finalOptions);
            
            // Debug output for response
            if (stream == nullptr) {
                std::cout << "[ClaudeClient] Failed to create input stream. Status code: " << statusCode << std::endl;
                
                // Log headers for debugging
                std::cout << "[ClaudeClient] Response headers: " << responseHeaders.getDescription() << std::endl;
            } else {
                std::cout << "[ClaudeClient] Successfully created input stream. Status code: " << statusCode << std::endl;
            }
            
            // No need to store a task reference with the direct approach
            
            // Wait for completion
            bool success = false;
            String response;
            
            if (stream != nullptr)
            {
                // Read the response
                response = stream->readEntireStreamAsString();
                
                // Debug output for response content
                std::cout << "[ClaudeClient] Response content: " << response << std::endl;
                
                if (response.isNotEmpty())
                {
                    success = true;
                    
                    // Parse the JSON response
                    var responseJson;
                    try {
                        Result result = JSON::parse(response, responseJson);
                        
                        if (result.wasOk())
                        {
                            // Check for error response
                            if (responseJson.hasProperty("error"))
                            {
                                success = false;
                                String errorType = responseJson["error"].hasProperty("type") ? 
                                    responseJson["error"]["type"].toString() : "unknown";
                                String errorMessage = responseJson["error"].hasProperty("message") ? 
                                    responseJson["error"]["message"].toString() : "Unknown error";
                                
                                response = "Claude API Error: " + errorType + " - " + errorMessage;
                                std::cout << "[ClaudeClient] Error from API: " << response << std::endl;
                            }
                            // Check for content in the expected format
                            else if (responseJson.hasProperty("content") && 
                                responseJson["content"].isArray() &&
                                responseJson["content"].getArray()->size() > 0)
                            {
                                var content = responseJson["content"];
                                String extractedText;
                                
                                // Extract text from the content array
                                for (int i = 0; i < content.getArray()->size(); ++i)
                                {
                                    var item = content[i];
                                    if (item.hasProperty("type") && 
                                        item["type"] == "text" && 
                                        item.hasProperty("text"))
                                    {
                                        extractedText += item["text"].toString();
                                    }
                                }
                                
                                // Only replace the response if we extracted something
                                if (extractedText.isNotEmpty())
                                {
                                    response = extractedText;
                                    std::cout << "[ClaudeClient] Successfully extracted text: " << response.substring(0, 100) << "..." << std::endl;
                                }
                                else 
                                {
                                    response = "No text content found in Claude response";
                                    success = false;
                                    std::cout << "[ClaudeClient] No text content found in response" << std::endl;
                                }
                            }
                            else
                            {
                                // Dump the response structure for debugging
                                std::cout << "[ClaudeClient] Unexpected response structure. Keys: ";
                                if (auto* obj = responseJson.getDynamicObject())
                                {
                                    for (auto& prop : obj->getProperties())
                                        std::cout << prop.name.toString() << ", ";
                                }
                                std::cout << std::endl;
                                
                                response = "Error parsing Claude response: Unexpected structure";
                                success = false;
                            }
                        }
                        else
                        {
                            response = "Error parsing Claude response JSON: " + result.getErrorMessage();
                            success = false;
                            std::cout << "[ClaudeClient] JSON parse error: " << result.getErrorMessage() << std::endl;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        response = "Exception parsing JSON response: " + String(e.what());
                        success = false;
                    }
                }
                else
                {
                    success = false;
                    response = "Failed to connect to Claude API";
                }
                
                // No need to clear a task reference with the direct approach
            }
            
            // Call the callback on the message thread
            MessageManager::callAsync([request, success, response]() {
                if (request.callback)
                    request.callback(success, response);
            });
        }
    }
}

var ClaudeClient::createRequestBody(const String& message, 
                                   const Array<std::pair<String, String>>& history)
{
    DynamicObject* requestObj = new DynamicObject();
    
    // Set the model
    requestObj->setProperty("model", model);
    
    // Set max tokens
    requestObj->setProperty("max_tokens", 4000);
    
    // Create messages array
    Array<var> messagesArray;
    
    // Add history messages
    for (auto& turn : history)
    {
        // User message
        DynamicObject* userMsgObj = new DynamicObject();
        userMsgObj->setProperty("role", "user");
        userMsgObj->setProperty("content", turn.first);
        messagesArray.add(var(userMsgObj));
        
        // Assistant message
        DynamicObject* assistantMsgObj = new DynamicObject();
        assistantMsgObj->setProperty("role", "assistant");
        assistantMsgObj->setProperty("content", turn.second);
        messagesArray.add(var(assistantMsgObj));
    }
    
    // Add the current message
    DynamicObject* userMsgObj = new DynamicObject();
    userMsgObj->setProperty("role", "user");
    userMsgObj->setProperty("content", message);
    messagesArray.add(var(userMsgObj));
    
    // Set the messages array
    requestObj->setProperty("messages", messagesArray);
    
    // Add system prompt for music generation
    requestObj->setProperty("system", "You are an AI assistant specialized in music theory and composition. "
                                    "Help the user generate musical patterns, understand music theory concepts, "
                                    "and create interesting musical ideas. When asked to generate patterns, "
                                    "provide them in a clear, structured format that can be parsed by a music system.");
    
    // Set temperature
    requestObj->setProperty("temperature", 0.7);
    
    return var(requestObj);
}

} // namespace covertracks