# Sprint 2 - Musician Integration: Detailed Plan

## Sprint Goal
Extend our player system to incorporate musician capabilities for AI-driven pattern generation, creating a unified Player architecture that handles both audio processing and musical intelligence.

## Key Deliverables
1. Updated Player class with integrated musician component
2. Instrument class for managing PluginGraph and MIDI playback
3. Musician class with LLM pattern generation
4. UI for configuring both instrument and musician aspects
5. Basic ensemble and performance capability

## Tasks Breakdown

### Day 1-2: Core Architecture Implementation
1. **Create Instrument Class**
   - Implement wrapper for PluginGraph that handles MIDI playback
   - Add pattern playback functionality
   - Create callback mechanism for pattern completion
   - Add serialization support

2. **Update Player Structure**
   - Extend Player class to incorporate Instrument and Musician
   - Implement serialization for the unified structure
   - Create methods for pattern generation and playback
   - Add combined operations (generateAndPlay)

### Day 3-4: Musician Implementation
3. **Implement Musician Class**
   - Core musician functionality for pattern generation
   - Style and personality configuration
   - Integration with LLM client
   - Pattern caching and optimization

4. **Build LLM Integration**
   - ClaudeClient implementation for API access
   - PatternProcessor for converting text to MIDI events
   - ShorthandProcessor for music notation
   - Prompt templates for different instruments

### Day 5-6: UI Implementation
5. **Update Player Editor UI**
   - Add tabbed interface (Sound/Style tabs)
   - Create musician configuration UI in Style tab
   - Add pattern generation and testing controls
   - Implement playback visualization

6. **Create Player Testing UI**
   - Add pattern generation testing
   - Implement playback controls
   - Create visualization of generated patterns
   - Add feedback mechanisms

### Day 7-8: Ensemble Framework
7. **Build Ensemble Class**
   - Create basic ensemble management
   - Implement player coordination
   - Add song structure handling
   - Create performance control methods

8. **Implement Performance UI (Prototype)**
   - Create simple ensemble builder UI
   - Implement basic song structure editor
   - Add playback controls for ensemble
   - Create performance visualization

## Detailed Class Structure

### Instrument Class
```cpp
class Instrument {
public:
    Instrument(const String& name);
    ~Instrument();
    
    // Plugin graph management
    bool initialize();
    bool loadPluginGraph(const XmlElement& graphState);
    XmlElement* getPluginGraphState();
    
    // MIDI playback
    bool playMIDIPattern(const json& pattern);
    void stopPlayback();
    
    // Accessors
    PluginGraph* getPluginGraph() { return pluginGraph.get(); }
    
    // Callbacks
    std::function<void()> onPatternFinished;
    
private:
    std::unique_ptr<PluginGraph> pluginGraph;
    std::unique_ptr<AudioDeviceManager> deviceManager;
    String name;
    
    // MIDI playback state
    bool isPlaying = false;
    std::unique_ptr<MidiMessageSequence> currentSequence;
    int nextEventIndex = 0;
    double currentTime = 0.0;
    
    // Audio processing
    void processMIDIEvents(MidiBuffer& midiBuffer, int numSamples);
    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill);
};
```

### Updated Player Class
```cpp
// In PlayerManager.h
struct Player {
    PlayerMetadata metadata;
    
    // Core components
    std::unique_ptr<Instrument> instrument;
    std::unique_ptr<Musician> musician;
    
    // JSON serialization
    var toVar() const;
    static std::unique_ptr<Player> fromVar(const var& v);
    
    // Pattern operations
    json generatePattern(const json& parameters);
    bool playPattern(const json& pattern);
    void stopPlayback();
    bool generateAndPlay(const json& parameters);
    
    // Component access
    Instrument* getInstrument() { return instrument.get(); }
    Musician* getMusician() { return musician.get(); }
};
```

### Musician Class
```cpp
class Musician {
public:
    Musician();
    ~Musician();
    
    // Configuration
    void setLLMClient(std::shared_ptr<LLMClient> client);
    void setStyle(const String& style);
    void setPersonality(const String& personality);
    
    // Pattern generation
    json generatePattern(const json& parameters);
    
    // Serialization
    var toVar() const;
    static std::unique_ptr<Musician> fromVar(const var& v);
    
private:
    std::shared_ptr<LLMClient> llmClient;
    String style;
    String personality;
    std::map<String, json> patternCache;
};
```

### LLM Client
```cpp
class LLMClient {
public:
    virtual ~LLMClient() = default;
    virtual json generateResponse(const std::string& prompt) = 0;
};

class ClaudeClient : public LLMClient {
public:
    ClaudeClient(const std::string& apiKey);
    json generateResponse(const std::string& prompt) override;
    
private:
    std::string apiKey;
    URL apiEndpoint;
};
```

