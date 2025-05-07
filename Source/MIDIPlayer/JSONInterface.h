#pragma once
#include <JuceHeader.h>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "PlayerController.h"

class JSONInterface {
public:
    JSONInterface();
    ~JSONInterface();
    
    // Handle JSON command
    std::string handleCommand(const std::string& jsonCommand);
    
    // Set feedback callback for patterns
    void setPatternFinishedCallback(
        std::function<void(const std::string&, const std::string&)> callback);
    
private:
    // Command handlers
    std::string handleCreatePlayer(const nlohmann::json& json);
    std::string handlePlayPattern(const nlohmann::json& json);
    std::string handleStopPlayer(const nlohmann::json& json);
    std::string handleShutdown(const nlohmann::json& json);
    
    // Response formatters
    std::string createSuccessResponse(const std::string& message);
    std::string createErrorResponse(const std::string& error);
    
    // MIDI conversion helpers
    juce::MidiMessageSequence convertJSONToMIDISequence(const nlohmann::json& events);
    
    // Player controller
    PlayerController controller;
    
    // Store the pattern finished callback
    std::function<void(const std::string&, const std::string&)> patternFinishedCallback;
};