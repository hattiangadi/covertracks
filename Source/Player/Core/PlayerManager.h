// PlayerManager.h Updates

#pragma once

#include <JuceHeader.h>
#include "../Core/Player.h" // Include the new Player class
#include "../../Plugins/PluginGraph.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../Audio/AudioEngine.h"

namespace covertracks {

    class PlayerManager
    {
    public:
        /**
         * @brief Construct a new Player Manager
         * 
         * Creates and owns its own AudioPluginFormatManager and KnownPluginList
         */
        PlayerManager();
                      
        ~PlayerManager();
        
        // Get all players
        Array<Player*> getPlayers() const;
        
        // Add a player to the manager
        void addPlayer(std::unique_ptr<Player> player);
        
        // Save a player to disk
        bool savePlayer(Player* player);
        
        // Save a player to disk and take ownership of it
        // This should be used when saving a player that is owned by another component
        bool savePlayerAndTakeOwnership(std::unique_ptr<Player> player);
        
        // Create a new player with default settings
        std::unique_ptr<Player> createPlayer();
        
        // Load a player from a file
        std::unique_ptr<Player> loadPlayer(const File& file);
        
        // Delete a player
        bool deletePlayer(Player* player);
        
        // Get the audio engine
        AudioEngine* getAudioEngine() const { return audioEngine.get(); }
        
        // Get the plugin format manager
        AudioPluginFormatManager& getFormatManager() const { return *formatManager; }
        
        // Get the known plugin list (both methods provided for consistency)
        KnownPluginList& getKnownPluginList() const { return *knownPluginList; }
        KnownPluginList& getPluginList() const { return *knownPluginList; }
        
        // Get the internal plugin types
        const std::vector<PluginDescription>& getInternalPluginTypes() const { return internalTypes; }
        
        // Player state management
        void activatePlayer(Player* player);
        void deactivatePlayer(Player* player);
        bool isPlayerActive(Player* player) const;
        Player* getActivePlayer() const { return activePlayer; }
        
    private:
        // Get the directory where players are stored
        File getPlayersDirectory();
        
        // Load all players from disk
        void loadAllPlayers();
        
        // Plugin environment owned by the PlayerManager
        std::unique_ptr<AudioPluginFormatManager> formatManager;
        std::unique_ptr<KnownPluginList> knownPluginList;
        
        // AudioEngine is owned by the PlayerManager
        std::unique_ptr<AudioEngine> audioEngine;
        
        // Own the internal plugin format to ensure proper lifecycle management
        std::unique_ptr<InternalPluginFormat> internalPluginFormat;
        
        // Store internal plugin types for use throughout the application
        std::vector<PluginDescription> internalTypes;
        
        // Currently active player (if any)
        Player* activePlayer = nullptr;
        
        // List of managed players
        OwnedArray<Player> players;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerManager)
    };

} // namespace covertracks
