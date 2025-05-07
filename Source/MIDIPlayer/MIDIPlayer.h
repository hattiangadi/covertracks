#pragma once
#include <JuceHeader.h>
#include "../VST/VSTHost.h"
#include <nlohmann/json.hpp>

class MIDIPlayer : public juce::AudioSource {
public:
    /**
     * @brief Construct a new MIDIPlayer with JSON configuration
     * 
     * @param config JSON configuration containing:
     *   - player_id: Unique identifier for this player
     *   - instrument_type: Type of instrument (e.g., "piano", "bass", "drums")
     *   - plugin_id: (Optional) ID of the plugin to use, defaults to sampler
     *   - sample_name: (Optional) Name of the sample to load for sampler plugin
     *   - additional instrument-specific parameters
     */
    MIDIPlayer(const nlohmann::json& config);
    ~MIDIPlayer() override;
    
    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    
    // Player interface
    bool initialize();
    void playMIDISequence(const juce::MidiMessageSequence& sequence, const juce::String& patternId);
    void stop();
    
    // State access
    bool isPlaying() const { return playing; }
    juce::String getInstrumentType() const { return instrumentType; }
    juce::String getCurrentPatternId() const { return currentPatternId; }
    
    // Callback for pattern completion
    void setPatternFinishedCallback(std::function<void()> callback);
    
private:
    // Setup helpers
    bool setupAudio();
    bool setupVST();
    
    // MIDI playback
    void processMIDISequence(juce::MidiBuffer& midiBuffer, int numSamples);
    
    // Member variables
    nlohmann::json config; // Full configuration
    juce::String playerId;
    juce::String instrumentType;
    juce::String pluginId;
    juce::String sampleName;
    
    // Additional configuration parameters
    int ppq = 480;
    double tempo = 120.0;
    
    // Audio components
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    std::unique_ptr<coverband::VSTHost> vstHost;
    
    // Playback state
    bool playing = false;
    juce::MidiMessageSequence currentSequence;
    juce::String currentPatternId;
    double sampleRate = 44100.0;
    int nextEventIndex = 0;
    double currentTime = 0.0;
    
    // Callback
    std::function<void()> patternFinishedCallback;
};