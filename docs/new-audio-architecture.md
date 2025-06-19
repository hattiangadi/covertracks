# New AudioEngine Architecture Design

## Goals & Requirements

### Primary Goals
1. **🔥 CRITICAL**: Fix performance mode clicking/popping
2. **Centralize audio resources** - Single AudioEngine shared by all Players  
3. **Simplify MIDI routing** - Centralized distribution to active Players
4. **Eliminate graph duplication** - Single AudioProcessorGraph per Instrument
5. **Clear resource ownership** - Unambiguous ownership contracts

### Performance Requirements
- **No audio dropouts** during multi-player coordination
- **Sub-10ms latency** for real-time audio processing
- **Thread-safe** Player registration/deactivation
- **Graceful degradation** under heavy CPU load

## New Architecture Overview

### Single AudioEngine Pattern
```
Application
├── AudioEngine (SINGLETON)
│   ├── AudioDeviceManager (owned)
│   ├── AudioPluginFormatManager (owned) 
│   ├── KnownPluginList (owned)
│   ├── MasterMixerNode (owned)
│   └── ActivePlayerRegistry (owned)
└── PlayerManager
    └── Players[] (reference AudioEngine)
        └── Instrument (single AudioProcessorGraph)
```

### Key Changes from Current Architecture

#### 1. Centralized Resource Ownership ✅
```cpp
class AudioEngine {
private:
    // AudioEngine owns ALL shared audio resources
    std::unique_ptr<AudioDeviceManager> deviceManager;
    std::unique_ptr<AudioPluginFormatManager> formatManager; 
    std::unique_ptr<KnownPluginList> knownPluginList;
    
    // Master audio processing graph
    std::unique_ptr<AudioProcessorGraph> masterGraph;
    
    // Player management
    std::map<Player*, PlayerAudioNode*> activePlayers;
    std::mutex playerRegistryMutex;
};
```

#### 2. Simplified Player Integration ✅
```cpp
class Player {
private:
    AudioEngine* audioEngine; // Reference only (not owned)
    std::unique_ptr<Instrument> instrument;
    
public:
    // Clean lifecycle
    bool connectToAudioEngine(AudioEngine* engine);
    void disconnectFromAudioEngine();
    bool startPlayback();
    void stopPlayback();
};
```

#### 3. Eliminated Graph Duplication ✅
```cpp
class Instrument {
private:
    // ONLY ONE GRAPH - no more duplication
    std::unique_ptr<AudioProcessorGraph> processorGraph;
    // Remove: std::unique_ptr<PluginGraph> pluginGraph;
    
public:
    AudioProcessorGraph* getProcessorGraph() { return processorGraph.get(); }
};
```

## Detailed Component Design

### AudioEngine - Centralized Audio Management

#### Core Responsibilities
1. **Device Management** - Audio device configuration and monitoring
2. **Player Coordination** - Thread-safe Player registration and routing
3. **MIDI Distribution** - Centralized MIDI input routing to active Players
4. **Master Mixing** - Audio output mixing and quality control
5. **Resource Lifecycle** - Plugin format discovery and management

#### Public Interface
```cpp
class AudioEngine {
public:
    // Singleton access
    static AudioEngine& getInstance();
    
    // Player lifecycle (thread-safe)
    bool registerPlayer(Player* player);
    void unregisterPlayer(Player* player);
    bool activatePlayer(Player* player);  // Ready for audio
    void deactivatePlayer(Player* player); // Pause audio processing
    
    // MIDI routing
    void routeMidiToPlayer(Player* player, const MidiMessage& message);
    void routeMidiToAllActivePlayers(const MidiMessage& message);
    
    // Device management
    AudioDeviceManager& getDeviceManager();
    AudioPluginFormatManager& getFormatManager();
    KnownPluginList& getKnownPluginList();
    
    // Quality monitoring  
    bool isAudioQualityGood() const;
    double getCpuUsage() const;
    int getBufferUnderruns() const;
};
```

