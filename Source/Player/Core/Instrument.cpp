#include "Instrument.h"
#include <iostream>

namespace covertracks {

Instrument::Instrument(const String& name, 
                     AudioPluginFormatManager& fm,
                     KnownPluginList& kpl,
                     AudioEngine* engine)
    : instrumentName(name), 
      resourceState(ResourceState::Unloaded),
      audioEngine(engine),
      formatManager(fm),
      knownPluginList(kpl)
{
    std::cout << "Creating Instrument: " << name << std::endl;
    
    // Initialize the plugin graph with the shared format manager and plugin list
    pluginGraph = std::make_unique<PluginGraph>(formatManager, knownPluginList);
    
    // We'll create the processor graph when needed
    
    // Start the timer at 20ms interval (50Hz) for MIDI playback tracking
    startTimer(20);
}

void Instrument::prepareForEditing()
{
    // Only load what's needed for editing
    if (resourceState == ResourceState::Unloaded)
    {
        // Create processor graph if it doesn't exist
        if (!processorGraph)
        {
            processorGraph = std::make_unique<AudioProcessorGraph>();
        }
        
        // Set basic configuration so the graph can be edited
        double sampleRateToUse = audioEngine ? audioEngine->getSampleRate() : 44100.0;
        int bufferSizeToUse = audioEngine ? audioEngine->getBufferSize() : 512;
        
        processorGraph->setPlayConfigDetails(2, 2, sampleRateToUse, bufferSizeToUse);
        
        resourceState = ResourceState::EditReady;
        std::cout << "Instrument " << instrumentName << " prepared for editing" << std::endl;
    }
}

void Instrument::prepareForPlayback()
{
    // Load all resources needed for playback
    if (resourceState != ResourceState::PlaybackReady)
    {
        // First make sure we're edit-ready
        if (resourceState == ResourceState::Unloaded)
            prepareForEditing();
        
        // Then prepare for playback
        if (audioEngine)
        {
            double sampleRateToUse = audioEngine->getSampleRate();
            int bufferSizeToUse = audioEngine->getBufferSize();
            
            // Update the processor graph
            processorGraph->setPlayConfigDetails(2, 2, sampleRateToUse, bufferSizeToUse);
            processorGraph->prepareToPlay(sampleRateToUse, bufferSizeToUse);
            
            // Connect to the AudioEngine
            connectToAudioEngine(audioEngine);
            
            sampleRate = sampleRateToUse;
        }
        
        resourceState = ResourceState::PlaybackReady;
        std::cout << "Instrument " << instrumentName << " prepared for playback" << std::endl;
    }
}

void Instrument::releaseResources()
{
    // Release all heavy resources
    if (resourceState != ResourceState::Unloaded)
    {
        // Disconnect from AudioEngine
        disconnectFromAudioEngine();
        
        // Release processor graph resources
        if (processorGraph)
            processorGraph->releaseResources();
        
        resourceState = ResourceState::Unloaded;
        std::cout << "Instrument " << instrumentName << " resources released" << std::endl;
    }
}

bool Instrument::connectToAudioEngine(AudioEngine* engine)
{
    if (engine == nullptr)
        return false;
    
    // Store the engine reference
    audioEngine = engine;
    
    // Register the processor graph
    if (processorGraph && !engine->registerProcessor(processorGraph.get()))
    {
        std::cerr << "Failed to register processor with AudioEngine" << std::endl;
        return false;
    }
    
    // Register for MIDI input
    String midiInputId = instrumentName + "_MidiInput";
    registeredMidiInputs.add(midiInputId);
    
    if (!engine->registerMidiInput(midiInputId, this))
    {
        std::cerr << "Failed to register MIDI input with AudioEngine" << std::endl;
        return false;
    }
    
    return true;
}

void Instrument::disconnectFromAudioEngine()
{
    if (audioEngine)
    {
        // Unregister processor graph
        if (processorGraph)
            audioEngine->unregisterProcessor(processorGraph.get());
        
        // Unregister MIDI inputs
        for (const auto& midiInputId : registeredMidiInputs)
            audioEngine->unregisterMidiInput(midiInputId, this);
        
        registeredMidiInputs.clear();
        
        // Clear engine reference
        audioEngine = nullptr;
    }
}
// Static method to notify all Instruments that the AudioEngine is being destroyed
void Instrument::notifyAudioEngineDestroyed()
{
    std::cout << "[Instrument] Static notification: AudioEngine is being destroyed" << std::endl;
}

Instrument::~Instrument()
{
    // Stop any ongoing playback
    stopPlayback();
    
    // Stop timer if running
    stopTimer();
    
    // Release all resources
    releaseResources();
    
    std::cout << "[Instrument] Destroyed: " << instrumentName << std::endl;
}

// The initialize method has been replaced by prepareForEditing and prepareForPlayback methods

bool Instrument::loadPluginGraph(const XmlElement& graphState)
{
    if (!pluginGraph)
    {
        std::cerr << "Cannot load plugin graph - no plugin graph exists" << std::endl;
        return false;
    }
    
    // Make sure we have a processor graph
    if (!processorGraph)
    {
        processorGraph = std::make_unique<AudioProcessorGraph>();
    }
    
    // First attempt to restore the graph state
    pluginGraph->restoreFromXml(graphState);
    
    // Check for audio output nodes with 0 input channels
    bool outputNodesFixed = false;
    
    // Get all nodes in the graph
    auto& graph = pluginGraph->graph;
    for (auto* node : graph.getNodes())
    {
        auto* processor = node->getProcessor();
        
        // Check if this is an audio output node
        auto* ioProcessor = dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor);
        if (ioProcessor != nullptr && ioProcessor->getType() == AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode)
        {
            std::cout << "Found audio output node (ID: " << node->nodeID.uid << "), current inputs: " 
                      << processor->getTotalNumInputChannels() << std::endl;
            
            // If the audio output node has 0 input channels, we need to fix it
            if (processor->getTotalNumInputChannels() == 0)
            {
                // Configure the node for stereo input
                AudioProcessor::BusesLayout layout = processor->getBusesLayout();
                if (layout.inputBuses.size() > 0)
                {
                    layout.inputBuses[0] = AudioChannelSet::stereo();
                    processor->setBusesLayout(layout);
                    outputNodesFixed = true;
                    
                    std::cout << "Fixed audio output node to have stereo input" << std::endl;
                }
            }
        }
    }
    
    // If we fixed any output nodes, we need to restore connections again
    if (outputNodesFixed)
    {
        std::cout << "Re-restoring connections after fixing audio output nodes" << std::endl;
        pluginGraph->restoreFromXml(graphState);
    }
    
    return true;
}

std::unique_ptr<XmlElement> Instrument::getPluginGraphState()
{
    if (!pluginGraph)
    {
        std::cerr << "Cannot get plugin graph state - no plugin graph exists" << std::endl;
        return nullptr;
    }
    
    return pluginGraph->createXml();
}


// The detailed implementation is already defined above

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

void Instrument::setPatternFinishedCallback(std::function<void()> callback)
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
