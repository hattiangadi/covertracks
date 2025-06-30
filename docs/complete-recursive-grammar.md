# Complete Recursive Grammar with Full Expression Parity

## Enhanced MusicalContainer with Expression Features

Building on the recursive grammar foundation, we now add all missing expression features from music-language-design.md:

```bnf
MusicalContainer ::= {
  // Core recursive structure (unchanged)
  "events"?: EventList,
  "layers"?: ContainerMap,
  "variations"?: ContainerMap,
  "patterns"?: ContainerMap,
  "context"?: Context,
  "reference"?: String,
  
  // NEW: Expression features
  "shortcuts"?: ShorthandMap,
  "transforms"?: TransformList,
  "chordProgression"?: ChordProgression,
  "processorChains"?: ProcessorChainMap,
  "constraints"?: AIConstraints,
  "performance"?: PerformanceContext
}
```

## 1. Relative Harmony System

### Chord Degree Events
```json
{
  "chordProgression": [
    {"bar": 1, "chord": "Cmaj7", "duration": 4},
    {"bar": 5, "chord": "Am7", "duration": 4}
  ],
  "layers": {
    "bass": {
      "context": {"chordContext": "progression"},
      "events": [
        {"t": 1, "d": 1},     // Root of current chord
        {"t": 2, "d": 3},     // Third of current chord  
        {"t": 3, "d": 5},     // Fifth of current chord
        {"t": 4, "d": 7}      // Seventh of current chord
      ]
    },
    "piano": {
      "events": [
        {"t": 1, "d": [3, 7, 9]},    // Shell voicing + extension
        {"t": 3, "d": [1, 5]}        // Root-fifth punch
      ]
    }
  }
}
```

### Advanced Chord Definitions
```json
{
  "harmony": {
    "key": "C major",
    "chords": {
      "Cmaj9": {
        "root": "C",
        "quality": "maj9",
        "voicing": "rootless",
        "inversion": 1,
        "extensions": [9, 11, 13]
      },
      "Am7b5": {
        "root": "A", 
        "quality": "m7b5",
        "voicing": "close"
      }
    }
  }
}
```

## 2. Expression System

### Event-Level Expression
```json
{
  "events": [
    {
      "t": 1,
      "n": "C4",
      "expression": {
        "dynamics": "ff",
        "articulation": "staccato", 
        "accent": "strong",
        "vibrato": {"rate": 5.5, "depth": 0.2},
        "processors": {
          "distortion": {"drive": 0.7},
          "wah": {"frequency": "auto", "depth": 0.5},
          "reverb": {"room": 0.3, "decay": 2.0}
        },
        "automation": {
          "velocity": {"curve": "linear", "end": 0.3},
          "wah.frequency": {"curve": "sine", "rate": "1/8"}
        }
      }
    }
  ]
}
```

### Context-Level Expression
```json
{
  "context": {
    "dynamics": "mf",
    "articulation": "legato",
    "groove": {
      "swing": 0.67,
      "humanize": 0.05,
      "velocity": 0.1
    },
    "processorChain": "jazzOrgan"
  }
}
```

## 3. Shorthand System

### Pattern Shortcuts with Dot Notation
```json
{
  "shortcuts": {
    // Note shortcuts
    "k": "kick", "s": "snare", "h": "hihat", "c": "crash",
    "o": "openHat", "r": "ride", "t1": "tom1", "t2": "tom2",
    
    // Pattern shortcuts with dot notation
    "basic4": "k..s..k..s",           // . = quarter beat rest
    "16ths": "k.h.s.h.k.h.s.h",      // 16th note pattern
    "shuffle": "k..h.s..h.k..h.s..h", // Shuffle feel
    "fill": "s.s.t1.t2.t3.c",         // Tom fill + crash
    
    // Chord shortcuts
    "I": [1, 3, 5], 
    "ii7": [2, 4, 6, 1], 
    "V7": [5, 7, 2, 4],
    "imaj7": [1, 3, 5, 7],
    
    // Rhythm shortcuts
    "swing": {"groove": "shuffle", "amount": 0.67},
    "latin": {"groove": "bossa", "clave": "3-2"}
  },
  
  "events": [
    {"t": 0, "pattern": "basic4", "repeat": 3},
    {"t": 12, "pattern": "fill"},
    {"t": 16, "chord": "I", "duration": 4}
  ]
}
```