### Pattern Processing
```cpp
class PatternProcessor {
public:
    static json parsePatternToEvents(const std::string& patternText, 
                                    const std::string& instrumentType);
};

class ShorthandProcessor {
public:
    static std::string createPrompt(const std::string& instrumentType,
                                   const json& parameters);
    static json shorthandToJson(const std::string& response,
                               const std::string& instrumentType);
};
```

### UI Extensions
```cpp
// Updated PlayerEditorComponent
class PlayerEditorComponent : public Component {
public:
    // Existing code...
    
    // UI components
    TabbedComponent tabs;
    
    // Sound tab (existing plugin editor)
    Component soundTab;
    
    // Style tab (musician configuration)
    Component styleTab;
    Label styleLabel;
    TextEditor styleEditor;
    Label personalityLabel;
    TextEditor personalityEditor;
    TextButton generateButton;
    TextButton playButton;
    
    // Methods
    void setupTabs();
    void handleGenerateClicked();
    void handlePlayClicked();
};
```

## Implementation Notes

### Shorthand Pattern Format
```
|pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
|pattern: syncopated, chord: Dm7| pattern: quarter, chords: G7, C7, F7, Bb7|
```

This pattern-based approach is:
- More token-efficient for LLM generation
- Musically meaningful and easier to read
- Allows musical thinking rather than MIDI event details
- Can be converted to MIDI events by the ShorthandProcessor

### Pattern Generation Prompt Format
```
Generate a {{instrument}} pattern in {{key}} at {{tempo}} BPM.

Style: {{style}}
Personality: {{personality}}

Create a pattern that sounds like {{description}}.

Format the pattern using:
|pattern: {{patternType}}, chord: {{chord}}|

Choose from these patterns:
- whole: One whole note chord
- half: Two half note chords
- quarter: Four quarter note chords
- syncopated: Quarter, half, quarter rhythm
- comping: Jazz comping with syncopated rhythm
- bossa: Bossa nova rhythm
```

This approach leverages our existing pattern templates from `PromptManager`, `PromptTemplates`, and `ClaudePatternTemplates` which are already designed to produce efficient, musically meaningful patterns.

### Extension Points for Future Sprints
1. **Advanced Ensemble Features**
   - Multiple musician coordination
   - Section-based arrangements
   - Dynamic response to human musicians

2. **Enhanced Musician Intelligence**
   - Learning from feedback
   - Style adaptation
   - Context-aware pattern generation

3. **Performance Visualization**
   - Real-time notation display
   - Visual feedback of musical patterns
   - Interactive control during playback

## Testing Strategy
1. Unit tests for each component
2. Integration tests for Player-Musician-Instrument interaction
3. End-to-end tests for pattern generation and playback
4. UI testing for configuration workflow

## Dependencies
1. JUCE Framework (existing)
2. LLM API access (Claude API key)
3. JUCE var-based JSON (replacing nlohmann::json)
4. Audio plugin support (VST, AU)

## Code Transition Strategy

### Converting from Separated to Unified Architecture

1. **Musician Integration**
   - Move core Musician functionality into the Player class
   - Adapt existing Musician methods to work within the Player context
   - Convert external JSON communication to internal function calls

2. **JSON Library Transition**
   - Replace nlohmann::json with JUCE's var-based approach
   - Convert JSON operations:
     ```cpp
     // Old (nlohmann::json)
     json object = {{"key", "value"}, {"number", 42}};
     std::string jsonString = object.dump();
     
     // New (JUCE var)
     DynamicObject* object = new DynamicObject();
     object->setProperty("key", "value");
     object->setProperty("number", 42);
     var jsonVar(object);
     String jsonString = JSON::toString(jsonVar);
     ```

3. **Existing Musician Code Reuse**
   - Keep core pattern generation logic from existing Musician class
   - Retain ShorthandProcessor and PatternProcessor logic
   - Adapt interfaces to work with the Player/Instrument classes

4. **Refactoring MIDIPlayer to Instrument**
   - Extract MIDI playback functionality from MIDIPlayer
   - Adapt it to work with PluginGraph instead of direct audio output
   - Migrate callback mechanisms for pattern completion

5. **JSONInterface Transition**
   - Remove external JSON communication layer
   - Convert to direct method calls between components
   - Maintain similar command structure internally for consistency

### Class Mapping from Old to New Architecture

| Old Architecture | New Architecture | Transition Notes |
|------------------|------------------|-----------------|
| MIDIPlayer | Instrument | Extract MIDI playback, adapt to PluginGraph |
| Musician | Player::musician | Move functionality into Player |
| JSONInterface | Internal method calls | Remove serialization overhead |
| PlayerController | Player management | Integrate with PlayerManager |
| ClaudeClient | Unchanged | Keep LLM integration mostly the same |
| ShorthandProcessor | Unchanged | Keep pattern processing logic |

## Definition of Done
- All components implemented with proper tests
- UI for configuring both instrument and musician aspects
- Ability to generate and play patterns through the unified Player
- Basic ensemble capabilities for coordinating multiple players
- Documentation of the architecture and usage examples