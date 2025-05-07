#include "MIDIPlayer.h"
#include "../VST/SamplerPlugin.h"

MIDIPlayer::MIDIPlayer(const nlohmann::json& configJson)
    : config(configJson)
{
    // Extract required fields
    playerId = config.contains("player_id") ? juce::String(config["player_id"].get<std::string>()) : juce::String("unknown");
    instrumentType = config.contains("instrument_type") ? juce::String(config["instrument_type"].get<std::string>()) : juce::String("unknown");
    
    // Extract optional fields with defaults
    pluginId = config.contains("plugin_id") ? juce::String(config["plugin_id"].get<std::string>()) : juce::String(coverband::VSTHost::SAMPLER_PLUGIN_ID);
    
    // For sample name, we need to handle the concatenation differently
    if (config.contains("sample_name")) {
        sampleName = juce::String(config["sample_name"].get<std::string>());
    } else {
        sampleName = instrumentType + "_default.wav";
    }
    
    // Extract timing parameters if available
    if (config.contains("ppq") && config["ppq"].is_number()) {
        ppq = config["ppq"];
    }
    
    if (config.contains("tempo") && config["tempo"].is_number()) {
        tempo = config["tempo"];
    }
    
    std::cout << "Creating MIDIPlayer: " << playerId << " (" << instrumentType 
              << ") with plugin: " << pluginId << std::endl;
}

MIDIPlayer::~MIDIPlayer()
{
    std::cout << "Destroying MIDIPlayer: " << playerId << std::endl;
    
    // Stop playback first
    stop();
    
    // Clean up audio resources
    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    deviceManager.closeAudioDevice();
    
    // Clean up VST host
    vstHost.reset();
}

void MIDIPlayer::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    sampleRate = newSampleRate;
    
    // Prepare VST host if available
    if (vstHost != nullptr) {
        vstHost->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
    std::cout << "MIDIPlayer [" << playerId << "] prepared with sample rate: " 
              << sampleRate << ", buffer size: " << samplesPerBlockExpected << std::endl;
}

void MIDIPlayer::releaseResources()
{
    // Release VST host resources if available
    if (vstHost != nullptr) {
        vstHost->releaseResources();
    }
}

void MIDIPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Clear the buffer
    bufferToFill.clearActiveBufferRegion();
    
    if (!playing) {
        return;
    }
    
    // Create a MIDI buffer for this block
    juce::MidiBuffer midiBuffer;
    
    // Process MIDI events for this block
    processMIDISequence(midiBuffer, bufferToFill.numSamples);
    
    // Process through VST host if available
    if (vstHost != nullptr) {
        vstHost->processBlock(*bufferToFill.buffer, midiBuffer);
    }
}

bool MIDIPlayer::initialize()
{
    std::cout << "Initializing MIDIPlayer [" << playerId << "]" << std::endl;
    
    // Setup audio
    if (!setupAudio()) {
        std::cout << "Failed to setup audio for MIDIPlayer [" << playerId << "]" << std::endl;
        return false;
    }
    
    // Setup VST
    if (!setupVST()) {
        std::cout << "Failed to setup VST for MIDIPlayer [" << playerId << "]" << std::endl;
        return false;
    }
    
    // Set up audio source player
    audioSourcePlayer.setSource(this);
    deviceManager.addAudioCallback(&audioSourcePlayer);
    
    std::cout << "MIDIPlayer [" << playerId << "] initialized successfully" << std::endl;
    return true;
}

