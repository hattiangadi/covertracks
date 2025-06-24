#include "Instrument.h"
#include "../../Audio/AudioEngineInterface.h"
#include <iostream>

namespace covertracks {

Instrument::Instrument(const juce::String& name,
                                       juce::AudioPluginFormatManager& fm,
                                       juce::KnownPluginList& kpl)
    : instrumentName(name)
    , formatManager(fm)
    , knownPluginList(kpl)
{
    // Start timer for MIDI playback timing (50Hz = 20ms intervals)
    startTimer(20);
    
    DBG("Instrument created: " << name);
}

Instrument::~Instrument()
{
    stopTimer();
    stopPlayback();
    unregisterFromAudioEngine();
    
    DBG("Instrument destroyed: " << instrumentName);
}

// =========================================================================
// Audio Engine Integration (Self-Managed)
// =========================================================================

bool Instrument::registerWithAudioEngine()
{
    if (myTrack != nullptr)
    {
        DBG("Instrument " << instrumentName << " already registered");
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
    
    // Note: No default plugins added automatically
    // Users can add plugins through the UI or by loading saved state
    DBG("Instrument " << instrumentName << " created with empty plugin chain");
    
    // Ensure the Edit transport is in a state that allows audio processing
    auto& transport = currentEdit->getTransport();
    if (!transport.isPlaying())
    {
        std::cout << "Starting Tracktion Edit transport for real-time audio processing..." << std::endl;
        transport.play(false);  // Start playing (false = don't punch record)
    }
    
    DBG("Instrument " << instrumentName << " registered with Track: " << myTrack->getName());
    return true;
}

void Instrument::unregisterFromAudioEngine()
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
    DBG("Instrument " << instrumentName << " unregistered from audio engine");
}

// =========================================================================
// Plugin Management (Complete Ownership)
// =========================================================================

bool Instrument::addPlugin(const juce::PluginDescription& pluginDesc)
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
    
    std::cout << "Instrument: Adding plugin " << pluginDesc.name 
              << " (format: " << pluginDesc.pluginFormatName << ")" << std::endl;
    
    // Use different approaches for built-in vs external plugins
    if (pluginDesc.pluginFormatName == te::PluginManager::builtInPluginFormatName)
    {
        // Built-in Tracktion plugin - use standard approach
        std::cout << "Instrument: Built-in plugin - using category: " << pluginDesc.category << std::endl;
        
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
        std::cout << "Instrument: Creating external plugin: " << pluginDesc.name << std::endl;
        
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
            std::cout << "Instrument: Added external plugin " << plugin->getName() << " to track" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Instrument: Track is not an AudioTrack" << std::endl;
            return false;
        }
    }
    
    std::cerr << "Track is not an AudioTrack - cannot add plugins" << std::endl;
    return false;
}

void Instrument::clearPlugins()
{
    if (myTrack == nullptr)
        return;
        
    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
    {
        audioTrack->pluginList.clear();
        DBG("Cleared all plugins from " << instrumentName);
    }
}

bool Instrument::addDefaultSynth()
{
    if (myTrack == nullptr)
    {
        std::cerr << "Cannot add default synth: instrument not registered with audio engine" << std::endl;
        return false;
    }
    
    auto& audioEngine = getAudioEngine();
    auto* currentEdit = audioEngine.getCurrentEdit();
    if (!currentEdit)
    {
        std::cerr << "No Tracktion Edit available" << std::endl;
        return false;
    }
    
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
                audioTrack->pluginList.insertPlugin(synthPlugin, -1, nullptr);
                DBG("Instrument " << instrumentName << " added default synthesizer: " << synthDesc.name);
                return true;
            }
        }
        else
        {
            std::cerr << "Failed to create default synthesizer for " << instrumentName << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error adding default synthesizer to " << instrumentName << ": " << e.what() << std::endl;
    }
    
    return false;
}

bool Instrument::removePlugin(int index)
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

bool Instrument::movePlugin(int fromIndex, int toIndex)
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

int Instrument::getPluginCount() const
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

juce::StringArray Instrument::getPluginNames() const
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

te::Plugin* Instrument::getPlugin(int index) const
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

juce::AudioProcessor* Instrument::getJucePlugin(int index) const
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

bool Instrument::playChart(const juce::var& chart, double tempo)
{
    auto sequence = convertChartToMidiSequence(chart);
    if (!sequence)
    {
        std::cerr << "Failed to convert chart to MIDI sequence" << std::endl;
        return false;
    }
    
    return playMidiSequence(*sequence, tempo);
}

