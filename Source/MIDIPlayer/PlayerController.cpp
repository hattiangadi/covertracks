#include "PlayerController.h"

PlayerController::PlayerController()
{
    std::cout << "Creating PlayerController" << std::endl;
}

PlayerController::~PlayerController()
{
    std::cout << "Destroying PlayerController" << std::endl;
    
    // Clean up all players
    stopAllPlayers();
    players.clear();
}

std::string PlayerController::createPlayer(const std::string& playerId, const nlohmann::json& config)
{
    // Check if player already exists
    if (players.find(playerId) != players.end()) {
        return "Error: Player with ID '" + playerId + "' already exists";
    }
    
    // Create configuration with player ID
    nlohmann::json playerConfig = config;
    playerConfig["player_id"] = playerId;
    
    // Get instrument type
    std::string instrumentType;
    if (config.contains("instrument_type") && config["instrument_type"].is_string()) {
        instrumentType = config["instrument_type"];
    } else {
        return "Error: Missing required 'instrument_type' field";
    }
    
    // Create player
    auto player = std::make_unique<MIDIPlayer>(playerConfig);
    
    // Initialize player
    if (!player->initialize()) {
        return "Error: Failed to initialize player with ID '" + playerId + "'";
    }
    
    // Store player
    players[playerId] = std::move(player);
    
    return "Successfully created player with ID '" + playerId + "'";
}

std::string PlayerController::deletePlayer(const std::string& playerId)
{
    // Find the player
    auto it = players.find(playerId);
    if (it == players.end()) {
        return "Error: Player with ID '" + playerId + "' not found";
    }
    
    // Stop player first
    it->second->stop();
    
    // Remove from map
    players.erase(it);
    
    return "Successfully deleted player with ID '" + playerId + "'";
}

std::string PlayerController::playPattern(const std::string& playerId, const std::string& patternId, 
                                        const juce::MidiMessageSequence& sequence)
{
    // Find the player
    MIDIPlayer* player = getPlayer(playerId);
    if (!player) {
        return "Error: Player with ID '" + playerId + "' not found";
    }
    
    // Play the pattern
    player->playMIDISequence(sequence, patternId);
    
    return "Successfully started pattern '" + patternId + "' on player '" + playerId + "'";
}

std::string PlayerController::stopPlayer(const std::string& playerId)
{
    // Find the player
    MIDIPlayer* player = getPlayer(playerId);
    if (!player) {
        return "Error: Player with ID '" + playerId + "' not found";
    }
    
    // Stop the player
    player->stop();
    
    return "Successfully stopped player '" + playerId + "'";
}

std::string PlayerController::stopAllPlayers()
{
    // Stop all players
    for (auto& pair : players) {
        pair.second->stop();
    }
    
    return "Successfully stopped all players";
}

std::string PlayerController::shutdown()
{
    if (isShuttingDown) {
        return "Shutdown already in progress";
    }
    
    isShuttingDown = true;
    
    // Stop all players
    stopAllPlayers();
    
    // Clear players
    players.clear();
    
    return "Successfully initiated shutdown";
}

void PlayerController::setPatternFinishedCallback(const std::string& playerId,
                                               std::function<void(const std::string&, const std::string&)> callback)
{
    // Find the player
    MIDIPlayer* player = getPlayer(playerId);
    if (!player) {
        std::cout << "Warning: Cannot set callback for nonexistent player '" << playerId << "'" << std::endl;
        return;
    }
    
    // Store the callback for this player
    patternCallbacks[playerId] = callback;
    
    // Set the callback - we need to capture 'this' to access the current pattern ID when triggered
    player->setPatternFinishedCallback([this, playerId]() {
        // Get the player again to ensure it still exists
        MIDIPlayer* player = getPlayer(playerId);
        if (!player) {
            std::cout << "Warning: Player '" << playerId << "' no longer exists during callback" << std::endl;
            return;
        }
        
        // Get the current pattern ID at the time the callback is invoked
        juce::String patternId = player->getCurrentPatternId();
        std::cout << "Pattern finished callback triggered for player " << playerId << ", pattern " << patternId << std::endl;
        
        // Find and call the stored callback
        auto it = patternCallbacks.find(playerId);
        if (it != patternCallbacks.end()) {
            it->second(playerId, patternId.toStdString());
        }
    });
}

MIDIPlayer* PlayerController::getPlayer(const std::string& playerId)
{
    auto it = players.find(playerId);
    if (it != players.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> PlayerController::getPlayerIds() const
{
    std::vector<std::string> ids;
    for (const auto& pair : players) {
        ids.push_back(pair.first);
    }
    return ids;
}