### Compact Context Notation
```json
{
  "events": [
    {"t": 0, "ctx": "C:4:mf"},        // Chord:Octave:Dynamics
    {"t": 1, "d": 1},                 // Uses context
    {"t": 2, "ctx": "Am:5:f"},        // Change context
    {"t": 2, "d": 3}                  // Uses new context
  ]
}
```

## 4. Transform System

### Composable Transforms
```json
{
  "base": [
    {"t": 1, "n": "kick"},
    {"t": 3, "n": "snare"}
  ],
  "transforms": [
    {"type": "repeat", "bars": 4},
    {"type": "swing", "amount": 0.67},
    {"type": "humanize", "timing": 0.05, "velocity": 0.1},
    {"type": "transpose", "semitones": 2},
    {"type": "timeStretch", "factor": 0.8},
    {"type": "reversePhrase", "probability": 0.2},
    {"type": "processors", "chain": "rockGuitar"}
  ]
}
```

### Groove Transforms
```json
{
  "groove": {
    "swing": 0.67,
    "shuffle": true,
    "humanize": {
      "timing": 0.05,     // ±5% timing variation
      "velocity": 0.1,    // ±10% velocity variation
      "microTiming": 0.02 // Subtle timing adjustments
    },
    "feel": "latin_montuno"
  }
}
```

## 5. MIDI Processor Integration

### Processor Chain Definitions
```json
{
  "processorChains": {
    "rockGuitar": [
      {"type": "overdrive", "gain": 0.8, "tone": 0.6},
      {"type": "eq", "low": 1.2, "mid": 0.8, "high": 1.1},
      {"type": "delay", "time": "1/8", "feedback": 0.3, "mix": 0.2},
      {"type": "reverb", "type": "hall", "size": 0.6, "decay": 3.0}
    ],
    "jazzOrgan": [
      {"type": "leslie", "speed": "slow", "depth": 0.7},
      {"type": "overdrive", "gain": 0.2, "tube": true},
      {"type": "eq", "low": 0.9, "mid": 1.1, "high": 0.8},
      {"type": "reverb", "type": "spring", "size": 0.4}
    ],
    "synthPad": [
      {"type": "filter", "cutoff": 800, "resonance": 0.3, "type": "lowpass"},
      {"type": "chorus", "rate": 0.5, "depth": 0.4, "mix": 0.3},
      {"type": "reverb", "type": "hall", "size": 0.8, "decay": 5.0}
    ]
  }
}
```

### Automation Curves
```json
{
  "automation": {
    "velocity": {
      "curve": "exponential",
      "points": [
        {"time": 0, "value": 0.3},
        {"time": 2, "value": 0.8},
        {"time": 4, "value": 0.1}
      ]
    },
    "filter.cutoff": {
      "curve": "sine",
      "rate": "1/4",
      "depth": 400,
      "center": 800
    }
  }
}
```

## 6. AI-Assisted Composition

### Constraint-Based Generation
```json
{
  "constraints": {
    "melody": {
      "range": ["C3", "C5"],
      "scale": "C major pentatonic",
      "contour": "arch",
      "rhythm": "syncopated",
      "density": 0.7,
      "stepwise": 0.8
    },
    "harmony": {
      "progression": "ii-V-I",
      "extensions": ["9", "11", "13"],
      "voicing": "rootless",
      "voice_leading": "smooth"
    },
    "generate": {
      "bars": 8,
      "style": "bebop",
      "complexity": 0.8,
      "variation": 0.3
    }
  }
}
```

### Performance Context
```json
{
  "performance": {
    "ensemble": "jazz_quartet",
    "interaction": {
      "listen_to": ["bass", "drums"],
      "respond_to": "dynamics",
      "trading": {"bars": 4, "with": "piano"},
      "solo_section": true
    },
    "style": {
      "genre": "bebop",
      "era": "1950s", 
      "reference": "Charlie Parker",
      "energy": 0.8,
      "complexity": 0.9
    }
  }
}
```

