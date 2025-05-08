#include "ClaudeClient.h"
#include "PatternProcessor.h"
#include "PromptManager.h"
#include "ClaudeLogger.h"
#include "ShorthandProcessor.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <string>
#include <regex>
#include <thread>
#include <memory>

using json = nlohmann::json;

namespace covertracks {

// Constants
const std::string API_BASE_URL = "https://api.anthropic.com/v1/messages";
const std::string DEFAULT_MODEL = "claude-3-sonnet-20240229";
const std::string SYSTEM_PROMPT_BASE = "You are an expert musician and composer helping to generate musical patterns for the covertracks application. You generate music notation using a specific shorthand format described in the user prompt. Follow the instructions and format EXACTLY.";

ClaudeClient::ClaudeClient(const std::string& apiKeyParam, const json& configParam)
    : apiKey(apiKeyParam),
      apiBaseUrl(API_BASE_URL),
      config(configParam),
      totalTokensUsed(0),
      isInitialized(false)
{
    // Initialize with default model if not specified
    if (config.contains("model") && config["model"].is_string()) {
        modelName = config["model"];
    } else {
        modelName = DEFAULT_MODEL;
    }
    
    // Log initialization
    std::cout << "ClaudeClient initialized with model: " << modelName << std::endl;
    
    // Mark as initialized if API key is provided
    isInitialized = !apiKey.empty();
    
    if (!isInitialized) {
        std::cerr << "ERROR: ClaudeClient initialized without API key" << std::endl;
    }
}

ClaudeClient::~ClaudeClient() {
    std::cout << "ClaudeClient shutting down. Total tokens used: " << totalTokensUsed.load() << std::endl;
}

json ClaudeClient::generateResponse(const std::string& prompt) {
    if (!isReady()) {
        std::cerr << "ERROR: ClaudeClient not ready (invalid API key or initialization error)" << std::endl;
        return json::object({{"error", "Client not ready"}});
    }
    
    std::string patternPrompt = prompt;

    // Check cache first (key is the input pattern prompt)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = responseCache.find(patternPrompt);
        if (it != responseCache.end()) {
            std::cout << "Using cached response for prompt" << std::endl;
            return it->second; // Return cached JSON events
        }
    }
    
    std::string systemPrompt = SYSTEM_PROMPT_BASE;
    
    std::cout << "Cache miss for Claude prompt. Requesting generation..." << std::endl;
    
    // Log the prompt
    ClaudeLogger::logPrompt("unknown", patternPrompt);
    
    // Make API request
    json apiResponse = makeApiRequest(patternPrompt, systemPrompt);
    
    // Check for errors in the response itself
    if (apiResponse.contains("error")) {
        std::cerr << "ERROR from Claude API: " << apiResponse["error"].dump() << std::endl;
        return apiResponse; // Return error response
    }
    
    // Basic validation of expected response structure
    if (!apiResponse.contains("content") || !apiResponse["content"].is_array() || 
        apiResponse["content"].empty() || !apiResponse["content"][0].contains("text")) {
        std::cerr << "ERROR: Unexpected Claude response format" << std::endl;
        std::cerr << "Raw response: " << apiResponse.dump(2) << std::endl;
        return json::object({{"error", "Unexpected response format"}});
    }

    // Extract text content
    std::string responseText = "";
    try {
        responseText = apiResponse["content"][0]["text"].get<std::string>();
        std::cout << "Claude response text extracted successfully." << std::endl;
        std::cout << "Raw Text Response:\n" << responseText << "\n--- End Raw Text ---" << std::endl;
    } catch (const json::exception& e) {
        std::cerr << "ERROR: Failed to extract text from Claude response: " << e.what() << std::endl;
        return json::object({{"error", "Failed to extract text from response"}, {"raw_response", apiResponse}});
    }

    // Calculate token usage
    if (apiResponse.contains("usage")) {
        TokenUsage usage;
        if (apiResponse["usage"].contains("input_tokens")) {
            usage.promptTokens = apiResponse["usage"]["input_tokens"];
        }
        if (apiResponse["usage"].contains("output_tokens")) {
            usage.completionTokens = apiResponse["usage"]["output_tokens"];
        }
        usage.totalTokens = usage.promptTokens + usage.completionTokens;
        totalTokensUsed += usage.totalTokens;
        std::cout << "Claude Tokens Used: Prompt=" << usage.promptTokens 
                  << ", Completion=" << usage.completionTokens 
                  << ", Total=" << usage.totalTokens << std::endl;
        std::cout << "Total Claude Tokens Accumulated: " << totalTokensUsed.load() << std::endl;
    }

