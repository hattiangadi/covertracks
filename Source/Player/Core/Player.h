#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include "../../Musician/Core/Musician.h"

namespace covertracks {

/**
 * @brief Instrument capability types
 */
enum class InstrumentCapabilityType {
    NoteRange,    // Has min/max MIDI note range (e.g., Piano: C2-G5)
    Mapping       // Has specific note-to-sound mappings (e.g., Drums: kick=36, snare=38)
};

/**
 * @brief Instrument capabilities for LLM integration
 */
struct InstrumentCapabilities {
    InstrumentCapabilityType type;
    juce::String instrumentType;  // "drums", "piano", "bass", "guitar", etc.
    
    // For NoteRange type
    int minNote = 0;        // Minimum MIDI note number
    int maxNote = 127;      // Maximum MIDI note number
    
    // For Mapping type  
    std::map<juce::String, int> soundMappings;  // sound name -> MIDI note (e.g., "kick" -> 36)
};

/**
 * @brief Player metadata structure
 */
struct PlayerMetadata {
    juce::String name;           // Player display name
    juce::String id;            // Unique player identifier
    juce::String instrumentType; // Type of instrument (drums, piano, etc.)
    juce::String category;       // Instrument category (Keyboard, Drums, Bass, Guitar, Other)
    juce::String description;    // Player description
    juce::Time created;         // Creation timestamp
    juce::Time lastModified;    // Last modification timestamp
    
    PlayerMetadata() = default;
    PlayerMetadata(const juce::String& playerName, const juce::String& playerId, const juce::String& instType = "general")
        : name(playerName), id(playerId), instrumentType(instType), category("Other"), description(""),
          created(juce::Time::getCurrentTime()), lastModified(juce::Time::getCurrentTime()) {}
};

/**
 * @brief Orchestration layer - coordinates Musician and Instrument
 * 
 * Player is a thin orchestration layer that:
 * - Coordinates between Musician (chart generation) and Instrument (audio playback)
 * - Handles persistence (save/load player configurations and states)
 * - Provides high-level player operations (generateAndPlay, addPlugin, etc.)
 * - Delegates all audio concerns to Instrument
 * 
 * This class represents the "conductor" that coordinates musical generation
 * and audio playback, keeping these concerns properly separated.
 */
class Player
{
public:
    /**
     * @brief Create a Player
     * @param id Unique player identifier
     * @param name Display name for this player
     * @param formatManager Reference to shared plugin format manager
     * @param knownPluginList Reference to shared plugin list
     */
    Player(const juce::String& id,
           const juce::String& name,
           juce::AudioPluginFormatManager& formatManager,
           juce::KnownPluginList& knownPluginList);
    
    ~Player();
    
    // =========================================================================
    // Player Identity & Metadata
    // =========================================================================
    
    /**
     * @brief Get player ID
     * @return Unique player identifier
     */
    const juce::String& getId() const { return playerId; }
    
    /**
     * @brief Get player name
     * @return Display name
     */
    const juce::String& getName() const { return playerName; }
    
    /**
     * @brief Set player name
     * @param name New display name
     */
    void setName(const juce::String& name) { playerName = name; }
    
    /**
     * @brief Get player metadata
     * @return Player metadata structure
     */
    PlayerMetadata getMetadata() const;
    
    /**
     * @brief Get mutable player metadata
     * @return Reference to player metadata structure
     */
    PlayerMetadata& getMetadata();
    
    /**
     * @brief Set player metadata
     * @param newMetadata New metadata to set
     */
    void setMetadata(const PlayerMetadata& newMetadata);
    
    // =========================================================================
    // High-Level Orchestration (Musician ↔ Instrument)
    // =========================================================================
    
    /**
     * @brief State Machine Workflow - cleaner approach for pattern playback
     * These methods manage internal state transitions without data passing
     */
    
    /**
     * @brief Step 1: Prepare a pattern for playback
     * @param patternJson JSON string containing the pattern
     * @return true if pattern prepared successfully
     */
    bool preparePattern(const juce::String& patternJson);
    
    /**
     * @brief Step 2: Convert prepared pattern to MIDI
     * @return true if conversion successful
     */
    bool convertToMidi();
    
    /**
     * @brief Step 3: Load converted MIDI into instrument
     * @return true if loading successful
     */
    bool loadPreparedMidi();
    
    /**
     * @brief Generate a musical chart and immediately play it
     * Coordinates Musician chart generation → Instrument playback
     * @param parameters Chart generation parameters
     * @param callback Completion callback (success/failure)
     */
    void generateAndPlay(const juce::var& parameters, 
                        std::function<void(bool success)> callback = nullptr);
    
    /**
     * @brief Load a chart for later playback (pre-conversion for synchronized timing)
     * @param chart MIDI chart data to load
     * @param tempo Playback tempo (default: 120 BPM)
     * @return true if chart loaded successfully
     */
    bool loadChart(const juce::var& chart, double tempo = 120.0);
    
    /**
     * @brief Load and play a pre-generated chart
     * @param chart MIDI chart data to play
     * @param tempo Playback tempo (default: 120 BPM)
     * @return true if playback started successfully
     */
    bool playChart(const juce::var& chart, double tempo = 120.0);
    
