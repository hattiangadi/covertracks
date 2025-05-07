#pragma once

#include <JuceHeader.h>
#include <nlohmann/json.hpp>
#include "Instrument.h"
#include "../Musician/Musician.h"

using json = nlohmann::json;

namespace covertracks {

/**
 * @brief Metadata for the Player
 */
struct PlayerMetadata {
    String name;
    String category;
    String description;
    Time created;
    Time lastModified;
    
    // Serialize to/from var
    var toVar() const;
    void fromVar(const var& v);
};

/**
 * @brief Core class for the unified Player architecture
 * 
 * Player combines instrument (audio processing) and musician (pattern generation)
 * capabilities in a unified interface.
 */
class Player {
public:
    /**
     * @brief Construct a new Player
     * 
     * @param id Unique identifier for this player
     * @param name Display name for this player
     */
    Player(const String& id, const String& name);
    
    /**
     * @brief Destroy the Player
     */
    ~Player();
    
    /**
     * @brief Initialize the player with default settings
     * 
     * @return true If initialization was successful
     * @return false If initialization failed
     */
    bool initialize();
    
    /**
     * @brief Load a plugin graph configuration
     * 
     * @param graphState XML representation of plugin graph
     * @return true If loaded successfully
     * @return false If loading failed
     */
    bool loadPluginGraph(const XmlElement& graphState);
    
    /**
     * @brief Get the current plugin graph state
     * 
     * @return std::unique_ptr<XmlElement> XML representation of the current graph
     */
    std::unique_ptr<XmlElement> getPluginGraphState();
    
    /**
     * @brief Set the LLM client for the musician
     * 
     * @param client Shared pointer to an LLM client
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Generate a musical pattern
     * 
     * @param parameters Parameters for pattern generation (key, tempo, etc.)
     * @return json The generated pattern
     */
    json generatePattern(const json& parameters);
    
    /**
     * @brief Play a MIDI pattern
     * 
     * @param pattern MIDI events in JSON format
     * @param tempo Tempo in BPM
     * @return true If playback started successfully
     * @return false If playback failed to start
     */
    bool playPattern(const json& pattern, double tempo = 120.0);
    
    /**
     * @brief Generate and immediately play a pattern
     * 
     * @param parameters Parameters for pattern generation
     * @return true If generation and playback were successful
     * @return false If an error occurred
     */
    bool generateAndPlay(const json& parameters);
    
    /**
     * @brief Stop current playback
     */
    void stopPlayback();
    
    /**
     * @brief Set callback for pattern completion
     * 
     * @param callback Function to call when pattern playback completes
     */
    void setPatternFinishedCallback(std::function<void(const String&)> callback);
    
    /**
     * @brief Serialize player to JSON
     * 
     * @return var JSON representation
     */
    var toVar() const;
    
    /**
     * @brief Create a Player from JSON
     * 
     * @param v JSON representation
     * @return std::unique_ptr<Player> The created player
     */
    static std::unique_ptr<Player> fromVar(const var& v);
    
    /**
     * @brief Get the ID of this player
     * 
     * @return const String& Player ID
     */
    const String& getId() const { return id; }
    
    /**
     * @brief Get the metadata for this player
     * 
     * @return PlayerMetadata& Metadata reference
     */
    PlayerMetadata& getMetadata() { return metadata; }
    
    /**
     * @brief Get the instrument component
     * 
     * @return Instrument* Pointer to the instrument (not owned)
     */
    Instrument* getInstrument() { return instrument.get(); }
    
    /**
     * @brief Get the musician component
     * 
     * @return Musician* Pointer to the musician (not owned)
     */
    Musician* getMusician() { return musician.get(); }
    
private:
    // Core properties
    String id;
    PlayerMetadata metadata;
    
    // Core components
    std::unique_ptr<Instrument> instrument;
    std::unique_ptr<Musician> musician;
    
    // Most recently generated pattern
    json lastGeneratedPattern;
    
    // Pattern finished callback
    std::function<void(const String&)> onPatternFinished;
    
    // Handle pattern completion
    void handlePatternFinished();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Player)