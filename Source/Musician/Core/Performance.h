#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"
#include "Song.h"
#include "EnsembleManager.h"

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
     * @brief Get the generated patterns for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @return std::shared_ptr<MidiMessageSequence> The generated pattern, or nullptr if not found
     */
    std::shared_ptr<juce::MidiMessageSequence> getGeneratedPattern(const juce::String& roleName) const;
    
    /**
     * @brief Set the generated pattern for a specific musician role
     * 
     * @param roleName The name of the musician role
     * @param pattern The generated pattern
     */
    void setGeneratedPattern(const juce::String& roleName, std::shared_ptr<juce::MidiMessageSequence> pattern);
    
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
    void clearGeneratedPatterns();
    
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
    
private:
    juce::String name;
    juce::String uniqueID;
    std::shared_ptr<Song> song;
    std::shared_ptr<Chart> chart;
    juce::String ensembleName;
    juce::String instructions;
    
    // Performance-specific attributes
    juce::String key;
    juce::String genre;
    juce::String mood;
    
    // Maps musician role names to their generated patterns
    std::map<juce::String, std::shared_ptr<juce::MidiMessageSequence>> generatedPatterns;
    
    // Maps musician role names to their status messages
    std::map<juce::String, juce::String> statusMessages;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Performance)
};

} // namespace covertracks