bool Instrument::playMidiSequence(const juce::MidiMessageSequence& sequence, double tempo)
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
        DBG("Instrument " << instrumentName << " started Edit transport for audio processing");
    }
    
    // Start the timer to process MIDI events (20ms intervals for smooth playback)
    startTimer(20);
    
    DBG("Instrument " << instrumentName << " started MIDI playback with " 
        << currentSequence->getNumEvents() << " events at " << tempo << " BPM");
    
    return true;
}

void Instrument::stopPlayback()
{
    if (isPlaybackActive)
    {
        isPlaybackActive = false;
        isPlaybackPaused = false;
        
        // Stop the timer first
        stopTimer();
        
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
            DBG("Instrument " << instrumentName << " stopped Edit transport");
        }
        
        // Call completion callback if set
        if (onPlaybackFinished)
        {
            onPlaybackFinished();
        }
        
        DBG("Instrument " << instrumentName << " stopped playback");
    }
}

void Instrument::pausePlayback()
{
    if (isPlaybackActive && !isPlaybackPaused)
    {
        isPlaybackPaused = true;
        DBG("Instrument " << instrumentName << " paused playback");
    }
}

void Instrument::resumePlayback()
{
    if (isPlaybackActive && isPlaybackPaused)
    {
        isPlaybackPaused = false;
        DBG("Instrument " << instrumentName << " resumed playback");
    }
}

void Instrument::setPlaybackFinishedCallback(std::function<void()> callback)
{
    onPlaybackFinished = callback;
}


// =========================================================================
// MIDI Input Handling
// =========================================================================

void Instrument::handleIncomingMidiMessage(juce::MidiInput* /*source*/, 
                                                   const juce::MidiMessage& message)
{
    processMidiMessage(message);
}

void Instrument::processMidiMessage(const juce::MidiMessage& message)
{
    sendMidiToTrack(message);
}

// =========================================================================
// State Management
// =========================================================================

std::unique_ptr<juce::XmlElement> Instrument::getPluginState() const
{
    auto state = std::make_unique<juce::XmlElement>("InstrumentState");
    state->setAttribute("name", instrumentName);
    state->setAttribute("pluginCount", getPluginCount());
    
    if (myTrack)
    {
        state->setAttribute("trackName", myTrack->getName());
        
        // Save detailed plugin chain with configurations
        auto* pluginsElement = state->createNewChildElement("PluginChain");
        
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
        {
            for (int i = 0; i < audioTrack->pluginList.size(); ++i)
            {
                auto* plugin = audioTrack->pluginList[i];
                if (!plugin) continue;
                
                auto* pluginElement = pluginsElement->createNewChildElement("Plugin");
                pluginElement->setAttribute("index", i);
                pluginElement->setAttribute("name", plugin->getName());
                pluginElement->setAttribute("type", plugin->getPluginType());
                
                // Save plugin-specific state using Tracktion's state management
                plugin->flushPluginStateToValueTree();
                auto pluginState = plugin->state;
                if (pluginState.isValid())
                {
                    if (auto stateXml = pluginState.createXml())
                    {
                        pluginElement->addChildElement(stateXml.release());
                    }
                }
                
                // For external plugins, save the plugin description
                if (auto* externalPlugin = dynamic_cast<te::ExternalPlugin*>(plugin))
                {
                    auto* descElement = pluginElement->createNewChildElement("PluginDescription");
                    descElement->setAttribute("name", externalPlugin->desc.name);
                    descElement->setAttribute("format", externalPlugin->desc.pluginFormatName);
                    descElement->setAttribute("identifier", externalPlugin->desc.fileOrIdentifier);
                    descElement->setAttribute("manufacturer", externalPlugin->desc.manufacturerName);
                    descElement->setAttribute("isInstrument", externalPlugin->desc.isInstrument);
                }
            }
        }
    }
    
    return state;
}

