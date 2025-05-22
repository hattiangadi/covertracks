#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include "../../Audio/AudioEngine.h"
#include "../../Musician/Core/Musician.h"

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
     * @brief Generate a musical pattern (stubbed)
     * 
     * @param parameters Parameters for pattern generation (key, tempo, etc.)
     * @return var The generated pattern as JUCE var/JSON
     */
    var generatePattern(const var& parameters);
    
    /**
     * @brief Play a MIDI pattern
     * 
     * @param pattern MIDI events in JUCE var/JSON format
     * @param tempo Tempo in BPM
     * @return true If playback started successfully
     * @return false If playback failed to start
     */
    bool playPattern(const var& pattern, double tempo = 120.0);
    
    /**
     * @brief Generate and immediately play a pattern (stubbed)
     * 
     * @param parameters Parameters for pattern generation
     * @return true If generation and playback were successful
     * @return false If an error occurred
     */
    bool generateAndPlay(const var& parameters);
    
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
    
    // Most recently generated pattern
    var lastGeneratedPattern;
    
    // Pattern finished callback
    std::function<void(const String&)> onPatternFinished;
    
    // Handle pattern completion
    void handlePatternFinished();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Player)

};

}