#### Internal Architecture
```cpp
class AudioEngine {
private:
    // Master audio processing graph
    struct MasterAudioGraph {
        std::unique_ptr<AudioProcessorGraph> graph;
        AudioProcessorGraph::Node::Ptr midiInputNode;
        AudioProcessorGraph::Node::Ptr mixerNode;
        AudioProcessorGraph::Node::Ptr audioOutputNode;
    } masterGraph;
    
    // Player audio nodes in master graph
    struct PlayerAudioNode {
        Player* player;
        AudioProcessorGraph::Node::Ptr graphNode;
        std::atomic<bool> isActive{false};
        std::atomic<bool> isPlaying{false};
    };
    
    std::map<Player*, std::unique_ptr<PlayerAudioNode>> activePlayerNodes;
    std::mutex playerNodesMutex;
    
    // Quality monitoring
    std::atomic<int> bufferUnderruns{0};
    std::atomic<double> cpuUsage{0.0};
    
    // MIDI routing
    void distributeMidiMessage(const MidiMessage& message);
};
```

### Player - Simplified Audio Integration

#### New Lifecycle
```cpp
enum class PlayerAudioState {
    Disconnected,    // No AudioEngine connection
    Connected,       // Connected but not processing audio
    Active,          // Processing audio but not playing
    Playing          // Actively playing audio
};

class Player {
private:
    PlayerAudioState audioState = PlayerAudioState::Disconnected;
    AudioEngine* audioEngine = nullptr;
    
public:
    // Simplified connection lifecycle
    bool connectToAudioEngine(AudioEngine* engine) {
        if (audioEngine != nullptr) return false;
        
        audioEngine = engine;
        bool success = audioEngine->registerPlayer(this);
        if (success) {
            audioState = PlayerAudioState::Connected;
        }
        return success;
    }
    
    bool prepareForPlayback() {
        if (audioState < PlayerAudioState::Connected) return false;
        
        // Prepare instrument
        if (!instrument->prepareForPlayback()) return false;
        
        // Activate in AudioEngine
        bool success = audioEngine->activatePlayer(this);
        if (success) {
            audioState = PlayerAudioState::Active;
        }
        return success;
    }
    
    bool startPlayback() {
        if (audioState < PlayerAudioState::Active) {
            if (!prepareForPlayback()) return false;
        }
        
        // Start instrument playback
        bool success = instrument->startPlayback();
        if (success) {
            audioState = PlayerAudioState::Playing;
        }
        return success;
    }
};
```

### Instrument - Single Graph Architecture

#### Simplified Design
```cpp
class Instrument {
private:
    // ONLY ONE GRAPH
    std::unique_ptr<AudioProcessorGraph> processorGraph;
    
    // Audio state
    std::atomic<bool> isProcessingAudio{false};
    std::atomic<bool> isPlayingPattern{false};
    
    // MIDI playback
    std::unique_ptr<MidiMessageSequence> currentSequence;
    std::atomic<int> nextEventIndex{0};
    double currentPlaybackTime = 0.0;
    
public:
    // Simplified interface
    bool prepareForPlayback();
    bool startPlayback();
    void stopPlayback();
    
    // Graph access
    AudioProcessorGraph* getProcessorGraph() { return processorGraph.get(); }
    
    // State queries
    bool isReady() const;
    bool isPlaying() const;
};
```

## Clicking Fix Strategy

### Root Cause Analysis
Based on current architecture, clicking likely caused by:

1. **Buffer Underruns** - Multiple AudioEngines competing for audio device
2. **MIDI Timing Issues** - Uncoordinated MIDI event scheduling  
3. **Thread Blocking** - Audio thread blocked by LLM/UI operations
4. **Graph Switching** - Dynamic plugin graph changes during playback

### Specific Fixes

