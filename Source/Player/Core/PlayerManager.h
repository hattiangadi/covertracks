// PlayerManager.h - Update Player struct to ensure proper plugin state handling
#pragma once

#include <JuceHeader.h>
#include "../../Plugins/PluginGraph.h"

class PlayerManager
{
public:
    PlayerManager();
    ~PlayerManager();
    
    // PlayerCapabilities struct has been removed
    
    struct PlayerMetadata
    {
        String name;
        String category;
        String description;
        Time created;
        Time lastModified;
        
        // Serialize to/from JSON
        var toVar() const;
        void fromVar(const var& v);
    };
    
    struct Player
    {
        PlayerMetadata metadata;
        
        // Graph state (connections between plugins)
        std::unique_ptr<XmlElement> graphState;
        
        // Create a JSON representation of this player
        var toVar() const;
        
        // Create a Player from JSON
        static std::unique_ptr<Player> fromVar(const var& v);
    };
    
    // Get all available players
    Array<Player*> getPlayers() const;
    
    // Save a player configuration
    bool savePlayer(Player* player);
    
    // Create a new player with default settings
    std::unique_ptr<Player> createPlayer();
    
    // Load a player from a file
    std::unique_ptr<Player> loadPlayer(const File& file);
    
    // Delete a player
    bool deletePlayer(Player* player);
    
    // Add a player to the collection (takes ownership)
    void addPlayer(std::unique_ptr<Player> player);
    
    // Get the directory where players are stored
    static File getPlayersDirectory();
    
private:
    OwnedArray<Player> players;
    
    // Load all available players from the players directory
    void loadAllPlayers();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerManager)
};