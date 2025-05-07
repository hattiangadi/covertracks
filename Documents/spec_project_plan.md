# AI Jam Companion: Updated System Specification

## 1. System Overview

The AI Jam Companion is a music creation application that enables users to create AI musicians that can perform together with humans. The system is built on a unified architecture that combines audio processing and AI-driven musical intelligence.

Users configure Players (instruments with associated musician capabilities), assign them musical styles and personalities, and then create performances where these Players collaborate.

## 2. Core Functionality

### 2.1 Player Configuration
- Create instrument configurations using audio plugins
- Configure musical styles and personalities
- Save and load player templates
- Test sound output with pattern generation
- Define player capabilities for performance

### 2.2 Ensemble Setup
- Combine multiple players into ensembles
- Define song structure, key, tempo, and other parameters
- Configure human integration points
- Save/load ensemble configurations

### 2.3 Live Performance
- Real-time AI-driven musical performance
- Interactive control during playback
- Recording and export capabilities
- Visualization of musical decisions

## 3. Technical Architecture

### 3.1 Unified Player Architecture
- **Player**: Main class that combines instrument and musician functionality
  - Plugin management and audio processing
  - Pattern generation and musical intelligence
  - State saving/loading
- **Instrument**: Handles audio plugin chain and MIDI playback
- **Musician**: Handles LLM-based pattern generation
- **PluginGraph**: Manages audio plugins and connections

### 3.2 User Interface
- **Dashboard**: Main navigation hub with sections for
  - My Players: Library of configured players
  - My Performances: Saved performance configurations
  - Quick Start: Templates and guided setup
  - Settings: Application configuration
- **Player Editor**: Tabbed interface with two main areas:
  - Sound Tab: Graph-based plugin configuration
  - Style Tab: Musician style and personality configuration
- **Ensemble Builder**: Performance setup and structure definition
- **Performance View**: Live visualization and control

## 4. Component Details

### 4.1 Player Class
```cpp
struct Player {
    PlayerMetadata metadata;
    
    // Core components
    std::unique_ptr<Instrument> instrument;
    std::unique_ptr<Musician> musician;
    
    // Pattern operations
    var generatePattern(const var& parameters);
    bool playPattern(const var& pattern);
    void stopPlayback();
    bool generateAndPlay(const var& parameters);
    
    // Component access
    Instrument* getInstrument() { return instrument.get(); }
    Musician* getMusician() { return musician.get(); }
};
```

### 4.2 Instrument Class
```cpp
class Instrument {
public:
    // Plugin graph management
    bool initialize();
    bool loadPluginGraph(const XmlElement& graphState);
    XmlElement* getPluginGraphState();
    
    // MIDI playback
    bool playMIDIPattern(const var& pattern);
    void stopPlayback();
    
    // Component access
    PluginGraph* getPluginGraph() { return pluginGraph.get(); }
    
private:
    std::unique_ptr<PluginGraph> pluginGraph;
    std::unique_ptr<AudioDeviceManager> deviceManager;
};
```

### 4.3 Musician Class
```cpp
class Musician {
public:
    // Configuration
    void setLLMClient(std::shared_ptr<LLMClient> client);
    void setStyle(const String& style);
    void setPersonality(const String& personality);
    
    // Pattern generation
    var generatePattern(const var& parameters);
    
private:
    std::shared_ptr<LLMClient> llmClient;
    String style;
    String personality;
};
```

### 4.4 Player Editor UI
The Player Editor will use a tabbed interface following best practices:
- Horizontal tab placement above content
- Clear visual distinction for the selected tab
- Short, descriptive labels
- Two primary tabs: "Sound" and "Style"

```
+---------------------------------------+
|  PLAYER EDITOR: Jazz Piano           X|
+---------------------------------------+
|  [Sound] [Style]                      |
+---------------------------------------+
|                                       |
|  // Sound Tab Content                 |
|  +--------+      +--------+           |
|  | Piano  |----->| Reverb |---->     |
|  +--------+      +--------+           |
|                                       |
|  // Style Tab Content                 |
|  Style: [Jazz___________________]     |
|  Personality: [Expressive, bluesy     |
|                with modern influences]|
|                                       |
|  [Generate Pattern] [Play]            |
|                                       |
+---------------------------------------+
```

