#include "Instrument.h"
#include "../../Audio/AudioEngineInterface.h"
#include <iostream>

namespace covertracks {

Instrument::Instrument(const String& name, 
                     AudioPluginFormatManager& fm,
                     KnownPluginList& kpl,
                     AudioEngineImpl* engine)
    : instrumentName(name), 
      resourceState(ResourceState::Unloaded),
      audioEngine(engine),
      formatManager(fm),
      knownPluginList(kpl)
{
    // std::cout << "Creating Instrument: " << name << std::endl;
    
    // Note: Custom plugin graph initialization removed - now using Tracktion Track-based management
    
    // Start the timer at 20ms interval (50Hz) for MIDI playback tracking
    startTimer(20);
}

Instrument::~Instrument()
{
    stopTimer();
    stopPlayback();
    disconnectFromAudioEngine();
}

void Instrument::prepareForEditing()
{
    // Note: Custom graph resource loading removed - plugins now managed by Tracktion Track
    if (resourceState == ResourceState::Unloaded)
    {
        resourceState = ResourceState::EditReady;
        // std::cout << "Instrument " << instrumentName << " prepared for editing" << std::endl;
    }
}

bool Instrument::prepareForPlayback()
{
    // Note: Custom graph preparation removed - audio processing now handled by TracktionAudioEngine
    if (resourceState == ResourceState::EditReady || resourceState == ResourceState::Unloaded)
    {
        resourceState = ResourceState::PlaybackReady;
        // std::cout << "Instrument " << instrumentName << " prepared for playback" << std::endl;
        return true;
    }
    return true; // Already ready
}

void Instrument::releaseResources()
{
    // Stop playback and release MIDI resources
    stopPlayback();
    resourceState = ResourceState::Unloaded;
    // std::cout << "Released resources for " << instrumentName << std::endl;
}

bool Instrument::connectToAudioEngine(AudioEngineImpl* engine)
{
    if (engine == nullptr)
    {
        std::cerr << "Cannot connect to null audio engine" << std::endl;
        return false;
    }
    
    audioEngine = engine;
    // std::cout << "Connected " << instrumentName << " to AudioEngine" << std::endl;
    return true;
}

void Instrument::disconnectFromAudioEngine()
{
    audioEngine = nullptr;
    // std::cout << "Disconnected " << instrumentName << " from AudioEngine" << std::endl;
}

// Static method implementation
void Instrument::notifyAudioEngineDestroyed()
{
    // Note: With Tracktion architecture, this is handled by TracktionAudioEngine lifecycle
    std::cout << "AudioEngine destroyed notification received" << std::endl;
}

// MIDI playback methods
bool Instrument::playMIDISequence(const MidiMessageSequence& sequence, double newTempo)
{
    // Stop any current playback
    stopPlayback();
    
    // Set the new tempo if provided
    if (newTempo > 0)
    {
        tempo = newTempo;
    }
    
    // Make sure we're prepared for playback
    if (resourceState != ResourceState::PlaybackReady)
    {
        if (!prepareForPlayback())
        {
            std::cerr << "Failed to prepare for playback" << std::endl;
            return false;
        }
    }
    
    // Make sure the audio engine is available
    if (audioEngine == nullptr)
    {
        std::cerr << "No audio engine available for playback" << std::endl;
        return false;
    }
    
    // Create a copy of the sequence for playback
    currentSequence = std::make_unique<MidiMessageSequence>(sequence);
    
    // Calculate total duration
    totalDuration = 0.0;
    if (currentSequence->getNumEvents() > 0)
    {
        auto lastEvent = currentSequence->getEventPointer(currentSequence->getNumEvents() - 1);
        totalDuration = lastEvent->message.getTimeStamp();
    }
    
    // Reset playback state
    nextEventIndex = 0;
    currentTime = 0.0;
    paused = false;
    playing = true;
    
    // std::cout << "Started MIDI sequence playback with " << currentSequence->getNumEvents() 
    //           << " events, duration: " << totalDuration << " seconds" << std::endl;
    
    return true;
}

bool Instrument::playMIDIPattern(const var& pattern, double newTempo)
{
    // Convert the pattern to a MIDI sequence
    auto sequence = convertJSONToMIDISequence(pattern);
    if (sequence == nullptr)
    {
        std::cerr << "Failed to convert pattern to MIDI sequence" << std::endl;
        return false;
    }
    
    return playMIDISequence(*sequence, newTempo);
}

void Instrument::stopPlayback()
{
    if (playing)
    {
        playing = false;
        paused = false;
        currentSequence.reset();
        nextEventIndex = 0;
        currentTime = 0.0;
        
        // Call completion callback if set
        if (onPatternFinished)
        {
            onPatternFinished();
        }
        
        // std::cout << "Stopped playback for " << instrumentName << std::endl;
    }
}

void Instrument::pausePlayback()
{
    if (playing && !paused)
    {
        paused = true;
        // std::cout << "Paused playback for " << instrumentName << std::endl;
    }
}

void Instrument::resumePlayback()
{
    if (playing && paused)
    {
        paused = false;
        // std::cout << "Resumed playback for " << instrumentName << std::endl;
    }
}

bool Instrument::isPlaying() const
{
    return playing && !paused;
}

bool Instrument::isPaused() const
{
    return playing && paused;
}

void Instrument::setPatternFinishedCallback(std::function<void()> callback)
{
    onPatternFinished = callback;
}

// MidiInputCallback implementation
void Instrument::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    // Forward MIDI messages to listeners
    for (auto* listener : midiInputListeners)
    {
        if (listener != nullptr)
        {
            listener->handleIncomingMidiMessage(source, message);
        }
    }
    
