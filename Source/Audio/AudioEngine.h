/*
  ==============================================================================
   AudioEngine.h
   
   Centralized singleton audio engine for managing shared audio resources
   and coordinating multiple Players without conflicts.
   
   Key improvements:
   - Singleton pattern for shared access
   - Thread-safe Player registration
   - Centralized MIDI routing 
   - Audio quality monitoring
   - Buffer underrun prevention
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>

namespace covertracks {
    // Forward declarations
    class Player;
}

namespace covertracks {

/**
 * @brief Centralized singleton audio engine for managing all audio resources
 * 
 * This singleton class manages all shared audio resources and coordinates
 * multiple Players to prevent audio conflicts, clicking, and buffer underruns.
 * 
 * Key Features:
 * - Single point of audio device access
 * - Thread-safe Player registration and coordination  
 * - Centralized MIDI routing and distribution
 * - Audio quality monitoring and buffer underrun prevention
 * - Master audio graph hosting all Player audio graphs
 */
class AudioEngine final : public juce::AudioIODeviceCallback,
                          public juce::ChangeListener
{
public:
    // Singleton access
    static AudioEngine& getInstance();
    
    // Copy/move constructors handled by JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    
    ~AudioEngine() override;
    
    // =========================================================================
    // Player Lifecycle Management (Thread-Safe)
    // =========================================================================
    
    /**
     * @brief Register a Player with the AudioEngine
     * @param player Player to register (must not be null)
     * @return true if registration succeeded, false if already registered
     */
    bool registerPlayer(Player* player);
    
    /**
     * @brief Unregister a Player from the AudioEngine
     * @param player Player to unregister
     */
    void unregisterPlayer(Player* player);
    
    /**
     * @brief Activate a Player for audio processing
     * @param player Player to activate (must be registered first)
     * @return true if activation succeeded
     */
    bool activatePlayer(Player* player);
    
    /**
     * @brief Deactivate a Player from audio processing
     * @param player Player to deactivate
     */
    void deactivatePlayer(Player* player);
    
    /**
     * @brief Check if Player is registered
     * @param player Player to check
     * @return true if registered
     */
    bool isPlayerRegistered(Player* player) const;
    
    /**
     * @brief Check if Player is active
     * @param player Player to check  
     * @return true if active
     */
    bool isPlayerActive(Player* player) const;
    
    // =========================================================================
    // MIDI Routing and Distribution
    // =========================================================================
    
    /**
     * @brief Route MIDI message to specific Player
     * @param player Target Player (must be active)
     * @param message MIDI message to route
     */
    void routeMidiToPlayer(Player* player, const juce::MidiMessage& message);
    
    /**
     * @brief Route MIDI message to all active Players
     * @param message MIDI message to broadcast
     */
    void routeMidiToAllActivePlayers(const juce::MidiMessage& message);
    
    /**
     * @brief Enable/disable MIDI input processing
     * @param enabled Whether to process MIDI input
     */
    void setMidiInputEnabled(bool enabled);
    
    /**
     * @brief Check if MIDI input is enabled
     * @return true if MIDI input is enabled
     */
    bool isMidiInputEnabled() const;
    
    /**
     * @brief Send MIDI message to all active players
     * @param message MIDI message to send
     */
    void sendMidiMessage(const juce::MidiMessage& message);
    
    /**
     * @brief Enable/disable MIDI input globally
     * @param enabled Whether to enable MIDI input
     */
    void enableMidiInput(bool enabled);
    
    /**
     * @brief Register MIDI input callback (backward compatibility)
     * @param deviceName MIDI device name
     * @param callback Callback object
     * @return true if successful
     */
    bool registerMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback);
    
    /**
     * @brief Unregister MIDI input callback (backward compatibility)
     * @param deviceName MIDI device name  
     * @param callback Callback object
     */
    void unregisterMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback);
    
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
    // Audio Device Management
    // =========================================================================
    
    /**
     * @brief Get current sample rate
     * @return Sample rate in Hz
     */
    double getSampleRate() const;
    
    /**
     * @brief Get current buffer size
     * @return Buffer size in samples
     */
    int getBufferSize() const;
    
    /**
     * @brief Get input latency
     * @return Input latency in samples
     */
    int getInputLatencySamples() const;
    
    /**
     * @brief Get output latency
     * @return Output latency in samples
     */
    int getOutputLatencySamples() const;
    
    /**
     * @brief Show audio settings dialog
     * @param parentComponent Parent component for dialog
     */
    void showAudioSettings(juce::Component* parentComponent);
    
    /**
     * @brief Set double precision processing (backward compatibility)
     * @param doublePrecision Whether to use double precision
     */
    void setDoublePrecisionProcessing(bool doublePrecision);
    
    /**
     * @brief Unregister processor (backward compatibility - no-op)
     * @param processor Processor to unregister
     */
    void unregisterProcessor(juce::AudioProcessor* processor);
    
    /**
     * @brief Process MIDI keyboard state (backward compatibility)
     * @param keyboardState Keyboard state to process
     */
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
    AudioEngine();
    
    // Initialize audio resources
    bool initializeAudioResources();
    void shutdownAudioResources();
    
    // =========================================================================
    // Player Management Internal
    // =========================================================================
    
    struct PlayerAudioNode {
        Player* player;
        juce::AudioProcessorGraph::Node::Ptr graphNode;
        std::atomic<bool> isActive{false};
        std::atomic<bool> isPlaying{false};
        
        PlayerAudioNode(Player* p) : player(p) {}
    };
    
    // Thread-safe player registry
    mutable juce::CriticalSection playerRegistryMutex;
    std::map<Player*, std::unique_ptr<PlayerAudioNode>> playerNodes;
    
    // =========================================================================
    // Audio Resources (Owned by AudioEngine)
    // =========================================================================
    
    // Core audio resources
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    std::unique_ptr<juce::AudioPluginFormatManager> formatManager;
    std::unique_ptr<juce::KnownPluginList> knownPluginList;
    
    // Master audio processing graph
    std::unique_ptr<juce::AudioProcessorGraph> masterGraph;
    juce::AudioProcessorGraph::Node::Ptr midiInputNode;
    juce::AudioProcessorGraph::Node::Ptr audioOutputNode;
    
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
    
    // Process MIDI events for active players
    void processMidiEvents(juce::MidiBuffer& midiBuffer, int numSamples);
    
    // =========================================================================
    // Audio Graph Management
    // =========================================================================
    
    // Initialize master audio graph
    bool initializeMasterGraph();
    
    // Add/remove player nodes in master graph
    bool addPlayerToMasterGraph(PlayerAudioNode* playerNode);
    void removePlayerFromMasterGraph(PlayerAudioNode* playerNode);
    
    // =========================================================================
    // Quality Monitoring Internal
    // =========================================================================
    
    // Update quality metrics
    void updateQualityMetrics(double processingTime);
    
    // Check for audio issues
    void checkAudioQuality();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace covertracks