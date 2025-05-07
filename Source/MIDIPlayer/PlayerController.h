#pragma once
#include <JuceHeader.h>
#include <map>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "MIDIPlayer.h"

class PlayerController {
public:
    PlayerController();
    ~PlayerController();
    
    // Player management
    std::string createPlayer(const std::string& playerId, const nlohmann::json& config);
    std::string deletePlayer(const std::string& playerId);
    
    // Command methods
    std::string playPattern(const std::string& playerId, const std::string& patternId, 
                          const juce::MidiMessageSequence& sequence);
    std::string stopPlayer(const std::string& playerId);
    std::string stopAllPlayers();
    std::string shutdown();
    
    // Set feedback callback
    void setPatternFinishedCallback(const std::string& playerId,
                                  std::function<void(const std::string&, const std::string&)> callback);
    
    // Player information
    std::vector<std::string> getPlayerIds() const;
    
private:
    // Helper methods
    MIDIPlayer* getPlayer(const std::string& playerId);
    
    // Player registry
    std::map<std::string, std::unique_ptr<MIDIPlayer>> players;
    
    // Store callbacks for each player
    std::map<std::string, std::function<void(const std::string&, const std::string&)>> patternCallbacks;
    
    // Track if shutdown has been called
    bool isShuttingDown = false;
};