## 5. Workflow

### 5.1 Player Creation
1. User creates a new Player from the dashboard
2. In the Sound tab, user configures the plugin chain
3. In the Style tab, user sets musical style and personality
4. User tests the Player by generating and playing patterns
5. User saves the Player to the library

### 5.2 Ensemble Creation
1. User creates a new Ensemble from the dashboard
2. User adds Players to the Ensemble
3. User configures musical parameters (key, tempo, etc.)
4. User defines song structure and section assignments
5. User saves the Ensemble configuration

### 5.3 Performance
1. User loads an Ensemble
2. User starts the performance
3. Players generate and play patterns according to the song structure
4. User can interact with the performance in real-time
5. User can record and export the performance

## 6. User Interface Mockups

### 6.1 Main Dashboard
```
+---------------------------------------+
|  AI JAM COMPANION                     |
+---------------------------------------+
|                                       |
|  [My Players]    [My Performances]    |
|                                       |
|  [Quick Start]   [Settings]           |
|                                       |
+---------------------------------------+
|  Status: Ready for jamming            |
+---------------------------------------+
```

### 6.2 Player Editor
```
+---------------------------------------+
|  PLAYER EDITOR: Jazz Piano           X|
+---------------------------------------+
|  [Sound] [Style]                      |
+---------------------------------------+
|                                       |
|  // Sound Tab Content                 |
|  +--------+      +--------+           |
|  | Piano  |----->| Reverb |---->     |
|  +--------+      +--------+           |
|                                       |
|  [Add Plugin]  [Save]  [Test]         |
|                                       |
+---------------------------------------+
```

### 6.3 Style Tab
```
+---------------------------------------+
|  PLAYER EDITOR: Jazz Piano           X|
+---------------------------------------+
|  [Sound] [Style]                      |
+---------------------------------------+
|                                       |
|  Style:                               |
|  [ ] Classical  [X] Jazz  [ ] Rock    |
|                                       |
|  Personality:                         |
|  "Like Miles Davis, with modern       |
|  influences and experimental edges"   |
|                                       |
|  [Generate Pattern]  [Play]  [Save]   |
|                                       |
+---------------------------------------+
```

### 6.4 Performance View
```
+---------------------------------------+
|  PERFORMANCE: Late Night Session     X|
+---------------------------------------+
|                                       |
|  [Jazz Piano]  [Bass]  [Drums]  [You] |
|                                       |
|  Structure:                           |
|  [Intro] [Verse] [Chorus] [Solo] [End]|
|            ^                          |
|                                       |
|  Controls:                            |
|  [Play/Pause] [Record] [Export]       |
|  Tempo: 120 BPM   Key: C minor        |
|                                       |
+---------------------------------------+
```

## 7. Implementation Strategy

### 7.1 Integration Approach
The Player will maintain separation of concerns internally by having distinct Instrument and Musician components, but present a unified interface to users. This gives us:

1. Clean separation of audio processing and pattern generation internally
2. Simple, intuitive user experience with a unified concept
3. Clear delegation of responsibilities between components
4. Flexibility for future expansion

### 7.2 Pattern Generation
The system will use LLM-based pattern generation with:
1. Shorthand pattern format for efficient token usage
2. Instrument-specific templates for appropriate patterns
3. Conversion from patterns to MIDI events
4. Support for different musical styles and techniques

Example pattern format:
```
|pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
```

### 7.3 UI Implementation
The tabbed interface for the Player Editor will follow these best practices:
1. Position tabs horizontally above the panel
2. Use clear visual indicators for the selected tab
3. Ensure tab labels are short and descriptive
4. Maintain consistent visual styling
5. Group related content logically within each tab

## 8. Future Extensions

- **Advanced Ensemble Features**: Sophisticated coordination between multiple players
- **Interactive Human Integration**: Real-time adaptation to human performance
- **Visual Pattern Editor**: UI for modifying generated patterns
- **Style Library**: Pre-defined musical styles and personalities
- **Learning Capabilities**: Players that adapt based on feedback and performance history

This updated specification provides a roadmap for implementing the AI Jam Companion with a unified Player architecture that maintains separation of concerns internally while presenting a cohesive interface to users.