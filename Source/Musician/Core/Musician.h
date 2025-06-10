// Source/Musician/Core/Musician.h
#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"
#include "Song.h"
#include "../../LLM/Core/LLMClient.h"

/*
 * CHART FORMAT DOCUMENTATION:
 * 
 * This system uses several different "chart" formats at different stages:
 * 
 * 1. LLM_RESPONSE_TEXT: Raw JSON text from the LLM containing musical notation
 *    - Format: String containing JSON with sections, primitives, chords, patterns
 *    - Example: "{\"title\":\"Take the A Train\",\"sections\":{\"A1\":[...]}}"
 * 
 * 2. PARSED_CHART: C++ Chart object parsed from LLM response
 *    - Format: std::shared_ptr<Chart> with Section/Phrase/Bar hierarchy
 *    - Contains: Structured musical data ready for MIDI conversion
 * 
 * 3. MIDI_EVENT_CHART: MIDI events in JSON/var format for Player consumption
 *    - Format: var object with "events" array containing note_on/note_off messages
 *    - Example: {"success":true,"tempo":120,"events":[{"type":"note_on",...}]}
 * 
 * 4. PLAYER_COMPATIBLE_CHART: Final format expected by Player::loadChart()
 *    - Format: Same as MIDI_EVENT_CHART but validated for Player use
 *    - Contains: All necessary properties for playback (tempo, events, metadata)
 * 
 * Flow: LLM_RESPONSE_TEXT → PARSED_CHART → MIDI_EVENT_CHART → PLAYER_COMPATIBLE_CHART
 */

namespace covertracks {

// Forward declarations
class PatternGenerator;
class MusicPatternSystem;
class PromptManager;

/**
 * @brief Core class for generating and managing musical patterns
 * 
 * This class integrates with the Player architecture and provides functionality
 * for generating musical patterns using LLMs, which can then be played by instruments.
 * 
 * Musician manages the background thread for pattern generation and coordinates
 * between the UI layer and the PatternGenerator utility class.
 */
class Musician : public ChangeBroadcaster, public Thread {
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
    ~Musician() override;
    
    /**
     * @brief Set the LLM client
     * 
     * @param client Shared pointer to an LLM client
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Generate a musical chart from LLM
     * 
     * @param parameters Parameters for chart generation (var object)
     * @param callback Callback with MIDI_EVENT_CHART (Player-compatible format)
     */
    void generateChart(const var& parameters, 
                      std::function<void(bool success, const var& midiEventChart)> callback);
    
    /**
     * @brief Generate a specific section from LLM
     * 
     * @param section The section type (e.g., "verse", "chorus")
     * @param parameters Parameters for section generation (var object)
     * @param callback Callback with MIDI_EVENT_CHART (Player-compatible format)
     */
    void generateSection(const String& section, const var& parameters,
                        std::function<void(bool success, const var& midiEventChart)> callback);
    
    /**
     * @brief Modify an existing chart
     * 
     * @param currentChart The current MIDI_EVENT_CHART
     * @param modification Description of modifications to make
     * @param callback Callback with modified MIDI_EVENT_CHART (Player-compatible format)
     */
    void modifyChart(const var& currentChart, const String& modification,
                    std::function<void(bool success, const var& midiEventChart)> callback);
    
    /**
     * @brief Get the last generated chart
     * 
     * @return const var& The last MIDI_EVENT_CHART (Player-compatible format)
     */
    const var& getLastGeneratedChart() const;
    
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
    PromptManager& getPromptManager() { return *promptManager; }
    
    /**
     * @brief Get the pattern generator
     * 
     * @return PatternGenerator& Reference to the pattern generator
     */
    PatternGenerator& getPatternGenerator() { return *patternGenerator; }
    
    /**
     * @brief Check if pattern generation is in progress
     * 
     * @return bool True if generation is in progress
     */
    bool isGenerating() const;
    
    /**
     * @brief Convert LLM_RESPONSE_TEXT to MIDI_EVENT_CHART
     * 
     * @param notation LLM_RESPONSE_TEXT (JSON string from LLM)
     * @param tempo Tempo in BPM
     * @return var MIDI_EVENT_CHART (Player-compatible format)
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
    // Thread interface
    void run() override;
    
    // Core properties
    String id;
    String style;
    
    // LLM integration
    std::shared_ptr<LLMClient> llmClient;
    
    // Music pattern generation
    std::unique_ptr<MusicPatternSystem> patternSystem;
    std::unique_ptr<PromptManager> promptManager;
    std::unique_ptr<PatternGenerator> patternGenerator;
    
    // Thread management for async LLM requests
    struct GenerationRequest {
        String prompt;
        std::function<void(bool success, const var& midiEventChart)> callback;
    };
    
    std::atomic<bool> generating{false};
    std::atomic<bool> requestInProgress{false};
    String currentPrompt;
    std::function<void(bool success, const var& midiEventChart)> currentCallback;
    WaitableEvent waitEvent;
    
    // Pattern storage
    String lastResponseText;
    std::shared_ptr<Chart> lastGeneratedChart;
    
    // Convert MIDI message sequence to MIDI_EVENT_CHART format
    var midiMessageSequenceToVar(const MidiMessageSequence& sequence, double tempo = 120.0) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Musician)
};

} // namespace covertracks