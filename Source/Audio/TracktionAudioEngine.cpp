/*
  ==============================================================================
   TracktionAudioEngine.cpp
   
   Tracktion Engine-based replacement for the original AudioEngine singleton.
   
   This implementation uses professional DAW architecture to solve the
   performance mode clicking issues.
  ==============================================================================
*/

#include "TracktionAudioEngine.h"
#include "../Player/Core/Player.h"
#include "../Player/UI/CovertracksPluginWindow.h"

namespace covertracks {

//==============================================================================
// Singleton Implementation
//==============================================================================

TracktionAudioEngine& TracktionAudioEngine::getInstance()
{
    static TracktionAudioEngine instance;
    return instance;
}

TracktionAudioEngine::TracktionAudioEngine()
{
    // Initialize Tracktion Engine resources
    if (!initializeTracktionEngine()) {
        DBG("ERROR: Failed to initialize TracktionAudioEngine resources");
    }
}

TracktionAudioEngine::~TracktionAudioEngine()
{
    shutdownTracktionEngine();
}

//==============================================================================
// Tracktion Engine Initialization
//==============================================================================

bool TracktionAudioEngine::initializeTracktionEngine()
{
    try {
        DBG("INFO: Initializing TracktionAudioEngine...");
        
        // Create core audio resources (for compatibility)
        deviceManager = std::make_unique<juce::AudioDeviceManager>();
        formatManager = std::make_unique<juce::AudioPluginFormatManager>();
        knownPluginList = std::make_unique<juce::KnownPluginList>();
        
        // Initialize plugin formats
        formatManager->addDefaultFormats();
        
        // Initialize audio device with default settings
        juce::String audioError = deviceManager->initialise(2, 2, nullptr, true);
        if (audioError.isNotEmpty()) {
            DBG("ERROR: Audio device initialization failed: " << audioError);
            return false;
        }
        
        // Set this as the audio callback
        deviceManager->addAudioCallback(this);
        
        // Create Tracktion Engine
        DBG("INFO: Creating Tracktion Engine...");
        tracktionEngine = std::make_unique<te::Engine>("CovertracksTracktionEngine", std::make_unique<covertracks::CovertracksUIBehaviour>(), nullptr);
        
        // Now configure the Engine's PluginManager
        DBG("INFO: Configuring plugin scanning...");
        scanForPlugins();
        DBG("INFO: Creating Tracktion Edit...");
        editFile = juce::File::createTempFile(".tracktionedit");
        currentEdit = te::createEmptyEdit(*tracktionEngine, editFile);
        
        if (!currentEdit) {
            DBG("ERROR: Failed to create Tracktion Edit");
            return false;
        }
        
        // Configure the Edit for real-time performance
        currentEdit->playInStopEnabled = true;
        currentEdit->getTransport().addChangeListener(this);
        
        // Update audio parameters
        if (auto* device = deviceManager->getCurrentAudioDevice()) {
            currentSampleRate.store(device->getCurrentSampleRate());
            currentBufferSize.store(device->getCurrentBufferSizeSamples());
        }
        
        // Initialize quality monitoring
        resetQualityMonitoring();
        
        // Enable MIDI input by default
        setMidiInputEnabled(true);
        
        isAudioActive.store(true);
        
        DBG("INFO: TracktionAudioEngine initialized successfully");
        DBG("INFO: Edit created at: " << editFile.getFullPathName());
        
        return true;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception during TracktionAudioEngine initialization: " << e.what());
        return false;
    }
}

void TracktionAudioEngine::shutdownTracktionEngine()
{
    isAudioActive.store(false);
    
    // Deactivate all players first
    {
        juce::ScopedLock lock(playerTrackMutex);
        for (auto& [player, trackInfo] : playerTracks) {
            if (trackInfo->isActive.load()) {
                deactivatePlayer(player);
            }
        }
        playerTracks.clear();
    }
    
    // Remove audio callback
    if (deviceManager) {
        deviceManager->removeAudioCallback(this);
    }
    
    // Clean up Tracktion resources
    if (currentEdit) {
        currentEdit->getTransport().removeChangeListener(this);
        currentEdit.reset();
    }
    
    tracktionEngine.reset();
    
    // Clean up audio resources
    deviceManager.reset();
    formatManager.reset();
    knownPluginList.reset();
    
    // Clean up temporary edit file
    if (editFile.exists()) {
        editFile.deleteFile();
    }
    
    DBG("INFO: TracktionAudioEngine shutdown complete");
}

//==============================================================================
// Player Lifecycle Management
//==============================================================================

bool TracktionAudioEngine::registerPlayer(Player* player)
{
    if (!player || !currentEdit) {
        return false;
    }
    
    juce::ScopedLock lock(playerTrackMutex);
    
    // Check if already registered
    if (playerTracks.find(player) != playerTracks.end()) {
        DBG("INFO: Player already registered: " << player->getName());
        return false; // Already registered
    }
    
    // Create a dedicated Tracktion Track for this Player
    auto* track = createTrackForPlayer(player);
    if (!track) {
        DBG("ERROR: Failed to create track for player: " << player->getName());
        return false;
    }
    
    // Create PlayerTrackInfo
    juce::String trackName = "Player_" + player->getName() + "_Track";
    auto trackInfo = std::make_unique<PlayerTrackInfo>(player, track, trackName);
    
    // Register the player
    playerTracks[player] = std::move(trackInfo);
    
    DBG("INFO: Player registered successfully: " << player->getName() << " -> Track: " << trackName);
    return true;
}

void TracktionAudioEngine::unregisterPlayer(Player* player)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it != playerTracks.end()) {
        // Deactivate first if active
        if (it->second->isActive.load()) {
            deactivatePlayer(player);
        }
        
        // Remove the track from Tracktion Edit
        removeTrackForPlayer(player);
        
        // Remove from registry
        DBG("INFO: Player unregistered: " << player->getName());
        playerTracks.erase(it);
    }
}