void MIDIPlayer::playMIDISequence(const juce::MidiMessageSequence& sequence, const juce::String& patternId)
{
    stop();
    
    // Store sequence for playback
    currentSequence = sequence;
    currentPatternId = patternId;
    
    // Reset playback state
    nextEventIndex = 0;
    currentTime = 0.0;
    
    // Debug: Check if sequence is empty
    if (currentSequence.getNumEvents() == 0) {
        std::cout << "WARNING: MIDIPlayer [" << playerId << "] received empty sequence for pattern: " << patternId << std::endl;
        
        // Create a dummy event to ensure the pattern completes
        juce::MidiMessage dummyOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
        dummyOn.setTimeStamp(0.0);
        currentSequence.addEvent(dummyOn);
        
        juce::MidiMessage dummyOff = juce::MidiMessage::noteOff(1, 60);
        dummyOff.setTimeStamp(480.0); // One quarter note duration
        currentSequence.addEvent(dummyOff);
        
        std::cout << "Added dummy events to ensure pattern completion" << std::endl;
    }
    
    // Start playback
    playing = true;
    
    std::cout << "MIDIPlayer [" << playerId << "] playing MIDI sequence with " 
              << sequence.getNumEvents() << " events, pattern: " << patternId << std::endl;
              
    // Debug the sequence to verify timestamps
    for (int i = 0; i < std::min(5, sequence.getNumEvents()); i++) {
        auto* event = sequence.getEventPointer(i);
        std::cout << "  Event " << i << ": tick=" << event->message.getTimeStamp();
        if (event->message.isNoteOn()) {
            std::cout << ", Note On: " << event->message.getNoteNumber() 
                      << ", velocity: " << (int)event->message.getVelocity();
        }
        else if (event->message.isNoteOff()) {
            std::cout << ", Note Off: " << event->message.getNoteNumber();
        }
        std::cout << std::endl;
    }
    
    // Show the last event to check the total duration
    if (sequence.getNumEvents() > 5) {
        auto* lastEvent = sequence.getEventPointer(sequence.getNumEvents() - 1);
        std::cout << "  Last event: tick=" << lastEvent->message.getTimeStamp();
        if (lastEvent->message.isNoteOn()) {
            std::cout << ", Note On: " << lastEvent->message.getNoteNumber();
        }
        else if (lastEvent->message.isNoteOff()) {
            std::cout << ", Note Off: " << lastEvent->message.getNoteNumber();
        }
        std::cout << std::endl;
    }
}

void MIDIPlayer::stop()
{
    if (playing) {
        playing = false;
        
        // Send all-notes-off message to prevent stuck notes
        if (vstHost != nullptr) {
            juce::MidiBuffer midiBuffer;
            midiBuffer.addEvent(juce::MidiMessage::allNotesOff(1), 0);
            juce::AudioBuffer<float> tempBuffer(2, 512);
            vstHost->processBlock(tempBuffer, midiBuffer);
        }
        
        std::cout << "MIDIPlayer [" << playerId << "] stopped" << std::endl;
    }
}

void MIDIPlayer::setPatternFinishedCallback(std::function<void()> callback)
{
    patternFinishedCallback = std::move(callback);
}

bool MIDIPlayer::setupAudio()
{
    std::cout << "[DEBUG] Setting up audio for MIDIPlayer [" << playerId << "]" << std::endl;
    
    // Initialize audio device with explicit channel configuration
    std::cout << "[DEBUG] Initializing audio device manager..." << std::endl;
    deviceManager.initialise(0,  // num input channels
                           2,  // num output channels
                           nullptr,  // no XML settings
                           true,  // select default device
                           "",  // preferred device name
                           nullptr);  // preferred setup options

    // Get current setup
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    
    // Log current device info
    std::cout << "[DEBUG] Current audio device: " << setup.outputDeviceName << std::endl;
    std::cout << "[DEBUG] Current sample rate: " << setup.sampleRate << std::endl;
    std::cout << "[DEBUG] Current buffer size: " << setup.bufferSize << std::endl;
    
    // Ensure output channels are enabled
    setup.useDefaultOutputChannels = false;
    setup.outputChannels.clear();
    setup.outputChannels.setRange(0, 2, true);  // Enable first two channels
    
    // Set audio parameters
    setup.sampleRate = 44100.0;
    setup.bufferSize = 256;  // Smaller buffer size for lower latency
    
    std::cout << "[DEBUG] Setting audio device setup with sample rate: " << setup.sampleRate 
              << ", buffer size: " << setup.bufferSize << std::endl;
    
    juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
    if (!error.isEmpty()) {
        std::cout << "[ERROR] Audio device setup failed: " << error << std::endl;
        return false;
    }
    
    return true;
}

bool MIDIPlayer::setupVST()
{
    // Create and initialize VST host
    vstHost = std::make_unique<coverband::VSTHost>();

    // Create config with polyphony
    json vstConfig;
    if (config.contains("polyphony") && config["polyphony"].is_number()) {
        vstConfig["polyphony"] = config["polyphony"];
    }

    if (!vstHost->initialize(sampleRate, 256, vstConfig)) {
        std::cout << "Failed to initialize VST host" << std::endl;
        return false;
    }

    // Load the configured plugin
    if (!vstHost->loadPlugin(pluginId)) {
        std::cout << "Failed to load plugin: " << pluginId << std::endl;
        return false;
    }
    
    // If this is a sampler plugin and we have a sample name specified, load the appropriate sample
    if (pluginId == coverband::VSTHost::SAMPLER_PLUGIN_ID && !sampleName.isEmpty()) {
        // Get the sampler plugin instance
        if (auto* samplerPlugin = dynamic_cast<coverband::SamplerPlugin*>(vstHost->getPluginByID(pluginId))) {
            // Load the specific sample for this instrument
            if (samplerPlugin->loadSampleByName(sampleName)) {
                std::cout << "Successfully loaded sample " << sampleName << " for " << instrumentType << std::endl;
            } else {
                std::cout << "Failed to load sample " << sampleName << " for " << instrumentType << std::endl;
            }
        } else {
            std::cout << "Could not cast to SamplerPlugin" << std::endl;
        }
    }
    
    std::cout << "Successfully loaded plugin for " << playerId << std::endl;
    return true;
}

