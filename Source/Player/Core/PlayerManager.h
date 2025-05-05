// PlayerManager.h
#pragma once

#include <JuceHeader.h>
#include "../../Plugins/PluginGraph.h"

/**
 * A class that manages Player configurations, which consist of a PluginGraph
 * and associated metadata.
 */
class PlayerManager
{
public:
    PlayerManager();
    ~PlayerManager();
    
    struct PlayerCapabilities
    {
        Range<int> pitchRange = {36, 96}; // Default MIDI range
        StringArray techniques;
        
        // Serialize to/from JSON
        var toVar() const;
        void fromVar(const var& v);
    };
    
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
        PlayerCapabilities capabilities;
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
    
    // Get the directory where players are stored
    static File getPlayersDirectory();
    
private:
    OwnedArray<Player> players;
    
    // Load all available players from the players directory
    void loadAllPlayers();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerManager)
};