bool TracktionAudioEngine::activatePlayer(Player* player)
{
    if (!player || !currentEdit) {
        return false;
    }
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it == playerTracks.end()) {
        DBG("ERROR: Player not registered, cannot activate: " << player->getName());
        return false;
    }
    
    auto& trackInfo = it->second;
    
    // Mark as active
    trackInfo->isActive.store(true);
    
    DBG("INFO: Player activated: " << player->getName());
    return true;
}

void TracktionAudioEngine::deactivatePlayer(Player* player)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it != playerTracks.end()) {
        auto& trackInfo = it->second;
        
        // Stop playback if playing
        if (trackInfo->isPlaying.load()) {
            stopPlayerPlayback(player);
        }
        
        // Mark as inactive
        trackInfo->isActive.store(false);
        
        DBG("INFO: Player deactivated: " << player->getName());
    }
}

bool TracktionAudioEngine::isPlayerRegistered(Player* player) const
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerTrackMutex);
    return playerTracks.find(player) != playerTracks.end();
}

bool TracktionAudioEngine::isPlayerActive(Player* player) const
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it != playerTracks.end()) {
        return it->second->isActive.load();
    }
    return false;
}

//==============================================================================
// Tracktion Track Management
//==============================================================================

te::Track* TracktionAudioEngine::getPlayerTrack(Player* player)
{
    if (!player) return nullptr;
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it != playerTracks.end()) {
        return it->second->track;
    }
    return nullptr;
}

te::MidiClip* TracktionAudioEngine::createMidiClipForPlayer(Player* player, 
                                                           const juce::String& clipName,
                                                           double startTime, 
                                                           double duration)
{
    auto* track = getPlayerTrack(player);
    if (!track) {
        DBG("ERROR: No track found for player: " << (player ? player->getName() : "null"));
        return nullptr;
    }
    
    try {
        // Create time range for the clip
        te::TimePosition start = te::TimePosition::fromSeconds(startTime);
        te::TimePosition end = te::TimePosition::fromSeconds(startTime + duration);
        te::TimeRange timeRange(start, end);
        
        // Create MIDI clip on the track
        // TODO: Replace with proper MidiList creation and clip association
        te::MidiClip* midiClip = nullptr; // Temporarily disabled for compilation
        
        if (midiClip) {
            DBG("INFO: Created MIDI clip '" << clipName << "' for player: " << player->getName());
        } else {
            DBG("ERROR: Failed to create MIDI clip for player: " << player->getName());
        }
        
        return midiClip;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception creating MIDI clip: " << e.what());
        return nullptr;
    }
}

