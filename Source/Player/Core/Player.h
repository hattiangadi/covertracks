#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include "../../Audio/AudioEngine.h"
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
    String instrumentType;  // "drums", "piano", "bass", "guitar", etc.
    
    // For NoteRange type
    int minNote = 0;        // Minimum MIDI note number
    int maxNote = 127;      // Maximum MIDI note number
    
    // For Mapping type  
    std::map<String, int> soundMappings;  // sound name -> MIDI note (e.g., "kick" -> 36)
    
    // Serialize to/from var
    var toVar() const;
    void fromVar(const var& v);
    
    /**
     * @brief Find a MIDI note for the given note name using the capabilities
     * 
     * For mapping type, looks up the note name in soundMappings
     * For range type, checks if the parsed note is within range
     * 
     * @param noteName The name of the note to find
     * @return int The MIDI note number, or -1 if not found
     */
    int findNote(const String& noteName) const;
};

/**
 * @brief Metadata for the Player
 */
struct PlayerMetadata {
    String name;
    String category;
    String description;
    Time created;
    Time lastModified;
    
    // Instrument capabilities for LLM integration
    InstrumentCapabilities capabilities;
    
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
     * @param formatManager Reference to the shared AudioPluginFormatManager
     * @param knownPluginList Reference to the shared KnownPluginList
     * @param audioEngine Optional AudioEngine reference for centralized audio management
     */
     Player(const String& id, const String& name, 
        AudioPluginFormatManager& formatManager,
        KnownPluginList& knownPluginList,
        AudioEngine* audioEngine = nullptr);
    
    /**
     * @brief Destroy the Player
     */
    ~Player();

    // Resource management
    void loadMinimalResources();  // Load metadata and minimal state
    void prepareForEditing();     // Prepare for UI editing
    void prepareForPlayback();    // Prepare for audio playback
    void releaseResources();      // Release heavy resources
    bool initializeMinimal();     // Initialize with minimal resources
    
    // Graph state management for deferred loading
    void setGraphFileName(const String& fileName) { graphFileName = fileName; }
    void setEmbeddedGraphState(const String& base64State) { embeddedGraphState = base64State; }
    bool loadGraphStateIfNeeded();  // Load graph state if not already loaded
        
    // AudioEngine connection
    bool connectToAudioEngine(AudioEngine* engine);
    void disconnectFromAudioEngine();
    bool isConnectedToAudioEngine() const;
        
    // Resource state tracking
    bool isFullyLoaded() const;
    bool isReadyForPlayback() const;
    bool isReadyForEditing() const;
    
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
     * @brief Save the current graph state to a file
     * 
     * @return true if successful
     * @return false if failed
     */
    bool saveGraphState();
    
    /**
     * @brief Save a specific graph state XML to the player's graph file
     * 
     * @param graphState XML representation of plugin graph
     * @return true if successful
     * @return false if failed
     */
    bool savePluginGraphToFile(const XmlElement& graphState);
    
    /**
     * @brief Get the current plugin graph state
     * 
     * @return std::unique_ptr<XmlElement> XML representation of the current graph
     */
    std::unique_ptr<XmlElement> getPluginGraphState();
    
    /**
     * @brief Load the plugin graph state from the saved file
     * 
     * @return std::unique_ptr<XmlElement> XML representation of the graph from file
     */
    std::unique_ptr<XmlElement> loadGraphStateFromFile();
    
    /**
     * @brief Set the LLM client for the musician (stubbed)
     * 
     * @param client Shared pointer to an LLM client
     */
    // void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Generate a new musical chart
     * 
     * @param parameters Parameters for chart generation (key, tempo, etc.)
     * @param callback Callback to be invoked when generation completes
     */
    void generateNewChart(const var& parameters, std::function<void(bool success)> callback = nullptr);
    
    /**
     * @brief Load a chart for playback (from any source)
     * 
     * @param chart The chart as a var/JSON object
     * @return true If the chart was loaded successfully
     * @return false If there was an error loading the chart
     */
    bool loadChart(const var& chart);
    
    /**
     * @brief Load chart from musician's last generated chart
     * 
     * @return true If the chart was loaded successfully
     * @return false If there was an error loading the chart
     */
    bool loadChartFromMusician();
    
    /**
     * @brief Convert a pattern string to MIDI events using the same code path as Musician
     * 
     * @param patternString The pattern string to convert (JSON or LLM response format)
     * @param tempo The tempo for conversion (default: 120 BPM)
     * @return var The converted MIDI events in Player-compatible format
     */
    var convertPatternStringToChart(const String& patternString, double tempo = 120.0);
    
    /**
     * @brief Generate a chart using the internal Musician
     * 
     * @param parameters Parameters for chart generation (var object)
     * @param callback Callback with MIDI_EVENT_CHART (Player-compatible format)
     */
    void generateChart(const var& parameters, 
                      std::function<void(bool success, const var& midiEventChart)> callback);
    
    /**
     * @brief Modify the current chart
     * 
     * @param modifications Description of modifications to make
     * @param callback Callback to be invoked when modification completes
     */
    void modifyCurrentChart(const String& modifications, std::function<void(bool success)> callback = nullptr);
    
    /**
     * @brief Get the current loaded chart
     * 
     * @return const var& The current chart
     */
    const var& getCurrentChart() const;
    
