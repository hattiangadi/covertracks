#pragma once

#include <JuceHeader.h>
// Note: PluginGraph.h removed - now using Tracktion Track-based plugin management
#include "../../Audio/AudioEngineInterface.h"

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
AudioEngineImpl* engine = nullptr);
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
bool connectToAudioEngine(AudioEngineImpl* engine);
void disconnectFromAudioEngine();
bool isConnectedToAudioEngine() const { return audioEngine != nullptr; }
    
// Static method to notify all instruments that the AudioEngine is being destroyed
static void notifyAudioEngineDestroyed();

// Graph management (existing methods)
// Note: loadPluginGraph removed - use Player::addTracktionPlugin() instead
// Note: getPluginGraphState removed - plugins now managed by Tracktion Track
// Note: getPluginGraph removed - use Player::getTracktionTrack() instead
// Note: getProcessorGraph removed - audio processing now handled by TracktionAudioEngine

// MIDI pattern playback (existing methods)
bool playMIDIPattern(const var& pattern, double tempo = 120.0);
bool playMIDISequence(const MidiMessageSequence& sequence, double tempo = 120.0);
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
AudioEngineImpl* audioEngine = nullptr;

// References to shared plugin environment (not owned)
AudioPluginFormatManager& formatManager;
KnownPluginList& knownPluginList;

// Note: Custom plugin graphs removed - now using Tracktion Track-based plugin management
// Audio processing is handled by TracktionAudioEngine through Player's dedicated Track

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