// Source/Musician/Core/Musician.h
#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"
#include "../../LLM/Core/LLMClient.h"
#include "../../LLM/Core/PromptManager.h"

namespace covertracks {

// Forward declarations
class PatternGenerator;
class MusicPatternSystem;

/**
 * @brief Core class for generating and managing musical patterns
 * 
 * This class integrates with the Player architecture and provides functionality
 * for generating musical patterns using LLMs, which can then be played by instruments.
 */
class Musician : public ChangeBroadcaster {
public:
    /**
     * @brief Construct a new Musician
     * 
     * @param id Unique identifier for this musician
     * @param style Default musical style
     */
    Musician(const String& id, const String& style = "");
    
    /**
     * @brief Destroy the Musician
     */
    ~Musician();
    
    /**
     * @brief Set the LLM client
     * 
     * @param client Shared pointer to an LLM client
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Generate a musical pattern
     * 
     * @param parameters Parameters for pattern generation (JSON/var)
     * @param callback Callback to be invoked when generation completes
     */
    void generatePattern(const var& parameters, 
                        std::function<void(bool success, const var& pattern)> callback);
    
    /**
     * @brief Generate a specific section
     * 
     * @param section The section type (e.g., "verse", "chorus")
     * @param parameters Parameters for section generation (JSON/var)
     * @param callback Callback to be invoked when generation completes
     */
    void generateSection(const String& section, const var& parameters,
                        std::function<void(bool success, const var& pattern)> callback);
    
    /**
     * @brief Modify an existing pattern
     * 
     * @param currentPattern The current pattern (JSON/var)
     * @param modification Description of modifications to make
     * @param callback Callback to be invoked when generation completes
     */
    void modifyPattern(const var& currentPattern, const String& modification,
                      std::function<void(bool success, const var& pattern)> callback);
    
    /**
     * @brief Set the command interface for playing patterns
     * 
     * @param commandInterface Function to handle player commands (JSON string in/out)
     */
    void setPlayerInterface(std::function<String(const String&)> commandInterface);
    
    /**
     * @brief Direct interface for playing patterns
     * 
     * @param pattern The pattern to play (JSON/var)
     * @param tempo The tempo in BPM
     * @return true if playback started successfully
     */
    bool playPattern(const var& pattern, double tempo = 120.0);
    
    /**
     * @brief Stop current playback
     */
    void stopPlayback();
    
    /**
     * @brief Get the ID of this musician
     * 
     * @return const String& Musician ID
     */
    const String& getId() const { return id; }
    
    /**
     * @brief Get the default style of this musician
     * 
     * @return const String& Default style
     */
    const String& getStyle() const { return style; }
    
    /**
     * @brief Set the default style of this musician
     * 
     * @param style Default style
     */
    void setStyle(const String& styleValue) { this->style = styleValue; }
    
    /**
     * @brief Get the pattern system
     * 
     * @return MusicPatternSystem& Reference to the pattern system
     */
    MusicPatternSystem& getPatternSystem() { return *patternSystem; }
    
    /**
     * @brief Get the prompt manager
     * 
     * @return PromptManager& Reference to the prompt manager
     */
    PromptManager& getPromptManager() { return promptManager; }
    
    /**
     * @brief Get the pattern generator
     * 
     * @return PatternGenerator& Reference to the pattern generator
     */
    PatternGenerator& getPatternGenerator() { return *patternGenerator; }
    
    /**
     * @brief Convert notation to MIDI events
     * 
     * @param notation Pattern notation
     * @param tempo Tempo in BPM
     * @return var MIDI events as JSON/var
     */
    var convertNotationToMidiEvents(const String& notation, double tempo = 120.0);
    
    /**
     * @brief Check if the musician is connected to an LLM
     * 
     * @return true if connected to an LLM
     */
    bool isConnectedToLLM() const { return llmClient != nullptr && llmClient->isConnected(); }
    
    /**
     * @brief Get the LLM service name
     * 
     * @return String The name of the LLM service, or empty if not connected
     */
    String getLLMServiceName() const { 
        return llmClient ? llmClient->getServiceName() : String(); 
    }
    
    /**
     * @brief Serialize musician to JSON
     * 
     * @return var JSON representation
     */
    var toVar() const;
    
    /**
     * @brief Create a Musician from JSON
     * 
     * @param v JSON representation
     * @return std::unique_ptr<Musician> The created musician
     */
    static std::unique_ptr<Musician> fromVar(const var& v);
    
private:
    // Core properties
    String id;
    String style;
    
    // LLM integration
    std::shared_ptr<LLMClient> llmClient;
    
    // Music pattern generation
    std::unique_ptr<MusicPatternSystem> patternSystem;
    PromptManager promptManager;
    std::unique_ptr<PatternGenerator> patternGenerator;
    
    // Player interface
    std::function<String(const String&)> playerCommandInterface;
    
    // Most recently generated song/pattern
    std::shared_ptr<Song> lastGeneratedSong;
    String lastResponseText;
    
    // Convert MIDI message sequence to JSON/var
    var midiMessageSequenceToVar(const MidiMessageSequence& sequence, double tempo = 120.0) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Musician)
};

} // namespace covertracks