    /**
     * @brief Start playing the current pattern
     * 
     * @return true If playback started successfully
     * @return false If there was an error starting playback
     */
    bool play();
    
    /**
     * @brief Stop playing the current pattern
     */
    void stop();
    
    
    /**
     * @brief Generate a new chart and immediately load it for playback
     * 
     * @param parameters Parameters for chart generation
     * @param callback Callback to be invoked when generation and loading completes
     */
    void generateAndLoad(const var& parameters, std::function<void(bool success)> callback = nullptr);
    
    /**
     * @brief Stop current playback
     */
    void stopPlayback();
    
    /**
     * @brief Set callback for chart playback completion
     * 
     * @param callback Function to call when chart playback completes
     */
    void setChartFinishedCallback(std::function<void(const String&)> callback);
    
    /**
     * @brief Get instrument capabilities for LLM integration
     * 
     * @return const InstrumentCapabilities& Instrument capabilities
     */
    const InstrumentCapabilities& getInstrumentCapabilities() const { return metadata.capabilities; }
    
    /**
     * @brief Set instrument capabilities for LLM integration
     * 
     * @param capabilities The instrument capabilities to set
     */
    void setInstrumentCapabilities(const InstrumentCapabilities& capabilities) { metadata.capabilities = capabilities; }
    
    /**
     * @brief Create standard drum capabilities with GM percussion mapping
     * 
     * @return InstrumentCapabilities Standard drum capabilities
     */
    static InstrumentCapabilities createDrumCapabilities();
    
    /**
     * @brief Create piano capabilities with note range
     * 
     * @param minNote Minimum MIDI note (default: C2 = 36)
     * @param maxNote Maximum MIDI note (default: G5 = 79)
     * @return InstrumentCapabilities Piano capabilities
     */
    static InstrumentCapabilities createPianoCapabilities(int minNote = 36, int maxNote = 79);
    
    /**
     * @brief Create bass capabilities with note range
     * 
     * @param minNote Minimum MIDI note (default: E1 = 28)
     * @param maxNote Maximum MIDI note (default: G3 = 55)
     * @return InstrumentCapabilities Bass capabilities
     */
    static InstrumentCapabilities createBassCapabilities(int minNote = 28, int maxNote = 55);
    
    /**
     * @brief Create guitar capabilities with note range
     * 
     * @param minNote Minimum MIDI note (default: E2 = 40)
     * @param maxNote Maximum MIDI note (default: E6 = 88)
     * @return InstrumentCapabilities Guitar capabilities
     */
    static InstrumentCapabilities createGuitarCapabilities(int minNote = 40, int maxNote = 88);
    
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
     * @param formatManager Reference to the shared AudioPluginFormatManager
     * @param knownPluginList Reference to the shared KnownPluginList
     * @param audioEngine Optional AudioEngine reference for centralized audio management
     * @return std::unique_ptr<Player> The created player
     */
    static std::unique_ptr<Player> fromVar(const var& v, 
                                           AudioPluginFormatManager& formatManager,
                                           KnownPluginList& knownPluginList,
                                           AudioEngine* audioEngine = nullptr);
    
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
     * @brief Set the musician component
     * 
     * @param musician The musician to set
     */
     void setMusician(std::unique_ptr<Musician> musician);
    
     /**
      * @brief Get the musician component
      * 
      * @return Musician* Pointer to the musician (not owned)
      */
     Musician* getMusician() { return musician.get(); }
     
     /**
      * @brief Set the LLM client for the musician
      * 
      * @param client Shared pointer to an LLM client
      */
     void setLLMClient(std::shared_ptr<LLMClient> client);
 
    
private:
    // Core properties
    String id;
    PlayerMetadata metadata;
    
    // References to shared plugin environment (not owned)
    AudioPluginFormatManager& formatManager;
    KnownPluginList& knownPluginList;

    AudioEngine* audioEngine = nullptr;

    
    // Core components
    std::unique_ptr<Instrument> instrument;
    std::unique_ptr<Musician> musician;
    
    // Resource state
    bool fullyLoaded = false;
    bool readyForEditing = false;
    bool readyForPlayback = false;
    bool graphLoaded = false;      // Whether the graph has been loaded
    
    // Deferred loading data
    String graphFileName;         // Name of the graph file to load when needed
    String embeddedGraphState;    // Base64-encoded graph state to load when needed
    
    /**
     * @brief Get the path to the graph file for this player
     * 
     * @return File The graph file path
     */
    File getGraphFilePath() const
    {
        // Get the application data directory
        File appDataDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                          .getChildFile("Covertracks")
                          .getChildFile("Graphs");
        
        // Create a filename based on the player ID
        return appDataDir.getChildFile(id + ".filtergraph");
    }
    
    // Chart storage and playback
    var currentLoadedChart;
    std::shared_ptr<juce::MidiMessageSequence> currentMidiSequence;
    double chartTempo = 120.0; // Default tempo in BPM
    
    // Chart finished callback
    std::function<void(const String&)> onChartFinished;
    
    // Handle chart completion
    void handleChartFinished();
    
    // Convert chart to MIDI sequence (internal helper)
    bool convertChartToMidi(const var& chart);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Player)

};

}
