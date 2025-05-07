#include "Instrument.h"
#include <iostream>

namespace covertracks {

Instrument::Instrument(const String& name)
    : instrumentName(name), 
      deviceManager(std::make_unique<AudioDeviceManager>()),
      pluginGraph(std::make_unique<PluginGraph>())
{
    // Output initialization message
    std::cout << "Creating Instrument: " << name << std::endl;
}

Instrument::~Instrument()
{
    // Stop playback and cleanup
    stopPlayback();
    
    // Stop timer if running
    stopTimer();
    
    // Remove as callback from device manager
    if (deviceManager != nullptr)
    {
        deviceManager->removeAudioCallback(this);
        deviceManager->removeMidiInputCallback(String(), this);
    }
    
    // Output cleanup message
    std::cout << "Destroying Instrument: " << instrumentName << std::endl;
}

bool Instrument::initialize()
{
    // Initialize the device manager first
    auto error = deviceManager->initialiseWithDefaultDevices(2, 2);
    
    if (error.isNotEmpty())
    {
        std::cerr << "Error initializing audio device: " << error << std::endl;
        return false;
    }
    
    // Register as audio callback
    deviceManager->addAudioCallback(this);
    
    // Enable MIDI input
    deviceManager->setMidiInputEnabled(String(), true);
    deviceManager->addMidiInputCallback(String(), this);
    
    // Initialize the plugin graph
    if (pluginGraph == nullptr)
    {
        pluginGraph = std::make_unique<PluginGraph>();
    }
    
    // Initialize the plugin graph with device manager's settings
    auto& deviceManager = this->deviceManager;
    auto currentAudioDevice = deviceManager->getCurrentAudioDevice();
    
    if (currentAudioDevice != nullptr)
    {
        // Set sample rate and buffer size from the audio device
        double sampleRate = currentAudioDevice->getCurrentSampleRate();
        int bufferSize = currentAudioDevice->getCurrentBufferSizeSamples();
        
        // Initialize the plugin graph's processor
        pluginGraph->setPlayConfigDetails(currentAudioDevice->getActiveInputChannels().countNumberOfSetBits(),
                                        currentAudioDevice->getActiveOutputChannels().countNumberOfSetBits(),
                                        sampleRate, bufferSize);
        
        // Store the sample rate for MIDI timing calculations
        this->sampleRate = sampleRate;
    }
    else
    {
        // Use default values if no audio device is available
        pluginGraph->setPlayConfigDetails(2, 2, 44100.0, 512);
    }
    
    // Prepare the plugin graph for playback
    pluginGraph->prepareToPlay(sampleRate, 512);
    
    // Start the timer at 20ms interval (50Hz) for MIDI playback tracking
    startTimer(20);
    
    std::cout << "Instrument initialized: " << instrumentName 
             << " (Sample rate: " << sampleRate << "Hz)" << std::endl;
    
    return true;
}

bool Instrument::loadPluginGraph(const XmlElement& graphState)
{
    if (!graphState.hasTagName("FILTERGRAPH"))
    {
        std::cerr << "Invalid plugin graph XML (missing FILTERGRAPH tag)" << std::endl;
        return false;
    }
    
    // Ensure we have a plugin graph
    if (pluginGraph == nullptr)
    {
        pluginGraph = std::make_unique<PluginGraph>();
    }
    
    // Attempt to restore from XML
    bool result = pluginGraph->restoreFromXml(graphState);
    
    if (result)
    {
        std::cout << "Plugin graph loaded successfully for " << instrumentName << std::endl;
    }
    else
    {
        std::cerr << "Failed to load plugin graph for " << instrumentName << std::endl;
    }
    
    return result;
}

std::unique_ptr<XmlElement> Instrument::getPluginGraphState()
{
    if (pluginGraph == nullptr)
    {
        return nullptr;
    }
    
    return pluginGraph->createXml();
}

bool Instrument::playMIDIPattern(const json& pattern, double newTempo)
{
    // Stop any existing playback first
    stopPlayback();
    
    // Convert the JSON pattern to a MIDI sequence
    currentSequence = convertJSONToMIDISequence(pattern);
    
    if (currentSequence == nullptr || currentSequence->getNumEvents() == 0)
    {
        std::cerr << "Failed to create MIDI sequence or sequence is empty" << std::endl;
        return false;
    }
    
    // Update tempo
    tempo = newTempo;
    
    // Calculate total duration in seconds based on the sequence
    // Find the end time of the last event
    auto& lastEvent = currentSequence->getEventPointer(currentSequence->getNumEvents() - 1)->message;
    double lastTimestamp = lastEvent.getTimeStamp();
    
    // Convert from ticks to seconds
    // Assuming 480 ticks per quarter note (standard MIDI resolution)
    // tempo is in beats per minute, so beats per second is tempo/60
    double ticksPerSecond = 480.0 * (tempo / 60.0);
    totalDuration = lastTimestamp / ticksPerSecond;
    
    // Make sure we add a bit of extra time for note release
    totalDuration += 0.5; // Add half a second
    
    // Reset playback state
    nextEventIndex = 0;
    currentTime = 0.0;
    
    // Start playback
    playing = true;
    paused = false;
    
    std::cout << "Started MIDI pattern playback on " << instrumentName
             << " (Duration: " << totalDuration << "s, Tempo: " << tempo << " BPM)" << std::endl;
    
    return true;
}

void Instrument::stopPlayback()
{
    // Only send MIDI Off messages if we were playing
    if (playing)
    {
        // Send all notes off message on all channels
        MidiBuffer allNotesOff;
        for (int channel = 1; channel <= 16; ++channel)
        {
            allNotesOff.addEvent(MidiMessage::allNotesOff(channel), 0);
            allNotesOff.addEvent(MidiMessage::allSoundOff(channel), 0);
        }
        
        // Process the all notes off messages
        AudioBuffer<float> tempBuffer(2, 512);
        MidiBuffer midiBuffer(allNotesOff);
        pluginGraph->processBlock(tempBuffer, midiBuffer);
    }
    
    // Reset state
    playing = false;
    paused = false;
    nextEventIndex = 0;
    currentTime = 0.0;
    currentSequence.reset(nullptr);
    
    std::cout << "Stopped MIDI pattern playback on " << instrumentName << std::endl;
}

void Instrument::pausePlayback()
{
    if (playing && !paused)
    {
        paused = true;
        
        // Send all notes off message on all channels
        MidiBuffer allNotesOff;
        for (int channel = 1; channel <= 16; ++channel)
        {
            allNotesOff.addEvent(MidiMessage::allNotesOff(channel), 0);
        }
        
        // Process the all notes off messages
        AudioBuffer<float> tempBuffer(2, 512);
        MidiBuffer midiBuffer(allNotesOff);
        pluginGraph->processBlock(tempBuffer, midiBuffer);
        
        std::cout << "Paused MIDI pattern playback on " << instrumentName << std::endl;
    }
}

void Instrument::resumePlayback()
{
    if (playing && paused)
    {
        paused = false;
        std::cout << "Resumed MIDI pattern playback on " << instrumentName << std::endl;
    }
}

void Instrument::addMidiInputCallback(MidiInputCallback* listener)
{
    if (listener != nullptr && !midiInputListeners.contains(listener))
    {
        midiInputListeners.add(listener);
    }
}

void Instrument::removeMidiInputCallback(MidiInputCallback* listener)
{
    midiInputListeners.removeFirstMatchingValue(listener);
}

void Instrument::audioDeviceIOCallback(const float** inputChannelData,
                                      int numInputChannels,
                                      float** outputChannelData,
                                      int numOutputChannels,
                                      int numSamples)
{
    // Create a buffer with the correct number of channels
    AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    
    // Clear the output buffer
    buffer.clear();
    
    // Create a MIDI buffer for this block
    MidiBuffer midiBuffer;
    
    // Process any MIDI events for the current time position
    if (playing && !paused && currentSequence != nullptr)
    {
        processMIDIEvents(midiBuffer, numSamples);
    }
    
    // Process audio with the plugin graph
    if (pluginGraph != nullptr)
    {
        // Add any input audio to the buffer if needed
        if (numInputChannels > 0)
        {
            for (int channel = 0; channel < jmin(numInputChannels, numOutputChannels); ++channel)
            {
                buffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);
            }
        }
        
        // Process the block through the plugin graph
        pluginGraph->processBlock(buffer, midiBuffer);
    }
    
