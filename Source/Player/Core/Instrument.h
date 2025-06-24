#pragma once

#include <JuceHeader.h>
#include "../../Audio/AudioEngineInterface.h"

// Include Tracktion Engine headers
#include "../../../modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"
namespace te = tracktion;

namespace covertracks {

/**
 * @brief Complete audio system - owns Tracktion Track, plugins, and playback
 * 
 * Instrument is a self-contained audio system that:
 * - Registers itself with TracktionAudioEngine 
 * - Owns and manages its dedicated Tracktion Track
 * - Handles plugin management (add/remove/clear plugins)
 * - Processes MIDI chart playback through its Track
 * - Handles all audio concerns independently
 * 
 * This class represents the "audio engine" part of a Player, completely
 * separated from the orchestration concerns handled by Player.
 */
class Instrument : public juce::Timer,
                   public juce::MidiInputCallback
{
public:
    /**
     * @brief Create an Instrument
     * @param name Instrument name (used for Track naming)
     * @param formatManager Reference to shared plugin format manager
     * @param knownPluginList Reference to shared plugin list
     */
    Instrument(const juce::String& name,
               juce::AudioPluginFormatManager& formatManager,
               juce::KnownPluginList& knownPluginList);
    
    ~Instrument() override;
    
    // =========================================================================
    // Audio Engine Integration (Self-Managed)
    // =========================================================================
    
    /**
     * @brief Register this instrument with TracktionAudioEngine
     * Creates and owns a dedicated Tracktion Track
     * @return true if registration successful
     */
    bool registerWithAudioEngine();
    
    /**
     * @brief Unregister from TracktionAudioEngine
     * Cleans up Track and audio resources
     */
    void unregisterFromAudioEngine();
    
    /**
     * @brief Check if registered with audio engine
     * @return true if currently registered
     */
    bool isRegisteredWithAudioEngine() const { return myTrack != nullptr; }
    
    /**
     * @brief Get this instrument's dedicated Tracktion Track
     * @return Pointer to Track, or nullptr if not registered
     */
    te::Track* getTrack() const { return myTrack; }
    
    // =========================================================================
    // Plugin Management (Complete Ownership)
    // =========================================================================
    
    /**
     * @brief Add a plugin to this instrument's Track
     * @param pluginDesc Plugin description to add
     * @return true if plugin added successfully
     */
    bool addPlugin(const juce::PluginDescription& pluginDesc);
    
    /**
     * @brief Remove all plugins from this instrument's Track  
     */
    void clearPlugins();
    
    /**
     * @brief Add a default built-in synthesizer (optional)
     * @return true if added successfully
     */
    bool addDefaultSynth();
    
    /**
     * @brief Remove a plugin at specified index from this instrument's Track
     * @param index Index of plugin to remove (0-based)
     * @return true if plugin removed successfully
     */
    bool removePlugin(int index);
    
    /**
     * @brief Move a plugin from one position to another in this instrument's Track
     * @param fromIndex Current position of plugin (0-based)
     * @param toIndex Target position of plugin (0-based)
     * @return true if plugin moved successfully
     */
    bool movePlugin(int fromIndex, int toIndex);
    
    /**
     * @brief Get number of plugins on this instrument's Track
     * @return Plugin count
     */
    int getPluginCount() const;
    
    /**
     * @brief Get list of plugins on this instrument's Track
     * @return Array of plugin names
     */
    juce::StringArray getPluginNames() const;
    
    /**
     * @brief Get plugin instance by index
     * @param index Index of plugin to get (0-based)
     * @return Pointer to plugin, or nullptr if index invalid
     */
    te::Plugin* getPlugin(int index) const;
    
    /**
     * @brief Get JUCE AudioProcessor by index (for external plugins)
     * @param index Index of plugin to get (0-based) 
     * @return Pointer to AudioProcessor, or nullptr if not a JUCE plugin
     */
    juce::AudioProcessor* getJucePlugin(int index) const;
    
    // =========================================================================
    // MIDI Chart Playback (Complete Audio Control)
    // =========================================================================
    
    /**
     * @brief Play a MIDI chart through this instrument's Track
     * @param chart MIDI chart data (JSON format)
     * @param tempo Playback tempo in BPM
     * @return true if playback started successfully
     */
    bool playChart(const juce::var& chart, double tempo = 120.0);
    
    /**
     * @brief Play a MIDI sequence through this instrument's Track
     * @param sequence MIDI sequence to play
     * @param tempo Playback tempo in BPM  
     * @return true if playback started successfully
     */
    bool playMidiSequence(const juce::MidiMessageSequence& sequence, double tempo = 120.0);
    
    /**
     * @brief Stop current playback
     */
    void stopPlayback();
    
    /**
     * @brief Pause current playback
     */
    void pausePlayback();
    
    /**
     * @brief Resume paused playback
     */
    void resumePlayback();
    
    /**
     * @brief Check if currently playing
     * @return true if playing (not paused)
     */
    bool isPlaying() const { return isPlaybackActive && !isPlaybackPaused; }
    
    /**
     * @brief Check if playback is paused
     * @return true if paused
     */
    bool isPaused() const { return isPlaybackActive && isPlaybackPaused; }
    
    /**
     * @brief Set callback for when chart playback finishes
     * @param callback Function to call on completion
     */
    void setPlaybackFinishedCallback(std::function<void()> callback);
    
    
    // =========================================================================
    // MIDI Input Handling
    // =========================================================================
    
    /**
     * @brief Handle incoming MIDI messages
     * Routes MIDI to this instrument's Track for real-time playing
     */
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    
    /**
     * @brief Send MIDI message to this instrument's Track
     * @param message MIDI message to process
     */
    void processMidiMessage(const juce::MidiMessage& message);
    
    // =========================================================================
    // State Management
    // =========================================================================
    
    /**
     * @brief Get current plugin state for persistence (includes configurations)
     * @return Plugin state as XML
     */
    std::unique_ptr<juce::XmlElement> getPluginState() const;
    
    /**
     * @brief Restore plugin state from XML (includes configurations)
     * @param state Plugin state XML
     * @return true if restored successfully
     */
    bool restorePluginState(const juce::XmlElement& state);
    
    /**
     * @brief Save complete instrument state including plugin parameters
     * @return JSON representation of instrument state
     */
    juce::var saveInstrumentState() const;
    
    /**
     * @brief Load complete instrument state including plugin parameters
     * @param state JSON representation of instrument state
     * @return true if loaded successfully
     */
    bool loadInstrumentState(const juce::var& state);
    
    /**
     * @brief Get instrument name
     * @return Instrument name
     */
    const juce::String& getName() const { return instrumentName; }
    
    /**
     * @brief Convert MIDI chart to sequence for pre-loading
     * @param chart MIDI chart data (JSON format)
     * @return MIDI message sequence, or nullptr if conversion failed
     */
    std::unique_ptr<juce::MidiMessageSequence> convertChartToMidiSequence(const juce::var& chart);
    
    // =========================================================================
    // Timer Implementation (for MIDI playback timing)
    // =========================================================================
    
    void timerCallback() override;

private:
    // Core properties
    juce::String instrumentName;
    
    // Plugin environment references (not owned)
    juce::AudioPluginFormatManager& formatManager;
    juce::KnownPluginList& knownPluginList;
    
    // Tracktion resources (owned)
    te::Track* myTrack = nullptr;  // This instrument's dedicated Track
    
    // MIDI playback state
    std::unique_ptr<juce::MidiMessageSequence> currentSequence;
    std::atomic<bool> isPlaybackActive{false};
    std::atomic<bool> isPlaybackPaused{false};
    double currentPlaybackTime = 0.0;
    double playbackTempo = 120.0;
    int nextEventIndex = 0;
    
    // Callbacks
    std::function<void()> onPlaybackFinished;
    
    // Helper methods
    void updatePlaybackPosition();
    void sendMidiToTrack(const juce::MidiMessage& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Instrument)
};

} // namespace covertracks