    /**
     * @brief Play the previously loaded chart (instant start for synchronized timing)
     * @return true if playback started successfully
     */
    bool playLoadedChart();
    
    /**
     * @brief Generate a new chart using the Musician
     * @param parameters Generation parameters
     * @param callback Called with (success, generatedChart)
     */
    void generateChart(const juce::var& parameters,
                      std::function<void(bool success, const juce::var& chart)> callback);
    
    /**
     * @brief Get the last generated chart
     * @return Chart data, or invalid var if none generated
     */
    const juce::var& getLastChart() const { return lastGeneratedChart; }
    
    // =========================================================================
    // Audio Delegation (Pass-through to Instrument)
    // =========================================================================
    
    /**
     * @brief Add a plugin to this player's instrument
     * Delegates to Instrument::addPlugin()
     * @param pluginDesc Plugin to add
     * @return true if added successfully
     */
    bool addPlugin(const juce::PluginDescription& pluginDesc);
    
    /**
     * @brief Clear all plugins from this player's instrument
     * Delegates to Instrument::clearPlugins()
     */
    void clearPlugins();
    
    /**
     * @brief Get plugin count
     * @return Number of plugins on instrument
     */
    int getPluginCount() const;
    
    /**
     * @brief Get plugin names
     * @return Array of plugin names
     */
    juce::StringArray getPluginNames() const;
    
    /**
     * @brief Stop current playback
     * Delegates to Instrument::stopPlayback()
     */
    void stop();
    
    /**
     * @brief Start playback of loaded chart
     * @return true if playback started successfully
     */
    bool play();
    
    // =========================================================================
    // State Persistence (Complete Player + Instrument + Plugin Configurations)
    // =========================================================================
    
    /**
     * @brief Save complete player state including instrument and plugin configurations
     * @return JSON representation of the full player state
     */
    juce::var saveCompleteState() const;
    
    /**
     * @brief Load complete player state including instrument and plugin configurations
     * @param state JSON representation of the player state
     * @return true if loaded successfully
     */
    bool loadCompleteState(const juce::var& state);
    
    /**
     * @brief Save just the plugin chain configuration (for presets)
     * @return JSON representation of the plugin chain
     */
    juce::var savePluginChain() const;
    
    /**
     * @brief Load plugin chain configuration (for presets)
     * @param pluginChainState JSON representation of the plugin chain
     * @return true if loaded successfully
     */
    bool loadPluginChain(const juce::var& pluginChainState);
    
    /**
     * @brief Export player as preset file
     * @param file File to save preset to
     * @return true if saved successfully
     */
    bool exportAsPreset(const juce::File& file) const;
    
    /**
     * @brief Import player from preset file
     * @param file File to load preset from
     * @return true if loaded successfully
     */
    bool importFromPreset(const juce::File& file);
    
    /**
     * @brief Pause current playback
     * Delegates to Instrument::pausePlayback()
     */
    void pause();
    
    /**
     * @brief Resume paused playback
     * Delegates to Instrument::resumePlayback()
     */
    void resume();
    
    /**
     * @brief Check if currently playing
     * @return true if playing
     */
    bool isPlaying() const;
    
    /**
     * @brief Check if playback is paused
     * @return true if paused
     */
    bool isPaused() const;
    
    /**
     * @brief Process incoming MIDI message
     * Delegates to Instrument for real-time playing
     * @param message MIDI message to process
     */
    void processMidiMessage(const juce::MidiMessage& message);
    
    // =========================================================================
    // Component Access (for advanced use)
    // =========================================================================
    
    /**
     * @brief Get the Instrument component
     * @return Pointer to instrument (not owned by caller)
     */
    Instrument* getInstrument() { return instrument.get(); }
    
    /**
     * @brief Get the Musician component
     * @return Pointer to musician (not owned by caller)
     */
    Musician* getMusician() { return musician.get(); }
    
    /**
     * @brief Set the Musician component
     * @param newMusician Musician to use for chart generation
     */
    void setMusician(std::unique_ptr<Musician> newMusician);
    
    /**
     * @brief Set LLM client for the musician
     * @param client LLM client for chart generation
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Convert LLM pattern string to MIDI chart (same as legacy Player)
     * 
     * @param patternString The LLM JSON pattern string
     * @param tempo The tempo for the pattern
     * @return juce::var The MIDI chart with success flag
     */
    juce::var convertPatternStringToChart(const juce::String& patternString, double tempo = 120.0);
    
    /**
     * @brief Get instrument capabilities for LLM integration
     * @return Instrument capabilities structure
     */
    const InstrumentCapabilities& getInstrumentCapabilities() const;
    
    /**
     * @brief Set instrument capabilities for LLM integration
     * @param capabilities New instrument capabilities
     */
    void setInstrumentCapabilities(const InstrumentCapabilities& capabilities);
    
    /**
     * @brief Update player metadata from UI data (backend method)
     * @param name Player name from UI
     * @param category Category from UI
     * @param description Description from UI
     */
    void updateMetadataFromUI(const juce::String& name, const juce::String& category, const juce::String& description);
    
