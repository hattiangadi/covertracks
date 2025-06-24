/*
  ==============================================================================
   TracktionAudioEngine.h
   
   Tracktion Engine-based replacement for the original AudioEngine singleton.
   
   This class replaces the custom AudioEngine with professional DAW architecture
   using Tracktion Engine to solve performance mode clicking issues.
   
   Key improvements:
   - Professional DAW audio routing and processing
   - Per-TracktionPlayer Tracktion Track isolation
   - Proper multi-track simultaneous playback
   - Professional buffer management and threading
   - Eliminates custom audio graph complexity
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>

// Include Tracktion Engine headers
#include "../../modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"

namespace te = tracktion;

namespace covertracks {
    // Forward declarations
    class TracktionPlayer;
}

namespace covertracks {

/**
 * @brief Tracktion Engine-based singleton audio engine 
 * 
 * This singleton class replaces the original AudioEngine with a professional
 * DAW architecture using Tracktion Engine. Each TracktionPlayer gets its own Tracktion
 * Track for proper isolation and professional audio routing.
 * 
 * Key Features:
 * - Professional DAW architecture with Tracktion Engine
 * - Per-TracktionPlayer Track isolation prevents audio conflicts
 * - Proper multi-track simultaneous playback
 * - Professional buffer management and threading
 * - Built-in audio quality monitoring
 * - MIDI routing through Tracktion's professional system
 */
