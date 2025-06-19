/*
  ==============================================================================
   AudioEngine.cpp
   
   Centralized singleton audio engine implementation for managing shared audio 
   resources and coordinating multiple Players without conflicts.
  ==============================================================================
*/

#include "AudioEngine.h"
#include "../Player/Core/Player.h"

namespace covertracks {

//==============================================================================
// Singleton Implementation
//==============================================================================

AudioEngine& AudioEngine::getInstance()
{
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine()
{
    // Initialize audio resources
    if (!initializeAudioResources()) {
        DBG("ERROR: Failed to initialize AudioEngine resources");
    }
}

AudioEngine::~AudioEngine()
{
    shutdownAudioResources();
}

//==============================================================================
// Resource Initialization
//==============================================================================

bool AudioEngine::initializeAudioResources()
{
    try {
        // Create core audio resources
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
        
        // Initialize master audio graph
        if (!initializeMasterGraph()) {
            DBG("ERROR: Failed to initialize master audio graph");
            return false;
        }
        
        // Update audio parameters
        if (auto* device = deviceManager->getCurrentAudioDevice()) {
            currentSampleRate.store(device->getCurrentSampleRate());
            currentBufferSize.store(device->getCurrentBufferSizeSamples());
        }
        
        // Initialize quality monitoring
        resetQualityMonitoring();
        
        isAudioActive.store(true);
        
        DBG("INFO: AudioEngine initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception during AudioEngine initialization: " << e.what());
        return false;
    }
}

void AudioEngine::shutdownAudioResources()
{
    isAudioActive.store(false);
    
    // Deactivate all players first
    {
        juce::ScopedLock lock(playerRegistryMutex);
        for (auto& [player, node] : playerNodes) {
            if (node->isActive.load()) {
                removePlayerFromMasterGraph(node.get());
            }
        }
        playerNodes.clear();
    }
    
    // Remove audio callback
    if (deviceManager) {
        deviceManager->removeAudioCallback(this);
    }
    
    // Clear master graph
    masterGraph.reset();
    midiInputNode.reset();
    audioOutputNode.reset();
    
    // Clear resources
    knownPluginList.reset();
    formatManager.reset();
    deviceManager.reset();
    
    DBG("INFO: AudioEngine shutdown complete");
}

bool AudioEngine::initializeMasterGraph()
{
    try {
        masterGraph = std::make_unique<juce::AudioProcessorGraph>();
        
        // Create MIDI input node
        midiInputNode = masterGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
        
        // Create audio output node  
        audioOutputNode = masterGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
        
        if (!midiInputNode || !audioOutputNode) {
            DBG("ERROR: Failed to create master graph I/O nodes");
            return false;
        }
        
        // Prepare the graph
        if (auto* device = deviceManager->getCurrentAudioDevice()) {
            double sampleRate = device->getCurrentSampleRate();
            int bufferSize = device->getCurrentBufferSizeSamples();
            
            masterGraph->prepareToPlay(sampleRate, bufferSize);
        }
        
        DBG("INFO: Master audio graph initialized");
        return true;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception during master graph initialization: " << e.what());
        return false;
    }
}

//==============================================================================
// Player Lifecycle Management (Thread-Safe)
//==============================================================================

bool AudioEngine::registerPlayer(Player* player)
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    // Check if already registered
    if (playerNodes.find(player) != playerNodes.end()) {
        return false; // Already registered
    }
    
    // Create player audio node
    auto playerNode = std::make_unique<PlayerAudioNode>(player);
    playerNodes[player] = std::move(playerNode);
    
    DBG("INFO: Player registered with AudioEngine");
    return true;
}

void AudioEngine::unregisterPlayer(Player* player)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    auto it = playerNodes.find(player);
    if (it != playerNodes.end()) {
        // Deactivate if active
        if (it->second->isActive.load()) {
            removePlayerFromMasterGraph(it->second.get());
        }
        
        playerNodes.erase(it);
        DBG("INFO: Player unregistered from AudioEngine");
    }
}