    // Update current time
    updateCurrentTime(numSamples);
}

void Instrument::audioDeviceAboutToStart(AudioIODevice* device)
{
    if (device != nullptr)
    {
        sampleRate = device->getCurrentSampleRate();
        
        if (pluginGraph != nullptr)
        {
            pluginGraph->setPlayConfigDetails(device->getActiveInputChannels().countNumberOfSetBits(),
                                            device->getActiveOutputChannels().countNumberOfSetBits(),
                                            sampleRate, device->getCurrentBufferSizeSamples());
            
            pluginGraph->prepareToPlay(sampleRate, device->getCurrentBufferSizeSamples());
        }
    }
}

void Instrument::audioDeviceStopped()
{
    if (pluginGraph != nullptr)
    {
        pluginGraph->releaseResources();
    }
}

void Instrument::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    // Forward MIDI message to registered listeners
    for (auto* listener : midiInputListeners)
    {
        listener->handleIncomingMidiMessage(source, message);
    }
    
    // Also process the MIDI message through our plugin graph on the message thread
    // This ensures we respond to MIDI input immediately rather than waiting for the next audio block
    if (pluginGraph != nullptr && message.getRawDataSize() > 0)
    {
        MessageManager::callAsync([this, messageData = message.getRawData(), 
                                 messageSize = message.getRawDataSize()]() {
            MidiMessage copiedMessage(messageData, messageSize);
            MidiBuffer tempBuffer;
            tempBuffer.addEvent(copiedMessage, 0);
            
            AudioBuffer<float> tempAudioBuffer(2, 512);
            tempAudioBuffer.clear();
            
            pluginGraph->processBlock(tempAudioBuffer, tempBuffer);
        });
    }
}

