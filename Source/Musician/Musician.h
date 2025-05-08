#pragma once

#include <JuceHeader.h>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <regex>
#include "LLMClient.h"
#include "PromptTemplates.h"
#include "MusicalShorthand.h"
#include "PatternProcessor.h"
// Removed ShorthandProcessor.h and ShorthandTemplates.h

// Forward include the JSONInterface class
#include "../Player/Core/JSONInterface.h"

// Forward declare the friend class in the global scope
class MultiMusicianTest;

namespace covertracks {

class Musician;
class Ensemble;

/**
 * @brief Base class for all musicians
 * 
 * This class provides a configuration-driven approach to musician behavior,
 * allowing for different instrument types without requiring subclasses.
 */
class Musician {
public:
    /**
     * @brief Construct a new Musician
     * 
     * @param id Unique identifier for this musician
     * @param type Type of musician (e.g., "piano", "bass", "drums")
     * @param jsonInterface Pointer to the JSON interface for Player layer communication
     */
    Musician(const std::string& id, const std::string& type, 
             JSONInterface* jsonInterface = nullptr);
    
    /**
     * @brief Destroy the Musician
     */
    virtual ~Musician() = default;
    
    /**
     * @brief Initialize the musician with instrument-specific configuration
     * 
     * @param config Configuration options as JSON, may include:
     *        - initialState: Basic musical state (tempo, key, etc.)
     *        - piano: Piano-specific configuration
     *        - bass: Bass-specific configuration 
     *        - drums: Drum-specific configuration
     *        - guitar: Guitar-specific configuration
     *        - promptTemplates: Custom LLM prompt templates
     * @return true If initialization was successful
     * @return false If initialization failed
     */
    bool initialize(const var& config = var());
    
    /**
     * @brief Set the LLM client to use for pattern generation
     * 
     * @param client Shared pointer to an LLM client
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Set the callback function for sending commands to the Player layer
     * 
     * @param callback Function that accepts a JSON string and returns a response
     */
    void setPlayerInterface(std::function<std::string(const std::string&)> callback);
    
    /**
     * @brief Generate and play a pattern
     * 
     * @param parameters Parameters for pattern generation, may include:
     *        - style: Musical style (e.g., "jazz", "rock", "pop")
     *        - section: Song section (e.g., "verse", "chorus")
     *        - tempo: Tempo in BPM
     *        - key: Musical key
     *        - chords: Chord progression
     *        - intensity: Dynamic level (0.0-1.0)
     * @return var The generated pattern (for reference) as JUCE var/JSON
     */
    virtual var generateAndPlayPattern(const var& parameters);
    
    /**
     * @brief Generate a pattern but don't play it
     * 
     * @param parameters Parameters for pattern generation
     * @return var The generated pattern as JUCE var/JSON
     */
    var generatePattern(const var& parameters);
    
    /**
     * @brief Process feedback from the Player layer
     * 
     * @param feedback Feedback data in JUCE var/JSON format
     */
    void processFeedback(const var& feedback);
    
    /**
     * @brief Get the musician ID
     * 
     * @return const std::string& The musician ID
     */
    const std::string& getId() const { return id; }
    
    /**
     * @brief Get the musician type
     * 
     * @return const std::string& The musician type (instrument)
     */
    const std::string& getType() const { return type; }
    
    /**
     * @brief Check if the musician is ready to generate patterns
     * 
     * @return true If the musician is initialized and has an LLM client
     * @return false If the musician is not ready
     */
    bool isReady() const;
    
protected:
    // Allow MultiMusicianTest to call sendCommand
    friend class ::MultiMusicianTest;

    /**
     * @brief Create player configuration based on instrument type
     * 
     * @return var Configuration for the player as JUCE var/JSON
     */
    var createPlayerConfig();
    
    /**
     * @brief Process pattern completion feedback
     * 
     * @param patternId ID of the completed pattern
     */
    virtual void onPatternCompleted(const std::string& patternId);
    
    /**
     * @brief Send a command to the Player layer
     * 
     * @param command JUCE var/JSON command to send
     * @return var Response from the Player layer as JUCE var/JSON
     */
    var sendCommand(const var& command);
    
    /**
     * @brief Send a command to the Player layer as a string
     * 
     * @param commandString JSON command string to send
     * @return var Response from the Player layer as JUCE var/JSON
     */
    var sendCommandString(const std::string& commandString);
    
    /**
     * @brief Load prompt templates for this instrument
     * 
     * @param config Configuration containing templates as JUCE var/JSON
     */
    void loadPromptTemplates(const var& config);
    
    /**
     * @brief Adapt pattern parameters based on feedback history
     * 
     * @param parameters Original parameters as JUCE var/JSON
     * @return var Adapted parameters as JUCE var/JSON
     */
    var adaptParameters(const var& parameters);
    
    // Unique identifier for this musician
    std::string id;
    
    // Type of musician (e.g., "piano", "bass", "drums")
    std::string type;
    
    // JSON interface for Player layer communication
    JSONInterface* jsonInterface;
    
    // Function for sending commands to the Player layer
    std::function<std::string(const std::string&)> playerInterfaceCallback;
    
    // LLM client for pattern generation
    std::shared_ptr<LLMClient> llmClient;
    
    // Musical state
    var musicalState;
    
    // Instrument-specific configuration
    var instrumentConfig;
    
    // Prompt templates by style and section
    std::map<std::string, std::string> promptTemplates;
    
    // Player state
    bool playerCreated = false;
    std::string currentPatternId;
    var lastGeneratedPatternCommand; // Store the last generated pattern command
    
    // Feedback history
    var feedbackHistory;
    
    // Whether the musician is initialized
    bool initialized = false;
};

class MusicianFactory {
    public:
        static std::unique_ptr<Musician> createMusician(
            const std::string& type,
            const std::string& id,
            JSONInterface* jsonInterface,
            const var& config);
    };
    
}   // namespace covertracks