## Complete Event Structure

### Full Event with All Features
```json
{
  "t": 1.0,                    // Time
  "n": "C4",                   // Note (or MIDI number)
  "d": 1,                      // Chord degree (relative harmony)
  "v": 0.8,                    // Velocity
  "dur": 0.5,                  // Duration
  "c": "Cmaj7",                // Chord context override
  
  "expression": {
    "dynamics": "ff",
    "articulation": "marcato",
    "accent": "strong",
    "vibrato": {"rate": 6, "depth": 0.3},
    "slide": {"target": "D4", "time": 0.1}
  },
  
  "processors": {
    "wah": {"frequency": 800, "resonance": 0.5},
    "distortion": {"drive": 0.6}
  },
  
  "automation": {
    "velocity": {"curve": "linear", "end": 0.2, "time": 0.5},
    "wah.frequency": {"curve": "sine", "rate": "1/16"}
  },
  
  "meta": {
    "importance": 0.9,
    "humanize": 0.1,
    "source": "generated"
  }
}
```

## Implementation Extensions

### Enhanced MusicalContainer Class
```cpp
class MusicalContainer {
public:
    // Core recursive structure
    std::vector<Event> events;
    std::map<String, MusicalContainer> layers;
    std::map<String, MusicalContainer> variations;
    std::map<String, MusicalContainer> patterns;
    Context context;
    String reference;
    
    // NEW: Expression features
    std::map<String, String> shortcuts;
    std::vector<Transform> transforms;
    ChordProgression chordProgression;
    std::map<String, ProcessorChain> processorChains;
    AIConstraints constraints;
    PerformanceContext performance;
    
    // Enhanced rendering with full expression
    EventStream render(const Context& parentContext, int currentBar, 
                      int totalBars, const PerformanceState& state);
                      
private:
    // Expression processing
    void applyExpression(Event& event, const Context& context);
    void applyProcessors(EventStream& stream, const ProcessorChain& chain);
    void applyAutomation(EventStream& stream, const AutomationMap& automation);
    void applyGroove(EventStream& stream, const GrooveSettings& groove);
    
    // Harmony resolution
    MidiNote resolveChordDegree(int degree, const String& chord, int octave);
    ChordDefinition resolveChord(const String& chordName, const Context& context);
    
    // AI integration
    EventStream generateContent(const AIConstraints& constraints, const Context& context);
    void applyStyleModeling(EventStream& stream, const PerformanceContext& perf);
};
```

### Context with Full Expression Support
```cpp
struct Context {
    // Basic attributes
    int octave = 4;
    float velocity = 0.8f;
    String chord = "C";
    String key = "C major";
    
    // NEW: Expression attributes
    String dynamics = "mf";              // pp, p, mp, mf, f, ff
    String articulation = "normal";      // staccato, legato, marcato, tenuto
    GrooveSettings groove;
    String processorChain;
    AutomationMap automation;
    
    // AI/Performance attributes
    float complexity = 0.5f;
    float energy = 0.5f;
    String style = "default";
    PerformanceRole role;
    
    Context merge(const Context& child) const;
    bool has(const String& attribute) const;
    var get(const String& attribute) const;
};
```

## Benefits of Complete System

### 1. **Expression Parity**
✅ All features from music-language-design.md included
✅ Relative harmony with chord degrees
✅ Advanced expression and articulation
✅ MIDI processor integration
✅ AI-assisted composition

### 2. **Recursive Power** 
✅ All expression features work at any nesting level
✅ Context inheritance through hierarchy
✅ Transforms apply to any container
✅ AI constraints at any scope

### 3. **LLM Optimization**
✅ Shorthand reduces token usage
✅ Consistent structure at all levels
✅ Natural language concepts (dynamics, articulation)
✅ Constraint-based generation prompts

This complete system provides **full expression parity** with the vision while maintaining the **unified recursive architecture** that eliminates code duplication!