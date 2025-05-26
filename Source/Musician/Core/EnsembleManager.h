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
        
    // Temporary methods to support PerformanceComponent
    juce::String getName() const { return name; }
    juce::String getInstrumentName() const { return instrument; }
};

/**
 * @brief Manages an ensemble of musicians playing together
 * 
 * This class coordinates multiple players with different roles,
 * handles LLM requests for each musician, and synchronizes playback.
 */
class EnsembleManager : public juce::Timer, public juce::ReferenceCountedObject {
public:
    /**
     * @brief Construct a new Ensemble Manager
     * 
     * @param playerManager The player manager to use
     */
    EnsembleManager(PlayerManager* playerManager, const juce::String& id = juce::String());
    
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
     * @brief Get the player manager used by this ensemble
     * 
     * @return PlayerManager* The player manager
     */
    PlayerManager* getPlayerManager() const { return playerManager; }
    
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
    
    /**
     * @brief Get the unique ID of this ensemble
     * 
     * @return juce::String The ensemble ID
     */
    juce::String getId() const { return ensembleId; }
    
    /**
     * @brief Convert the ensemble to a var for serialization
     * 
     * @return var The serialized ensemble
     */
    juce::var toVar() const;
    
    /**
     * @brief Create an ensemble from a var
     * 
     * @param v The serialized ensemble
     * @param playerManager The player manager to use
     * @return std::unique_ptr<EnsembleManager> The created ensemble
     */
    static std::unique_ptr<EnsembleManager> fromVar(const juce::var& v, PlayerManager* playerManager);
    
    /**
     * @brief Save the ensemble to a file
     * 
     * @param file The file to save to
     * @return true If the save was successful
     * @return false If the save failed
     */
    bool saveToFile(const juce::File& file) const;
    
    /**
     * @brief Load an ensemble from a file
     * 
     * @param file The file to load from
     * @param playerManager The player manager to use
     * @return std::unique_ptr<EnsembleManager> The loaded ensemble, or nullptr if loading failed
     */
    static std::unique_ptr<EnsembleManager> loadFromFile(const juce::File& file, PlayerManager* playerManager);
    
    /**
     * @brief Get the default directory for ensemble files
     * 
     * @return juce::File The directory
     */
    static juce::File getEnsemblesDirectory();
    
    /**
     * @brief Get all ensemble files in the default directory
     * 
     * @return juce::Array<juce::File> The ensemble files
     */
    static juce::Array<juce::File> getEnsembleFiles();
    
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
    
    // Unique ID for this ensemble
    juce::String ensembleId;
    
public:
    /**
     * @brief Play all patterns in the ensemble
     * 
     * @return true If at least one player started playing successfully
     * @return false If no players could start playing
     */
    bool playPatterns()
    {
        bool anySuccess = false;
        
        // Start playback for all players with patterns
        for (auto& role : musicianRoles)
        {
            if (role.player)
            {
                // Use the player's play method to start playback
                bool success = role.player->play();
                if (success)
                {
                    anySuccess = true;
                    std::cout << "EnsembleManager::playPatterns - Started playback for role: " 
                              << role.name << std::endl;
                }
            }
        }
        
        playing = anySuccess;
        return anySuccess;
    }
    
    /**
     * @brief Stop playback of all patterns
     */
    void stopPlayback()
    {
        // Stop playback for all players
        for (auto& role : musicianRoles)
        {
            if (role.player)
            {
                // Use the player's stop method to stop playback
                role.player->stop();
                std::cout << "EnsembleManager::stopPlayback - Stopped playback for role: " 
                          << role.name << std::endl;
            }
        }
        
        playing = false;
    }
    