#### 1. Centralized Audio Device Access ✅
```cpp
// OLD: Multiple AudioEngines
Player1 -> AudioEngine1 -> AudioDevice  // CONFLICT!
Player2 -> AudioEngine2 -> AudioDevice  // CONFLICT!

// NEW: Single AudioEngine  
Player1 ┐
Player2 ├─ AudioEngine -> AudioDevice  // NO CONFLICT
Player3 ┘
```

#### 2. Coordinated MIDI Scheduling ✅
```cpp
class AudioEngine {
    void processMidiEvents(MidiBuffer& buffer, int numSamples) {
        // Collect MIDI from all active players
        for (auto& [player, node] : activePlayerNodes) {
            if (node->isPlaying) {
                player->addMidiEventsToBuffer(buffer, numSamples);
            }
        }
        
        // Process all MIDI in single coordinated batch
        masterGraph.graph->processBlock(audioBuffer, buffer);
    }
};
```

#### 3. Audio Thread Protection ✅
```cpp
class AudioEngine {
    // Ensure audio thread never blocks
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override {
        // Lock-free operations only
        juce::ScopedTryLock lock(playerNodesMutex);
        if (!lock.isLocked()) {
            // If can't get lock immediately, skip this block
            buffer.clear();
            return;
        }
        
        // Process audio with guaranteed no blocking
        masterGraph.graph->processBlock(buffer, midiMessages);
    }
};
```

#### 4. Stable Graph Configuration ✅
```cpp
class Instrument {
    // No dynamic graph changes during playback
    bool loadPluginGraph(const XmlElement& graphState) {
        if (isPlayingPattern) {
            // Queue graph changes for next stop
            pendingGraphState = std::make_unique<XmlElement>(graphState);
            return true;
        }
        
        // Apply immediately when not playing
        return applyGraphState(graphState);
    }
};
```

## Migration Strategy

### Phase 1: Create New AudioEngine (Test-Driven)
1. **Create AudioEngineTests** - Comprehensive test suite
2. **Implement new AudioEngine** - Pass all tests
3. **Add quality monitoring** - Buffer underrun detection, CPU monitoring

### Phase 2: Update Instrument (Remove Duplication)  
1. **Create InstrumentTests** - Test single graph functionality
2. **Remove PluginGraph** - Keep only AudioProcessorGraph
3. **Simplify state management** - Single graph, clear states

### Phase 3: Update Player Integration
1. **Create PlayerAudioTests** - Test new lifecycle
2. **Update Player class** - New AudioEngine integration
3. **Remove old AudioEngine per Player**

### Phase 4: Migrate PlayerManager
1. **Update PlayerManager** - Use singleton AudioEngine
2. **Test full integration** - Multi-player scenarios
3. **Validate audio quality** - No clicking, good performance

### Phase 5: Performance Testing & Validation
1. **Stress testing** - Multiple players, complex patterns
2. **Audio quality validation** - Automated clicking detection
3. **Performance benchmarking** - CPU usage, latency measurements

## Success Criteria

### Audio Quality ✅
- [ ] **No clicking/popping** in performance mode
- [ ] **< 10ms latency** for real-time audio
- [ ] **< 50% CPU usage** during complex multi-player scenarios
- [ ] **Zero buffer underruns** during normal operation

### Architecture Quality ✅  
- [ ] **Single AudioEngine** shared by all Players
- [ ] **Clear resource ownership** - No ambiguous references
- [ ] **Thread-safe operations** - All audio operations safe
- [ ] **Comprehensive test coverage** - All new components tested

### Compatibility ✅
- [ ] **All existing functionality** works unchanged
- [ ] **No user workflow disruption** - Same UI, same features
- [ ] **Performance improvements** - Better than before
- [ ] **Ready for Stage 3** - Clean foundation for component cleanup

This new architecture eliminates the fundamental causes of audio clicking while providing a much cleaner foundation for future development.