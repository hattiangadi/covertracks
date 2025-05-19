#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include "../../Audio/AudioEngine.h"
// Temporarily commenting out Musician for compilation
// #include "../../Musician/Musician.h"

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
     * @brief Get the current plugin graph state
     * 
     * @return std::unique_ptr<XmlElement> XML representation of the current graph
     */
    std::unique_ptr<XmlElement> getPluginGraphState();
    
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
     * @brief Get the musician component (stubbed)
     * 
     * @return Musician* Pointer to the musician (not owned)
     */
    // Musician* getMusician() { return musician.get(); }
    void* getMusician() { return nullptr; } // Stubbed implementation
    
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
    // std::unique_ptr<Musician> musician; // Temporarily commented out
    
    // Resource state
    bool fullyLoaded = false;
    bool readyForEditing = false;
    bool readyForPlayback = false;
    
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
