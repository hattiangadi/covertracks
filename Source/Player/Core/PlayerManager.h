// PlayerManager.h Updates

#pragma once

#include <JuceHeader.h>
#include "../Core/TracktionPlayer.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../Audio/AudioEngineInterface.h"
#include "../../LLM/Core/LLMClient.h"
#include "../../LLM/Core/ClaudeClient.h"

namespace covertracks {

    class PlayerManager : public ChangeBroadcaster
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
        Array<TracktionPlayer*> getPlayers() const;
        
        // Add a player to the manager
        void addPlayer(std::unique_ptr<TracktionPlayer> player);
        
        // Save a player to disk
        bool savePlayer(TracktionPlayer* player);
        
        // Save a player to disk and take ownership of it
        // This should be used when saving a player that is owned by another component
        bool savePlayerAndTakeOwnership(std::unique_ptr<TracktionPlayer> player);
        
        // Create a new player with default settings
        std::unique_ptr<TracktionPlayer> createPlayer();
        
        // Load a player from a file
        std::unique_ptr<TracktionPlayer> loadPlayer(const File& file);
        
        // Delete a player
        bool deletePlayer(TracktionPlayer* player);
        
        // Get the audio engine (singleton)
        AudioEngineImpl* getAudioEngine() const { return &covertracks::getAudioEngine(); }
        
        // Get the plugin format manager (from singleton)
        AudioPluginFormatManager& getFormatManager() const { return covertracks::getAudioEngine().getFormatManager(); }
        
        // Get the known plugin list (from singleton) - both methods provided for consistency
        KnownPluginList& getKnownPluginList() const { return covertracks::getAudioEngine().getKnownPluginList(); }
        KnownPluginList& getPluginList() const { return covertracks::getAudioEngine().getKnownPluginList(); }
        
        // Get the internal plugin types directly from the format
        std::vector<PluginDescription> getInternalPluginTypes() const { 
            return internalPluginFormat ? internalPluginFormat->getAllTypes() : std::vector<PluginDescription>(); 
        }
        
        // Player state management
        void activatePlayer(TracktionPlayer* player);
        void deactivatePlayer(TracktionPlayer* player);
        bool isPlayerActive(TracktionPlayer* player) const;
        TracktionPlayer* getActivePlayer() const { return activePlayer; }
        
        // Pattern playback management
        bool playPattern(TracktionPlayer* player, const var& pattern, double tempo = 120.0);
        bool playPatternOnAllActivePlayers(const var& pattern, double tempo = 120.0);
        void stopPlayback(TracktionPlayer* player = nullptr); // If nullptr, stops all players

        /**
     * @brief Create or get shared LLM client
     * 
     * @param apiKey API key for the LLM service (if creating)
     * @return std::shared_ptr<LLMClient> The LLM client
     */
    std::shared_ptr<LLMClient> getLLMClient(const String& apiKey = String());
    
    /**
     * @brief Set the API key for the LLM client
     * 
     * @param apiKey API key for the LLM service
     */
    void setLLMApiKey(const String& apiKey);
    
    /**
     * @brief Generate a pattern for a player using the LLM
     * 
     * @param player The player to generate a pattern for
     * @param parameters Parameters for the pattern generation
     * @param callback Callback to be invoked when generation completes
     */
    void generatePatternForPlayer(TracktionPlayer* player, 
                                const var& parameters,
                                std::function<void(bool success, const var& pattern)> callback);
    
    /**
     * @brief Generate a pattern for all active players using the LLM
     * 
     * @param parameters Parameters for the pattern generation
     * @param callback Callback to be invoked when generation completes
     */
    void generatePatternForAllPlayers(const var& parameters,
                                    std::function<void(bool success, const var& pattern)> callback);

    private:
        // Get the directory where players are stored
        File getPlayersDirectory();
        
        // Load all players from disk
        void loadAllPlayers();
        
        // Plugin environment owned by the PlayerManager
        // Note: Order matters for destruction - internalPluginFormat must be destroyed AFTER formatManager
        // since formatManager holds a reference to it
        
        // AudioEngine is now a singleton - no need to own formatManager or knownPluginList
        
        // Pointer to the internal plugin format (not owned, formatManager owns it)
        InternalPluginFormat* internalPluginFormat = nullptr;
        
        // Currently active player (if any)
        TracktionPlayer* activePlayer = nullptr;
        
        // List of managed players
        OwnedArray<TracktionPlayer> players;

        std::shared_ptr<LLMClient> llmClient;
        String llmApiKey;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerManager)
    };

} // namespace covertracks