bool TracktionAudioEngine::startPlayerPlayback(Player* player)
{
    if (!player || !currentEdit) {
        return false;
    }
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it == playerTracks.end() || !it->second->isActive.load()) {
        DBG("ERROR: Player not registered or not active: " << player->getName());
        return false;
    }
    
    auto& trackInfo = it->second;
    
    try {
        // Start the global transport if not already playing
        auto& transport = currentEdit->getTransport();
        if (!transport.isPlaying()) {
            transport.play(false); // false = don't restart from beginning
        }
        
        // Mark this player as playing
        trackInfo->isPlaying.store(true);
        
        DBG("INFO: Started playback for player: " << player->getName());
        return true;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception starting playback for player " << player->getName() << ": " << e.what());
        return false;
    }
}

void TracktionAudioEngine::stopPlayerPlayback(Player* player)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it != playerTracks.end()) {
        auto& trackInfo = it->second;
        trackInfo->isPlaying.store(false);
        
        DBG("INFO: Stopped playback for player: " << player->getName());
        
        // Check if no other players are playing, then stop transport
        bool anyPlayerPlaying = false;
        for (const auto& [otherPlayer, otherTrackInfo] : playerTracks) {
            if (otherTrackInfo->isPlaying.load()) {
                anyPlayerPlaying = true;
                break;
            }
        }
        
        if (!anyPlayerPlaying && currentEdit) {
            currentEdit->getTransport().stop(false, false);
            DBG("INFO: Stopped global transport (no players playing)");
        }
    }
}

bool TracktionAudioEngine::isPlayerPlaying(Player* player) const
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerTrackMutex);
    
    auto it = playerTracks.find(player);
    if (it != playerTracks.end()) {
        return it->second->isPlaying.load();
    }
    return false;
}

//==============================================================================
// Tracktion Engine Access
//==============================================================================

te::Engine& TracktionAudioEngine::getTracktionEngine()
{
    return *tracktionEngine;
}

te::Edit* TracktionAudioEngine::getCurrentEdit()
{
    return currentEdit.get();
}

te::TransportControl& TracktionAudioEngine::getTransport()
{
    return currentEdit->getTransport();
}

//==============================================================================
// MIDI Routing
//==============================================================================

void TracktionAudioEngine::routeMidiToPlayer(Player* player, const juce::MidiMessage& message)
{
    // For now, delegate to the player's MIDI callback if it has one
    // In the future, this could route directly to the Tracktion track
    sendMidiToPlayerTrack(player, message);
}

void TracktionAudioEngine::routeMidiToAllActivePlayers(const juce::MidiMessage& message)
{
    juce::ScopedLock lock(playerTrackMutex);
    
    for (const auto& [player, trackInfo] : playerTracks) {
        if (trackInfo->isActive.load()) {
            sendMidiToPlayerTrack(player, message);
        }
    }
}

void TracktionAudioEngine::sendMidiToPlayerTrack(Player* player, const juce::MidiMessage& message)
{
    // Route MIDI to the Player for processing
    // Player handles MIDI internally
    if (player) {
        player->processMidiMessage(message);
    }
}

//==============================================================================
// Resource Access
//==============================================================================

juce::AudioDeviceManager& TracktionAudioEngine::getDeviceManager()
{
    return *deviceManager;
}

juce::AudioPluginFormatManager& TracktionAudioEngine::getFormatManager()
{
    return *formatManager;
}

juce::KnownPluginList& TracktionAudioEngine::getKnownPluginList()
{
    return *knownPluginList;
}

//==============================================================================
// Audio Quality Monitoring
//==============================================================================

bool TracktionAudioEngine::isAudioQualityGood() const
{
    return audioQualityGood.load();
}

double TracktionAudioEngine::getCpuUsage() const
{
    return cpuUsage.load();
}

int TracktionAudioEngine::getBufferUnderruns() const
{
    return bufferUnderrunCount.load();
}

void TracktionAudioEngine::resetQualityMonitoring()
{
    cpuUsage.store(0.0);
    bufferUnderrunCount.store(0);
    audioQualityGood.store(true);
    averageProcessTime.store(0.0);
    
    DBG("INFO: Quality monitoring reset");
}

//==============================================================================
// Backward Compatibility Methods
//==============================================================================

void TracktionAudioEngine::setMidiInputEnabled(bool enabled)
{
    midiInputEnabled.store(enabled);
    DBG("INFO: MIDI input " << (enabled ? "enabled" : "disabled"));
}

