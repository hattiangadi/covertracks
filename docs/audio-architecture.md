# Audio Architecture - Player-as-AudioProcessor Design

## Overview
The audio system follows a clean **plugin host pattern** where the **AudioEngine** acts as a JUCE plugin host, and **Players** are first-class **AudioProcessor** instances. This provides proper audio threading, timing, and integration with JUCE's mature audio infrastructure.

## Core Components

### AudioEngine (Plugin Host)
**Location**: `Source/Audio/AudioEngine.*`  
**Purpose**: JUCE plugin host managing Player processors  
**Owns**: 
- `AudioDeviceManager` (audio device configuration)
- `AudioProcessorGraph` (hosts Player AudioProcessor instances)
- `AudioPluginFormatManager` (plugin format discovery)  
- `KnownPluginList` (discovered plugins)

**Key Methods**:
- `addPlayer(Player*)` - Adds Player as AudioProcessor to graph
- `removePlayer(Player*)` - Removes Player from graph
- Standard `AudioIODeviceCallback` for audio thread processing

### Player (AudioProcessor)
**Player** (`Source/Player/Core/Player.*`) **extends `juce::AudioProcessor`**
- **Primary Interface**: Standard JUCE AudioProcessor (processBlock, prepareToPlay, etc.)
- **Music Functionality**: Pattern management, MIDI sequencing, LLM integration
- **Audio Processing**: Real-time MIDI event processing in audio thread
- **Timing**: Sample-accurate timing based on audio buffers, not timers

**Key Methods**:
```cpp
// AudioProcessor interface
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiBuffer) override;
void prepareToPlay(double sampleRate, int blockSize) override;
void releaseResources() override;

// Player-specific interface  
bool loadChart(const var& chart);
void startPlayback();
void stopPlayback();
```

### Instrument (Internal Audio Graph)
**Instrument** (`Source/Player/Core/Instrument.*`)  
- **Purpose**: Internal audio processing graph for Player
- **Ownership**: Owned by Player, not exposed to AudioEngine
- **Processing**: Called from Player::processBlock() only

### PluginGraph System
**PluginGraph** (`Source/Plugins/PluginGraph.*`)
- JUCE-based plugin hosting
- Manages plugin connections and state
- Handles plugin discovery and instantiation

## New Resource Flow

```
PlayerManager
├── AudioEngine (owned) - Plugin Host
│   ├── AudioDeviceManager (owned)
│   ├── AudioProcessorGraph (owned) - hosts Players
│   ├── AudioPluginFormatManager (owned)
│   └── KnownPluginList (owned)
└── Players[] (AudioProcessor instances)
    └── Instrument (internal)
        └── processorGraph (AudioProcessorGraph)
```

## Audio Thread Processing Flow

```
AudioEngine::audioDeviceIOCallback()
└── masterGraph.processBlock()
    └── Player::processBlock() [audio thread]
        ├── Process MIDI events for current buffer
        ├── Update playback position (sample-accurate)
        └── Instrument::processBlock()
            └── Internal plugin graph processing
```

## Key Methods & Interfaces

### AudioEngine (Plugin Host)
```cpp
// Player management (simplified)
bool addPlayer(Player* player);        // Add as AudioProcessor to graph
void removePlayer(Player* player);     // Remove from graph

// Standard plugin host functionality
AudioProcessorGraph& getMainGraph();
AudioDeviceManager& getDeviceManager();
```

### Player (AudioProcessor)
```cpp
// AudioProcessor interface (JUCE standard)
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiBuffer) override;
void prepareToPlay(double sampleRate, int blockSize) override;
void releaseResources() override;
bool acceptsMidi() const override { return true; }
bool producesMidi() const override { return false; }

// Player-specific interface
bool loadChart(const var& chart);      // Load MIDI sequence for playback
void startPlayback();                  // Start real-time playback
void stopPlayback();                   // Stop playback
bool isPlaying() const;               // Playback state
```

### Instrument (Internal)
```cpp
// Called only from Player::processBlock()
void processAudioBlock(AudioBuffer<float>& buffer, MidiBuffer& midiBuffer);
void prepareForPlayback(double sampleRate, int blockSize);
void releaseResources();
```

## Benefits of New Architecture

### ✅ **Proper Audio Threading**
- All audio processing happens in audio thread via standard JUCE AudioProcessor interface
- No more timer-based processing or message thread audio work
- Sample-accurate timing based on actual audio buffers

### ✅ **Simplified Resource Management**
- Clear ownership: AudioEngine owns shared resources, Players are self-contained
- No complex PlayerGraphWrapper or manual routing
- Standard JUCE plugin host patterns

### ✅ **Better MIDI Timing**
- MIDI events processed in real-time during processBlock()
- Perfect synchronization with audio buffers
- No timing drift or latency issues

### ✅ **Easier Testing**
- Players can be tested as standard AudioProcessor instances
- Clean separation of concerns
- Mock audio devices and test frameworks work naturally

### ✅ **JUCE Integration**
- Leverage mature JUCE infrastructure
- Standard plugin development patterns
- Better debugging and profiling tools

## Migration Strategy

### Phase 1: Foundation
1. Update Player to extend AudioProcessor
2. Write comprehensive tests for Player AudioProcessor interface
3. Update AudioEngine to simple plugin host

### Phase 2: Implementation  
1. Move MIDI processing from timer to processBlock
2. Remove PlayerGraphWrapper complexity
3. Simplify Instrument to internal-only processing

### Phase 3: Testing & Validation
1. Update PlayerMode UI for testing
2. Validate audio quality and timing
3. Performance testing and optimization

## Testing Status
- **AudioEngine**: No dedicated tests found
- **Instrument**: No dedicated tests found  
- **PluginGraph**: No dedicated tests found
- **Player**: Basic tests exist but don't cover audio functionality

## Next Steps for Rearchitecture
1. **Clarify ownership** - Who owns what resources?
2. **Simplify MIDI routing** - Centralized MIDI distribution
3. **Reduce graph duplication** - Single graph per Instrument
4. **Add comprehensive tests** - Test audio functionality in isolation
5. **Document resource contracts** - Clear interfaces and responsibilities