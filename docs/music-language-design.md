# Covertracks Music Language Design v2.1

**A Unified Architecture for Musical Expression and Robust Parsing**

## Core Concepts

### Container Aliasing System

While everything is a MusicalContainer at the implementation level, we use semantic aliases to describe the musical hierarchy:

- **Arrangement**: A container that contains sections (sequential playback order)
- **Section**: A container that contains phrases (verse, chorus, bridge, etc.)
- **Phrase**: A container that contains bars (musical sentences)
- **Bar**: A container that contains beats (measures)
- **Beat**: A container that contains events (notes, rests)

This aliasing provides:
1. **Semantic Clarity**: Musicians understand "section" better than "container"
2. **Universal Implementation**: All aliases map to the same MusicalContainer class
3. **Flexible Nesting**: Any level can contain any other level when needed
4. **Context Inheritance**: Each level inherits and can override parent context

Example:
```json
{
  "arrangement": {  // Container with sections
    "intro": {     // Section container
      "phrase1": { // Phrase container
        "bar1": {  // Bar container
          "beat1": {  // Beat container
            "events": [{"n": "C4", "t": 1.0}]
          }
        }
      }
    }
  }
}
```

### Unified Container Architecture

Instead of different types (layers, variations, patterns, sections), we have one MusicalContainer with aliased relationships:

```json
{
  // Traditional approach with type-specific fields
  "sections": { "verse": {...} },
  "layers": { "piano": {...} },
  "variations": { "bar4": {...} },
  
  // Unified approach with semantic aliases
  "arrangement": {              // Top-level container
    "verse": {                  // Section container
      "simultaneous": {         // Layers become simultaneous containers
        "piano": {...},
        "bass": {...}
      },
      "sequential": {           // Variations become sequential containers
        "bar1": {...},
        "bar2": {...}
      }
    }
  }
}
```

## Executive Summary

This document proposes a comprehensive redesign of the Covertracks music language to address two critical needs:

1. **Musical Expression**: Rich expressive capabilities including MIDI processor chains, dynamics, articulation, and performance techniques
2. **Parser Robustness**: Unified parsing architecture that eliminates code duplication and makes adding new features trivial

The current language suffers from significant parsing complexity where similar features are implemented multiple times across different contexts (variations in layers vs bars), making maintenance difficult and feature addition expensive.

## Current Language Analysis

### Strengths
- **Clean separation of concerns**: Primitives (vocabulary) vs. Patterns (syntax) vs. Arrangement (structure)
- **Compositional**: Patterns can reference other patterns (DRY principle)
- **Multi-layer support**: Handles multiple instruments well
- **JSON-based**: Familiar, parseable, human-readable

### Critical Gaps

#### 1. Parsing Architecture Issues
- **Code Duplication**: BasePattern + variations logic duplicated across section-level and layer-level parsing
- **Mixed Responsibilities**: PatternParser handles JSON parsing, musical concept resolution, beat timing, MIDI mapping, and more
- **Multiple Parsing Contexts**: Four different parsing paths for similar musical concepts
- **Complex Nested Logic**: 6+ levels of nesting in section parsing makes testing and maintenance difficult

#### 2. Musical Expression Limitations
- **No Dynamic Expression**: Missing crescendo, diminuendo, accents, articulation
- **Limited Rhythmic Feel**: No swing, groove, humanization support
- **No Effect Integration**: Missing connection to MIDI processor chains
- **Basic Harmony**: Limited jazz extensions and voice leading
- **No Relative Harmony**: Cannot express patterns as chord degrees (1,3,5) that adapt to different chord contexts
- **Verbose Velocity Control**: Must specify velocity per note instead of setting dynamic contexts
- **No Shorthand System**: Repetitive patterns require full specification every time

## Proposed Solution: Unified Music Language Architecture

### Core Principle: Everything is an Event Stream

Instead of different parsing contexts for layers/bars/variations, treat everything as **transformations of event streams**.