    /**
     * @brief Update instrument capabilities from UI data (backend method)
     * @param instrumentType Instrument type from combo box
     * @param capabilityType Capability type (1=NoteRange, 2=Mapping)
     * @param minNote Minimum note value (for NoteRange)
     * @param maxNote Maximum note value (for NoteRange)
     * @param soundMappingText Sound mapping text (for Mapping)
     */
    void updateCapabilitiesFromUI(const juce::String& instrumentType, int capabilityType, 
                                 int minNote, int maxNote, const juce::String& soundMappingText);
    
    /**
     * @brief Transfer plugin state from a working Player instance (backend method)
     * @param sourcePlayer Source player with plugin state to transfer
     * @return bool True if transfer was successful
     */
    bool transferPluginStateFrom(const Player* sourcePlayer);
    
    // =========================================================================
    // Legacy Player Compatibility Methods (for PlayerEditorComponent)
    // =========================================================================
    
    /**
     * @brief Prepare player for editing (legacy compatibility)
     */
    void prepareForEditing();
    
    
    /**
     * @brief Set player state for persistence (legacy compatibility)
     * @param state State data to store
     */
    void setPlayerState(const juce::var& state);
    
    /**
     * @brief Get stored player state (legacy compatibility)
     * @return Stored state data
     */
    juce::var getPlayerState() const;
    
    /**
     * @brief Check if player has stored state (legacy compatibility)
     * @return true if state is stored
     */
    bool hasPlayerState() const;
    
    /**
     * @brief Create drum capabilities (legacy compatibility)
     * @return Default drum instrument capabilities
     */
    static InstrumentCapabilities createDrumCapabilities();
    
    /**
     * @brief Create piano capabilities (legacy compatibility)
     * @return Default piano instrument capabilities
     */
    static InstrumentCapabilities createPianoCapabilities();
    
    /**
     * @brief Create bass capabilities (legacy compatibility)
     * @return Default bass instrument capabilities
     */
    static InstrumentCapabilities createBassCapabilities();
    
    /**
     * @brief Create guitar capabilities (legacy compatibility)
     * @return Default guitar instrument capabilities
     */
    static InstrumentCapabilities createGuitarCapabilities();
    
    /**
     * @brief Set player level/volume (legacy compatibility)
     * @param level Volume level (0.0 to 1.0)
     */
    void setLevel(float level);
    
    // =========================================================================
    // Persistence (Player State Management)
    // =========================================================================
    
    /**
     * @brief Save complete player state to file
     * Includes instrument plugins, musician settings, and player metadata
     * @return true if saved successfully
     */
    bool saveState();
    
    /**
     * @brief Load player state from file
     * @return true if loaded successfully
     */
    bool loadState();
    
    /**
     * @brief Export player state to XML
     * @return Complete player state as XML
     */
    std::unique_ptr<juce::XmlElement> exportState() const;
    
    /**
     * @brief Import player state from XML
     * @param state Player state XML
     * @return true if imported successfully
     */
    bool importState(const juce::XmlElement& state);
    
    /**
     * @brief Serialize player to var (for JSON export)
     * @return Player data as var
     */
    juce::var toVar() const;
    
    /**
     * @brief Create player from var data
     * @param data Player data from toVar()
     * @param formatManager Plugin format manager
     * @param knownPluginList Plugin list
     * @return Created player, or nullptr if failed
     */
    static std::unique_ptr<Player> fromVar(const juce::var& data,
                                           juce::AudioPluginFormatManager& formatManager,
                                           juce::KnownPluginList& knownPluginList);
    
    // =========================================================================
    // Lifecycle Management
    // =========================================================================
    
    /**
     * @brief Initialize player (registers instrument with audio engine)
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown player (unregisters from audio engine)
     */
    void shutdown();
    
    /**
     * @brief Check if player is initialized
     * @return true if ready for use
     */
    bool isInitialized() const { return initialized; }

private:
    // Core identity
    juce::String playerId;
    juce::String playerName;
    
    // Components (owned)
    std::unique_ptr<Instrument> instrument;
    std::unique_ptr<Musician> musician;
    
    // Plugin environment references (not owned)
    juce::AudioPluginFormatManager& formatManager;
    juce::KnownPluginList& knownPluginList;
    
    // State
    bool initialized = false;
    juce::var lastGeneratedChart;
    double lastTempo = 120.0;
    
    // Pre-converted MIDI sequence for instant synchronized playback
    std::unique_ptr<juce::MidiMessageSequence> preloadedSequence;
    
    // State Machine Workflow - internal state
    juce::String preparedPatternJson;
    std::unique_ptr<juce::MidiMessageSequence> convertedMidiSequence;
    juce::var preparedMidiChart;
    
    // Instrument capabilities for LLM integration
    InstrumentCapabilities instrumentCapabilities;
    
    // Player metadata
    PlayerMetadata metadata;
    
    // Legacy player state storage
    juce::var storedPlayerState;
    
    // Helper methods
    juce::File getStateFile() const;
    void setupDefaultMusician();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Player)
};

} // namespace covertracks