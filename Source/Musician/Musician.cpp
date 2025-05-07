#include "Musician.h"
#include "ClaudeClient.h"
#include "ShorthandProcessor.h"
#include "PatternProcessor.h"
#include "PromptManager.h"  // Added include
#include "ClaudeLogger.h"  // Added include
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <uuid/uuid.h>

using json = nlohmann::json;

namespace coverband {

// Helper function to generate UUIDs for pattern IDs
static std::string generateUUID() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];  // UUID is 36 characters + null terminator
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}


Musician::Musician(const std::string& musicianId, const std::string& instrumentType, JSONInterface* interface)
: id(musicianId), type(instrumentType), jsonInterface(interface), initialized(false)
{
    // Initialize empty musical state
    musicalState = json::object();
    musicalState["currentSection"] = "intro";
    musicalState["tempo"] = 120.0;
    musicalState["key"] = "C";
    
    // Initialize empty instrument config
    instrumentConfig = json::object();
    
    // Initialize empty feedback history
    feedbackHistory = json::array();
    
    std::cout << "Creating " << type << " musician with ID: " << id << std::endl;
}

bool Musician::initialize(const json& config) {
    std::cout << "Initializing " << type << " musician with ID: " << id << std::endl;
    
    // Apply general configuration parameters to musical state
    if (config.contains("initialState") && config["initialState"].is_object()) {
        musicalState.merge_patch(config["initialState"]);
    }
    
    // Store instrument-specific configuration
    if (config.contains(type) && config[type].is_object()) {
        instrumentConfig = config[type];
        std::cout << "Loaded " << type << "-specific configuration" << std::endl;
    }
    
    // Load prompt templates
    loadPromptTemplates(config);
    
    // Create the player if it doesn't exist
    if (!playerCreated) {
        // Create player configuration
        json playerConfig = createPlayerConfig();
        
        // Create player command
        json createCommand = {
            {"command", "create_player"},
            {"player_id", id},
            {"instrument_type", type},
            {"config", playerConfig}
        };
        
        // Send command to create player
        json response = sendCommand(createCommand);
        
        // Check for success
        if (response["status"] == "success") {
            playerCreated = true;
            std::cout << "Player created successfully for " << type << " musician " << id << std::endl;
        } else {
            std::cerr << "Failed to create player: " << response["message"] << std::endl;
            return false;
        }
    }
    
    initialized = true;
    return true;
}

void Musician::setLLMClient(std::shared_ptr<LLMClient> client) {
    llmClient = client;
    
    if (llmClient) {
        std::cout << "LLM client set for " << type << " musician: " 
                 << llmClient->getModelName() << std::endl;
    } else {
        std::cout << "Warning: Null LLM client set for " << type << " musician" << std::endl;
    }
}

void Musician::setPlayerInterface(std::function<std::string(const std::string&)> callback) {
    playerInterfaceCallback = std::move(callback);
    std::cout << "Player interface callback set for " << type << " musician" << std::endl;
}

json Musician::generateAndPlayPattern(const json& parameters) {
    // Generate the pattern
    json pattern = generatePattern(parameters);
    
    // Play the pattern
    json response = sendCommand(pattern);
    
    // Check for success
    if (response["status"] == "success") {
        std::cout << "Pattern sent successfully to player" << std::endl;
    } else {
        std::cerr << "Failed to send pattern to player: " << response["message"] << std::endl;
    }
    
    return pattern;
}


