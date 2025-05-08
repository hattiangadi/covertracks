#pragma once

#include <JuceHeader.h>
#include "../../Plugins/PluginGraph.h"

namespace covertracks {

/**
 * @brief Wrapper class for PluginGraph that handles MIDI playback
 * 
 * This class provides a unified interface for managing a plugin graph
 * and playing MIDI patterns through it. It serves as the sound engine
 * component of the Player architecture.
 */
class Instrument : public AudioIODeviceCallback,
                   public MidiInputCallback,
                   public Timer
{
public:
    /**
     * @brief Construct a new Instrument
     * 
     * @param name The name of the instrument
     */
    Instrument(const String& name);
    
    /**
     * @brief Destroy the Instrument
     */
    ~Instrument() override;
    
    /**
     * @brief Initialize the instrument
     * 
     * @return true If initialization was successful
     * @return false If initialization failed
     */
    void initialize();
    
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
     * @brief Play a MIDI pattern
     * 
     * @param pattern MIDI events in JUCE var/JSON format
     * @param tempo Tempo in BPM
     * @return true If playback started successfully
     * @return false If playback failed to start
     */
    bool playMIDIPattern(const var& pattern, double tempo = 120.0);
    
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
     * @brief Add MIDI listener for incoming MIDI messages
     * 
     * @param listener The listener to add
     */
    void addMidiInputListener(MidiInputCallback* listener);
    
    /**
     * @brief Remove MIDI listener
     * 
     * @param listener The listener to remove
     */
    void removeMidiInputListener(MidiInputCallback* listener);
    
    /**
     * @brief Get direct access to the plugin graph
     * 
     * @return PluginGraph* Pointer to the plugin graph (not owned)
     */
    PluginGraph* getPluginGraph() { return pluginGraph.get(); }
    
    /**
     * @brief Check if the instrument is currently playing
     * 
     * @return true If playing
     * @return false If not playing
     */
    bool isPlaying() const;
    
    /**
     * @brief Check if the instrument is currently paused
     * 
     * @return true If paused
     * @return false If not paused
     */
    bool isPaused() const;
    
    /**
     * @brief Set callback for pattern completion
     * 
     * @param callback Function to call when pattern playback completes
     */
    void setOnPatternFinishedCallback(std::function<void()> callback);
    
    /**
     * @brief Get the current playback position in seconds
     * 
     * @return double Current position in seconds
     */
    double getCurrentPosition() const { return currentTime; }
    
    /**
     * @brief Get the total duration of the current pattern in seconds
     * 
     * @return double Total duration in seconds
     */
    double getTotalDuration() const { return totalDuration; }
    
    /**
     * @brief Set callback for pattern completion
     * 
     * @param callback Function to call when pattern playback completes
     */
    void setPatternFinishedCallback(std::function<void()> callback) {
        onPatternFinished = std::move(callback);
    }
    
private:
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallback(const float** inputChannelData,
                              int numInputChannels,
                              float** outputChannelData,
                              int numOutputChannels,
                              int numSamples);
    
    void audioDeviceAboutToStart(AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    // MidiInputCallback implementation
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;
    
    // Timer implementation
    void timerCallback() override;
    
    // Process MIDI events for the current buffer
    void processMIDIEvents(MidiBuffer& midiBuffer, int numSamples);
    
    // Update the current time based on sample count
    void updateCurrentTime(int numSamples);
    
    // Convert JSON pattern to MIDI sequence
    std::unique_ptr<MidiMessageSequence> convertJSONToMIDISequence(const var& events);
    
    // Core components
    String instrumentName;
    std::unique_ptr<AudioDeviceManager> deviceManager;
    std::unique_ptr<AudioPluginFormatManager> pluginFormatManager;
    std::unique_ptr<KnownPluginList> pluginList;
    std::unique_ptr<PluginGraph> pluginGraph;
    std::unique_ptr<AudioProcessorGraph> processorGraph; // For actual audio processing
    
    // MIDI playback state
    bool playing = false;
    bool paused = false;
    std::unique_ptr<MidiMessageSequence> currentSequence;
    int nextEventIndex = 0;
    double currentTime = 0.0;
    double sampleRate = 44100.0;
    double totalDuration = 0.0;
    double tempo = 120.0; // BPM
    
    // Callback for pattern completion
    std::function<void()> onPatternFinished;
    
    // MIDI input listeners
    Array<MidiInputCallback*> midiInputListeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Instrument)
};

} // namespace covertracks