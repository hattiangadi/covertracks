#include "Instrument.h"
#include <iostream>

namespace covertracks {

Instrument::Instrument(const String& name, 
                   AudioPluginFormatManager& formatManager,
                   KnownPluginList& knownPluginList)
    : instrumentName(name), 
      deviceManager(std::make_unique<AudioDeviceManager>()),
      formatManager(formatManager),
      knownPluginList(knownPluginList)
{
    // Initialize the plugin graph with the shared format manager and plugin list
    pluginGraph = std::make_unique<PluginGraph>(formatManager, knownPluginList);
    
    // Output initialization message
    std::cout << "Creating Instrument: " << name << std::endl;
}

Instrument::~Instrument()
{
    // Stop playback and cleanup
    stopPlayback();
    
    // Stop timer if running
    stopTimer();
    
    // Clean up MIDI input callbacks
    if (deviceManager != nullptr)
    {
        deviceManager->removeMidiInputDeviceCallback(String(), this);
        deviceManager->closeAudioDevice();
    }
    
    // Output cleanup message
    std::cout << "Destroying Instrument: " << instrumentName << std::endl;
}

void Instrument::initialize()
{
    // Initialize the audio device manager
    deviceManager->initialiseWithDefaultDevices(2, 2);
    
    // Register as an audio callback
    deviceManager->addAudioCallback(this);
    
    // Enable MIDI input
    deviceManager->setMidiInputDeviceEnabled(String(), true);
    deviceManager->addMidiInputDeviceCallback(String(), this);
    
    // Initialize the processor graph if it doesn't exist
    if (!processorGraph)
    {
        processorGraph = std::make_unique<AudioProcessorGraph>();
    }
    
    auto& deviceManagerRef = *deviceManager;
    auto currentAudioDevice = deviceManager->getCurrentAudioDevice();
    
    if (currentAudioDevice != nullptr)
    {
        // Get device settings
        double sampleRateValue = currentAudioDevice->getCurrentSampleRate();
        int bufferSize = currentAudioDevice->getCurrentBufferSizeSamples();
        
        // Configure the processor graph
        processorGraph->setPlayConfigDetails(
            currentAudioDevice->getActiveInputChannels().countNumberOfSetBits(),
            currentAudioDevice->getActiveOutputChannels().countNumberOfSetBits(),
            sampleRateValue,
            bufferSize);
            
        // Update the sample rate
        sampleRate = sampleRateValue;
    }
    else
    {
        // Default configuration if no audio device is available
        processorGraph->setPlayConfigDetails(2, 2, 44100.0, 512);
    }
    
    // Prepare the processor graph for playback
    processorGraph->prepareToPlay(sampleRate, 512);
    
    // Start the timer at 20ms interval (50Hz) for MIDI playback tracking
    startTimer(20);
    
    std::cout << "Initialized Instrument: " << instrumentName << std::endl;
}

bool Instrument::loadPluginGraph(const XmlElement& graphState)
{
    try
    {
        // Check if the XML is valid
        if (!graphState.hasTagName("FILTERGRAPH"))
        {
            std::cerr << "Invalid plugin graph XML for " << instrumentName << std::endl;
            return false;
        }
        
        // Ensure we have a processor graph
        if (processorGraph == nullptr)
        {
            processorGraph = std::make_unique<AudioProcessorGraph>();
        }
        
        // Ensure we have a plugin graph
        if (pluginGraph == nullptr)
        {
            std::cerr << "Plugin graph is null for " << instrumentName << std::endl;
            return false;
        }
        
        // We need to use the public API to restore the graph
        // First, we'll restore the entire graph using the standard method
        pluginGraph->restoreFromXml(graphState);
        
        // Then check if we have any audio output nodes that need fixing
        bool audioOutputNodeFixed = false;
        
        // Check for audio output nodes and fix them if needed
        for (auto* node : pluginGraph->graph.getNodes())
        {
            if (auto* processor = node->getProcessor())
            {
                if (auto* ioProcessor = dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor))
                {
                    if (ioProcessor->getType() == AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode)
                    {
                        std::cout << "Instrument::loadPluginGraph - Found audio output node (ID: " << node->nodeID.uid 
                                  << "), current inputs: " << ioProcessor->getTotalNumInputChannels() << std::endl;
                        
                        if (ioProcessor->getTotalNumInputChannels() == 0)
                        {
                            // If the audio output node has no input channels, we need to fix it
                            std::cout << "  Audio output node has 0 inputs, attempting to fix..." << std::endl;
                            
                            // Try to set the bus layout to match what we need
                            AudioProcessor::BusesLayout layout = ioProcessor->getBusesLayout();
                            if (layout.inputBuses.size() > 0)
                            {
                                layout.inputBuses.getReference(0) = AudioChannelSet::stereo();
                                if (ioProcessor->setBusesLayout(layout))
                                {
                                    std::cout << "  Successfully set audio output node to have stereo input" << std::endl;
                                    audioOutputNodeFixed = true;
                                }
                                else
                                {
                                    std::cout << "  Failed to set audio output node bus layout" << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // If we fixed any audio output nodes, we need to restore the connections again
        if (audioOutputNodeFixed)
        {
            std::cout << "Instrument::loadPluginGraph - Audio output node fixed, restoring connections again" << std::endl;
            
            // Save all the connections from the original XML
            Array<AudioProcessorGraph::Connection> connectionsToRestore;
            
            for (auto* e : graphState.getChildWithTagNameIterator("CONNECTION"))
            {
                auto srcFilter = (uint32) e->getIntAttribute("srcFilter");
                auto srcChannel = e->getIntAttribute("srcChannel");
                auto dstFilter = (uint32) e->getIntAttribute("dstFilter");
                auto dstChannel = e->getIntAttribute("dstChannel");
                
                // Create the connection
                AudioProcessorGraph::Connection connection = { 
                    { AudioProcessorGraph::NodeID(srcFilter), srcChannel },
                    { AudioProcessorGraph::NodeID(dstFilter), dstChannel }
                };
                
                connectionsToRestore.add(connection);
            }
            
            // Clear all existing connections
            for (auto& connection : pluginGraph->graph.getConnections())
            {
                pluginGraph->graph.removeConnection(connection);
            }
            
            // Restore all connections
            int connectionCount = 0;
            int successCount = 0;
            
            for (auto& connection : connectionsToRestore)
            {
                connectionCount++;
                
                // Check if the connection can be made
                if (pluginGraph->graph.canConnect(connection))
                {
                    bool result = pluginGraph->graph.addConnection(connection);
                    if (result)
                    {
                        successCount++;
                    }
                }
            }
            
            std::cout << "Instrument::loadPluginGraph - Successfully restored " << successCount 
                      << " out of " << connectionCount << " connections after fixing audio output nodes" << std::endl;
        }
        
        // Clean up any illegal connections
        pluginGraph->graph.removeIllegalConnections();
        
        // In a real implementation, we would need to update the processor graph
        // based on the plugin graph configuration
        
        std::cout << "Plugin graph loaded successfully for " << instrumentName << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in Instrument::loadPluginGraph: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr << "Unknown exception in Instrument::loadPluginGraph" << std::endl;
        return false;
    }
}

std::unique_ptr<XmlElement> Instrument::getPluginGraphState()
{
    if (pluginGraph == nullptr)
    {
        return nullptr;
    }
    
    return pluginGraph->createXml();
}

bool Instrument::playMIDIPattern(const var& pattern, double newTempo)
{
    // Stop any current playback
    stopPlayback();
    
    // Set the new tempo if provided
    if (newTempo > 0)
    {
        tempo = newTempo;
    }
    
    // Convert the pattern to a MIDI sequence
    currentSequence = convertJSONToMIDISequence(pattern);
    
    if (currentSequence == nullptr || currentSequence->getNumEvents() == 0)
    {
        std::cerr << "Failed to create MIDI sequence from pattern" << std::endl;
        return false;
    }
    
    // Calculate the total duration of the pattern
    totalDuration = 0.0;
    
    for (int i = 0; i < currentSequence->getNumEvents(); ++i)
    {
        auto event = currentSequence->getEventPointer(i);
        if (event->message.isNoteOff())
        {
            totalDuration = std::max(totalDuration, event->message.getTimeStamp());
        }
    }
    
    // Reset playback state
    nextEventIndex = 0;
    currentTime = 0.0;
    playing = true;
    paused = false;
    
    std::cout << "Started MIDI pattern playback on " << instrumentName 
              << " (tempo: " << tempo << " BPM, duration: " << totalDuration << " ticks)" << std::endl;
    
    return true;
}

void Instrument::stopPlayback()
{
    if (playing)
    {
        // Send all notes off
        MidiBuffer allNotesOff;
        
        for (int channel = 1; channel <= 16; ++channel)
        {
            allNotesOff.addEvent(MidiMessage::allNotesOff(channel), 0);
            allNotesOff.addEvent(MidiMessage::allControllersOff(channel), 0);
        }
        
        // Process the all notes off messages
        AudioBuffer<float> tempBuffer(2, 512);
        MidiBuffer midiBuffer(allNotesOff);
        if (processorGraph)
            processorGraph->processBlock(tempBuffer, midiBuffer);
    }
    
    // Reset state
    playing = false;
    paused = false;
    currentSequence = nullptr;
    nextEventIndex = 0;
    currentTime = 0.0;
    
    std::cout << "Stopped MIDI pattern playback on " << instrumentName << std::endl;
}

void Instrument::pausePlayback()
{
    if (playing && !paused)
    {
        // Send all notes off
        MidiBuffer allNotesOff;
        
        for (int channel = 1; channel <= 16; ++channel)
        {
            allNotesOff.addEvent(MidiMessage::allNotesOff(channel), 0);
            allNotesOff.addEvent(MidiMessage::allControllersOff(channel), 0);
        }
        
        // Process the all notes off messages
        AudioBuffer<float> tempBuffer(2, 512);
        MidiBuffer midiBuffer(allNotesOff);
        if (processorGraph)
            processorGraph->processBlock(tempBuffer, midiBuffer);
        
        std::cout << "Paused MIDI pattern playback on " << instrumentName << std::endl;
    }
    
    paused = true;
}

void Instrument::resumePlayback()
{
    if (playing && paused)
    {
        paused = false;
        std::cout << "Resumed MIDI pattern playback on " << instrumentName << std::endl;
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

void Instrument::audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
                                     float** outputChannelData, int numOutputChannels,
                                     int numSamples)
{
    // Create audio buffer wrapper around the output channels
    AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    
    // Clear the output buffer
    buffer.clear();
    
    // Create a MIDI buffer for this block
    MidiBuffer midiBuffer;
    
    // If playing, add MIDI events for this block
    if (playing && !paused)
    {
        processMIDIEvents(midiBuffer, numSamples);
    }
    
    // Process audio with the processor graph
    if (processorGraph != nullptr)
    {
        // Add any input audio to the buffer if needed
        if (numInputChannels > 0)
        {
            for (int channel = 0; channel < std::min(numInputChannels, numOutputChannels); ++channel)
            {
                buffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);
            }
        }
        
        // Process the block through the processor graph
        if (processorGraph)
            processorGraph->processBlock(buffer, midiBuffer);
    }
    
    // Update current time
    updateCurrentTime(numSamples);
}

void Instrument::audioDeviceAboutToStart(AudioIODevice* device)
{
    if (device != nullptr)
    {
        sampleRate = device->getCurrentSampleRate();
        
        if (processorGraph != nullptr)
        {
            processorGraph->setPlayConfigDetails(
                device->getActiveInputChannels().countNumberOfSetBits(),
                device->getActiveOutputChannels().countNumberOfSetBits(),
                sampleRate,
                device->getCurrentBufferSizeSamples());
            
            processorGraph->prepareToPlay(sampleRate, device->getCurrentBufferSizeSamples());
        }
    }
}

void Instrument::audioDeviceStopped()
{
    if (processorGraph != nullptr)
    {
        processorGraph->releaseResources();
    }
}

void Instrument::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    // Forward the message to any registered listeners
    for (auto* listener : midiInputListeners)
    {
        listener->handleIncomingMidiMessage(source, message);
    }
    
    // If we're not playing a pattern, we can process live MIDI input
    if (!playing)
    {
        // Process the MIDI message through the plugin graph
        MessageManager::callAsync([this, message]()
        {
            MidiBuffer tempBuffer;
            tempBuffer.addEvent(message, 0);
            
            AudioBuffer<float> tempAudioBuffer(2, 512);
            tempAudioBuffer.clear();
            
            if (processorGraph)
                processorGraph->processBlock(tempAudioBuffer, tempBuffer);
        });
    }
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
    if (listener != nullptr)
    {
        midiInputListeners.removeFirstMatchingValue(listener);
    }
}

void Instrument::timerCallback()
{
    // This is called at regular intervals to update the playback state
    if (playing && !paused)
    {
        // Check if we've reached the end of the pattern
        if (currentTime >= totalDuration && totalDuration > 0)
        {
            // Stop playback
            stopPlayback();
            
            // Notify any registered callback
            if (onPatternFinished)
            {
                onPatternFinished();
            }
        }
    }
}

void Instrument::setOnPatternFinishedCallback(std::function<void()> callback)
{
    onPatternFinished = std::move(callback);
}

void Instrument::processMIDIEvents(MidiBuffer& midiBuffer, int numSamples)
{
    if (currentSequence == nullptr || nextEventIndex >= currentSequence->getNumEvents())
    {
        return;
    }
    
    // Calculate the time range for this block
    double blockStartTime = currentTime;
    double blockEndTime = blockStartTime + (numSamples / sampleRate) * (60.0 / tempo) * 480.0; // Assuming 480 ticks per quarter note
    
    // Add all events that fall within this time range
    while (nextEventIndex < currentSequence->getNumEvents())
    {
        auto event = currentSequence->getEventPointer(nextEventIndex);
        double eventTime = event->message.getTimeStamp();
        
        if (eventTime >= blockEndTime)
        {
            break;
        }
        
        if (eventTime >= blockStartTime)
        {
            // Calculate the sample position within this block
            int samplePosition = static_cast<int>((eventTime - blockStartTime) / (blockEndTime - blockStartTime) * numSamples);
            samplePosition = jlimit(0, numSamples - 1, samplePosition);
            
            // Add the event to the buffer
            midiBuffer.addEvent(event->message, samplePosition);
        }
        
        nextEventIndex++;
    }
}

void Instrument::updateCurrentTime(int numSamples)
{
    if (playing && !paused)
    {
        // Update the current time based on the number of samples processed
        double blockDuration = (numSamples / sampleRate) * (60.0 / tempo) * 480.0; // Assuming 480 ticks per quarter note
        currentTime += blockDuration;
    }
}

std::unique_ptr<MidiMessageSequence> Instrument::convertJSONToMIDISequence(const var& events)
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    try
    {
        if (!events.isArray())
        {
            throw std::runtime_error("Events must be an array");
        }
        
        for (int i = 0; i < events.size(); ++i)
        {
            auto event = events[i];
            
            if (!event.isObject())
            {
                continue;
            }
            
            // Extract event properties
            int channel = event.getProperty("channel", 1);
            int note = event.getProperty("note", 60);
            int velocity = event.getProperty("velocity", 100);
            double time = event.getProperty("time", 0.0);
            double duration = event.getProperty("duration", 0.25);
            
            // Create note on message
            MidiMessage noteOn = MidiMessage::noteOn(channel, note, static_cast<float>(velocity) / 127.0f);
            noteOn.setTimeStamp(time);
            sequence->addEvent(noteOn);
            
            // Create note off message
            MidiMessage noteOff = MidiMessage::noteOff(channel, note);
            noteOff.setTimeStamp(time + duration);
            sequence->addEvent(noteOff);
        }
        
        // Sort the sequence by timestamp
        sequence->sort();
        
        std::cout << "Converted JSON to MIDI sequence with " 
                 << sequence->getNumEvents() << " events" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing JSON MIDI events: " << e.what() << std::endl;
    }
    
    return sequence;
}

} // namespace covertracks