bool AudioEngine::activatePlayer(Player* player)
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    auto it = playerNodes.find(player);
    if (it == playerNodes.end()) {
        return false; // Not registered
    }
    
    auto* playerNode = it->second.get();
    if (playerNode->isActive.load()) {
        return true; // Already active
    }
    
    // Add to master graph
    if (addPlayerToMasterGraph(playerNode)) {
        playerNode->isActive.store(true);
        DBG("INFO: Player activated in AudioEngine");
        return true;
    }
    
    return false;
}

void AudioEngine::deactivatePlayer(Player* player)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    auto it = playerNodes.find(player);
    if (it != playerNodes.end()) {
        auto* playerNode = it->second.get();
        if (playerNode->isActive.load()) {
            removePlayerFromMasterGraph(playerNode);
            playerNode->isActive.store(false);
            playerNode->isPlaying.store(false);
            DBG("INFO: Player deactivated in AudioEngine");
        }
    }
}

bool AudioEngine::isPlayerRegistered(Player* player) const
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    return playerNodes.find(player) != playerNodes.end();
}

bool AudioEngine::isPlayerActive(Player* player) const
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    auto it = playerNodes.find(player);
    return (it != playerNodes.end()) && it->second->isActive.load();
}

//==============================================================================
// Audio Graph Management
//==============================================================================

bool AudioEngine::addPlayerToMasterGraph(PlayerAudioNode* playerNode)
{
    if (!playerNode || !masterGraph) return false;
    
    try {
        // TODO: Add player's processor graph as node when Player interface is updated
        // For now, just mark as ready
        DBG("INFO: Player added to master audio graph (placeholder)");
        return true;
        
    } catch (const std::exception& e) {
        DBG("ERROR: Failed to add player to master graph: " << e.what());
        return false;
    }
}

void AudioEngine::removePlayerFromMasterGraph(PlayerAudioNode* playerNode)
{
    if (!playerNode || !masterGraph) return;
    
    try {
        // Remove player's graph node
        if (playerNode->graphNode) {
            masterGraph->removeNode(playerNode->graphNode);
            playerNode->graphNode.reset();
        }
        
        DBG("INFO: Player removed from master audio graph");
        
    } catch (const std::exception& e) {
        DBG("ERROR: Failed to remove player from master graph: " << e.what());
    }
}

//==============================================================================
// MIDI Routing and Distribution
//==============================================================================

void AudioEngine::routeMidiToPlayer(Player* player, const juce::MidiMessage& message)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerRegistryMutex);
    auto it = playerNodes.find(player);
    if (it != playerNodes.end() && it->second->isActive.load()) {
        // TODO: Route MIDI to specific player when Player interface is updated
        DBG("INFO: MIDI message routed to player (placeholder)");
    }
}

void AudioEngine::routeMidiToAllActivePlayers(const juce::MidiMessage& message)
{
    juce::ScopedLock lock(playerRegistryMutex);
    
    for (auto& [player, node] : playerNodes) {
        if (node->isActive.load()) {
            // TODO: Route MIDI to player when Player interface is updated
        }
    }
}

void AudioEngine::setMidiInputEnabled(bool enabled)
{
    midiInputEnabled.store(enabled);
    DBG("INFO: MIDI input " << (enabled ? "enabled" : "disabled"));
}

bool AudioEngine::isMidiInputEnabled() const
{
    return midiInputEnabled.load();
}

void AudioEngine::sendMidiMessage(const juce::MidiMessage& message)
{
    // Same as routeMidiToAllActivePlayers but for backward compatibility
    routeMidiToAllActivePlayers(message);
}

void AudioEngine::enableMidiInput(bool enabled)
{
    // Same as setMidiInputEnabled but for backward compatibility
    setMidiInputEnabled(enabled);
}

bool AudioEngine::registerMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback)
{
    if (!deviceManager || !callback) return false;
    
    try {
        deviceManager->addMidiInputDeviceCallback(deviceName, callback);
        DBG("INFO: Registered MIDI input callback for device: " << deviceName);
        return true;
    } catch (const std::exception& e) {
        DBG("ERROR: Failed to register MIDI input callback: " << e.what());
        return false;
    }
}

void AudioEngine::unregisterMidiInput(const juce::String& deviceName, juce::MidiInputCallback* callback)
{
    if (!deviceManager || !callback) return;
    
    try {
        deviceManager->removeMidiInputDeviceCallback(deviceName, callback);
        DBG("INFO: Unregistered MIDI input callback for device: " << deviceName);
    } catch (const std::exception& e) {
        DBG("ERROR: Failed to unregister MIDI input callback: " << e.what());
    }
}