    // Log the response
    json textResponse = json::object({{"text", responseText}});
    ClaudeLogger::logResponse("unknown", textResponse);

    // Return the extracted text within a JSON object
    return textResponse;
}

void ClaudeClient::generateResponseAsync(const std::string& prompt, 
                                       std::function<void(const json&)> callback) 
{
    if (!isReady()) {
        std::cerr << "ERROR: ClaudeClient not ready (async)" << std::endl;
        callback(json::object({{"error", "Client not ready"}}));
        return;
    }

    std::thread([this, prompt, callback]() {
        json result = generateResponse(prompt); // Call the synchronous version
        callback(result);
    }).detach();
}

bool ClaudeClient::updateConfiguration(const json& updatedConfig) {
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        for (auto& [key, value] : updatedConfig.items()) {
            config[key] = value;
        }
        
        if (updatedConfig.contains("model") && updatedConfig["model"].is_string()) {
            modelName = updatedConfig["model"];
        }
        
        std::cout << "ClaudeClient configuration updated" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to update configuration: " << e.what() << std::endl;
        return false;
    }
}

bool ClaudeClient::isReady() const {
    return isInitialized && !apiKey.empty();
}

std::string ClaudeClient::getModelName() const {
    std::lock_guard<std::mutex> lock(mutex);
    return modelName;
}

json ClaudeClient::makeApiRequest(const std::string& prompt, const std::string& systemPrompt) {
    std::lock_guard<std::mutex> lock(mutex);

    json payload = {
        {"model", modelName},
        {"max_tokens", 4096},
        {"messages", {
            {
                {"role", "user"},
                {"content", prompt}
            }
        }}
    };

    if (!systemPrompt.empty()) {
        payload["system"] = systemPrompt;
    }

    if (config.contains("temperature") && config["temperature"].is_number()) {
        payload["temperature"] = config["temperature"];
    } else {
        payload["temperature"] = 0.7; 
    }
    
    if (config.contains("top_p") && config["top_p"].is_number()) {
        payload["top_p"] = config["top_p"];
    }
    
    if (config.contains("top_k") && config["top_k"].is_number()) {
        payload["top_k"] = config["top_k"];
    }
    
    std::string payloadStr = payload.dump();
    std::cout << "Making Claude API request to " << apiBaseUrl.toString(false) << std::endl;
    std::cout << "User Prompt (first 200 chars): " << prompt.substr(0, 200) << "..." << std::endl;
    std::cout << "Estimated prompt tokens: " << estimateTokenCount(prompt) << std::endl;
    
    juce::URL requestUrl = apiBaseUrl;
    juce::String headersString;
    headersString += "Content-Type: application/json\r\n";
    headersString += "Accept: application/json\r\n";
    headersString += "anthropic-version: 2023-06-01\r\n";
    headersString += "x-api-key: " + juce::String(apiKey) + "\r\n";
    
    // Declare variables for status code and headers
    int statusCode = 0;
    juce::StringPairArray responseHeaders;

    juce::URL::InputStreamOptions options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders(headersString)
        .withConnectionTimeoutMs(30000)
        .withStatusCode(&statusCode)
        .withResponseHeaders(&responseHeaders);

    std::unique_ptr<juce::InputStream> stream = requestUrl.withPOSTData(payloadStr).createInputStream(options);
    
    if (!stream) {
        std::cerr << "ERROR: Failed to connect to Claude API. Status code: " << statusCode << std::endl;
        return json::object();
    }
    
    juce::String responseString = stream->readEntireStreamAsString();
    
    std::cout << "Received response from Claude API (status " << statusCode << ")" << std::endl;
    std::cout << "Raw Response (first 200 chars): " << responseString.substring(0, 200) << "..." << std::endl; 

    if (statusCode < 200 || statusCode >= 300) {
        std::cerr << "ERROR: Claude API request failed with status code: " << statusCode << std::endl;
        std::cerr << "Response body: " << responseString.toStdString() << std::endl;
        try {
            json errorJson = json::parse(responseString.toStdString());
            return {{"error", {{"code", statusCode}, {"message", errorJson}}}}; 
        } catch (...) {
             return {{"error", {{"code", statusCode}, {"message", responseString.toStdString()}}}}; 
        }
    }
    
    try {
        return json::parse(responseString.toStdString());
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse Claude API response JSON: " << e.what() << std::endl;
        std::cerr << "Response: " << responseString.toStdString() << std::endl;
        return json::object({{"error", {{"message", "Failed to parse API response JSON"}, {"details", e.what()}}}});
    }
}

int ClaudeClient::estimateTokenCount(const std::string& text) const {
    return static_cast<int>(text.length() / 4) + 1;
}

} // namespace covertracks