void MIDIPlayer::processMIDISequence(juce::MidiBuffer& midiBuffer, int numSamples)
{
    if (!playing)
        return;
        
    // Calculate time advancement for this block
    double secondsPerSample = 1.0 / sampleRate;
    double blockDuration = numSamples * secondsPerSample;
    
    // Convert block duration to MIDI ticks (assuming 480 ticks per quarter note at 120 BPM)
    // At 120 BPM, a quarter note is 0.5 seconds, so 480 ticks = 0.5 seconds
    // This means 1 second = 960 ticks, and 1 tick = 1/960 seconds
    const double TICKS_PER_SECOND = ppq * (tempo / 60.0) * 2.0;
    double ticksPerBlock = blockDuration * TICKS_PER_SECOND;
    double endTick = currentTime + ticksPerBlock;
    
    // Process events that should occur in this block
    int numEvents = currentSequence.getNumEvents();
    
    // Debug the sequence progress
    if (nextEventIndex == 0 || nextEventIndex % 5 == 0) {
        std::cout << "MIDIPlayer [" << playerId << "] processing events: " 
                  << nextEventIndex << "/" << numEvents 
                  << ", current tick: " << currentTime 
                  << ", end tick: " << endTick << std::endl;
    }
    
    while (nextEventIndex < numEvents) {
        auto* event = currentSequence.getEventPointer(nextEventIndex);
        double eventTick = event->message.getTimeStamp();
        
        if (eventTick >= endTick)
            break;
            
        if (eventTick >= currentTime) {
            // Calculate the exact sample offset for this event
            double eventOffset = (eventTick - currentTime) / ticksPerBlock;
            int sampleOffset = static_cast<int>(eventOffset * numSamples);
            
            // Clamp to valid range
            sampleOffset = juce::jlimit(0, numSamples - 1, sampleOffset);
            
            // Add the MIDI message to the output buffer
            midiBuffer.addEvent(event->message, sampleOffset);
            
            // Debug info
            if (event->message.isNoteOn()) {
                std::cout << "MIDIPlayer [" << playerId << "] Note On: " 
                          << event->message.getNoteNumber() << ", velocity: " 
                          << (int)event->message.getVelocity() << ", tick: " << eventTick << std::endl;
            }
            else if (event->message.isNoteOff()) {
                std::cout << "MIDIPlayer [" << playerId << "] Note Off: " 
                          << event->message.getNoteNumber() << ", tick: " << eventTick << std::endl;
            }
        }
        
        // Move to next event
        nextEventIndex++;
    }
    
    // Update current time
    currentTime = endTick;
    
    // Check if sequence is complete
    if (nextEventIndex >= numEvents) {
        // The sequence is complete
        playing = false;
        
        std::cout << "MIDIPlayer [" << playerId << "] completed pattern: " << currentPatternId << std::endl;
        std::cout << "MIDIPlayer [" << playerId << "] processed all " << numEvents << " events" << std::endl;
        
        // Debug: Check if we have events in the sequence
        std::cout << "MIDIPlayer [" << playerId << "] sequence has " << currentSequence.getNumEvents() << " events" << std::endl;
        
        // Calculate the expected total duration of the pattern in milliseconds
        if (numEvents > 0) {
            auto* lastEvent = currentSequence.getEventPointer(numEvents - 1);
            double lastEventTimeMs = (lastEvent->message.getTimeStamp() / ppq) * (60000.0 / tempo);
            std::cout << "MIDIPlayer [" << playerId << "] last event time: " << lastEventTimeMs << "ms" << std::endl;
            
            // Add a small buffer (500ms) to ensure all audio has finished playing
            currentTime = lastEventTimeMs + 500.0;
        }
        
        // Notify pattern completion - use direct call instead of async to ensure it happens immediately
        if (patternFinishedCallback) {
            std::cout << "MIDIPlayer [" << playerId << "] calling pattern finished callback" << std::endl;
            patternFinishedCallback();
            std::cout << "MIDIPlayer [" << playerId << "] pattern finished callback completed" << std::endl;
        } else {
            std::cout << "MIDIPlayer [" << playerId << "] WARNING: No pattern finished callback set!" << std::endl;
        }
        
        // Reset state for next pattern
        nextEventIndex = 0;
        currentTime = 0.0;
    }
}