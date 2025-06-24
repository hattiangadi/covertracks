#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"
#include "Song.h"
#include "EnsembleManager.h"

// Forward declarations for playback system
namespace covertracks {
    class PlayerManager;
    class Player;
}

namespace covertracks {

/**
 * @brief Represents a musical performance with a song, ensemble, and performance instructions
 * 
 * A Performance contains all the information needed to generate and play a musical performance,
 * including the song, ensemble, performance instructions, and the generated patterns for each musician.
 */
class Performance
{
public:
    /**
     * @brief Construct a new Performance
     * 
     * @param name The name of the performance
     */
    Performance(const juce::String& name = "New Performance");
    
    /**
     * @brief Destroy the Performance
     */
    ~Performance();
    
    /**
     * @brief Get the name of the performance
     * 
     * @return juce::String The name
     */
    juce::String getName() const { return name; }
    
    /**
     * @brief Set the name of the performance
     * 
     * @param newName The new name
     */
    void setName(const juce::String& newName) { name = newName; }
    
    /**
     * @brief Get the description of the performance
     * 
     * @return juce::String The description
     */
    juce::String getDescription() const { return description; }
    
    /**
     * @brief Set the description of the performance
     * 
     * @param newDescription The new description
     */
    void setDescription(const juce::String& newDescription) { description = newDescription; }
    
    /**
     * @brief Get the chart for this performance
     * 
     * @return std::shared_ptr<Chart> The chart
     */
    std::shared_ptr<Chart> getChart() const { return chart; }
    
    /**
     * @brief Set the chart for this performance
     * 
     * @param newChart The new chart
     */
    void setChart(std::shared_ptr<Chart> newChart) { chart = newChart; }
    
    /**
     * @brief Get the song for this performance
     * 
     * @return std::shared_ptr<Song> The song
     */
    std::shared_ptr<Song> getSong() const { return song; }
    
    /**
     * @brief Set the song for this performance
     * 
     * @param newSong The new song
     */
    void setSong(std::shared_ptr<Song> newSong) { song = newSong; }
    
    /**
     * @brief Get the ensemble name for this performance
     * 
     * @return juce::String The ensemble name
     */
    juce::String getEnsembleName() const { return ensembleName; }
    
    /**
     * @brief Set the ensemble name for this performance
     * 
     * @param newEnsembleName The new ensemble name
     */
    void setEnsembleName(const juce::String& newEnsembleName) { ensembleName = newEnsembleName; }
    
    /**
     * @brief Get the performance instructions
     * 
     * @return juce::String The performance instructions
     */
    juce::String getInstructions() const { return instructions; }
    
    /**
     * @brief Set the performance instructions
     * 
     * @param newInstructions The new instructions
     */
    void setInstructions(const juce::String& newInstructions) { instructions = newInstructions; }
    
    /**
     * @brief Get the key for this performance
     * 
     * @return juce::String The key
     */
    juce::String getKey() const { return key; }
    
    /**
     * @brief Set the key for this performance
     * 
     * @param newKey The new key
     */
    void setKey(const juce::String& newKey) { key = newKey; }
    
    /**
     * @brief Get the genre for this performance
     * 
     * @return juce::String The genre
     */
    juce::String getGenre() const { return genre; }
    
    /**
     * @brief Set the genre for this performance
     * 
     * @param newGenre The new genre
     */
    void setGenre(const juce::String& newGenre) { genre = newGenre; }
    
    /**
     * @brief Get the mood for this performance
     * 
     * @return juce::String The mood
     */
    juce::String getMood() const { return mood; }
    
    /**
     * @brief Set the mood for this performance
     * 
     * @param newMood The new mood
     */
    void setMood(const juce::String& newMood) { mood = newMood; }
    
    /**
     * @brief Get the ensemble for this performance
     * 
     * @return juce::Array<MusicianRole> The ensemble roles (derived from ensemble name)
     */
    juce::Array<MusicianRole> getEnsemble() const;
    
    /**
     * @brief Set the ensemble for this performance by name
     * 
     * @param newEnsembleName The name of the ensemble to use
     */
    void setEnsemble(const juce::String& newEnsembleName) { 
        ensembleName = newEnsembleName; 
        ensembleCacheValid = false; // Invalidate cache
    }
    
    /**
     * @brief Refresh the cached ensemble roles from disk
     */
    void refreshEnsembleCache();
    
    /**
     * @brief Get the generated chart for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @return juce::var The generated chart, or empty var if not found
     */
    juce::var getGeneratedChart(const juce::String& roleName) const;
    
    /**
     * @brief Set the generated chart for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @param chart The generated chart
     */
    void setGeneratedChart(const juce::String& roleName, const juce::var& generatedChart);
    
    /**
     * @brief Get the LLM-generated pattern for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @return juce::String The LLM pattern, or empty string if not found
     */
    juce::String getLLMPattern(const juce::String& roleName) const;
    
    /**
     * @brief Set the LLM-generated pattern for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @param pattern The LLM-generated pattern
     */
    void setLLMPattern(const juce::String& roleName, const juce::String& pattern);
    
    /**
     * @brief Get the MIDI chart for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @return juce::var The MIDI chart, or empty var if not found
     */
    juce::var getMIDIChart(const juce::String& roleName) const;
    
