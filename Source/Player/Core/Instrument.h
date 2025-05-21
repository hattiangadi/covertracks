#pragma once

#include <JuceHeader.h>
#include "../../Plugins/PluginGraph.h"
#include "../../Audio/AudioEngine.h"

namespace covertracks {

/**
 * @brief Wrapper class for PluginGraph that handles MIDI playback
 * 
 * This class provides a unified interface for managing a plugin graph
 * and playing MIDI patterns through it. It serves as the sound engine
 * component of the Player architecture.
 */
 class Instrument : public MidiInputCallback,
 public Timer
{
public:
// New constructor without AudioDeviceManager
Instrument(const String& name, 
AudioPluginFormatManager& formatManager,
KnownPluginList& knownPluginList,
AudioEngine* engine = nullptr);
~Instrument() override;

// Resource state management
enum class ResourceState {
Unloaded,      // Minimal resources loaded
EditReady,     // Resources needed for editing loaded
PlaybackReady  // Resources needed for playback loaded
};

ResourceState getResourceState() const { return resourceState; }

// Resource lifecycle methods
void prepareForEditing();   // Load resources needed for editing
bool prepareForPlayback();  // Load resources needed for playback, returns success
void releaseResources();    // Release all heavy resources

// AudioEngine connection methods
bool connectToAudioEngine(AudioEngine* engine);
void disconnectFromAudioEngine();
bool isConnectedToAudioEngine() const { return audioEngine != nullptr; }
    
// Static method to notify all instruments that the AudioEngine is being destroyed
static void notifyAudioEngineDestroyed();

// Graph management (existing methods)
bool loadPluginGraph(const XmlElement& graphState);
std::unique_ptr<XmlElement> getPluginGraphState();
PluginGraph* getPluginGraph() { return pluginGraph.get(); }
AudioProcessorGraph* getProcessorGraph() { return processorGraph.get(); }

// MIDI pattern playback (existing methods)
bool playMIDIPattern(const var& pattern, double tempo = 120.0);
void stopPlayback();
void pausePlayback();
void resumePlayback();

// MidiInputCallback implementation
void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;

// Timer implementation for pattern playback
void timerCallback() override;

// MIDI callback management
void addMidiInputListener(MidiInputCallback* listener);
void removeMidiInputListener(MidiInputCallback* listener);

// State queries
bool isPlaying() const;
bool isPaused() const;

// Callback for pattern completion
void setPatternFinishedCallback(std::function<void()> callback);

private:
// Core state
String instrumentName;
ResourceState resourceState = ResourceState::Unloaded;
AudioEngine* audioEngine = nullptr;

// References to shared plugin environment (not owned)
AudioPluginFormatManager& formatManager;
KnownPluginList& knownPluginList;

// Plugin and processor graphs
std::unique_ptr<PluginGraph> pluginGraph;
std::unique_ptr<AudioProcessorGraph> processorGraph;

// Audio processor player for handling audio callbacks
std::unique_ptr<AudioProcessorPlayer> audioProcessorPlayer;

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

// Track registered MIDI input device names
StringArray registeredMidiInputs;

// Helper methods
void processMIDIEvents(MidiBuffer& midiBuffer, int numSamples);
void updateCurrentTime(int numSamples);
std::unique_ptr<MidiMessageSequence> convertJSONToMIDISequence(const var& events);

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Instrument)
};

} // namespace covertracks