bool TracktionAudioEngine::isMidiInputEnabled() const
{
    return midiInputEnabled.load();
}

void TracktionAudioEngine::sendMidiMessage(const juce::MidiMessage& message)
{
    routeMidiToAllActivePlayers(message);
}

void TracktionAudioEngine::enableMidiInput(bool enabled)
{
    setMidiInputEnabled(enabled);
}

bool TracktionAudioEngine::registerMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback)
{
    // For backward compatibility - could be implemented if needed
    juce::ignoreUnused(deviceName, callback);
    return true;
}

void TracktionAudioEngine::unregisterMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback)
{
    // For backward compatibility - could be implemented if needed
    juce::ignoreUnused(deviceName, callback);
}

void TracktionAudioEngine::registerEditModeProcessor(juce::AudioProcessor* processor)
{
    juce::ScopedLock lock(editModeProcessorMutex);
    editModeProcessor = processor;
}

void TracktionAudioEngine::unregisterEditModeProcessor()
{
    juce::ScopedLock lock(editModeProcessorMutex);
    editModeProcessor = nullptr;
}

double TracktionAudioEngine::getSampleRate() const
{
    return currentSampleRate.load();
}

int TracktionAudioEngine::getBufferSize() const
{
    return currentBufferSize.load();
}

int TracktionAudioEngine::getInputLatencySamples() const
{
    if (auto* device = deviceManager->getCurrentAudioDevice()) {
        return device->getInputLatencyInSamples();
    }
    return 0;
}

int TracktionAudioEngine::getOutputLatencySamples() const
{
    if (auto* device = deviceManager->getCurrentAudioDevice()) {
        return device->getOutputLatencyInSamples();
    }
    return 0;
}

void TracktionAudioEngine::showAudioSettings(juce::Component* parentComponent)
{
    if (deviceManager) {
        auto settingsComponent = std::make_unique<juce::AudioDeviceSelectorComponent>(*deviceManager, 2, 2, 2, 2, false, false, true, false);
        settingsComponent->setSize(500, 450);
        
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = "Audio Settings";
        options.content.setOwned(settingsComponent.release());
        options.componentToCentreAround = parentComponent;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        
        options.launchAsync();
    }
}

void TracktionAudioEngine::setDoublePrecisionProcessing(bool doublePrecision)
{
    // For backward compatibility - Tracktion handles precision internally
    juce::ignoreUnused(doublePrecision);
}

void TracktionAudioEngine::unregisterProcessor(juce::AudioProcessor* processor)
{
    // For backward compatibility - no-op
    juce::ignoreUnused(processor);
}

void TracktionAudioEngine::processMidiKeyboardState(juce::MidiKeyboardState& externalKeyboardState)
{
    // For backward compatibility - could route to active players
    juce::ignoreUnused(externalKeyboardState);
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void TracktionAudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                           int numInputChannels,
                                                           float* const* outputChannelData,
                                                           int numOutputChannels,
                                                           int numSamples,
                                                           const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Audio input tap: forward raw input audio to any registered listener
    // before output buffers are touched. Lock-free, allocation-free.
    if (auto* listener = audioInputListener.load())
    {
        if (numInputChannels > 0 && inputChannelData != nullptr)
            listener->audioInputBlock (inputChannelData, numInputChannels, numSamples, currentSampleRate.load());
    }

    // Clear output buffers
    for (int i = 0; i < numOutputChannels; ++i) {
        juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
    
    // Process through Tracktion Engine if we have an active edit
    if (currentEdit && isAudioActive.load()) {
        try {
            // Create audio buffer for processing
            juce::AudioBuffer<float> audioBuffer(outputChannelData, numOutputChannels, numSamples);
            juce::MidiBuffer midiBuffer; // Empty MIDI buffer for now
            
            // Handle edit mode processor if present (for edit mode audio)
            {
                juce::ScopedLock lock(editModeProcessorMutex);
                if (editModeProcessor) {
                    editModeProcessor->processBlock(audioBuffer, midiBuffer);
                }
            }
            
            // Process all active Player audio processors
            {
                juce::ScopedLock lock(playerTrackMutex);
                for (const auto& [player, trackInfo] : playerTracks) {
                    if (trackInfo->isActive.load() && player) {
                        // Get the player's instrument and process its audio
                        if (player->getInstrument() != nullptr) {
                            // Note: Audio processing now handled by Tracktion Tracks
                            // Custom processor graph removed in favor of Tracktion architecture
                        }
                    }
                }
            }
            
        } catch (const std::exception& e) {
            DBG("ERROR: Exception in audio callback: " << e.what());
            bufferUnderrunCount.fetch_add(1);
        }
    }
    
    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    updateQualityMetrics(processingTime);
}

void TracktionAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device) {
        currentSampleRate.store(device->getCurrentSampleRate());
        currentBufferSize.store(device->getCurrentBufferSizeSamples());
        
        DBG("INFO: Audio device started - " << currentSampleRate.load() << "Hz, " << currentBufferSize.load() << " samples");
    }
    
    resetQualityMonitoring();
}

