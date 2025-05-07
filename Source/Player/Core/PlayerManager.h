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
        
        // Path to the graph file this player uses
        File graphFilePath;
        
        // Create a JSON representation of this player
        var toVar() const;
        
        // Create a Player from JSON
        static std::unique_ptr<Player> fromVar(const var& v);
        
        // Load a graph file into this player
        bool loadGraphFromFile(const File& file);
        
        // Save the graph to its current file path
        bool saveGraphToFile();
        
        // Save the graph to a new file path and update the reference
        bool saveGraphToFile(const File& file);
    };
    
    // Get all available players
    Array<Player*> getPlayers() const;
    
    // Save a player configuration
    bool savePlayer(Player* player);
    
    // Create a new player with default settings
    std::unique_ptr<Player> createPlayer();
    
    // Load a player from a file
    std::unique_ptr<Player> loadPlayer(const File& file);
    
    // Prompt user to select a player file to load
    std::unique_ptr<Player> promptForPlayerFile();
    
    // Delete a player
    bool deletePlayer(Player* player);
    
    // Add a player to the collection (takes ownership)
    void addPlayer(std::unique_ptr<Player> player);
    
    // Get the directory where players are stored (legacy support)
    static File getPlayersDirectory();

    // These methods are kept for backward compatibility
    bool setPlayersDirectory(const File& directory);
    File getUserSelectedPlayersDirectory(); 
    static File promptForPlayersDirectory();

    
private:
    OwnedArray<Player> players;
    
    // Load all available players from the players directory
    void loadAllPlayers();
    
    File customPlayersDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerManager)
};