bool Instrument::restorePluginState(const juce::XmlElement& state)
{
    if (state.getTagName() != "InstrumentState")
    {
        std::cerr << "Invalid state XML for Instrument" << std::endl;
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
    
    DBG("Instrument " << instrumentName << " state restored");
    return true;
}

juce::var Instrument::saveInstrumentState() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    // Basic instrument info
    obj->setProperty("name", instrumentName);
    obj->setProperty("pluginCount", getPluginCount());
    
    // Save plugin chain
    juce::Array<juce::var> pluginChain;
    
    if (myTrack)
    {
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
        {
            for (int i = 0; i < audioTrack->pluginList.size(); ++i)
            {
                auto* plugin = audioTrack->pluginList[i];
                if (!plugin) continue;
                
                juce::DynamicObject::Ptr pluginObj = new juce::DynamicObject();
                pluginObj->setProperty("index", i);
                pluginObj->setProperty("name", plugin->getName());
                pluginObj->setProperty("type", plugin->getPluginType());
                
                // Save plugin state as XML string
                plugin->flushPluginStateToValueTree();
                auto pluginState = plugin->state;
                if (pluginState.isValid())
                {
                    if (auto stateXml = pluginState.createXml())
                    {
                        pluginObj->setProperty("state", stateXml->toString());
                    }
                }
                
                // Save plugin description for recreation
                juce::DynamicObject::Ptr descObj = new juce::DynamicObject();
                
                if (auto* externalPlugin = dynamic_cast<te::ExternalPlugin*>(plugin))
                {
                    // External plugin - save full plugin description
                    descObj->setProperty("name", externalPlugin->desc.name);
                    descObj->setProperty("format", externalPlugin->desc.pluginFormatName);
                    descObj->setProperty("identifier", externalPlugin->desc.fileOrIdentifier);
                    descObj->setProperty("manufacturer", externalPlugin->desc.manufacturerName);
                    descObj->setProperty("isInstrument", externalPlugin->desc.isInstrument);
                    descObj->setProperty("uniqueId", externalPlugin->desc.uniqueId);
                    descObj->setProperty("version", externalPlugin->desc.version);
                }
                else
                {
                    // Built-in plugin - save plugin type information for recreation
                    descObj->setProperty("name", plugin->getName());
                    descObj->setProperty("format", te::PluginManager::builtInPluginFormatName);
                    descObj->setProperty("identifier", "builtin");
                    descObj->setProperty("manufacturer", "Tracktion");
                    
                    // For built-in plugins, determine if it's an instrument based on plugin type
                    juce::String pluginTypeName = plugin->getPluginType();
                    bool isInstrument = pluginTypeName.contains("Synth") || 
                                       pluginTypeName.contains("Instrument") ||
                                       pluginTypeName.contains("Osc");
                    descObj->setProperty("isInstrument", isInstrument);
                    descObj->setProperty("category", plugin->getPluginType()); // This is the key for built-in plugins!
                }
                
                pluginObj->setProperty("description", juce::var(descObj.get()));
                
                pluginChain.add(juce::var(pluginObj.get()));
            }
        }
    }
    
    obj->setProperty("pluginChain", pluginChain);
    
    return juce::var(obj.get());
}