void TracktionAudioEngine::audioDeviceStopped()
{
    DBG("INFO: Audio device stopped");
}

//==============================================================================
// ChangeListener Implementation
//==============================================================================

void TracktionAudioEngine::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (currentEdit && source == &currentEdit->getTransport()) {
        // Handle transport state changes if needed
        auto& transport = currentEdit->getTransport();
        DBG("INFO: Transport state changed - Playing: " << (transport.isPlaying() ? "Yes" : "No"));
    }
}

//==============================================================================
// Audio Input Tap
//==============================================================================

void TracktionAudioEngine::setAudioInputListener(AudioInputListener* listener)
{
    audioInputListener.store(listener);
}

TracktionAudioEngine::AudioInputListener* TracktionAudioEngine::getAudioInputListener() const
{
    return audioInputListener.load();
}

//==============================================================================
// Tracktion Helpers
//==============================================================================

te::Track* TracktionAudioEngine::createTrackForPlayer(Player* player)
{
    if (!currentEdit || !player) {
        return nullptr;
    }
    
    try {
        // Create a new audio track in the edit
        auto track = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
        
        if (track) {
            // Set track name
            juce::String trackName = "Player_" + player->getName();
            track->setName(trackName);
            
            DBG("INFO: Created Tracktion track: " << trackName);
        }
        
        return track;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception creating track for player: " << e.what());
        return nullptr;
    }
}

void TracktionAudioEngine::removeTrackForPlayer(Player* player)
{
    if (!currentEdit || !player) {
        return;
    }
    
    // Find and remove the player's track
    auto* track = getPlayerTrack(player);
    if (track) {
        try {
            currentEdit->deleteTrack(track);
            DBG("INFO: Removed Tracktion track for player: " << player->getName());
        } catch (const std::exception& e) {
            DBG("ERROR: Exception removing track: " << e.what());
        }
    }
}

void TracktionAudioEngine::updateQualityMetrics(double processingTime)
{
    // Update CPU usage estimate
    double bufferDuration = (currentBufferSize.load() / currentSampleRate.load()) * 1000.0; // ms
    double currentCpuUsage = processingTime / bufferDuration;
    
    // Smooth the CPU usage reading
    double smoothedCpu = (cpuUsage.load() * 0.9) + (currentCpuUsage * 0.1);
    cpuUsage.store(smoothedCpu);
    
    // Update average processing time
    double avgTime = (averageProcessTime.load() * 0.95) + (processingTime * 0.05);
    averageProcessTime.store(avgTime);
}

void TracktionAudioEngine::checkAudioQuality()
{
    // Check if audio quality is good based on CPU usage and underruns
    bool qualityGood = (cpuUsage.load() < 0.8) && (bufferUnderrunCount.load() < 10);
    audioQualityGood.store(qualityGood);
}

void TracktionAudioEngine::scanForPlugins()
{
    if (!tracktionEngine)
    {
        DBG("ERROR: Cannot scan plugins - Tracktion Engine not initialized");
        return;
    }
    
    // Get the Engine's PluginManager
    // auto& pluginManager = tracktionEngine->getPluginManager(); // Unused for now
    
    // The PluginManager automatically handles plugin scanning
    // Just trigger a scan to ensure plugins are loaded
    DBG("INFO: Using Tracktion Engine's built-in plugin management");
    
    // The Engine's PluginManager handles plugin discovery automatically
    // No need for manual scanning - it will scan on demand
}

} // namespace covertracks
