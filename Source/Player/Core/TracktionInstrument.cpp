#include "TracktionInstrument.h"
#include "../../Audio/AudioEngineInterface.h"
#include <iostream>

namespace covertracks {

TracktionInstrument::TracktionInstrument(const juce::String& name,
                                       juce::AudioPluginFormatManager& fm,
                                       juce::KnownPluginList& kpl)
    : instrumentName(name)
    , formatManager(fm)
    , knownPluginList(kpl)
{
    // Start timer for MIDI playback timing (50Hz = 20ms intervals)
    startTimer(20);
    
    DBG("TracktionInstrument created: " << name);
}

TracktionInstrument::~TracktionInstrument()
{
    stopTimer();
    stopPlayback();
    unregisterFromAudioEngine();
    
    DBG("TracktionInstrument destroyed: " << instrumentName);
}

// =========================================================================
// Audio Engine Integration (Self-Managed)
// =========================================================================

bool TracktionInstrument::registerWithAudioEngine()
{
    if (myTrack != nullptr)
    {
        DBG("TracktionInstrument " << instrumentName << " already registered");
        return true; // Already registered
    }
    
    auto& audioEngine = getAudioEngine();
    
    // Create a dummy Player pointer for Track creation
    // Note: This is a temporary approach - we'll refactor TracktionAudioEngine
    // to support direct Track creation without needing a Player
    auto* currentEdit = audioEngine.getCurrentEdit();
    if (!currentEdit)
    {
        std::cerr << "No Tracktion Edit available for Track creation" << std::endl;
        return false;
    }
    
    // Create a new audio track for this instrument
    auto insertPoint = te::TrackInsertPoint::getEndOfTracks(*currentEdit);
    auto trackPtr = currentEdit->insertNewAudioTrack(insertPoint, nullptr);
    
    if (!trackPtr)
    {
        std::cerr << "Failed to create Tracktion Track for " << instrumentName << std::endl;
        return false;
    }
    
    myTrack = trackPtr.get();
    myTrack->setName(instrumentName + "_Track");
    
    // Add a built-in synthesizer plugin automatically for MIDI-to-audio conversion
    try {
        juce::PluginDescription synthDesc;
        synthDesc.name = "4-Oscillator Synth";
        synthDesc.fileOrIdentifier = "1_trkbuiltin";
        synthDesc.pluginFormatName = te::PluginManager::builtInPluginFormatName;
        synthDesc.category = te::FourOscPlugin::xmlTypeName;
        synthDesc.isInstrument = true;
        
        // Create the built-in synth plugin
        if (auto synthPlugin = currentEdit->getPluginCache().createNewPlugin(te::FourOscPlugin::xmlTypeName, synthDesc))
        {
            // Add the plugin to our track
            if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
            {
                audioTrack->pluginList.insertPlugin(synthPlugin, 0, nullptr);
                DBG("TracktionInstrument " << instrumentName << " added built-in synthesizer: " << synthDesc.name);
            }
        }
        else
        {
            std::cerr << "Failed to create built-in synthesizer for " << instrumentName << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error adding synthesizer to " << instrumentName << ": " << e.what() << std::endl;
    }
    
    // Ensure the Edit transport is in a state that allows audio processing
    auto& transport = currentEdit->getTransport();
    if (!transport.isPlaying())
    {
        std::cout << "Starting Tracktion Edit transport for real-time audio processing..." << std::endl;
        transport.play(false);  // Start playing (false = don't punch record)
    }
    
    DBG("TracktionInstrument " << instrumentName << " registered with Track: " << myTrack->getName());
    return true;
}

void TracktionInstrument::unregisterFromAudioEngine()
{
    if (myTrack == nullptr)
        return;
    
    // Stop any playback first
    stopPlayback();
    
    // Remove track from Edit
    auto& audioEngine = getAudioEngine();
    auto* currentEdit = audioEngine.getCurrentEdit();
    if (currentEdit)
    {
        currentEdit->deleteTrack(myTrack);
    }
    
    myTrack = nullptr;
    DBG("TracktionInstrument " << instrumentName << " unregistered from audio engine");
}

// =========================================================================
// Plugin Management (Complete Ownership)
// =========================================================================

bool TracktionInstrument::addPlugin(const juce::PluginDescription& pluginDesc)
{
    if (myTrack == nullptr)
    {
        std::cerr << "Cannot add plugin: instrument not registered with audio engine" << std::endl;
        return false;
    }
    
    auto& audioEngine = getAudioEngine();
    auto* currentEdit = audioEngine.getCurrentEdit();
    if (!currentEdit)
    {
        std::cerr << "No Tracktion Edit available" << std::endl;
        return false;
    }
    
    std::cout << "TracktionInstrument: Adding plugin " << pluginDesc.name 
              << " (format: " << pluginDesc.pluginFormatName << ")" << std::endl;
    
    // Use different approaches for built-in vs external plugins
    if (pluginDesc.pluginFormatName == te::PluginManager::builtInPluginFormatName)
    {
        // Built-in Tracktion plugin - use standard approach
        std::cout << "TracktionInstrument: Built-in plugin - using category: " << pluginDesc.category << std::endl;
        
        auto plugin = currentEdit->getPluginCache().createNewPlugin(pluginDesc.category, pluginDesc);
        if (!plugin)
        {
            std::cerr << "Failed to create built-in plugin: " << pluginDesc.name << std::endl;
            return false;
        }
        
        // Add plugin to track
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
        {
            audioTrack->pluginList.insertPlugin(plugin, -1, nullptr);
            DBG("Added built-in plugin " << pluginDesc.name << " to " << instrumentName);
            return true;
        }
    }
    else
    {
        // External plugin - use standard Tracktion approach (same as official examples)
        std::cout << "TracktionInstrument: Creating external plugin: " << pluginDesc.name << std::endl;
        
        // Create plugin using exact same method as official Tracktion examples
        auto plugin = currentEdit->getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName, pluginDesc);
        if (!plugin)
        {
            std::cerr << "Failed to create external plugin: " << pluginDesc.name << std::endl;
            return false;
        }
        
        // Add to track immediately (don't wait for async loading - let Tracktion handle it)
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
        {
            audioTrack->pluginList.insertPlugin(plugin, -1, nullptr);
            std::cout << "TracktionInstrument: Added external plugin " << plugin->getName() << " to track" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "TracktionInstrument: Track is not an AudioTrack" << std::endl;
            return false;
        }
    }
    
    std::cerr << "Track is not an AudioTrack - cannot add plugins" << std::endl;
    return false;
}

void TracktionInstrument::clearPlugins()
{
    if (myTrack == nullptr)
        return;
        
    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
    {
        audioTrack->pluginList.clear();
        DBG("Cleared all plugins from " << instrumentName);
    }
}

bool TracktionInstrument::removePlugin(int index)
{
    if (myTrack == nullptr)
    {
        std::cerr << "Cannot remove plugin: instrument not registered with audio engine" << std::endl;
        return false;
    }
    
    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
    {
        if (index >= 0 && index < audioTrack->pluginList.size())
        {
            auto* plugin = audioTrack->pluginList[index];
            juce::String pluginName = plugin ? plugin->getName() : "Unknown";
            
            // Use Tracktion's proper plugin removal API
            plugin->deleteFromParent();
            DBG("Removed plugin " << pluginName << " at index " << index << " from " << instrumentName);
            return true;
        }
        else
        {
            std::cerr << "Invalid plugin index: " << index << " (track has " 
                      << audioTrack->pluginList.size() << " plugins)" << std::endl;
            return false;
        }
    }
    
    std::cerr << "Track is not an AudioTrack - cannot remove plugins" << std::endl;
    return false;
}

bool TracktionInstrument::movePlugin(int fromIndex, int toIndex)
{
    if (myTrack == nullptr)
    {
        std::cerr << "Cannot move plugin: instrument not registered with audio engine" << std::endl;
        return false;
    }
    
    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
    {
        int pluginCount = audioTrack->pluginList.size();
        
        // Validate indices
        if (fromIndex < 0 || fromIndex >= pluginCount)
        {
            std::cerr << "Invalid source index: " << fromIndex << " (track has " << pluginCount << " plugins)" << std::endl;
            return false;
        }
        
        if (toIndex < 0 || toIndex >= pluginCount)
        {
            std::cerr << "Invalid destination index: " << toIndex << " (track has " << pluginCount << " plugins)" << std::endl;
            return false;
        }
        
        if (fromIndex == toIndex)
        {
            return true; // Nothing to do
        }
        
        // Get the plugin to move
        auto* plugin = audioTrack->pluginList[fromIndex];
        if (!plugin)
        {
            std::cerr << "Plugin at index " << fromIndex << " is null" << std::endl;
            return false;
        }
        
        juce::String pluginName = plugin->getName();
        
        // Use Tracktion's proper plugin move API - remove from parent, then re-insert
        plugin->removeFromParent(); // Don't delete the plugin, just remove from list
        audioTrack->pluginList.insertPlugin(plugin, toIndex, nullptr);
        
        DBG("Moved plugin " << pluginName << " from index " << fromIndex << " to " << toIndex << " in " << instrumentName);
        return true;
    }
    
    std::cerr << "Track is not an AudioTrack - cannot move plugins" << std::endl;
    return false;
}

int TracktionInstrument::getPluginCount() const
{
    if (myTrack != nullptr)
    {
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
        {
            return audioTrack->pluginList.size();
        }
    }
    
    return 0;
}

juce::StringArray TracktionInstrument::getPluginNames() const
{
    juce::StringArray names;
    
    if (myTrack != nullptr)
    {
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
        {
            for (auto* plugin : audioTrack->pluginList)
            {
                if (plugin)
                    names.add(plugin->getName());
            }
        }
    }
    
    return names;
}

te::Plugin* TracktionInstrument::getPlugin(int index) const
{
    if (myTrack == nullptr)
        return nullptr;
        
    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
    {
        // Check if index is within Tracktion plugins range
        if (index >= 0 && index < audioTrack->pluginList.size())
        {
            return audioTrack->pluginList[index];
        }
    }
    
    // Index might be for a JUCE plugin - those don't have te::Plugin* interface
    return nullptr;
}

juce::AudioProcessor* TracktionInstrument::getJucePlugin(int index) const
{
    if (auto* tracktionPlugin = getPlugin(index))
    {
        // For external plugins, use getAudioPluginInstance()
        if (auto* externalPlugin = dynamic_cast<te::ExternalPlugin*>(tracktionPlugin))
        {
            auto* instance = externalPlugin->getAudioPluginInstance();
            // Don't log here as this is called frequently - async loading may take time
            return instance;
        }
        
        // For other plugins, use the wrapped processor method
        return tracktionPlugin->getWrappedAudioProcessor();
    }
    
    return nullptr;
}

// =========================================================================
// MIDI Chart Playback (Complete Audio Control)
// =========================================================================

bool TracktionInstrument::playChart(const juce::var& chart, double tempo)
{
    auto sequence = convertChartToMidiSequence(chart);
    if (!sequence)
    {
        std::cerr << "Failed to convert chart to MIDI sequence" << std::endl;
        return false;
    }
    
    return playMidiSequence(*sequence, tempo);
}

bool TracktionInstrument::playMidiSequence(const juce::MidiMessageSequence& sequence, double tempo)
{
    if (myTrack == nullptr)
    {
        std::cerr << "Cannot play sequence: instrument not registered with audio engine" << std::endl;
        return false;
    }
    
    // Stop any current playback
    stopPlayback();
    
    // Store the sequence and setup playback
    currentSequence = std::make_unique<juce::MidiMessageSequence>(sequence);
    playbackTempo = tempo;
    currentPlaybackTime = 0.0;
    nextEventIndex = 0;
    isPlaybackPaused = false;
    isPlaybackActive = true;
    
    // Start the Tracktion Edit's transport to enable audio processing
    auto& audioEngine = getAudioEngine();
    auto* currentEdit = audioEngine.getCurrentEdit();
    if (currentEdit)
    {
        auto& transport = currentEdit->getTransport();
        transport.setPosition(te::TimePosition::fromSeconds(0.0));
        transport.play(false);
        DBG("TracktionInstrument " << instrumentName << " started Edit transport for audio processing");
    }
    
    DBG("TracktionInstrument " << instrumentName << " started MIDI playback with " 
        << currentSequence->getNumEvents() << " events at " << tempo << " BPM");
    
    return true;
}

void TracktionInstrument::stopPlayback()
{
    if (isPlaybackActive)
    {
        isPlaybackActive = false;
        isPlaybackPaused = false;
        currentSequence.reset();
        currentPlaybackTime = 0.0;
        nextEventIndex = 0;
        
        // Stop the Tracktion Edit's transport
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        if (currentEdit)
        {
            auto& transport = currentEdit->getTransport();
            transport.stop(false, false);
            DBG("TracktionInstrument " << instrumentName << " stopped Edit transport");
        }
        
        // Call completion callback if set
        if (onPlaybackFinished)
        {
            onPlaybackFinished();
        }
        
        DBG("TracktionInstrument " << instrumentName << " stopped playback");
    }
}

void TracktionInstrument::pausePlayback()
{
    if (isPlaybackActive && !isPlaybackPaused)
    {
        isPlaybackPaused = true;
        DBG("TracktionInstrument " << instrumentName << " paused playback");
    }
}

void TracktionInstrument::resumePlayback()
{
    if (isPlaybackActive && isPlaybackPaused)
    {
        isPlaybackPaused = false;
        DBG("TracktionInstrument " << instrumentName << " resumed playback");
    }
}

void TracktionInstrument::setPlaybackFinishedCallback(std::function<void()> callback)
{
    onPlaybackFinished = callback;
}


// =========================================================================
// MIDI Input Handling
// =========================================================================

void TracktionInstrument::handleIncomingMidiMessage(juce::MidiInput* /*source*/, 
                                                   const juce::MidiMessage& message)
{
    processMidiMessage(message);
}

void TracktionInstrument::processMidiMessage(const juce::MidiMessage& message)
{
    sendMidiToTrack(message);
}

// =========================================================================
// State Management
// =========================================================================

std::unique_ptr<juce::XmlElement> TracktionInstrument::getPluginState() const
{
    auto state = std::make_unique<juce::XmlElement>("TracktionInstrumentState");
    state->setAttribute("name", instrumentName);
    state->setAttribute("pluginCount", getPluginCount());
    
    // TODO: Add detailed plugin state serialization when needed
    // For now, just track basic info
    
    if (myTrack)
    {
        state->setAttribute("trackName", myTrack->getName());
        
        // Add plugin names
        auto pluginNames = getPluginNames();
        auto* pluginsElement = state->createNewChildElement("Plugins");
        for (const auto& name : pluginNames)
        {
            auto* pluginElement = pluginsElement->createNewChildElement("Plugin");
            pluginElement->setAttribute("name", name);
        }
    }
    
    return state;
}

bool TracktionInstrument::restorePluginState(const juce::XmlElement& state)
{
    if (state.getTagName() != "TracktionInstrumentState")
    {
        std::cerr << "Invalid state XML for TracktionInstrument" << std::endl;
        return false;
    }
    
    // Clear current plugins
    clearPlugins();
    
    // TODO: Restore detailed plugin state when needed
    // For now, just basic validation
    
    auto storedName = state.getStringAttribute("name");
    if (storedName != instrumentName)
    {
        DBG("Warning: Restoring state for different instrument name: " 
            << storedName << " vs " << instrumentName);
    }
    
    DBG("TracktionInstrument " << instrumentName << " state restored");
    return true;
}

// =========================================================================
// Timer Implementation (for MIDI playback timing)
// =========================================================================

void TracktionInstrument::timerCallback()
{
    if (!isPlaybackActive || isPlaybackPaused || !currentSequence)
        return;
        
    updatePlaybackPosition();
    
    // Process MIDI events that should happen at current time
    bool processedEvents = false;
    while (nextEventIndex < currentSequence->getNumEvents())
    {
        auto* eventHolder = currentSequence->getEventPointer(nextEventIndex);
        if (eventHolder->message.getTimeStamp() <= currentPlaybackTime)
        {
            sendMidiToTrack(eventHolder->message);
            nextEventIndex++;
            processedEvents = true;
        }
        else
        {
            break; // No more events ready to play
        }
    }
    
    // Check if we've finished the sequence
    if (nextEventIndex >= currentSequence->getNumEvents() && !processedEvents)
    {
        // Give a bit of time for notes to finish, then stop
        static int finishCounter = 0;
        finishCounter++;
        if (finishCounter > 25) // ~500ms at 20ms intervals
        {
            finishCounter = 0;
            stopPlayback();
        }
    }
    else
    {
        static int finishCounter = 0;
        finishCounter = 0; // Reset counter if still playing
        (void)finishCounter; // Suppress unused variable warning
    }
}

// =========================================================================
// Helper Methods
// =========================================================================

void TracktionInstrument::updatePlaybackPosition()
{
    // Update playback time based on tempo
    double deltaTime = 0.02; // 20ms timer interval
    currentPlaybackTime += deltaTime;
}

void TracktionInstrument::sendMidiToTrack(const juce::MidiMessage& message)
{
    if (myTrack == nullptr)
        return;
        
    // Send MIDI directly to our track's input
    // Tracktion tracks process MIDI through their plugin chain during audio rendering
    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
    {
        // Create a MIDI buffer with our message
        juce::MidiBuffer midiBuffer;
        midiBuffer.addEvent(message, 0);
        
        // Send to track's MIDI input
        // Note: The actual audio processing happens during the Edit's render cycle
        audioTrack->injectLiveMidiMessage(message, true);
        
        DBG("TracktionInstrument " << instrumentName << " sent MIDI to track: " 
            << (message.isNoteOn() ? "NoteOn" : message.isNoteOff() ? "NoteOff" : "Other")
            << " Note=" << message.getNoteNumber() << " Vel=" << message.getVelocity());
    }
}

std::unique_ptr<juce::MidiMessageSequence> TracktionInstrument::convertChartToMidiSequence(const juce::var& chart)
{
    auto sequence = std::make_unique<juce::MidiMessageSequence>();
    
    if (!chart.isArray())
    {
        std::cerr << "Expected chart to be an array of MIDI events" << std::endl;
        return sequence;
    }
    
    auto* eventArray = chart.getArray();
    for (const auto& eventVar : *eventArray)
    {
        if (!eventVar.isObject())
            continue;
            
        auto* eventObj = eventVar.getDynamicObject();
        if (!eventObj)
            continue;
            
        // Extract event properties
        double time = eventObj->getProperty("time");
        juce::String type = eventObj->getProperty("type").toString();
        int note = eventObj->getProperty("note");
        int velocity = eventObj->getProperty("velocity");
        
        // Create appropriate MIDI message
        juce::MidiMessage message;
        if (type == "noteOn" && velocity > 0)
        {
            message = juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(velocity));
        }
        else if (type == "noteOff" || (type == "noteOn" && velocity == 0))
        {
            message = juce::MidiMessage::noteOff(1, note, static_cast<juce::uint8>(velocity));
        }
        else
        {
            continue; // Skip unknown event types
        }
        
        message.setTimeStamp(time);
        sequence->addEvent(message);
    }
    
    sequence->updateMatchedPairs();
    return sequence;
}

} // namespace covertracks