#### Basic Event Structure with Context System
```json
{
  "events": [
    {"time": 0.0, "context": {"chord": "Cmaj7", "octave": 4, "velocity": "mf"}},
    {"time": 1.0, "degree": 1},        // C4 at mf velocity
    {"time": 2.0, "degree": 3},        // E4 at mf velocity
    {"time": 3.0, "velocity": "f"},    // Change velocity context
    {"time": 3.0, "degree": 5},        // G4 at forte velocity
    {"time": 4.0, "degree": 7}         // B4 at forte velocity
  ]
}
```

#### Shorthand Event Structure
```json
{
  "shortcuts": {
    "k": "kick", "s": "snare", "h": "hihat",
    "std4": "k..s..k..s"  // Dot notation for beats
  },
  "events": [
    {"t": 0, "ctx": "C:4:mf"},         // Chord:Octave:Velocity
    {"t": 1, "d": 1},                  // Root
    {"t": 2, "d": 3},                  // Third  
    {"t": 3, "pattern": "std4"}        // Use shorthand
  ]
}
```

### 1. Relative Harmony and Chord Contexts

Express patterns as chord degrees that adapt to different harmonic contexts:

```json
{
  "patterns": {
    "walkingBass": [
      {"time": 1.0, "degree": 1},    // Root
      {"time": 2.0, "degree": 3},    // Third
      {"time": 3.0, "degree": 5},    // Fifth
      {"time": 4.0, "degree": 8}     // Octave
    ],
    "jazzComp": [
      {"time": 1.0, "degrees": [3, 7, 9]},      // Shell voicing + extension
      {"time": 2.5, "degrees": [1, 5]},         // Root-fifth punch
      {"time": 4.0, "degrees": [3, 7, 9, 13]}   // Full extension
    ]
  },
  
  "chordProgression": [
    {"time": 1.0, "chord": "Cmaj7", "bars": 2},
    {"time": 3.0, "chord": "Am7", "bars": 2},
    {"time": 5.0, "chord": "Dm7", "bars": 2},
    {"time": 7.0, "chord": "G7", "bars": 2}
  ],
  
  "layers": {
    "bass": {
      "pattern": "walkingBass",
      "chordContext": "progression"  // Pattern adapts to each chord
    },
    "piano": {
      "pattern": "jazzComp",
      "chordContext": "progression"
    }
  }
}
```

### 2. Context-Based Velocity and Dynamics

Set velocity contexts that apply to following events until changed:

```json
{
  "velocityContexts": {
    "default": 0.7,
    "accents": {"strong": 0.9, "weak": 0.5},
    "dynamics": {"pp": 0.2, "p": 0.4, "mf": 0.6, "f": 0.8, "ff": 1.0}
  },
  "events": [
    {"time": 0.0, "velocity": "mf"},           // Set velocity context
    {"time": 1.0, "degree": 1},               // Uses mf (0.6)
    {"time": 2.0, "degree": 3},               // Still mf
    {"time": 3.0, "velocity": "f"},           // Change context
    {"time": 3.0, "degree": 5},               // Uses f (0.8)
    {"time": 4.0, "accent": "strong"},        // Temporary accent
    {"time": 4.0, "degree": 8}                // Uses strong accent (0.9)
  ]
}
```

### 3. Shorthand and Abbreviation System

Compact notation for common patterns and repetitive structures:

```json
{
  "shortcuts": {
    // Note shortcuts
    "k": "kick", "s": "snare", "h": "hihat", "c": "crash",
    
    // Pattern shortcuts
    "basic4": "k..s..k..s",           // Dot notation: . = quarter beat
    "16ths": "k.h.s.h.k.h.s.h",      // 16th note hihat pattern
    "fill": "s.s.t1.t2.t3.c",        // Tom fill ending with crash
    
    // Chord shortcuts
    "I": [1, 3, 5], "IV": [4, 6, 1], "V7": [5, 7, 2, 4],
    
    // Rhythm shortcuts
    "swing": {"groove": "shuffle", "amount": 0.67}
  },
  
  "events": [
    {"time": 0, "pattern": "basic4", "repeat": 4},
    {"time": 16, "pattern": "fill"}
  ]
}
```