void Instrument::timerCallback()
{
    // Check if the playback has completed
    if (playing && !paused && currentTime >= totalDuration)
    {
        // Playback is complete, stop and notify
        stopPlayback();
        
        // Call the callback if set
        if (onPatternFinished)
        {
            onPatternFinished();
        }
    }
}

void Instrument::processMIDIEvents(MidiBuffer& midiBuffer, int numSamples)
{
    if (currentSequence == nullptr || nextEventIndex >= currentSequence->getNumEvents())
        return;
    
    // Calculate time range for this buffer
    double bufferStartTime = currentTime;
    double bufferEndTime = currentTime + (numSamples / sampleRate);
    
    // Calculate MIDI tick timing
    // Standard MIDI is 480 ticks per quarter note (TPQN)
    // Tempo is in beats per minute, so beats per second is tempo/60
    double ticksPerSecond = 480.0 * (tempo / 60.0);
    
    // Convert buffer times to ticks
    double bufferStartTicks = bufferStartTime * ticksPerSecond;
    double bufferEndTicks = bufferEndTime * ticksPerSecond;
    
    // Process all events in this time range
    while (nextEventIndex < currentSequence->getNumEvents())
    {
        auto* eventPtr = currentSequence->getEventPointer(nextEventIndex);
        double eventTicks = eventPtr->message.getTimeStamp();
        
        // If event is beyond this buffer, we're done for now
        if (eventTicks >= bufferEndTicks)
            break;
        
        // If event is in this buffer's range
        if (eventTicks >= bufferStartTicks)
        {
            // Calculate the offset within the buffer
            double eventTime = eventTicks / ticksPerSecond;
            int sampleOffset = jlimit(0, numSamples - 1, 
                           (int)((eventTime - bufferStartTime) * sampleRate));
            
            // Add the event to the buffer
            midiBuffer.addEvent(eventPtr->message, sampleOffset);
        }
        
        // Move to next event
        nextEventIndex++;
    }
}