    // Note: Direct audio processing removed - MIDI now routed through TracktionAudioEngine
}

void Instrument::addMidiInputListener(MidiInputCallback* listener)
{
    if (listener != nullptr && !midiInputListeners.contains(listener))
    {
        midiInputListeners.add(listener);
    }
}

void Instrument::removeMidiInputListener(MidiInputCallback* listener)
{
    midiInputListeners.removeAllInstancesOf(listener);
}

// Timer implementation for MIDI playback
void Instrument::timerCallback()
{
    if (playing && !paused && currentSequence != nullptr)
    {
        // Update current time based on tempo
        double deltaTime = 0.02; // 20ms timer interval
        currentTime += deltaTime;
        
        // Process MIDI events that should be played at this time
        bool addedEvents = false;
        while (nextEventIndex < currentSequence->getNumEvents())
        {
            auto* eventHolder = currentSequence->getEventPointer(nextEventIndex);
            if (eventHolder->message.getTimeStamp() <= currentTime)
            {
                // Send MIDI event through TracktionAudioEngine
                if (audioEngine)
                {
                    // Note: MIDI routing now handled by TracktionAudioEngine
                    // The actual MIDI processing will be done by the Player's Tracktion Track
                }
                
                nextEventIndex++;
                addedEvents = true;
            }
            else
            {
                break; // No more events to process at this time
            }
        }
        
        // Check if we've reached the end of the sequence
        if (nextEventIndex >= currentSequence->getNumEvents() && !addedEvents)
        {
            stopTimer();
            // Note: Don't automatically call stopPlayback() - let the TracktionAudioEngine
            // handle player lifecycle. Players should remain active for real-time audio.
        }
    }
}

// Helper methods
std::unique_ptr<MidiMessageSequence> Instrument::convertJSONToMIDISequence(const var& events)
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    if (!events.isArray())
    {
        std::cerr << "Expected array of MIDI events" << std::endl;
        return sequence;
    }
    
    Array<var>* eventArray = events.getArray();
    for (const auto& eventVar : *eventArray)
    {
        if (eventVar.isObject())
        {
            auto* eventObj = eventVar.getDynamicObject();
            if (eventObj != nullptr)
            {
                // Extract event properties
                double time = eventObj->getProperty("time");
                String type = eventObj->getProperty("type").toString();
                int note = eventObj->getProperty("note");
                int velocity = eventObj->getProperty("velocity");
                
                // Create MIDI message based on type
                MidiMessage message;
                if (type == "noteOn" && velocity > 0)
                {
                    message = MidiMessage::noteOn(1, note, static_cast<uint8>(velocity));
                }
                else if (type == "noteOff" || (type == "noteOn" && velocity == 0))
                {
                    message = MidiMessage::noteOff(1, note, static_cast<uint8>(velocity));
                }
                else
                {
                    continue; // Skip unknown event types
                }
                
                message.setTimeStamp(time);
                sequence->addEvent(message);
            }
        }
    }
    
    sequence->updateMatchedPairs();
    return sequence;
}

} // namespace covertracks