### 4. Unified Expression System

Universal attribute system that works in all contexts:

```json
{
  "events": [
    {
      "time": 1.0, 
      "note": "C4",
      "expression": {
        "dynamics": "ff",        // Musical dynamics
        "articulation": "staccato",
        "processors": {          // MIDI processor chain effects
          "distortion": {"drive": 0.7},
          "wah": {"frequency": "auto", "depth": 0.5},
          "reverb": {"room": 0.3, "decay": 2.0}
        },
        "automation": {          // Parameter automation
          "velocity": {"curve": "linear", "end": 0.3},
          "wah.frequency": {"curve": "sine", "rate": "1/8"}
        }
      }
    }
  ]
}
```

### 2. Compositional Transforms

Eliminates parsing duplication through composable transforms:

```json
{
  "base": [
    {"time": 1.0, "note": "kick"},
    {"time": 3.0, "note": "snare"}
  ],
  "transforms": [
    {"type": "repeat", "bars": 4},
    {"type": "variation", "bar": 4, "events": [{"time": 4.0, "note": "crash"}]},
    {"type": "swing", "amount": 0.67},
    {"type": "humanize", "timing": 0.05, "velocity": 0.1},
    {"type": "layer", "with": "bassline"},
    {"type": "processors", "chain": ["distortion", "delay"]}
  ]
}
```

### 3. MIDI Processor Integration

Rich expression through processor chains:

```json
{
  "processorChains": {
    "rockGuitar": [
      {"type": "overdrive", "gain": 0.8},
      {"type": "eq", "low": 1.2, "mid": 0.8, "high": 1.1},
      {"type": "delay", "time": "1/8", "feedback": 0.3},
      {"type": "reverb", "type": "hall", "size": 0.6}
    ],
    "jazzOrgan": [
      {"type": "leslie", "speed": "slow"},
      {"type": "overdrive", "gain": 0.2, "tube": true},
      {"type": "reverb", "type": "spring"}
    ]
  },
  "events": [
    {
      "time": 1.0, 
      "note": "C4",
      "processors": "rockGuitar",
      "automation": {
        "overdrive.gain": {"time": 2.0, "value": 1.0, "curve": "exponential"}
      }
    }
  ]
}
```

### 4. Robust Grammar with Natural Extensions

Extensible attribute grammar that makes adding new features trivial:

```bnf
Event ::= { 
  "time": Number,
  "note": NoteExpression,
  BasicAttributes*,
  ExtendedAttributes*
}

BasicAttributes ::= 
  | "velocity": Number 
  | "duration": Number
  | "channel": Number

ExtendedAttributes ::= 
  | "expression": ExpressionBlock
  | "processors": ProcessorBlock  
  | "automation": AutomationBlock
  | "metadata": MetadataBlock

ExpressionBlock ::= {
  MusicalAttribute*
}

MusicalAttribute ::= 
  | "dynamics": ("pp" | "p" | "mp" | "mf" | "f" | "ff")
  | "articulation": ("staccato" | "legato" | "marcato" | "tenuto")
  | "accent": Number
  | "swing": Number
  | "groove": GrooveName
```

### 5. Natural Language Extensions

Adding new features becomes trivial with the unified system:

```json
// New feature: vibrato
{
  "events": [
    {
      "time": 1.0,
      "note": "C4", 
      "expression": {
        "vibrato": {"rate": 5.5, "depth": 0.2}  // ← Natural extension
      }
    }
  ]
}

// New feature: complex polyrhythms  
{
  "events": [
    {
      "time": 1.0,
      "note": "C4",
      "timing": {
        "polyrhythm": "3:2",     // ← Natural extension
        "subdivision": "triplet"
      }
    }
  ]
}
```

## Context Resolution and Precedence Rules

### Clear Precedence Hierarchy

When multiple context levels specify the same attribute, the more specific level wins:

1. **Event-level** overrides everything
2. **Variation** overrides pattern  
3. **Pattern** overrides layer
4. **Layer** overrides section
5. **Section** overrides global
6. **Later contexts** override earlier ones in the stream

```
Event → Variation → Pattern → Layer → Section → Global
```

### Practical Context Resolution Example

```json
{
  "global": {
    "octave": 4,
    "velocity": 0.5
  },
  
  "sections": {
    "verse": {
      "octave": 3,              // Overrides global
      "chordProgression": [{"bar": 1, "chord": "C"}],
      
      "layers": {
        "bass": {
          "octave": 2,          // Overrides section
          
          "patterns": {
            "walking": {
              "velocity": 0.7,  // Overrides layer
              "events": [
                {"t": 1, "d": 1},                    // Uses pattern velocity 0.7
                {"t": 2, "d": 3, "vel": 0.9}        // Event override: 0.9
              ],
              
              "variations": {
                "bar4": {
                  "velocity": 0.8,  // Overrides pattern
                  "events": [
                    {"t": 1, "d": 1},                // Uses variation velocity 0.8
                    {"t": 3, "d": 5, "vel": 1.0}    // Event override: 1.0
                  ]
                }
              }
            }
          }
        }
      }
    }
  }
}
```

**Resolution for a note at bar 4, time 1:**
- Global says: octave 4, velocity 0.5
- Section says: octave 3
- Layer says: octave 2  
- Pattern says: velocity 0.7
- Variation says: velocity 0.8
- **Result**: octave 2 (from layer), velocity 0.8 (from variation)

### Context Streams and State Management

Multiple independent context streams can run in parallel:

```json
{
  "events": [
    // Harmony stream sets chord context
    {"t": 0, "stream": "harmony", "ctx": "C:4"},
    {"t": 4, "stream": "harmony", "ctx": "F:4"},
    
    // Dynamics stream independent of harmony
    {"t": 0, "stream": "dynamics", "vel": "p"},
    {"t": 2, "stream": "dynamics", "vel": "cresc"},
    {"t": 4, "stream": "dynamics", "vel": "f"},
    
    // Note events use both streams
    {"t": 1, "d": 1},  // C4, piano
    {"t": 3, "d": 3},  // E4, crescendo
    {"t": 5, "d": 1}   // F4, forte
  ]
}
```

### Complex Scenario Handling

#### Layer with Different Chord Progression
```json
{
  "layers": {
    "melody": {
      "chordContext": "main_progression"  // Follows main chords
    },
    "countermelody": {
      "chordContext": "substitute_progression",  // Different chords!
      "pattern": [{"t": 1, "d": 3}, {"t": 3, "d": 5}]
    }
  }
}
```

#### Variation with Temporary Context Change
```json
{
  "bass": {
    "pattern": [{"t": 1, "d": 1}, {"t": 3, "d": 5}],
    "variations": {
      "bar8": {
        "context": {"mode": "dorian"},  // Temporary mode change
        "events": [{"t": 1, "d": 1}, {"t": 2, "d": "b3"}, {"t": 3, "d": 4}]
      }
    }
  }
}
```

## Implementation Architecture

### Unified Parser Design

**Single parsing pipeline** that eliminates code duplication:

```cpp
class UnifiedPatternParser {
public:
    // Single entry point for all patterns
    EventStream parsePattern(const var& json);
    
private:
    // Pluggable handlers registry
    std::map<String, std::unique_ptr<PatternHandler>> handlers;
    
    // Single event processing pipeline
    Event parseEvent(const var& eventJson);
    EventStream applyTransforms(const EventStream& base, const Array<Transform>& transforms);
    
    // Composable attribute parsers
    ExpressionAttributes parseExpression(const var& expr);
    ProcessorChain parseProcessors(const var& processors);
    AutomationCurves parseAutomation(const var& automation);
};
```

### Core Data Structures

#### Unified Event Model
```cpp
struct UnifiedEvent {
    double time;
    String note;
    float velocity = 0.8f;
    float duration = 0.25f;
    
    // Extensible attributes
    var expression;      // Musical expression
    var processors;      // Effect chain
    var automation;      // Parameter automation
    var metadata;        // Source tracking, etc.
};
```