//==============================================================================
// Resource Access (Thread-Safe)
//==============================================================================

juce::AudioDeviceManager& AudioEngine::getDeviceManager()
{
    jassert(deviceManager != nullptr);
    return *deviceManager;
}

juce::AudioPluginFormatManager& AudioEngine::getFormatManager()
{
    jassert(formatManager != nullptr);
    return *formatManager;
}

juce::KnownPluginList& AudioEngine::getKnownPluginList()
{
    jassert(knownPluginList != nullptr);
    return *knownPluginList;
}

//==============================================================================
// Audio Quality Monitoring
//==============================================================================

bool AudioEngine::isAudioQualityGood() const
{
    return audioQualityGood.load();
}

double AudioEngine::getCpuUsage() const
{
    return cpuUsage.load();
}

int AudioEngine::getBufferUnderruns() const
{
    return bufferUnderrunCount.load();
}

void AudioEngine::resetQualityMonitoring()
{
    cpuUsage.store(0.0);
    bufferUnderrunCount.store(0);
    audioQualityGood.store(true);
    averageProcessTime.store(0.0);
    lastProcessTime = std::chrono::high_resolution_clock::now();
    
    DBG("INFO: Quality monitoring reset");
}

void AudioEngine::updateQualityMetrics(double processingTime)
{
    // Update average processing time with exponential moving average
    double currentAverage = averageProcessTime.load();
    double alpha = 0.1; // Smoothing factor
    double newAverage = alpha * processingTime + (1.0 - alpha) * currentAverage;
    averageProcessTime.store(newAverage);
    
    // Calculate CPU usage estimate (processing time / available time)
    double bufferTime = static_cast<double>(currentBufferSize.load()) / currentSampleRate.load();
    double usage = processingTime / bufferTime;
    cpuUsage.store(juce::jmin(1.0, usage));
    
    // Check for buffer underruns (processing took too long)
    if (processingTime > bufferTime * 0.8) { // 80% threshold
        bufferUnderrunCount.fetch_add(1);
        
        // Mark quality as bad if too many underruns
        if (bufferUnderrunCount.load() > 5) {
            audioQualityGood.store(false);
        }
    }
}

void AudioEngine::checkAudioQuality()
{
    // Reset quality to good if underruns have stopped
    if (audioQualityGood.load() == false) {
        static int checkCounter = 0;
        if (++checkCounter > 100) { // Check every ~2 seconds at 44.1kHz/512 samples
            if (cpuUsage.load() < 0.5) { // CPU usage under control
                audioQualityGood.store(true);
                DBG("INFO: Audio quality restored");
            }
            checkCounter = 0;
        }
    }
}

//==============================================================================
// Audio Device Management
//==============================================================================

double AudioEngine::getSampleRate() const
{
    return currentSampleRate.load();
}

int AudioEngine::getBufferSize() const
{
    return currentBufferSize.load();
}

int AudioEngine::getInputLatencySamples() const
{
    if (auto* device = deviceManager->getCurrentAudioDevice()) {
        return device->getInputLatencyInSamples();
    }
    return 0;
}

int AudioEngine::getOutputLatencySamples() const
{
    if (auto* device = deviceManager->getCurrentAudioDevice()) {
        return device->getOutputLatencyInSamples();
    }
    return 0;
}