class TracktionAudioEngine final : public juce::AudioIODeviceCallback,
                                   public juce::ChangeListener
{
public:
    // Singleton access
    static TracktionAudioEngine& getInstance();
    
    ~TracktionAudioEngine() override;
    
    // =========================================================================
    // TracktionPlayer Lifecycle Management (Thread-Safe)
    // =========================================================================
    
    /**
     * @brief Register a TracktionPlayer with the TracktionAudioEngine
     * Creates a dedicated Tracktion Track for the TracktionPlayer
     * @param player TracktionPlayer to register (must not be null)
     * @return true if registration succeeded, false if already registered
     */
    bool registerTracktionPlayer(TracktionPlayer* player);
    
    /**
     * @brief Unregister a TracktionPlayer from the TracktionAudioEngine
     * Removes the TracktionPlayer's Tracktion Track from the Edit
     * @param player TracktionPlayer to unregister
     */
    void unregisterTracktionPlayer(TracktionPlayer* player);
    
    /**
     * @brief Activate a TracktionPlayer for audio processing
     * @param player TracktionPlayer to activate (must be registered first)
     * @return true if activation succeeded
     */
    bool activateTracktionPlayer(TracktionPlayer* player);
    
    /**
     * @brief Deactivate a TracktionPlayer from audio processing
     * @param player TracktionPlayer to deactivate
     */
    void deactivateTracktionPlayer(TracktionPlayer* player);
    
    /**
     * @brief Check if TracktionPlayer is registered
     * @param player TracktionPlayer to check
     * @return true if registered
     */
    bool isTracktionPlayerRegistered(TracktionPlayer* player) const;
    
    /**
     * @brief Check if TracktionPlayer is active
     * @param player TracktionPlayer to check  
     * @return true if active
     */
    bool isTracktionPlayerActive(TracktionPlayer* player) const;
    
    // =========================================================================
    // Tracktion Track Management
    // =========================================================================
    
    /**
     * @brief Get the Tracktion Track for a TracktionPlayer
     * @param player TracktionPlayer to get track for
     * @return Pointer to the track, or nullptr if not found
     */
    te::Track* getTracktionPlayerTrack(TracktionPlayer* player);
    
    /**
     * @brief Create a MIDI clip on the TracktionPlayer's track
     * @param player TracktionPlayer to create clip for
     * @param clipName Name for the clip
     * @param startTime Start time in seconds
     * @param duration Duration in seconds
     * @return Pointer to created MIDI clip, or nullptr if failed
     */
    te::MidiClip* createMidiClipForTracktionPlayer(TracktionPlayer* player, 
                                          const juce::String& clipName,
                                          double startTime, 
                                          double duration);
    
    /**
     * @brief Start playback for a specific TracktionPlayer's track
     * @param player TracktionPlayer to start playback for
     * @return true if playback started successfully
     */
    bool startTracktionPlayerPlayback(TracktionPlayer* player);
    
    /**
     * @brief Stop playback for a specific TracktionPlayer's track
     * @param player TracktionPlayer to stop playback for
     */
    void stopTracktionPlayerPlayback(TracktionPlayer* player);
    
    /**
     * @brief Check if a TracktionPlayer's track is playing
     * @param player TracktionPlayer to check
     * @return true if the player's track is playing
     */
    bool isTracktionPlayerPlaying(TracktionPlayer* player) const;
    
    // =========================================================================
    // Tracktion Engine Access
    // =========================================================================
    
    /**
     * @brief Get the Tracktion Engine instance
     * @return Reference to the Tracktion Engine
     */
    te::Engine& getTracktionEngine();
    
    /**
     * @brief Get the current Tracktion Edit
     * @return Pointer to the current Edit, or nullptr if not initialized
     */
    te::Edit* getCurrentEdit();
    
    /**
     * @brief Get the Tracktion Transport for global playback control
     * @return Reference to the Transport
     */
    te::TransportControl& getTransport();
    
    // =========================================================================
    // MIDI Routing and Distribution
    // =========================================================================
    
    /**
     * @brief Route MIDI message to specific TracktionPlayer's track
     * @param player Target TracktionPlayer (must be active)
     * @param message MIDI message to route
     */
    void routeMidiToTracktionPlayer(TracktionPlayer* player, const juce::MidiMessage& message);
    
    /**
     * @brief Route MIDI message to all active TracktionPlayer tracks
     * @param message MIDI message to broadcast
     */
    void routeMidiToAllActiveTracktionPlayers(const juce::MidiMessage& message);
    
    /**
     * @brief Send MIDI message to a TracktionPlayer's track
     * @param player Target TracktionPlayer
     * @param message MIDI message to send
     */
    void sendMidiToTracktionPlayerTrack(TracktionPlayer* player, const juce::MidiMessage& message);
    
    // =========================================================================
    // Resource Access (Thread-Safe)
    // =========================================================================
    
    /**
     * @brief Get the shared AudioDeviceManager
     * @return Reference to the device manager
     */
    juce::AudioDeviceManager& getDeviceManager();
    
    /**
     * @brief Get the shared AudioPluginFormatManager  
     * @return Reference to the format manager
     */
    juce::AudioPluginFormatManager& getFormatManager();
    
    /**
     * @brief Get the shared KnownPluginList
     * @return Reference to the plugin list
     */
    juce::KnownPluginList& getKnownPluginList();
    
    // =========================================================================
    // Audio Quality Monitoring
    // =========================================================================
    
    /**
     * @brief Check if audio quality is good
     * @return true if no significant issues detected
     */
    bool isAudioQualityGood() const;
    
    /**
     * @brief Get current CPU usage
     * @return CPU usage as percentage (0.0 to 1.0)
     */
    double getCpuUsage() const;
    
    /**
     * @brief Get total buffer underruns since startup
     * @return Number of buffer underruns
     */
    int getBufferUnderruns() const;
    
    /**
     * @brief Reset quality monitoring counters
     */
    void resetQualityMonitoring();
    
    // =========================================================================
    // Backward Compatibility (for seamless replacement)
    // =========================================================================
    
    // All the methods from the original AudioEngine for drop-in replacement
    bool addTracktionPlayerGraph(TracktionPlayer* player) { return activateTracktionPlayer(player); }
    void removeTracktionPlayerGraph(TracktionPlayer* player) { deactivateTracktionPlayer(player); }
    bool isTracktionPlayerGraphIntegrated(TracktionPlayer* player) const { return isTracktionPlayerActive(player); }
    void setMidiInputEnabled(bool enabled);
    bool isMidiInputEnabled() const;
    void sendMidiMessage(const juce::MidiMessage& message);
    void enableMidiInput(bool enabled);
    bool registerMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback);
    void unregisterMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback);
    void registerEditModeProcessor(juce::AudioProcessor* processor);
    void unregisterEditModeProcessor();
    double getSampleRate() const;
    int getBufferSize() const;
    int getInputLatencySamples() const;
    int getOutputLatencySamples() const;
    void showAudioSettings(juce::Component* parentComponent);
    void setDoublePrecisionProcessing(bool doublePrecision);
    void unregisterProcessor(juce::AudioProcessor* processor);
    void processMidiKeyboardState(juce::MidiKeyboardState& keyboardState);
    
    // =========================================================================
    // AudioIODeviceCallback Implementation
    // =========================================================================
    
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    // =========================================================================
    // ChangeListener Implementation  
    // =========================================================================
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    // Private constructor for singleton
    TracktionAudioEngine();
    
    // Initialize Tracktion resources
    bool initializeTracktionEngine();
    void shutdownTracktionEngine();
    void scanForPlugins();
    
    // =========================================================================
    // TracktionPlayer-Track Mapping
    // =========================================================================
    
    struct TracktionPlayerTrackInfo {
        TracktionPlayer* player;
        te::Track* track;
        std::atomic<bool> isActive{false};
        std::atomic<bool> isPlaying{false};
        juce::String trackName;
        
        TracktionPlayerTrackInfo(TracktionPlayer* p, te::Track* t, const juce::String& name) 
            : player(p), track(t), trackName(name) {}
    };
    
    // Thread-safe player-track registry
    mutable juce::CriticalSection playerTrackMutex;
    std::map<TracktionPlayer*, std::unique_ptr<TracktionPlayerTrackInfo>> playerTracks;
    
    // =========================================================================
    // Tracktion Engine Resources
    // =========================================================================
    
    // Core Tracktion resources
    std::unique_ptr<te::Engine> tracktionEngine;
    std::unique_ptr<te::Edit> currentEdit;
    juce::File editFile;
    
    // Audio resources (reused from original for compatibility)
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    std::unique_ptr<juce::AudioPluginFormatManager> formatManager;
    std::unique_ptr<juce::KnownPluginList> knownPluginList;
    
    // =========================================================================
    // Audio Processing State
    // =========================================================================
    
    // Audio state
    std::atomic<double> currentSampleRate{44100.0};
    std::atomic<int> currentBufferSize{512};
    std::atomic<bool> isAudioActive{false};
    std::atomic<bool> midiInputEnabled{true};
    
    // Quality monitoring
    std::atomic<double> cpuUsage{0.0};
    std::atomic<int> bufferUnderrunCount{0};
    std::atomic<bool> audioQualityGood{true};
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point lastProcessTime;
    std::atomic<double> averageProcessTime{0.0};
    
    // =========================================================================
    // MIDI Processing
    // =========================================================================
    
    // MIDI state
    juce::MidiKeyboardState keyboardState;
    juce::MidiBuffer currentMidiBuffer;
    mutable juce::CriticalSection midiBufferMutex;
    
    // Edit mode processor support (for backward compatibility)
    mutable juce::CriticalSection editModeProcessorMutex;
    juce::AudioProcessor* editModeProcessor = nullptr;
    
    // =========================================================================
    // Tracktion Engine Helpers
    // =========================================================================
    
    // Create a new track for a player
    te::Track* createTrackForTracktionPlayer(TracktionPlayer* player);
    
    // Remove a track for a player
    void removeTrackForTracktionPlayer(TracktionPlayer* player);
    
    // Update quality metrics
    void updateQualityMetrics(double processingTime);
    
    // Check for audio issues
    void checkAudioQuality();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TracktionAudioEngine)
};

} // namespace covertracks