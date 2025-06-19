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
        
        // Enable MIDI input by default
        setMidiInputEnabled(true);
        
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
    std::cout << "🔧 registerPlayer() called for " << (player ? player->getMetadata().name : "null") << std::endl;
    
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    // Check if already registered
    if (playerNodes.find(player) != playerNodes.end()) {
        std::cout << "⚠️ Player " << player->getMetadata().name << " already registered" << std::endl;
        return false; // Already registered
    }
    
    // Create player audio node
    auto playerNode = std::make_unique<PlayerAudioNode>(player);
    playerNodes[player] = std::move(playerNode);
    
    std::cout << "✅ Player " << player->getMetadata().name << " registered with AudioEngine" << std::endl;
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
    std::cout << "🎯 activatePlayer() called for " << (player ? player->getMetadata().name : "null") << std::endl;
    
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    auto it = playerNodes.find(player);
    if (it == playerNodes.end()) {
        std::cout << "❌ Player " << player->getMetadata().name << " not registered, cannot activate" << std::endl;
        return false; // Not registered
    }
    
    auto* playerNode = it->second.get();
    if (playerNode->isActive.load()) {
        std::cout << "⚠️ Player " << player->getMetadata().name << " already active" << std::endl;
        return true; // Already active
    }
    
    // Set active first, then add to master graph
    playerNode->isActive.store(true);
    std::cout << "🔗 Adding player " << player->getMetadata().name << " to master graph..." << std::endl;
    if (addPlayerGraph(player)) {
        std::cout << "✅ Player " << player->getMetadata().name << " activated in AudioEngine" << std::endl;
        return true;
    }
    else
    {
        // Failed to add to graph, revert active state
        playerNode->isActive.store(false);
        std::cout << "❌ Failed to add player " << player->getMetadata().name << " to master graph" << std::endl;
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
// Player Graph Integration
//==============================================================================

bool AudioEngine::addPlayerGraph(Player* player)
{
    std::cout << "🔗 addPlayerGraph() called for " << (player ? player->getMetadata().name : "null") << std::endl;
    
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    // Check if player is registered and active
    auto it = playerNodes.find(player);
    if (it == playerNodes.end() || !it->second->isActive.load())
    {
        return false;
    }
    
    // Get the player's instrument and its processor graph
    auto* instrument = player->getInstrument();
    if (!instrument) return false;
    
    auto* processorGraph = instrument->getProcessorGraph();
    if (!processorGraph) return false;
    
    // Check if already integrated
    if (it->second->graphNode != nullptr) return true;
    
    try 
    {
        // Instead of copying the graph, we'll integrate by registering the player's processor directly
        // The player's graph will be processed independently but coordinated by AudioEngine
        
        // For now, we'll mark the graph as integrated conceptually
        // The actual audio processing will still go through the player's own graph
        // but MIDI routing and coordination will be handled by AudioEngine
        
        // Create a placeholder node to track integration status
        struct PlayerGraphWrapper : public juce::AudioProcessor 
        {
            Player* player;
            
            PlayerGraphWrapper(Player* p) : player(p) 
            {
                setPlayConfigDetails(0, 2, 44100, 512); // Stereo output
            }
            
            const juce::String getName() const override 
            { 
                return "Player: " + (player ? player->getMetadata().name : "Unknown");
            }
            
            void prepareToPlay(double, int) override {}
            void releaseResources() override {}
            void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override 
            {
                if (player) 
                {
                    // Debug: Check if wrapper is being called
                    if (midiMessages.getNumEvents() > 0 || player->isPlaying())
                    {
                        std::cout << "PlayerGraphWrapper: Processing " << midiMessages.getNumEvents() 
                                  << " MIDI events for player " << player->getMetadata().name 
                                  << " (playing: " << player->isPlaying() << ")" << std::endl;
                    }
                    
                    // Use the Player's AudioProcessor interface directly
                    player->processBlock(buffer, midiMessages);
                    
                    // Debug: Check what we got back
                    if (midiMessages.getNumEvents() > 0 || player->isPlaying())
                    {
                        std::cout << "PlayerGraphWrapper: After player->processBlock, buffer has " 
                                  << midiMessages.getNumEvents() << " MIDI events" << std::endl;
                    }
                }
            }
            
            double getTailLengthSeconds() const override { return 0.0; }
            bool acceptsMidi() const override { return true; }
            bool producesMidi() const override { return false; }
            
            juce::AudioProcessorEditor* createEditor() override { return nullptr; }
            bool hasEditor() const override { return false; }
            
            int getNumPrograms() override { return 1; }
            int getCurrentProgram() override { return 0; }
            void setCurrentProgram(int) override {}
            const juce::String getProgramName(int) override { return "Default"; }
            void changeProgramName(int, const juce::String&) override {}
            
            void getStateInformation(juce::MemoryBlock&) override {}
            void setStateInformation(const void*, int) override {}
        };
        
        auto wrapper = std::make_unique<PlayerGraphWrapper>(player);
        it->second->graphNode = masterGraph->addNode(std::move(wrapper));
        
        std::cout << "🔌 Created PlayerGraphWrapper for " << player->getMetadata().name 
                  << " (node ID: " << (it->second->graphNode ? it->second->graphNode->nodeID.uid : 0) << ")" << std::endl;
        
        if (it->second->graphNode != nullptr)
        {
            // Connect to MIDI input if available
            if (midiInputNode != nullptr)
            {
                bool midiConnected = masterGraph->addConnection({
                    { midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex },
                    { it->second->graphNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex }
                });
                std::cout << "🎹 MIDI connection to PlayerGraphWrapper: " << (midiConnected ? "SUCCESS" : "FAILED") << std::endl;
            }
            else
            {
                std::cout << "⚠️ No MIDI input node available - PlayerGraphWrapper won't receive MIDI!" << std::endl;
            }
            
            // Connect audio output to master audio output
            if (audioOutputNode != nullptr)
            {
                // Connect stereo audio output
                masterGraph->addConnection({
                    { it->second->graphNode->nodeID, 0 },
                    { audioOutputNode->nodeID, 0 }
                });
                masterGraph->addConnection({
                    { it->second->graphNode->nodeID, 1 },
                    { audioOutputNode->nodeID, 1 }
                });
            }
            
            return true;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error adding player graph: " << e.what() << std::endl;
    }
    
    return false;
}

void AudioEngine::removePlayerGraph(Player* player)
{
    if (!player) return;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    auto it = playerNodes.find(player);
    if (it != playerNodes.end() && it->second->graphNode != nullptr)
    {
        // Remove the player's graph node from master graph
        masterGraph->removeNode(it->second->graphNode->nodeID);
        it->second->graphNode = nullptr;
    }
}

bool AudioEngine::isPlayerGraphIntegrated(Player* player) const
{
    if (!player) return false;
    
    juce::ScopedLock lock(playerRegistryMutex);
    
    auto it = playerNodes.find(player);
    return (it != playerNodes.end()) && (it->second->graphNode != nullptr);
}

//==============================================================================
// Audio Graph Management
//==============================================================================

bool AudioEngine::addPlayerToMasterGraph(PlayerAudioNode* playerNode)
{
    std::cout << "🔧 addPlayerToMasterGraph() called" << std::endl;
    
    if (!playerNode || !masterGraph) 
    {
        std::cout << "❌ addPlayerToMasterGraph failed: playerNode=" << (playerNode ? "valid" : "null") 
                  << ", masterGraph=" << (masterGraph ? "valid" : "null") << std::endl;
        return false;
    }
    
    try {
        // Player graph integration is now handled by addPlayerGraph() method
        // This method (addPlayerToMasterGraph) is now primarily for internal use
        std::cout << "✅ Player added to master audio graph" << std::endl;
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
    if (it != playerNodes.end() && it->second->isActive.load()) 
    {
        // Route MIDI message to player's callback if available
        if (player->hasMidiCallback())
        {
            player->processMidiMessage(message);
        }
        else
        {
            // Fallback: could add direct instrument MIDI routing here if needed
            // The callback system is the primary mechanism for MIDI routing
            juce::ignoreUnused(player);
        }
    }
}

void AudioEngine::routeMidiToAllActivePlayers(const juce::MidiMessage& message)
{
    juce::ScopedLock lock(playerRegistryMutex);
    
    for (auto& [player, node] : playerNodes) {
        if (node->isActive.load()) {
            // Route MIDI message to each active player
            if (player->hasMidiCallback())
            {
                player->processMidiMessage(message);
            }
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
    // Add MIDI message to the buffer for master graph processing
    if (message.isNoteOn() || message.isNoteOff())
    {
        juce::ScopedLock lock(midiBufferMutex);
        // Add with timestamp 0 for immediate processing
        currentMidiBuffer.addEvent(message, 0);
        
        std::cout << "🎹 AudioEngine::sendMidiMessage: Added " 
                  << (message.isNoteOn() ? "note ON" : "note OFF") 
                  << " to MIDI buffer - Note: " << message.getNoteNumber() 
                  << ", MIDI enabled: " << midiInputEnabled.load() << std::endl;
    }
    
    // Also route to players via callback system (for backward compatibility)
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
        
        // Debug: Check active players and master graph processing
        if (midiBuffer.getNumEvents() > 0)
        {
            int activePlayerCount = 0;
            for (auto& [player, node] : playerNodes) {
                if (node->isActive.load()) activePlayerCount++;
            }
            std::cout << "🎛️ Master graph processing " << midiBuffer.getNumEvents() 
                      << " MIDI events with " << activePlayerCount << " active players" << std::endl;
        }
        
        // Process master graph
        masterGraph->processBlock(audioBuffer, midiBuffer);
        
        // Process edit mode processor if registered (for edit mode audio)
        {
            juce::ScopedLock lock(editModeProcessorMutex);
            if (editModeProcessor)
            {
                // Create a separate MIDI buffer for edit mode processing
                juce::MidiBuffer editMidiBuffer;
                
                // Copy the collected MIDI messages for edit mode processing
                {
                    juce::ScopedLock midiLock(midiBufferMutex);
                    if (currentMidiBuffer.getNumEvents() > 0)
                    {
                        editMidiBuffer.addEvents(currentMidiBuffer, 0, numSamples, 0);
                    }
                }
                
                // Create a temporary audio buffer for edit mode
                juce::AudioBuffer<float> editBuffer(audioBuffer.getNumChannels(), numSamples);
                editBuffer.clear();
                
                // Process edit mode
                editModeProcessor->processBlock(editBuffer, editMidiBuffer);
                
                // Mix edit mode audio with master graph output
                for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
                {
                    if (channel < editBuffer.getNumChannels())
                    {
                        audioBuffer.addFrom(channel, 0, editBuffer, channel, 0, numSamples);
                    }
                }
                
                // Debug: Check if edit mode generated audio
                bool hasEditAudio = false;
                for (int channel = 0; channel < editBuffer.getNumChannels(); ++channel)
                {
                    auto* channelData = editBuffer.getReadPointer(channel);
                    for (int sample = 0; sample < numSamples; ++sample)
                    {
                        if (std::abs(channelData[sample]) > 0.001f)
                        {
                            hasEditAudio = true;
                            break;
                        }
                    }
                    if (hasEditAudio) break;
                }
                
                if (hasEditAudio)
                {
                    std::cout << "🔊 Edit mode audio mixed into output!" << std::endl;
                }
            }
        }
        
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
    if (!midiInputEnabled.load()) {
        static bool warnedOnce = false;
        if (!warnedOnce) {
            std::cout << "⚠️ AudioEngine::processMidiEvents: MIDI input disabled - skipping" << std::endl;
            warnedOnce = true;
        }
        return;
    }
    
    // Clear any existing MIDI data
    midiBuffer.clear();
    
    // Copy MIDI events from the current buffer (thread-safe)
    {
        juce::ScopedLock lock(midiBufferMutex);
        if (currentMidiBuffer.getNumEvents() > 0)
        {
            // Copy all events to the output buffer
            midiBuffer.addEvents(currentMidiBuffer, 0, numSamples, 0);
            // Clear the current buffer after copying
            currentMidiBuffer.clear();
        }
    }
    
    // Debug: Show MIDI events being processed by master graph
    if (midiBuffer.getNumEvents() > 0)
    {
        std::cout << "🎹 AudioEngine::processMidiEvents: Processing " << midiBuffer.getNumEvents() 
                  << " MIDI events in master graph" << std::endl;
        
        // Show details of each MIDI event
        for (const auto metadata : midiBuffer)
        {
            auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                std::cout << "   📝 Note ON: " << message.getNoteNumber() 
                          << ", Velocity: " << message.getVelocity() 
                          << ", Channel: " << message.getChannel() << std::endl;
            }
            else if (message.isNoteOff())
            {
                std::cout << "   📝 Note OFF: " << message.getNoteNumber() 
                          << ", Channel: " << message.getChannel() << std::endl;
            }
        }
    }
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

//==============================================================================
// Edit Mode Support
//==============================================================================

void AudioEngine::registerEditModeProcessor(juce::AudioProcessor* processor)
{
    juce::ScopedLock lock(editModeProcessorMutex);
    
    if (editModeProcessor != nullptr)
    {
        std::cout << "⚠️ Edit mode processor already registered, replacing..." << std::endl;
    }
    
    editModeProcessor = processor;
    
    if (processor)
    {
        // Prepare the processor with current audio settings
        double sampleRate = getSampleRate();
        int blockSize = getBufferSize();
        
        processor->prepareToPlay(sampleRate, blockSize);
        
        std::cout << "✅ Edit mode processor registered and prepared (SR: " << sampleRate 
                  << ", BS: " << blockSize << ")" << std::endl;
    }
}

void AudioEngine::unregisterEditModeProcessor()
{
    juce::ScopedLock lock(editModeProcessorMutex);
    
    if (editModeProcessor)
    {
        editModeProcessor->releaseResources();
        editModeProcessor = nullptr;
        
        std::cout << "✅ Edit mode processor unregistered" << std::endl;
    }
}

} // namespace covertracks