void AudioEngine::showAudioSettings(juce::Component* parentComponent)
{
    if (deviceManager) {
        juce::AudioDeviceSelectorComponent selector(*deviceManager, 2, 2, 2, 2, true, true, true, false);
        
        juce::DialogWindow::LaunchOptions options;
        options.content.setNonOwned(&selector);
        options.dialogTitle = "Audio Settings";
        options.componentToCentreAround = parentComponent;
        options.dialogBackgroundColour = juce::Colours::lightgrey;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        
        options.launchAsync();
    }
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                  int numInputChannels,
                                                  float* const* outputChannelData,
                                                  int numOutputChannels,
                                                  int numSamples,
                                                  const juce::AudioIODeviceCallbackContext& context)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Clear output buffers
    for (int i = 0; i < numOutputChannels; ++i) {
        if (outputChannelData[i] != nullptr) {
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
        }
    }
    
    // Only process if audio is active and we have a master graph
    if (!isAudioActive.load() || !masterGraph) {
        return;
    }
    
    // Try to get player registry lock without blocking audio thread
    juce::ScopedTryLock lock(playerRegistryMutex);
    if (!lock.isLocked()) {
        // Skip this block if we can't get the lock immediately
        return;
    }
    
    try {
        // Create audio buffer for processing
        juce::AudioBuffer<float> audioBuffer(const_cast<float**>(outputChannelData), 
                                           numOutputChannels, numSamples);
        
        // Create MIDI buffer for processing
        juce::MidiBuffer midiBuffer;
        
        // Process MIDI events from active players
        processMidiEvents(midiBuffer, numSamples);
        
        // Process master graph
        masterGraph->processBlock(audioBuffer, midiBuffer);
        
        // Update quality metrics
        auto endTime = std::chrono::high_resolution_clock::now();
        double processingTime = std::chrono::duration<double>(endTime - startTime).count();
        updateQualityMetrics(processingTime);
        
        // Check audio quality periodically
        checkAudioQuality();
        
    } catch (const std::exception& e) {
        DBG("ERROR: Exception in audio callback: " << e.what());
        
        // Clear output on exception to prevent noise
        for (int i = 0; i < numOutputChannels; ++i) {
            if (outputChannelData[i] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
            }
        }
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device) {
        double sampleRate = device->getCurrentSampleRate();
        int bufferSize = device->getCurrentBufferSizeSamples();
        
        currentSampleRate.store(sampleRate);
        currentBufferSize.store(bufferSize);
        
        // Prepare master graph
        if (masterGraph) {
            masterGraph->prepareToPlay(sampleRate, bufferSize);
        }
        
        // Reset quality monitoring
        resetQualityMonitoring();
        
        DBG("INFO: Audio device started - " << sampleRate << "Hz, " << bufferSize << " samples");
    }
}

void AudioEngine::audioDeviceStopped()
{
    if (masterGraph) {
        masterGraph->releaseResources();
    }
    
    DBG("INFO: Audio device stopped");
}

//==============================================================================
// ChangeListener Implementation
//==============================================================================

void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Handle audio device changes
    if (source == deviceManager.get()) {
        DBG("INFO: Audio device configuration changed");
        
        if (auto* device = deviceManager->getCurrentAudioDevice()) {
            currentSampleRate.store(device->getCurrentSampleRate());
            currentBufferSize.store(device->getCurrentBufferSizeSamples());
        }
    }
}

//==============================================================================
// MIDI Processing
//==============================================================================

void AudioEngine::processMidiEvents(juce::MidiBuffer& midiBuffer, int numSamples)
{
    if (!midiInputEnabled.load()) return;
    
    // Clear any existing MIDI data
    midiBuffer.clear();
    
    // TODO: Collect MIDI events from active players when Player interface is updated
    // For now, this is a placeholder
    
    // Process keyboard state if we have MIDI input
    keyboardState.processNextMidiBuffer(midiBuffer, 0, numSamples, true);
}

void AudioEngine::setDoublePrecisionProcessing(bool doublePrecision)
{
    // For backward compatibility - the new AudioEngine doesn't currently support
    // changing precision dynamically, but we can acknowledge the setting
    DBG("INFO: Double precision processing " << (doublePrecision ? "enabled" : "disabled") << " (compatibility mode)");
}

void AudioEngine::unregisterProcessor(juce::AudioProcessor* processor)
{
    // For backward compatibility - no-op since we handle this at Player level
    juce::ignoreUnused(processor);
    DBG("INFO: unregisterProcessor called (compatibility mode - no action taken)");
}

void AudioEngine::processMidiKeyboardState(juce::MidiKeyboardState& keyboardState)
{
    // For backward compatibility - route MIDI messages to all active players
    juce::MidiBuffer midiBuffer;
    keyboardState.processNextMidiBuffer(midiBuffer, 0, getBufferSize(), true);
    
    // Send each MIDI message to all active players
    for (const auto metadata : midiBuffer) {
        sendMidiMessage(metadata.getMessage());
    }
}

} // namespace covertracks