bool Instrument::loadInstrumentState(const juce::var& state)
{
    if (!state.isObject())
    {
        std::cerr << "Invalid instrument state: not an object" << std::endl;
        return false;
    }
    
    auto* obj = state.getDynamicObject();
    if (!obj)
    {
        std::cerr << "Invalid instrument state: could not get dynamic object" << std::endl;
        return false;
    }
    
    // Clear existing plugins
    clearPlugins();
    
    // Load plugin chain
    auto pluginChainVar = obj->getProperty("pluginChain");
    if (pluginChainVar.isArray())
    {
        auto* pluginArray = pluginChainVar.getArray();
        for (const auto& pluginVar : *pluginArray)
        {
            if (!pluginVar.isObject()) continue;
            
            auto* pluginObj = pluginVar.getDynamicObject();
            if (!pluginObj) continue;
            
            juce::String pluginType = pluginObj->getProperty("type");
            
            // Recreate the plugin based on type
            if (pluginType == te::ExternalPlugin::xmlTypeName)
            {
                // External plugin - recreate from description
                auto descVar = pluginObj->getProperty("description");
                if (descVar.isObject())
                {
                    auto* descObj = descVar.getDynamicObject();
                    
                    juce::PluginDescription desc;
                    desc.name = descObj->getProperty("name");
                    desc.pluginFormatName = descObj->getProperty("format");
                    desc.fileOrIdentifier = descObj->getProperty("identifier");
                    desc.manufacturerName = descObj->getProperty("manufacturer");
                    desc.isInstrument = descObj->getProperty("isInstrument");
                    desc.uniqueId = descObj->getProperty("uniqueId");
                    desc.version = descObj->getProperty("version");
                    
                    // Try to add the plugin
                    if (addPlugin(desc))
                    {
                        // Restore plugin state if available
                        auto stateStr = pluginObj->getProperty("state").toString();
                        if (stateStr.isNotEmpty())
                        {
                            if (auto stateXml = juce::XmlDocument::parse(stateStr))
                            {
                                auto valueTree = juce::ValueTree::fromXml(*stateXml);
                                if (valueTree.isValid())
                                {
                                    // Get the last added plugin and restore its state
                                    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
                                    {
                                        if (audioTrack->pluginList.size() > 0)
                                        {
                                            auto* plugin = audioTrack->pluginList[audioTrack->pluginList.size() - 1];
                                            if (plugin)
                                            {
                                                plugin->restorePluginStateFromValueTree(valueTree);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // Built-in plugin - recreate from category
                auto descVar = pluginObj->getProperty("description");
                if (descVar.isObject())
                {
                    auto* descObj = descVar.getDynamicObject();
                    
                    // Check if this is a built-in plugin
                    auto format = descObj->getProperty("format").toString();
                    if (format == te::PluginManager::builtInPluginFormatName)
                    {
                        auto category = descObj->getProperty("category").toString();
                        auto pluginName = descObj->getProperty("name").toString();
                        
                        std::cout << "Restoring built-in plugin: " << pluginName << " (category: " << category << ")" << std::endl;
                        
                        // Create plugin description for built-in plugin
                        juce::PluginDescription desc;
                        desc.name = pluginName;
                        desc.pluginFormatName = te::PluginManager::builtInPluginFormatName;
                        desc.category = category;
                        desc.isInstrument = descObj->getProperty("isInstrument");
                        
                        // Try to add the built-in plugin
                        if (addPlugin(desc))
                        {
                            std::cout << "Successfully restored built-in plugin: " << pluginName << std::endl;
                            
                            // Restore plugin state if available
                            auto stateStr = pluginObj->getProperty("state").toString();
                            if (stateStr.isNotEmpty())
                            {
                                if (auto stateXml = juce::XmlDocument::parse(stateStr))
                                {
                                    auto valueTree = juce::ValueTree::fromXml(*stateXml);
                                    if (valueTree.isValid())
                                    {
                                        // Get the last added plugin and restore its state
                                        if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(myTrack))
                                        {
                                            if (audioTrack->pluginList.size() > 0)
                                            {
                                                auto* plugin = audioTrack->pluginList[audioTrack->pluginList.size() - 1];
                                                if (plugin)
                                                {
                                                    plugin->restorePluginStateFromValueTree(valueTree);
                                                    std::cout << "Restored state for built-in plugin: " << pluginName << std::endl;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            std::cerr << "Failed to restore built-in plugin: " << pluginName << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    DBG("Instrument " << instrumentName << " state loaded successfully");
    return true;
}

// =========================================================================
// Timer Implementation (for MIDI playback timing)
// =========================================================================

void Instrument::timerCallback()
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

void Instrument::updatePlaybackPosition()
{
    // Update playback time based on tempo
    double deltaTime = 0.02; // 20ms timer interval
    currentPlaybackTime += deltaTime;
}

void Instrument::sendMidiToTrack(const juce::MidiMessage& message)
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
        
        DBG("Instrument " << instrumentName << " sent MIDI to track: " 
            << (message.isNoteOn() ? "NoteOn" : message.isNoteOff() ? "NoteOff" : "Other")
            << " Note=" << message.getNoteNumber() << " Vel=" << message.getVelocity());
    }
}

std::unique_ptr<juce::MidiMessageSequence> Instrument::convertChartToMidiSequence(const juce::var& chart)
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
            
        // Extract event properties - handle both formats
        double time = eventObj->getProperty("time");
        if (time == 0.0) // Try alternative property name
            time = eventObj->getProperty("timestamp");
            
        juce::String type = eventObj->getProperty("type").toString();
        int note = eventObj->getProperty("note");
        
        // Handle velocity properly - convert float (0.0-1.0) to MIDI velocity (1-127)
        double velocityFloat = eventObj->getProperty("velocity");
        int velocity = static_cast<int>(velocityFloat * 127.0);
        velocity = juce::jlimit(1, 127, velocity); // Ensure non-zero velocity for NoteOn
        
        // Debug: Log the MIDI event details
        DBG("MIDI Event: type='" << type << "' note=" << note << " velocity=" << velocityFloat << " (MIDI: " << velocity << ") time=" << time);
        
        // Create appropriate MIDI message - handle both formats
        juce::MidiMessage message;
        if ((type == "noteOn" || type == "note_on") && velocity > 0)
        {
            message = juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(velocity));
            DBG("Created NoteOn: Note=" << note << " Vel=" << velocity);
        }
        else if (type == "noteOff" || type == "note_off" || ((type == "noteOn" || type == "note_on") && velocity == 0))
        {
            message = juce::MidiMessage::noteOff(1, note, static_cast<juce::uint8>(velocity));
            DBG("Created NoteOff: Note=" << note << " Vel=" << velocity);
        }
        else
        {
            DBG("Skipped unknown event type: '" << type << "'");
            continue; // Skip unknown event types
        }
        
        message.setTimeStamp(time);
        sequence->addEvent(message);
    }
    
    sequence->updateMatchedPairs();
    return sequence;
}

} // namespace covertracks