#### Transform System
```cpp
class TransformEngine {
public:
    EventStream apply(const EventStream& input, const Transform& transform);
    
    // Built-in transforms
    EventStream repeat(const EventStream& input, int bars);
    EventStream swing(const EventStream& input, float amount);
    EventStream layer(const EventStream& base, const EventStream& overlay);
    EventStream humanize(const EventStream& input, float timing, float velocity);
    EventStream addProcessors(const EventStream& input, const ProcessorChain& chain);
};
```

#### Expression Engine
```cpp
class ExpressionEngine {
public:
    MidiSequence render(const EventStream& events, const RenderContext& context);
    
private:
    void applyDynamics(MidiMessage& msg, const var& dynamics);
    void applyArticulation(MidiMessage& msg, const var& articulation);
    void applyProcessors(MidiMessage& msg, const ProcessorChain& chain);
    void applyAutomation(MidiSequence& sequence, const AutomationCurves& curves);
};
```

## Advanced Musical Features

### 1. Rhythmic Expression

```json
{
  "groove": {
    "swing": 0.67,        // 67% swing vs straight 16ths
    "humanize": 0.05,     // ±5% timing variation
    "velocity": 0.1       // ±10% velocity variation
  },
  "patterns": {
    "main": [
      [1.0, "kick", {"swing": true}],
      [1.67, "hihat", {"swing": true}]  // Swung 8th note
    ]
  }
}
```

### 2. Advanced Harmony

```json
{
  "harmony": {
    "key": "C major",
    "chords": {
      "Cmaj9": {
        "root": "C",
        "quality": "maj9",
        "voicing": "rootless",  // Jazz voicing
        "inversion": 1
      }
    }
  },
  "patterns": {
    "jazzChords": [
      [1.0, {"chord": "Cmaj9", "voicing": "spread"}],
      [3.0, {"chord": "F13", "voicing": "close"}]
    ]
  }
}
```

### 3. AI-Assisted Composition

```json
{
  "constraints": {
    "melody": {
      "range": ["C3", "C5"],
      "scale": "C major pentatonic",
      "contour": "arch",      // Shape constraint
      "rhythm": "syncopated"
    },
    "generate": {
      "bars": 4,
      "style": "blues_rock",
      "complexity": 0.7
    }
  }
}
```

### 4. Performance Context

```json
{
  "performance": {
    "ensemble": "jazz_trio",
    "interaction": {
      "listen_to": ["bass", "drums"],
      "respond_to": "dynamics",
      "solo_section": true
    },
    "style": {
      "genre": "bebop",
      "era": "1950s",
      "reference": "Charlie Parker"
    }
  }
}
```

## Implementation Roadmap

### Phase 1: Core Infrastructure (4-6 weeks)
1. **Unified Event Model**: Implement core UnifiedEvent structure
2. **Basic Transform System**: Implement repeat, layer, and basic timing transforms
3. **Expression Attributes**: Add support for dynamics and basic articulation
4. **Parser Refactoring**: Begin migration from current multi-context parser

### Phase 2: Musical Expression (4-6 weeks)
5. **MIDI Processor Integration**: Connect expression attributes to Tracktion processor chains
6. **Groove and Swing**: Implement rhythmic feel transforms
7. **Advanced Harmony**: Add jazz chord extensions and voicing support
8. **Automation System**: Parameter automation over time

### Phase 3: Advanced Features (6-8 weeks)
9. **AI Integration**: Constraint-based generation system
10. **Performance Context**: Interactive performance metadata
11. **Complex Timing**: Polyrhythms and advanced subdivision support
12. **Style Modeling**: Genre-specific generation rules

### Phase 4: Optimization and Polish (2-4 weeks)
13. **Performance Optimization**: Efficient event stream processing
14. **Error Handling**: Robust validation and graceful degradation
15. **Documentation**: Complete language specification and examples
16. **Migration Tools**: Convert existing patterns to new format