json Musician::generatePattern(const json& parameters) {
    if (!isReady()) {
        std::cerr << "Musician not ready to generate patterns" << std::endl;
        return json::object({{"error", "Musician not ready"}});
    }
    
    // Adapt parameters based on feedback history
    json adaptedParams = adaptParameters(parameters);
    
    // Merge parameters with musical state
    json mergedParams = musicalState;
    if (adaptedParams.is_object()) {
        mergedParams.merge_patch(adaptedParams);
    }
    
    // Add instrument config if available
    if (!instrumentConfig.is_null()){
         mergedParams["instrumentConfig"] = instrumentConfig;
    }

    // Determine style
    std::string style = mergedParams.value("style", "default"); 
    
    // Create the prompt using the PromptManager
    std::string prompt = PromptManager::createPrompt(type, mergedParams, style);
    
    if (prompt.empty()) {
        std::cerr << "Error creating prompt for pattern generation." << std::endl;
        return json::object({{"error", "Failed to create prompt"}});
    }

    std::cout << "[Musician::generatePattern] Generating pattern for " << type << " with prompt:\n--- START PROMPT ---\n" 
              << prompt 
              << "\n--- END PROMPT ---" << std::endl;
    
    // Generate response using LLM client
    json llmResponse = llmClient->generateResponse(prompt);
    
    // Check for errors from the LLM client itself
    if (llmResponse.contains("error")) {
        std::cerr << "Error from LLM client: " << llmResponse["error"].dump() << std::endl;
        return llmResponse;
    }
    
    // Extract the text response
    std::string shorthandResponse = "";
    if (llmResponse.contains("text") && llmResponse["text"].is_string()) {
        shorthandResponse = llmResponse["text"].get<std::string>();
        std::cout << "[Musician::generatePattern] Received raw text response from LLM:\n--- START RAW TEXT ---\n" 
                  << shorthandResponse 
                  << "\n--- END RAW TEXT ---" << std::endl;
    } else {
        std::cerr << "Error: LLM response does not contain valid 'text' field." << std::endl;
        return json::object({{"error", "Invalid LLM response format"}, {"raw_response", llmResponse}});
    }
    
    // Generate a unique ID for this specific pattern instance and assign to member
    this->currentPatternId = generateUUID(); 

    // Process the response - always using ShorthandProcessor
    json pattern;
    try {
        // Get expected number of measures from parameters if available
        int expectedMeasures = 0;
        if (mergedParams.contains("totalBars") && mergedParams["totalBars"].is_number()) {
            expectedMeasures = mergedParams["totalBars"].get<int>();
        }
        
        // Convert shorthand to JSON command using ShorthandProcessor
        pattern = ShorthandProcessor::shorthandToJson(
            shorthandResponse, 
            type, 
            id, 
            this->currentPatternId, 
            mergedParams.value("tempo", 120.0),
            expectedMeasures  // Pass expected measures for validation
        );
    } catch (const std::exception& e) {
        std::cerr << "[Musician::generatePattern] Exception during pattern processing: " << e.what() << std::endl;
        return json::object({{"error", "Exception during pattern processing"}, {"details", e.what()}});
    }
                
    // Log the generated pattern through ClaudeLogger
    ClaudeLogger::logPattern(type, pattern);
    
    // Log the generated pattern command
    std::cout << "[Musician::generatePattern] Generated command JSON for " << type << " (Pattern ID: " << this->currentPatternId << "):\n--- START COMMAND JSON ---\n" 
              << pattern.dump(2) // Log the processed pattern JSON
              << "\n--- END COMMAND JSON ---" << std::endl;
    
    lastGeneratedPatternCommand = pattern; // Assign the final processed pattern JSON
    return pattern; // Return the processed pattern JSON
}

