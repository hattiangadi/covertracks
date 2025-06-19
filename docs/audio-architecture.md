# Audio Architecture - Current State

## Overview
The audio system is built around a centralized **AudioEngine** that manages shared resources, with **Players** containing **Instruments** for actual audio processing.

## Core Components

### AudioEngine (Singleton Pattern)
**Location**: `Source/Audio/AudioEngine.*`  
**Purpose**: Centralized audio resource management  
**Owns**: 
- `AudioDeviceManager` (audio device configuration)
- `mainProcessorGraph` (hosts all player processor graphs)

**References** (shared, not owned):
- `AudioPluginFormatManager` (plugin format discovery)  
- `KnownPluginList` (discovered plugins)

### Player → Instrument Chain
**Player** (`Source/Player/Core/Player.*`)
- Manages high-level music functionality (patterns, LLM integration)
- Contains one **Instrument** for audio processing
- Registers with AudioEngine when needing audio output

**Instrument** (`Source/Player/Core/Instrument.*`)  
- Wraps JUCE `AudioProcessorGraph` for actual audio processing
- Handles MIDI input/output and audio effects
- Manages plugin state persistence and restoration

### PluginGraph System
**PluginGraph** (`Source/Plugins/PluginGraph.*`)
- JUCE-based plugin hosting
- Manages plugin connections and state
- Handles plugin discovery and instantiation

## Current Resource Flow

```
PlayerManager
├── AudioEngine (owned)
│   ├── AudioDeviceManager (owned)
│   ├── mainProcessorGraph (owned)
│   ├── AudioPluginFormatManager (referenced)
│   └── KnownPluginList (referenced)
└── Players[] 
    └── Instrument
        ├── pluginGraph (PluginGraph)
        ├── processorGraph (AudioProcessorGraph)
        └── audioEngine (reference)
```

## Key Methods & Interfaces

### AudioEngine Registration
```cpp
// Player lifecycle
void activatePlayer(Player* player);
void deactivatePlayer(Player* player);

// Processor management  
bool registerProcessor(AudioProcessor* processor);
void unregisterProcessor(AudioProcessor* processor);

// MIDI routing
bool registerMidiInput(const String& deviceName, MidiInputCallback* callback);
```

### Instrument Audio Processing
```cpp
// Resource states
enum class ResourceState { Unloaded, EditReady, PlaybackReady };

// Lifecycle
void prepareForEditing();   // Load UI resources
bool prepareForPlayback();  // Load audio resources
void releaseResources();    // Free heavy resources

// Playback
bool playMIDIPattern(const var& pattern, double tempo);
bool playMIDISequence(const MidiMessageSequence& sequence, double tempo);
```

## Current Issues & Architecture Problems

### ❌ CRITICAL: Audio Quality Issues
- **Performance Mode Clicking**: Significant audio clicking/popping during multi-player playback
- **Likely causes**: Buffer underruns, MIDI timing conflicts, audio thread blocking
- **Must be fixed in Stage 2 AudioEngine refactor**

### ❌ Resource Ownership Confusion
- AudioEngine owns deviceManager but references formatManager/pluginList
- Unclear who owns what between PlayerManager and AudioEngine
- Players hold references to shared resources without clear contracts

### ❌ Complex Initialization Dependencies  
- Player needs AudioEngine reference during construction
- Instrument needs AudioEngine for playback preparation
- Circular dependency potential between components

### ❌ State Synchronization Issues
- Players can be in different resource states (Unloaded/EditReady/PlaybackReady)
- AudioEngine doesn't know about Player resource states
- No centralized coordination of audio resource allocation

### ❌ MIDI Routing Complexity
- MIDI input registered per-device per-callback
- No centralized MIDI routing or distribution
- Each Instrument manages its own MIDI input independently

### ❌ Plugin Graph Duplication
- Each Instrument has both `pluginGraph` (PluginGraph) and `processorGraph` (AudioProcessorGraph)
- State synchronization between these two graphs
- Complex state transfer during playback preparation

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