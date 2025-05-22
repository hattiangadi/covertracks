#pragma once

#include <JuceHeader.h>
#include "../../Player/Core/Player.h"
#include "../../Player/Core/PlayerManager.h"
#include "../../LLM/Core/LLMManager.h"
#include "MusicPatternSystem.h"

namespace covertracks {

/**
 * @brief Represents a musician role in an ensemble
 */
struct MusicianRole {
    juce::String name;       // e.g., "Lead Guitar", "Bass", "Drums"
    juce::String instrument; // Instrument type
    Player* player;          // Associated player
    juce::String pattern;    // Generated pattern for this role
    
    // Default constructor required for Array<MusicianRole>
    MusicianRole()
        : name(""), instrument(""), player(nullptr), pattern("") {}
    
    MusicianRole(const juce::String& roleName, const juce::String& instrumentType, Player* playerPtr)
        : name(roleName), instrument(instrumentType), player(playerPtr), pattern("") {}
};

/**
 * @brief Manages an ensemble of musicians playing together
 * 
 * This class coordinates multiple players with different roles,
 * handles LLM requests for each musician, and synchronizes playback.
 */
class EnsembleManager : public juce::Timer {
public:
    /**
     * @brief Construct a new Ensemble Manager
     * 
     * @param playerManager The player manager to use
     */
    EnsembleManager(PlayerManager* playerManager);
    
    /**
     * @brief Destroy the Ensemble Manager
     */
    ~EnsembleManager() override;
    
    /**
     * @brief Set the name of the ensemble
     * 
     * @param name The name to set
     */
    void setName(const juce::String& name) { ensembleName = name; }
    
    /**
     * @brief Get the name of the ensemble
     * 
     * @return juce::String The ensemble name
     */
    juce::String getName() const { return ensembleName; }
    
    /**
     * @brief Set the description of the ensemble
     * 
     * @param description The description to set
     */
    void setDescription(const juce::String& description) { ensembleDescription = description; }
    
    /**
     * @brief Get the description of the ensemble
     * 
     * @return juce::String The ensemble description
     */
    juce::String getDescription() const { return ensembleDescription; }
    
    /**
     * @brief Set the current song description
     * 
     * @param description The song description
     */
    void setCurrentSongDescription(const juce::String& description) { currentSongDescription = description; }
    
    /**
     * @brief Get the current song description
     * 
     * @return juce::String The current song description
     */
    juce::String getCurrentSongDescription() const { return currentSongDescription; }
    
    /**
     * @brief Generate patterns for all musician roles
     * 
     * @param songDescription The song description to use
     * @param callback Callback to be invoked when pattern generation is complete
     */
    void generatePatterns(const juce::String& songDescription, std::function<void(bool)> callback) {
        generateEnsemblePatterns(songDescription, callback);
    }
    
    /**
     * @brief Add a musician role to the ensemble
     * 
     * @param roleName The name of the role (e.g., "Lead Guitar")
     * @param instrumentType The type of instrument
     * @param player The player to use for this role
     * @return true If the role was added successfully
     * @return false If the role could not be added
     */
    bool addMusicianRole(const juce::String& roleName, const juce::String& instrumentType, Player* player);
    
    /**
     * @brief Remove a musician role from the ensemble
     * 
     * @param roleName The name of the role to remove
     * @return true If the role was removed successfully
     * @return false If the role could not be removed
     */
    bool removeMusicianRole(const juce::String& roleName);
    
    /**
     * @brief Get all musician roles in the ensemble
     * 
     * @return const juce::Array<MusicianRole>& The musician roles
     */
    const juce::Array<MusicianRole>& getMusicianRoles() const;
    
    /**
     * @brief Generate patterns for all musicians in the ensemble
     * 
     * @param songDescription The description of the song to generate
     * @param callback Callback to call when all patterns are generated
     */
    void generateEnsemblePatterns(const juce::String& songDescription, 
                                std::function<void(bool)> callback);
    
    /**
     * @brief Start playing the ensemble
     * 
     * @return true If playback started successfully
     * @return false If playback could not be started
     */
    bool startPlaying();
    
    /**
     * @brief Stop playing the ensemble
     */
    void stopPlaying();
    
    /**
     * @brief Check if the ensemble is currently playing
     * 
     * @return true If the ensemble is playing
     * @return false If the ensemble is not playing
     */
    bool isPlaying() const;
    
    /**
     * @brief Log the ensemble conversation to a file
     * 
     * @param songDescription The song description
     * @param ensembleResponses The responses for each musician
     */
    void logEnsembleConversation(const juce::String& songDescription, 
                               const juce::StringArray& ensembleResponses);
    
private:
    /**
     * @brief Generate a pattern for a specific musician role
     * 
     * @param role The musician role
     * @param songDescription The song description
     * @param callback Callback to call when the pattern is generated
     */
    void generatePatternForRole(MusicianRole& role, 
                              const juce::String& songDescription,
                              std::function<void(bool)> callback);
    
    /**
     * @brief Create a prompt for a specific musician role
     * 
     * @param role The musician role
     * @param songDescription The song description
     * @return juce::String The generated prompt
     */
    juce::String createPromptForRole(const MusicianRole& role, 
                                   const juce::String& songDescription);
    
    /**
     * @brief Parse a pattern from an LLM response
     * 
     * @param role The musician role
     * @param llmResponse The LLM response
     * @return juce::String The parsed pattern
     */
    juce::String parsePatternFromResponse(const MusicianRole& role, 
                                        const juce::String& llmResponse);
    
    /**
     * @brief Timer callback for synchronizing playback
     */
    void timerCallback() override;
    
    // The player manager (needed for player access)
    PlayerManager* playerManager;
    
    // Helper method to use playerManager
    void ensurePlayerManagerIsUsed() { if (playerManager) { /* This ensures playerManager is used */ } }
    
    // The musician roles in the ensemble
    juce::Array<MusicianRole> musicianRoles;
    
    // Ensemble metadata
    juce::String ensembleName;
    juce::String ensembleDescription;
    juce::String currentSongDescription;
    
    // State
    bool playing; // Renamed from isPlaying to avoid conflict with the method
    int pendingPatternRequests;
    
    // Playback state
    juce::Time playbackStartTime;
    
    // Pattern generation state
    std::function<void(bool)> patternGenerationCallback;
    
    // Logging directory
    juce::File logDirectory;
};

} // namespace covertracks