void Musician::processFeedback(const json& feedback) {
    std::cout << "Processing feedback for " << type << " musician: " << id << std::endl;
    
    // Add to feedback history with timestamp
    json feedbackEntry = feedback;
    feedbackEntry["receivedAt"] = std::time(nullptr);
    feedbackHistory.push_back(feedbackEntry);
    
    // Limit history size
    const size_t MAX_HISTORY = 20;
    if (feedbackHistory.size() > MAX_HISTORY) {
        // Remove oldest entries
        feedbackHistory.erase(feedbackHistory.begin(), 
                            feedbackHistory.begin() + (feedbackHistory.size() - MAX_HISTORY));
    }
    
    // Extract event type from feedback
    std::string eventType = feedback.value("event", "unknown");
    
    if (eventType == "pattern_completed") {
        // Pattern completed, extract pattern ID
        std::string patternId = feedback.value("pattern_id", "");
        
        std::cout << "Pattern completed: " << patternId << std::endl;
        
        // Call pattern completed handler
        onPatternCompleted(patternId);
        
        // Update state if audio features are available
        if (feedback.contains("audio_features") && feedback["audio_features"].is_object()) {
            json audioFeatures = feedback["audio_features"];
            
            // Update tempo if detected
            if (audioFeatures.contains("detected_tempo")) {
                double detectedTempo = audioFeatures["detected_tempo"];
                double currentTempo = musicalState.value("tempo", 120.0);
                
                // Smooth adaptation (70% current, 30% detected)
                musicalState["tempo"] = 0.7 * currentTempo + 0.3 * detectedTempo;
            }
            
            // Update key if estimated
            if (audioFeatures.contains("estimated_key")) {
                musicalState["key"] = audioFeatures["estimated_key"];
            }
            
            // Store analysis in state for future reference
            musicalState["lastAnalysis"] = audioFeatures;
        }
    }
}

bool Musician::isReady() const {
    return initialized && llmClient && llmClient->isReady() && 
           (jsonInterface || playerInterfaceCallback);
}

json Musician::createPlayerConfig() {
    // Create base configuration for this musician's player
    json config = json::object();
    
    // Set instrument-specific defaults based on type
    if (type == "piano") {
        config["sample_name"] = "piano_a4.wav";
        config["polyphony"] = 8;
    }
    else if (type == "bass") {
        config["sample_name"] = "bass_c1.wav";
        config["polyphony"] = 1;
    }
    else if (type == "drums") {
        config["sample_name"] = "drum_kick.wav";
        config["polyphony"] = 16;
    }
    else if (type == "guitar") {
        config["sample_name"] = "guitar_e4.wav";
        config["polyphony"] = 6;
    }
    else {
        // Default for unknown instrument types
        config["sample_name"] = type + "_default.wav";
        config["polyphony"] = 4;
    }
    
    // Set plugin ID to sampler by default
    config["plugin_id"] = "sampler_plugin";
    
    // Override with any settings from instrumentConfig
    if (instrumentConfig.contains("player") && instrumentConfig["player"].is_object()) {
        config.merge_patch(instrumentConfig["player"]);
    }
    
    return config;
}

void Musician::onPatternCompleted(const std::string& patternId) {
    // Base implementation just logs the completion
    std::cout << "Pattern " << patternId << " completed for " << type << " musician " << id << std::endl;
    
    // Reset current pattern ID if it matches the completed one
    if (currentPatternId == patternId) {
        currentPatternId = "";
    }
}

json Musician::sendCommand(const json& command) {
    return sendCommandString(command.dump());
}

json Musician::sendCommandString(const std::string& commandString) {
    // If we have a callback function, use it
    if (playerInterfaceCallback) {
        std::string responseString = playerInterfaceCallback(commandString);
        
        // Parse response
        try {
            return json::parse(responseString);
        }
        catch (const json::exception& e) {
            std::cerr << "Error parsing response: " << e.what() << std::endl;
            json errorResponse = {
                {"status", "error"},
                {"message", "Error parsing response: " + std::string(e.what())}
            };
            return errorResponse;
        }
    }
    // Otherwise, use the JSONInterface if available
    else if (jsonInterface) {
        // Send command to JSON interface
        std::string responseString = jsonInterface->handleCommand(commandString);
        
        // Parse response
        try {
            return json::parse(responseString);
        }
        catch (const json::exception& e) {
            std::cerr << "Error parsing response: " << e.what() << std::endl;
            json errorResponse = {
                {"status", "error"},
                {"message", "Error parsing response: " + std::string(e.what())}
            };
            return errorResponse;
        }
    }
    else {
        // No way to send command
        std::cerr << "No JSON interface or callback available, cannot send command" << std::endl;
        json errorResponse = {
            {"status", "error"},
            {"message", "No JSON interface or callback available"}
        };
        return errorResponse;
    }
}