## Expected Benefits

### Technical Benefits
- **Eliminated Code Duplication**: Single parsing pipeline replaces 4+ different contexts
- **Simplified Maintenance**: Adding new features only requires changes in one place
- **Better Testing**: Composable components are easier to unit test
- **Performance**: More efficient event stream processing

### Musical Benefits
- **Rich Expression**: Dynamics, articulation, and performance techniques
- **Effect Integration**: Full MIDI processor chain support
- **Rhythmic Feel**: Swing, groove, and humanization
- **Advanced Harmony**: Jazz extensions and sophisticated chord voicings
- **Relative Harmony**: Patterns adapt to different chord contexts using degrees
- **Context-Based Dynamics**: Set velocity levels that apply to multiple events
- **AI Integration**: Constraint-based composition assistance

### Development Benefits
- **Faster Feature Development**: Natural language extensions
- **Easier Debugging**: Clear separation of concerns
- **Better Documentation**: Self-documenting grammar structure
- **Future-Proof**: Extensible architecture for new musical concepts
- **Concise Notation**: Shorthand system reduces verbosity for common patterns
- **Clear Precedence**: Predictable context resolution rules

## Conclusion

This unified music language design (v2.1) addresses both the immediate parsing complexity issues and the long-term need for rich musical expression. By treating everything as event streams with composable transforms and clear context precedence, we eliminate code duplication while opening up massive expressive possibilities.

**Key v2.1 Improvements:**
- **Relative harmony system** allows patterns to adapt to different chord contexts
- **Context-based velocity** eliminates per-note verbosity  
- **Shorthand notation** makes common patterns concise
- **Clear precedence rules** provide predictable behavior across all complexity levels

The result will be a language suitable for everything from simple drum patterns (`"k..s..k..s"`) to complex AI-assisted jazz compositions with relative chord voicing, dynamic expression, and effect automation - all with a robust parsing architecture that makes adding new features trivial rather than a maintenance nightmare.

## Unified Song-Chart Architecture

### Eliminating Dual Systems

Rather than maintaining separate "Song" and "Chart" systems, the v2.1 language unifies both under a single architecture with different complexity levels:

```json
{
  "type": "song",           // vs "chart"
  "style": "lead_sheet",    // vs "full_arrangement" 
  "complexity": "basic",    // vs "detailed"
  
  // Same underlying v2.1 language structure works for both
  "sections": { ... },
  "chordProgression": { ... },
  "melody": { ... },
  "lyrics": { ... }         // Songs add lyrics, charts focus on instruments
}
```

### Song Structure Integration

Songs map perfectly to the v2.1 context system:

#### Lead Sheet Example
```json
{
  "type": "song",
  "style": "lead_sheet",
  "title": "Autumn Leaves",
  "key": "G major",
  "timeSignature": [4, 4],
  
  "shortcuts": {
    "ii-V-I": [
      {"chord": "Am7", "duration": 2}, 
      {"chord": "D7", "duration": 2}, 
      {"chord": "Gmaj7", "duration": 4}
    ],
    "typical_melody": [
      {"beat": 1, "degree": 5, "duration": 1},
      {"beat": 2, "degree": 3, "duration": 0.5},
      {"beat": 2.5, "degree": 5, "duration": 0.5},
      {"beat": 3, "degree": 1, "duration": 2}
    ]
  },
  
  "sections": {
    "verse": {
      "bars": 8,
      "chordProgression": [
        {"bar": 1, "chord": "Gmaj7", "duration": 2},
        {"bar": 1.5, "chord": "Em7", "duration": 2}, 
        {"bar": 3, "pattern": "ii-V-I"},              // Shorthand expansion
        {"bar": 6, "chord": "Em7", "duration": 4}
      ],
      
      "lyrics": {
        "bar1": "The falling leaves drift by my window",
        "bar3": "The autumn leaves of red and gold",
        "bar5": "I see your lips, the summer kisses",
        "bar7": "The sunburned hands I used to hold"
      },
      
      "melody": {
        "pattern": "typical_melody",                   // Uses relative degrees
        "variations": {
          "bar3": [                                    // Adapts to Am7-D7-G progression
            {"beat": 1, "degree": 1},                 // A (in Am7)
            {"beat": 2, "degree": "b7"},              // G (in Am7) 
            {"beat": 3, "degree": 5},                 // A (in D7)
            {"beat": 4, "degree": 1}                  // G (in Gmaj7)
          ]
        }
      }
    },
    
    "bridge": {
      "bars": 8,
      "key": "Bb major",                               // Key change
      "chordProgression": [
        {"bar": 1, "chord": "Bbmaj7", "duration": 4},
        {"bar": 3, "chord": "Gm7", "duration": 4},
        {"bar": 5, "pattern": "ii-V-I"},              // Same pattern, new key
        {"bar": 8, "chord": "D7", "duration": 4}      // Back to G major
      ],
      
      "lyrics": {
        "bar1": "Since you went away the days grow long",
        "bar3": "And soon I'll hear old winter's song",
        "bar5": "But I miss you most of all",
        "bar7": "My darling, when autumn leaves start to fall"
      },
      
      "melody": {
        "pattern": "typical_melody"                    // Same pattern, auto-transposes
      }
    }
  },
  
  "arrangement": ["verse", "verse", "bridge", "verse"],
  
  "performance": {
    "tempo": 120,
    "style": "jazz_ballad",
    "feel": "swing_8ths"
  }
}
```

### Song vs Chart Complexity Levels

#### Simple Song (Lead Sheet)
```json
{
  "type": "song",
  "complexity": "lead_sheet",
  "elements": ["chords", "melody", "lyrics"],
  
  "melody": [
    {"bar": 1, "degree": 1},
    {"bar": 2, "degree": 3}
  ]
}
```

#### Detailed Chart (Full Arrangement)
```json
{
  "type": "chart", 
  "complexity": "full_arrangement",
  "elements": ["drums", "bass", "piano", "guitar", "vocals"],
  
  "layers": {
    "drums": {
      "pattern": "jazz_swing",
      "variations": {"bar8": "fill"}
    },
    "bass": {
      "pattern": "walking_bass",
      "octave": 2
    },
    "melody": [
      {"bar": 1, "degree": 1, "expression": {"vibrato": true}}
    ]
  }
}
```

### Automatic Complexity Scaling

The same song definition can generate different outputs:

```json
{
  "song": {
    "title": "Blue Bossa",
    // ... song definition
  },
  
  "renderAs": {
    "leadSheet": {
      "include": ["chords", "melody", "lyrics"],
      "format": "standard_notation"
    },
    "jazzTrio": {
      "include": ["drums", "bass", "piano"],
      "style": "bossa_nova",
      "complexity": "medium"
    },
    "fullBand": {
      "include": ["drums", "bass", "piano", "guitar", "horn_section"],
      "style": "latin_jazz",
      "complexity": "high",
      "improvisation": true
    }
  }
}
```

### Benefits of Unified Architecture

1. **Single Codebase**: One parser, one data model, one set of tests
2. **Automatic Scaling**: Simple songs can become complex arrangements
3. **LLM Integration**: Same language for lead sheet recognition and full composition
4. **Consistent Context Rules**: Same precedence system works for lyrics, chords, and instruments
5. **Relative Harmony**: Melody and arrangements adapt automatically to chord changes
6. **Style Templates**: "jazz_ballad" or "bossa_nova" can set appropriate defaults

### Migration Path

**Current dual system:**
- `Song` objects from LLM lead sheet parsing
- `Chart` objects from detailed pattern creation

**New unified system:**
- Single `MusicalWork` class using v2.1 language
- `complexity` and `style` parameters determine rendering level
- Existing songs convert to `type: "song"` format
- Existing charts convert to `type: "chart"` format
- Both use same underlying context and precedence system

This unification eliminates code duplication while making the system more powerful and easier to maintain.