    /**
     * @brief Temporary method to parse MIDI pattern from LLM response
     * 
     * @param response The LLM response containing a MIDI pattern
     * @return std::shared_ptr<juce::MidiMessageSequence> The parsed MIDI pattern
     */
    std::shared_ptr<juce::MidiMessageSequence> parseMIDIPattern(const juce::String& response)
    {
        // Create a new MIDI message sequence
        auto sequence = std::make_shared<juce::MidiMessageSequence>();
        
        // Look for MIDI pattern between ```midi and ``` markers
        int startIndex = response.indexOf(juce::StringRef("```midi"));
        if (startIndex == -1)
        {
            startIndex = response.indexOf(juce::StringRef("```MIDI"));
        }
        
        if (startIndex != -1)
        {
            // Find the next newline character after the ```midi marker
            for (int i = startIndex; i < response.length(); ++i)
            {
                if (response[i] == '\n')
                {
                    startIndex = i + 1;
                    break;
                }
            }
            
            // Find the closing ``` marker
            int endIndex = -1;
            juce::String closingMarker = "```";
            for (int i = startIndex; i < response.length() - closingMarker.length() + 1; ++i)
            {
                if (response.substring(i, i + closingMarker.length()) == closingMarker)
                {
                    endIndex = i;
                    break;
                }
            }
            
            if (endIndex != -1)
            {
                // Extract the MIDI pattern
                juce::String midiPattern = response.substring(startIndex, endIndex).trim();
                
                // Parse each line of the MIDI pattern
                juce::StringArray lines;
                lines.addLines(midiPattern);
                
                for (const auto& line : lines)
                {
                    if (line.trim().isEmpty())
                        continue;
                    
                    // Parse the line in format: channel:note:velocity:start_time:duration
                    juce::StringArray parts;
                    parts.addTokens(line, ":", "");
                    
                    if (parts.size() >= 5)
                    {
                        int channel = parts[0].getIntValue();
                        juce::String noteName = parts[1].trim();
                        int velocity = parts[2].getIntValue();
                        double startTime = parts[3].getDoubleValue();
                        double duration = parts[4].getDoubleValue();
                        
                        // Convert note name to MIDI note number
                        int noteNumber = getMidiNoteFromName(noteName);
                        
                        // Create note on message
                        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel, noteNumber, (float)velocity / 127.0f);
                        noteOn.setTimeStamp(startTime);
                        sequence->addEvent(noteOn);
                        
                        // Create note off message
                        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(channel, noteNumber);
                        noteOff.setTimeStamp(startTime + duration);
                        sequence->addEvent(noteOff);
                    }
                }
                
                // Sort the sequence by timestamp
                sequence->sort();
                return sequence;
            }
        }
        
        return nullptr;
    }
    
    /**
     * @brief Temporary method to convert a note name to a MIDI note number
     * 
     * @param name The note name in scientific pitch notation (e.g., C4, A#3)
     * @return int The MIDI note number
     */
    int getMidiNoteFromName(const juce::String& name)
    {
        // Parse note name in scientific pitch notation (e.g., C4, A#3, Db5)
        juce::String noteName = name.trim().toUpperCase();
        
        if (noteName.isEmpty())
            return 60; // Middle C as default
        
        // Extract the octave number (last character)
        int octave = noteName.getLastCharacter() - '0';
        juce::String note = noteName.dropLastCharacters(1);
        
        // Base values for each note in octave 0
        int baseNote;
        if (note == "C") baseNote = 0;
        else if (note == "C#" || note == "DB") baseNote = 1;
        else if (note == "D") baseNote = 2;
        else if (note == "D#" || note == "EB") baseNote = 3;
        else if (note == "E") baseNote = 4;
        else if (note == "F") baseNote = 5;
        else if (note == "F#" || note == "GB") baseNote = 6;
        else if (note == "G") baseNote = 7;
        else if (note == "G#" || note == "AB") baseNote = 8;
        else if (note == "A") baseNote = 9;
        else if (note == "A#" || note == "BB") baseNote = 10;
        else if (note == "B") baseNote = 11;
        else return 60; // Default to middle C if invalid note name
        
        // Calculate MIDI note number: 12 * octave + baseNote + 12 (to start from C0 = 12)
        return 12 * octave + baseNote + 12;
    }
    
    /**
     * @brief Temporary method to set a MIDI pattern for a musician role
     * 
     * @param roleName The name of the musician role
     * @param pattern The MIDI pattern to set
     */
    void setMIDIPatternForRole(const juce::String& roleName, std::shared_ptr<juce::MidiMessageSequence> pattern)
    {
        // Suppress unused parameter warning
        juce::ignoreUnused(pattern);
        
        // Find the musician role with the given name
        for (auto& role : musicianRoles)
        {
            if (role.name == roleName && role.player)
            {
                // We need to implement a way to set patterns on players
                // For now, we'll just store the pattern in the role
                // This will be used by the PerformanceComponent
                role.pattern = "MIDI pattern stored";
                break;
            }
        }
    }
    
private:
};

} // namespace covertracks