void Instrument::updateCurrentTime(int numSamples)
{
    // Only update time if playing and not paused
    if (playing && !paused)
    {
        currentTime += numSamples / sampleRate;
    }
}

std::unique_ptr<MidiMessageSequence> Instrument::convertJSONToMIDISequence(const json& events)
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    try
    {
        // Check if events array exists
        if (!events.contains("events") || !events["events"].is_array())
        {
            std::cerr << "Invalid JSON: missing 'events' array" << std::endl;
            return sequence;
        }
        
        // Process each event in the array
        for (const auto& event : events["events"])
        {
            // Validate required fields
            if (!event.contains("type") || !event["type"].is_string() || 
                !event.contains("time") || !event["time"].is_number())
            {
                continue; // Skip invalid events
            }
            
            std::string type = event["type"];
            int time = event["time"];
            
            MidiMessage message;
            
            if (type == "note_on")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("note") || !event["note"].is_number() ||
                    !event.contains("velocity") || !event["velocity"].is_number())
                {
                    continue; // Skip invalid note_on
                }
                
                int channel = event["channel"];
                int note = event["note"];
                int velocity = event["velocity"];
                
                // Create note on message
                message = MidiMessage::noteOn(channel + 1, note, (uint8)velocity);
            }
            else if (type == "note_off")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("note") || !event["note"].is_number())
                {
                    continue; // Skip invalid note_off
                }
                
                int channel = event["channel"];
                int note = event["note"];
                
                // Create note off message
                message = MidiMessage::noteOff(channel + 1, note);
            }
            else if (type == "controller")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("number") || !event["number"].is_number() ||
                    !event.contains("value") || !event["value"].is_number())
                {
                    continue; // Skip invalid controller
                }
                
                int channel = event["channel"];
                int number = event["number"];
                int value = event["value"];
                
                // Create controller message
                message = MidiMessage::controllerEvent(channel + 1, number, value);
            }
            else if (type == "program_change")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("program") || !event["program"].is_number())
                {
                    continue; // Skip invalid program_change
                }
                
                int channel = event["channel"];
                int program = event["program"];
                
                // Create program change message
                message = MidiMessage::programChange(channel + 1, program);
            }
            else if (type == "pitch_bend")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("value") || !event["value"].is_number())
                {
                    continue; // Skip invalid pitch_bend
                }
                
                int channel = event["channel"];
                int value = event["value"];
                
                // Create pitch bend message
                message = MidiMessage::pitchWheel(channel + 1, value);
            }
            else if (type == "aftertouch")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("note") || !event["note"].is_number() ||
                    !event.contains("pressure") || !event["pressure"].is_number())
                {
                    continue; // Skip invalid aftertouch
                }
                
                int channel = event["channel"];
                int note = event["note"];
                int pressure = event["pressure"];
                
                // Create aftertouch message
                message = MidiMessage::aftertouchChange(channel + 1, note, pressure);
            }
            else if (type == "channel_pressure")
            {
                // Check for required fields
                if (!event.contains("channel") || !event["channel"].is_number() ||
                    !event.contains("pressure") || !event["pressure"].is_number())
                {
                    continue; // Skip invalid channel_pressure
                }
                
                int channel = event["channel"];
                int pressure = event["pressure"];
                
                // Create channel pressure message
                message = MidiMessage::channelPressureChange(channel + 1, pressure);
            }
            else
            {
                // Unknown event type, skip
                continue;
            }
            
            // Set the timestamp
            message.setTimeStamp(time);
            
            // Add to sequence
            sequence->addEvent(message);
        }
        
        // Sort the sequence by timestamp
        sequence->sort();
        
        std::cout << "Converted JSON to MIDI sequence with " 
                 << sequence->getNumEvents() << " events" << std::endl;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Error parsing JSON MIDI events: " << e.what() << std::endl;
    }
    
    return sequence;
}

} // namespace covertracks