void Musician::loadPromptTemplates(const json& config) {
    // Clear existing templates
    promptTemplates.clear();
    
    // Import predefined templates for this instrument type
    json predefinedTemplates = PromptTemplates::getTemplates(type);
    for (auto it = predefinedTemplates.begin(); it != predefinedTemplates.end(); ++it) {
        if (it.value().is_string()) {
            promptTemplates[it.key()] = it.value();
        }
    }
    
    // Load custom templates from instrument config
    if (instrumentConfig.contains("promptTemplates") && instrumentConfig["promptTemplates"].is_object()) {
        auto templates = instrumentConfig["promptTemplates"];
        for (auto it = templates.begin(); it != templates.end(); ++it) {
            if (it.value().is_string()) {
                promptTemplates[it.key()] = it.value();
            }
        }
    }
    
    // Load custom templates from general config
    if (config.contains("promptTemplates") && config["promptTemplates"].is_object()) {
        auto templates = config["promptTemplates"];
        for (auto it = templates.begin(); it != templates.end(); ++it) {
            if (it.value().is_string()) {
                promptTemplates[it.key()] = it.value();
            }
        }
    }
    
    std::cout << "Loaded " << promptTemplates.size() << " prompt templates for " 
             << type << " musician" << std::endl;
}

json Musician::adaptParameters(const json& parameters) {
    // Start with original parameters
    json adaptedParams = parameters;
    
    // Apply learning from feedback history
    if (!feedbackHistory.empty()) {
        // Extract recent feedback
        const int MAX_FEEDBACK_TO_CONSIDER = 5;
        
        // Get up to the last 5 feedback entries
        auto start = feedbackHistory.size() <= MAX_FEEDBACK_TO_CONSIDER ? 
                    feedbackHistory.begin() : 
                    feedbackHistory.end() - MAX_FEEDBACK_TO_CONSIDER;
                    
        auto end = feedbackHistory.end();
        
        // Analyze recent feedback for trends
        double tempoSum = 0.0;
        int tempoCount = 0;
        std::map<std::string, int> keyFrequency;
        
        for (auto it = start; it != end; ++it) {
            if (it->contains("audio_features") && (*it)["audio_features"].is_object()) {
                auto features = (*it)["audio_features"];
                
                // Extract tempo if available
                if (features.contains("detected_tempo")) {
                    tempoSum += features["detected_tempo"].get<double>();
                    tempoCount++;
                }
                
                // Extract key if available
                if (features.contains("estimated_key")) {
                    std::string key = features["estimated_key"];
                    keyFrequency[key]++;
                }
            }
        }
        
        // Apply adaptations if we have enough data
        if (tempoCount > 0) {
            // Calculate average tempo
            double averageTempo = tempoSum / tempoCount;
            
            // Adjust tempo slightly toward the average (subtle adaptation)
            if (parameters.contains("tempo")) {
                double originalTempo = parameters["tempo"];
                // 80% original, 20% learned
                adaptedParams["tempo"] = 0.8 * originalTempo + 0.2 * averageTempo;
            } else {
                adaptedParams["tempo"] = averageTempo;
            }
        }
        
        // Use most frequent key if detected
        if (!keyFrequency.empty()) {
            // Find most frequent key
            std::string mostFrequentKey;
            int maxFrequency = 0;
            
            for (const auto& pair : keyFrequency) {
                if (pair.second > maxFrequency) {
                    mostFrequentKey = pair.first;
                    maxFrequency = pair.second;
                }
            }
            
            // Only adapt key if we have a strong signal
            if (maxFrequency >= 3 && !parameters.contains("key")) {
                adaptedParams["key"] = mostFrequentKey;
            }
        }
    }
    
    return adaptedParams;
}

std::unique_ptr<Musician> MusicianFactory::createMusician(
    const std::string& type, 
    const std::string& id,
    JSONInterface* jsonInterface,
    const json& config)
{
    // Create a Musician instance with the given parameters
    auto musician = std::make_unique<Musician>(id, type, jsonInterface);
    
    // Initialize the musician with the provided config
    if (musician) {
        musician->initialize(config);
    }
    
    return musician;
}

} // namespace coverband