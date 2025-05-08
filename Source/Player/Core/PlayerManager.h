// PlayerManager.h Updates

#pragma once

#include <JuceHeader.h>
#include "../Core/Player.h" // Include the new Player class
#include "../../Plugins/PluginGraph.h"

namespace covertracks {

class PlayerManager
{
public:
    PlayerManager();
    ~PlayerManager();
    
    // Get all players
    Array<Player*> getPlayers() const;
    
    // Add a player to the manager
    void addPlayer(std::unique_ptr<Player> player);
    
    // Save a player to disk
    bool savePlayer(Player* player);
    
    // Create a new player with default settings
    std::unique_ptr<Player> createPlayer();
    
    // Load a player from a file
    std::unique_ptr<Player> loadPlayer(const File& file);
    
    // Delete a player
    bool deletePlayer(Player* player);
    
private:
    // Get the directory where players are stored
    File getPlayersDirectory();
    
    // Load all players from disk
    void loadAllPlayers();
    
    // List of managed players
    OwnedArray<Player> players;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerManager)
};

} // namespace covertracks
