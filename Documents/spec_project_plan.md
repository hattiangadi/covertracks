# AI Jam Companion: System Specification and Project Plan

## 1. System Overview

The AI Jam Companion is a music creation application that enables users to create AI musicians that can perform together with humans. The system is built on a two-layer architecture:

1. **Player Layer**: Handles real-time audio processing through plugin chains
2. **Musician Layer**: Provides AI-driven musical decision making

Users configure players (instruments with effects), assign them to AI musicians with specific styles, and then create performances where these musicians collaborate.

## 2. Core Functionality

### 2.1 Player Configuration
- Create instrument configurations using audio plugins
- Save and load player templates
- Test sound output with MIDI keyboard input
- Define player capabilities for musician discovery

### 2.2 Musician Configuration
- Create AI musicians with specific musical styles
- Assign players to musicians
- Set musical personality traits and preferences
- Enable musicians to learn player capabilities

### 2.3 Performance Setup
- Combine multiple musicians into ensembles
- Define song structure, key, tempo, and other parameters
- Configure human integration points
- Save/load performance configurations

### 2.4 Live Performance
- Real-time AI-driven musical performance
- Interactive control during playback
- Recording and export capabilities
- Visualization of musical decisions

## 3. Technical Architecture

### 3.1 Player Layer
- **Plugin Graph**: Based on JUCE AudioPluginHost
  - Plugin loading and connection management
  - Parameter control and automation
  - State saving/loading
- **JSON Interface**: Standardized communication protocol
  - Command structure for musical instructions
  - Capability reporting
  - Performance feedback

### 3.2 Musician Layer
- **Capability Discovery**: Learn available sounds and techniques
- **Musical Planning**: Generate performance plans using LLM
- **Real-time Decision Making**: Adapt to musical context during performance
- **Inter-musician Communication**: Coordinate between multiple musicians

### 3.3 User Interface
- **Dashboard**: Main navigation hub
- **Player Editor**: Graph-based plugin configuration
- **Musician Editor**: Style and personality configuration
- **Ensemble Builder**: Performance setup and structure definition
- **Performance View**: Live visualization and control

## 4. Project Plan

### Phase 1: Foundation (Weeks 1-4)
- Reorganize UI flow from AudioPluginHost base
- Implement player template system with saving/loading
- Create basic dashboard and navigation
- Define JSON interface specification for musician-player communication

### Phase 2: Musician Layer Development (Weeks 5-8)
- Implement capability discovery mechanism
- Create musician configuration UI
- Develop LLM integration for musical planning
- Build basic musician-player communication

### Phase 3: Ensemble & Performance (Weeks 9-12)
- Create ensemble builder UI
- Implement performance coordination system
- Develop real-time control mechanisms
- Build recording and export capabilities

### Phase 4: Polish & Integration (Weeks 13-16)
- Implement performance visualization
- Add human integration capabilities
- Complete UI refinements and user experience improvements
- Conduct testing with musicians and composers
- Prepare for initial release

## 5. Technical Specifications

### 5.1 Player Template Format
```json
{
  "name": "Jazz Piano",
  "category": "Keyboard",
  "description": "Warm electric piano with reverb",
  "capabilities": {
    "pitchRange": { "low": 36, "high": 96 },
    "techniques": ["staccato", "legato", "chord"],
    "responsiveness": 0.9
  },
  "pluginGraph": {
    // Existing PluginGraph serialization format
  }
}
```

### 5.2 Musician-Player Communication Protocol
```json
// Command from Musician to Player
{
  "command": "playPattern",
  "patternId": "pattern_123",
  "events": [
    {"type": "noteOn", "time": 0.0, "note": 60, "velocity": 100},
    {"type": "noteOff", "time": 0.5, "note": 60},
    // Additional notes...
  ],
  "parameters": {
    "expression": 0.7,
    "articulation": "staccato"
  }
}

// Response from Player to Musician
{
  "status": "complete",
  "patternId": "pattern_123",
  "actualDuration": 2.35
}
```

### 5.3 LLM Integration for Musical Planning
- Use prompt engineering to create musical plans
- Send capability information to guide appropriate instrument use
- Request interpretations of musical styles and human playing
- Generate both high-level structure and detailed patterns

## 6. User Interface Mockups

### 6.1 Main Dashboard
```
+---------------------------------------+
|  AI JAM COMPANION                     |
+---------------------------------------+
|                                       |
|  [My Players]    [My Musicians]       |
|                                       |
|  [My Performances]                    |
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
|                                       |
|  +--------+      +--------+           |
|  | Piano  |----->| Reverb |---->     |
|  +--------+      +--------+           |
|                                       |
|  [Add Plugin]  [Save]  [Test]         |
|                                       |
+---------------------------------------+
|  MIDI Keyboard                        |
|  [C][D][E][F][G][A][B][C]             |
+---------------------------------------+
```

### 6.3 Musician Editor
```
+---------------------------------------+
|  MUSICIAN EDITOR                     X|
+---------------------------------------+
|  Name: Miles                          |
|  Player: Jazz Trumpet                 |
|                                       |
|  Style:                               |
|  [ ] Classical  [X] Jazz  [ ] Rock    |
|                                       |
|  Personality:                         |
|  "Like Miles Davis, with modern       |
|  influences and experimental edges"   |
|                                       |
|  [Save Musician]                      |
+---------------------------------------+
```

### 6.4 Performance View
```
+---------------------------------------+
|  PERFORMANCE: Late Night Session     X|
+---------------------------------------+
|                                       |
|  [Miles]  [Ella]  [Bill]  [You]       |
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

## 7. Implementation Details

### 7.1 Core Classes

- **AppController**: Main application controller
- **PlayerManager**: Manages player templates and instances
- **MusicianManager**: Handles musician creation and configuration
- **PerformanceCoordinator**: Coordinates musicians during performance
- **JSONInterface**: Handles musician-player communication
- **CapabilityDiscovery**: Analyzes player capabilities for musicians
- **LLMIntegration**: Manages interaction with language models

### 7.2 Data Flow

1. User creates player configurations through the graph editor
2. Player capabilities are analyzed and stored
3. User creates musicians and assigns players
4. Musician layer uses LLM to generate musical plans based on capabilities
5. During performance, musician layer sends JSON commands to player layer
6. Player layer executes audio processing and returns feedback
7. UI visualizes the performance and provides control points

### 7.3 Dependencies

- JUCE Framework (existing code base)
- External audio plugins (VST, AU, etc.)
- LLM API for musician intelligence
- Audio I/O management

## 8. Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Plugin compatibility issues | High | Medium | Implement robust error handling and alternative plugin paths |
| LLM latency affecting real-time performance | High | Medium | Develop caching and pre-planning strategies |
| Complex musician-player communication | Medium | High | Create thorough testing framework and fallback options |
| User confusion with complex UI | Medium | Medium | Develop progressive disclosure UI and strong onboarding |
| Performance issues with multiple AI musicians | High | Medium | Implement resource throttling and quality settings |

## 9. Future Extensions

- **Style Library**: Pre-defined musical styles for quick musician creation
- **Performance Sharing**: Export and share performances with community
- **Advanced Human Integration**: Real-time adaptation to human performance
- **Visual Music Notation**: Display and edit musical patterns visually
- **ML-Based Capability Learning**: Learn player capabilities through machine learning rather than manual description

This specification and project plan provides a roadmap for implementing the AI Jam Companion, building on the existing JUCE AudioPluginHost code while adding the musician layer and restructuring the UI to focus on the musical experience.