    /**
     * @brief Set the MIDI chart for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @param midiChart The MIDI chart
     */
    void setMIDIChart(const juce::String& roleName, const juce::var& midiChart);
    
    /**
     * @brief Get the status message for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @return juce::String The status message
     */
    juce::String getStatusMessage(const juce::String& roleName) const;
    
    /**
     * @brief Set the status message for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @param status The status message
     */
    void setStatusMessage(const juce::String& roleName, const juce::String& status);
    
    /**
     * @brief Clear all generated patterns
     */
    void clearGeneratedCharts();
    
    /**
     * @brief Get all role names that have generated charts (for debugging)
     * 
     * @return juce::StringArray Array of role names with charts
     */
    juce::StringArray getGeneratedChartRoleNames() const;
    
    /**
     * @brief Clean up status messages for roles that have no chart data
     */
    void cleanupStaleStatusMessages();
    
    /**
     * @brief Serialize the performance to a var object
     * 
     * @return juce::var The serialized performance
     */
    juce::var toVar() const;
    
    /**
     * @brief Create a Performance from a var object
     * 
     * @param v The var object
     * @return std::shared_ptr<Performance> The created performance
     */
    static std::shared_ptr<Performance> fromVar(const juce::var& v);
    
    /**
     * @brief Get the unique ID for this performance
     * 
     * @return juce::String The unique ID
     */
    juce::String getUniqueID() const { return uniqueID; }
    
    // =========================================================================
    // Unified Playback System
    // =========================================================================
    
    /**
     * @brief Play this performance with specified roles (unified playback)
     * 
     * @param playerManager The player manager to use for Player creation
     * @param roleNames Array of role names to play (empty = play all roles)
     * @return bool true if playback started successfully
     */
    bool play(class PlayerManager* playerManager, const juce::StringArray& roleNames = {});
    
    /**
     * @brief Play a single role (convenience method)
     * 
     * @param playerManager The player manager to use
     * @param roleName Single role name to play
     * @return bool true if playback started successfully
     */
    bool play(class PlayerManager* playerManager, const juce::String& roleName);
    
    /**
     * @brief Play with provided ensemble roles (overrides cached ensemble)
     * 
     * @param playerManager The player manager to use
     * @param ensembleRoles The actual ensemble roles with player pointers
     * @param roleNames Array of role names to play (empty = play all roles)
     * @return bool true if playback started successfully
     */
    bool playWithEnsemble(class PlayerManager* playerManager, const juce::Array<MusicianRole>& ensembleRoles, const juce::StringArray& roleNames = {});
    
    /**
     * @brief Load charts for specified roles (preparation for synchronized playback)
     * 
     * @param playerManager The player manager to use
     * @param roleNames Array of role names to load (empty = load all roles)
     * @return int Number of charts successfully loaded
     */
    int loadCharts(class PlayerManager* playerManager, const juce::StringArray& roleNames = {});
    
    /**
     * @brief Load charts with provided ensemble roles (overrides cached ensemble)
     * 
     * @param playerManager The player manager to use
     * @param ensembleRoles The actual ensemble roles with player pointers
     * @param roleNames Array of role names to load (empty = load all roles)
     * @return int Number of charts successfully loaded
     */
    int loadChartsWithEnsemble(class PlayerManager* playerManager, const juce::Array<MusicianRole>& ensembleRoles, const juce::StringArray& roleNames = {});
    
    /**
     * @brief Stop all active playback
     */
    void stopAll();
    
    /**
     * @brief Check if any roles are currently playing
     * 
     * @return bool true if playback is active
     */
    bool isPlaying() const;
    
private:
    juce::String name;
    juce::String description;
    juce::String uniqueID;
    std::shared_ptr<Song> song;
    std::shared_ptr<Chart> chart;
    juce::String ensembleName;
    juce::String instructions;
    
    // Performance-specific attributes
    juce::String key;
    juce::String genre;
    juce::String mood;
    
    // Maps musician role names to their generated charts (legacy support)
    std::map<juce::String, juce::var> generatedCharts;
    
    // Maps musician role names to their LLM-generated patterns
    std::map<juce::String, juce::String> llmPatterns;
    
    // Maps musician role names to their MIDI charts
    std::map<juce::String, juce::var> midiCharts;
    
    // Maps musician role names to their status messages
    std::map<juce::String, juce::String> statusMessages;
    
    // Cached ensemble roles to avoid repeated file loading
    mutable juce::Array<MusicianRole> cachedEnsembleRoles;
    mutable bool ensembleCacheValid = false;
    
    // =========================================================================
    // Unified Playback System (Private Members)
    // =========================================================================
    
    // Active Players for current playback session (references only, not owned)
    std::map<juce::String, Player*> activePlayerRefs;
    
    // Helper methods for unified playback
    // Legacy method removed - Player is now provided directly in MusicianRole
    juce::Array<MusicianRole> filterRolesToPlay(const juce::StringArray& roleNames);
    juce::Array<MusicianRole> filterRolesToPlay(const juce::Array<MusicianRole>& ensembleRoles, const juce::StringArray& roleNames);
    bool loadChartsForRoles(const juce::Array<MusicianRole>& roles, PlayerManager* playerManager);
    bool startPlaybackForLoadedRoles();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Performance)
};

} // namespace covertracks
