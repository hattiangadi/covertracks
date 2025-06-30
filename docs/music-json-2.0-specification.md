# Music JSON 2.0 Language Specification

## Table of Contents
1. [Overview](#overview)
2. [Core Philosophy](#core-philosophy)
3. [Architecture](#architecture)
4. [Basic Structure](#basic-structure)
5. [Event System](#event-system)
6. [Container Hierarchy](#container-hierarchy)
7. [Context System](#context-system)
8. [Pattern System](#pattern-system)
9. [Expression Features](#expression-features)
10. [Musical Examples](#musical-examples)

## Overview

Music JSON 2.0 is a unified, recursive grammar system for expressing musical patterns. It replaces the complex, multi-layered structure of JSON 1.0 with a single, elegant container-based approach that eliminates code duplication while maintaining full musical expressiveness.

### Key Improvements
- **Unified Recursive Grammar**: Everything is a `MusicalContainer`
- **Simplified Syntax**: LLM-friendly, consistent structure
- **Context Inheritance**: Clean hierarchical property inheritance
- **Eliminated Code Duplication**: Same variation system for patterns, layers, sections
- **Enhanced Expression**: Advanced dynamics, articulation, and processor support

### Format Version
```json
{
  "version": "2.0"
}
```

## Core Philosophy

### Everything is a MusicalContainer
In Music JSON 2.0, all musical entities follow the same recursive structure:
- **Sections** are containers with events and child containers
- **Layers** are containers that render simultaneously  
- **Patterns** are reusable containers referenced by name
- **Variations** are containers that replace content conditionally

### Temporal Relationships
- **Layers**: Simultaneous (rendered at the same time)
- **Variations**: Sequential replacement (one applies based on conditions)
- **Events**: Positioned in time with explicit timing

### Context Inheritance
Properties flow down the hierarchy with clear precedence:
`Event → Variation → Pattern → Layer → Section → Global`

## Architecture

### Dynamic Container System
There are **no fixed structural elements** like "bars" or "measures" in the JSON. Instead:

- **MusicalContainer**: The only structural element - a recursive container that can hold events and child containers
- **Sections/Layers/Patterns/Variations**: All become MusicalContainers at runtime
- **Dynamic Timing**: Events are positioned by beat time (`"t": 5.0` = beat 1 of bar 2) without explicit bar boundaries
- **Flexible Structure**: Any container can contain any combination of events, layers, patterns, variations

### Runtime Conversion
```
JSON 2.0 → UniversalMusicParser → MusicalContainer tree → EventStream → MIDI
```

The parser dynamically creates a tree of MusicalContainers based on the JSON structure:
```cpp
MusicalContainer root;
  ├── patterns["basicBeat"] (MusicalContainer)
  ├── sections["verse"] (MusicalContainer)  
  │   └── layers["drums"] (MusicalContainer)
  │       └── variations["bar4"] (MusicalContainer)
  └── events[] (Event array)
```

## Basic Structure

### Root Container
```json
{
  "version": "2.0",
  "meta": {
    "title": "Song Title",
    "tempo": 120,
    "timeSignature": "4/4",
    "key": "C major"
  },
  "context": {
    "velocity": 0.8,
    "octave": 4,
    "dynamics": "mf",
    "articulation": "normal"
  },
  "events": [...],
  "layers": {...},
  "variations": {...},
  "patterns": {...},
  "sections": {...}
}
```

### MusicalContainer Structure
Every container can contain:
```json
{
  "context": {
    "velocity": 0.8,
    "octave": 4,
    "chord": "C",
    "dynamics": "mf",
    "articulation": "legato"
  },
  "events": [
    {"t": 1.0, "n": "C4", "v": 0.8, "dur": 1.0}
  ],
  "layers": {
    "bass": {...},
    "melody": {...}
  },
  "variations": {
    "bar4": {...},
    "chorus": {...}
  },
  "patterns": {
    "basicBeat": {...}
  },
  "sections": {
    "verse": {...},
    "chorus": {...}
  },
  "pattern": "patternName"
}
```

## Event System

### Basic Event Format
```json
{
  "t": 1.0,        // Time position (beats)
  "n": "C4",       // Note name or primitive name
  "v": 0.9,        // Velocity (optional - only for accents/emphasis)
  "dur": 1.0,      // Duration (beats)
  "d": 1,          // Chord degree (requires chord context)
  "chord": "Am"    // Local chord override
}
```

### Note Formats

#### Fixed Notes (Context-based velocity)
```json
{
  "context": {"velocity": 0.7, "dynamics": "mf"},
  "events": [
    {"t": 1.0, "n": "C4"},           // Uses context velocity
    {"t": 2.0, "n": "F#3"},          // Uses context velocity  
    {"t": 3.0, "n": "Bb2", "v": 0.9} // Explicit accent
  ]
}
```

#### Chord Degrees (Relative Harmony)
```json
{
  "context": {"chord": "C", "velocity": 0.8},
  "events": [
    {"t": 1.0, "d": 1},  // C - uses context velocity
    {"t": 2.0, "d": 3},  // E - uses context velocity
    {"t": 3.0, "d": 5, "v": 1.0}   // G - accented
  ]
}
```

#### Percussion/Primitives
```json
{
  "context": {"velocity": 0.7},
  "events": [
    {"t": 1.0, "n": "kick"},          // Uses context
    {"t": 2.0, "n": "snare"},         // Uses context
    {"t": 3.0, "n": "hihat", "v": 0.4} // Quieter ghost note
  ]
}
```

### Time Positioning
- **Beat-based**: `"t": 1.0` = beat 1, `"t": 2.5` = beat 2.5
- **Multi-bar**: `"t": 5.0` = beat 1 of bar 2 (auto-distributed)
- **Swing timing**: `"t": 1.33` for triplet feel

### Duration System
```json
{
  "context": {"velocity": 0.8},
  "events": [
    {"t": 1.0, "n": "C4", "dur": 2.0},    // 2-beat note, context velocity
    {"t": 1.0, "n": "kick", "dur": 0.25}   // Short percussion hit, context velocity
  ]
}
```

## Container Hierarchy

### Sections (Named Musical Sections)
```json
"sections": {
  "verse": {
    "context": {"velocity": 0.7, "dynamics": "mp"},
    "layers": {
      "drums": {...},
      "bass": {...},
      "melody": {...}
    }
  },
  "chorus": {
    "context": {"velocity": 0.9, "dynamics": "f"},  // Louder for chorus
    "layers": {...}
  }
}
```

### Layers (Simultaneous Parts)
```json
"layers": {
  "rhythm": {
    "context": {"velocity": 0.8},
    "layers": {
      "kick": {
        "events": [
          {"t": 1.0, "n": "kick"},       // Uses parent velocity
          {"t": 3.0, "n": "kick", "v": 1.0}  // Accented
        ]
      },
      "snare": {
        "context": {"velocity": 0.7},    // Slightly quieter
        "events": [
          {"t": 2.0, "n": "snare"},
          {"t": 4.0, "n": "snare"}
        ]
      }
    }
  },
  "melody": {
    "context": {"octave": 4, "velocity": 0.75},
    "events": [...]
  }
}
```

### Patterns (Reusable Templates)
```json
"patterns": {
  "basicBeat": {
    "context": {"velocity": 0.8},
    "events": [
      {"t": 1.0, "n": "kick"},        // Uses pattern velocity
      {"t": 3.0, "n": "snare", "v": 0.7}  // Slightly softer snare
    ]
  },
  "walkingBass": {
    "context": {"octave": 2, "velocity": 0.75},
    "events": [
      {"t": 1.0, "d": 1, "dur": 1.0},
      {"t": 2.0, "d": 5, "dur": 1.0},
      {"t": 3.0, "d": 3, "dur": 1.0},
      {"t": 4.0, "d": 7, "dur": 1.0, "v": 0.9}  // Emphasized beat 4
    ]
  }
}
```

### Pattern References
```json
"layers": {
  "drums": {
    "pattern": "basicBeat"  // Inherits pattern's velocity context
  },
  "bass": {
    "pattern": "walkingBass",
    "context": {"chord": "Am", "velocity": 0.8}  // Override pattern velocity
  }
}
```

### Variations (Conditional Replacements)
```json
"layers": {
  "drums": {
    "pattern": "basicBeat",
    "variations": {
      "bar4": {
        "events": [
          {"t": 3.0, "n": "snare", "v": 0.6},
          {"t": 3.5, "n": "snare", "v": 0.7},
          {"t": 4.0, "n": "crash", "v": 1.0}
        ]
      },
      "bar8": {
        "pattern": "bigFill"
      }
    }
  }
}
```

## Context System

### Context Properties
```json
"context": {
  "velocity": 0.8,          // Default velocity (0.0-1.0)
  "octave": 4,              // Default octave register
  "chord": "C",             // Current chord for degree resolution
  "key": "C major",         // Key signature
  "dynamics": "mf",         // Musical dynamics (pp, p, mp, mf, f, ff)
  "articulation": "legato", // Articulation style
  "complexity": 0.5,        // AI complexity hint (0.0-1.0)
  "energy": 0.7,            // AI energy level (0.0-1.0)
  "style": "jazz"           // Musical style hint
}
```

### Context Inheritance Rules
1. **Child Overrides Parent**: More specific contexts override general ones
2. **Octave Special Case**: Only override if child octave ≠ 4 (default)
3. **Precedence Chain**: `Event → Variation → Pattern → Layer → Section → Global`

### Context Merging Example
```json
{
  "context": {"velocity": 0.6, "octave": 4, "dynamics": "mp"},
  "sections": {
    "verse": {
      "context": {"velocity": 0.8, "dynamics": "mf"},
      "layers": {
        "bass": {
          "context": {"octave": 2},
          "events": [
            {"t": 1.0, "d": 1, "v": 0.9}
          ]
        }
      }
    }
  }
}
```
**Resolved Context for bass event:**
- `velocity`: 0.9 (event level)
- `octave`: 2 (layer level)  
- `dynamics`: "mf" (section level)

## Pattern System

### Pattern Definition and Reuse
```json
{
  "patterns": {
    "jazzDrums": {
      "events": [
        {"t": 1.0, "n": "kick", "v": 0.8},
        {"t": 1.33, "n": "hihat", "v": 0.4},
        {"t": 2.0, "n": "snare", "v": 0.7},
        {"t": 2.67, "n": "hihat", "v": 0.4}
      ]
    },
    "soulDrums": {
      "events": [
        {"t": 1.0, "n": "kick", "v": 0.8},
        {"t": 2.5, "n": "hihat", "v": 0.4},
        {"t": 3.0, "n": "snare", "v": 0.7}
      ]
    }
  },
  "sections": {
    "verse": {
      "layers": {
        "drums": {"pattern": "jazzDrums"}
      }
    },
    "chorus": {
      "layers": {
        "drums": {"pattern": "soulDrums"}
      }
    }
  }
}
```

### Pattern with Variations
```json
{
  "patterns": {
    "basicGroove": {
      "events": [
        {"t": 1.0, "n": "kick", "v": 0.8},
        {"t": 3.0, "n": "snare", "v": 0.7}
      ],
      "variations": {
        "fill": {
          "events": [
            {"t": 3.0, "n": "snare", "v": 0.6},
            {"t": 3.5, "n": "tom", "v": 0.8},
            {"t": 4.0, "n": "crash", "v": 1.0}
          ]
        }
      }
    }
  }
}
```

### Nested Patterns
```json
{
  "patterns": {
    "rhythm": {
      "layers": {
        "kick": {"pattern": "kickPattern"},
        "snare": {"pattern": "snarePattern"}
      }
    },
    "fullBand": {
      "layers": {
        "drums": {"pattern": "rhythm"},
        "bass": {"pattern": "bassLine"},
        "piano": {"pattern": "comping"}
      }
    }
  }
}
```

## Expression Features

### Dynamics and Articulation
```json
{
  "context": {
    "dynamics": "mf",
    "articulation": "legato"
  },
  "events": [
    {
      "t": 1.0, "n": "C4", "v": 0.8,
      "dynamics": "ff",
      "articulation": "staccato"
    }
  ]
}
```

### Expression Maps
```json
{
  "events": [
    {
      "t": 1.0, "n": "C4", "v": 0.8,
      "expression": {
        "dynamics": "ff",
        "articulation": "staccato",
        "vibrato": {"rate": 5, "depth": 0.2},
        "bend": {"amount": 2, "duration": 0.5}
      }
    }
  ]
}
```

### Audio Processors
```json
{
  "events": [
    {
      "t": 1.0, "n": "C4", "v": 0.8,
      "processors": {
        "reverb": {"room": 0.5, "decay": 3.0},
        "delay": {"time": 0.25, "feedback": 0.3},
        "distortion": {"drive": 0.7}
      }
    }
  ]
}
```

### Automation
```json
{
  "events": [
    {
      "t": 1.0, "n": "C4", "v": 0.8, "dur": 4.0,
      "automation": {
        "volume": [
          {"t": 0.0, "v": 0.8},
          {"t": 2.0, "v": 1.0},
          {"t": 4.0, "v": 0.3}
        ],
        "pan": [
          {"t": 0.0, "v": 0.0},
          {"t": 4.0, "v": 1.0}
        ]
      }
    }
  ]
}
```

## Musical Examples

### Simple Drum Pattern
```json
{
  "version": "2.0",
  "meta": {"title": "Basic Rock Beat", "tempo": 120},
  "context": {"velocity": 0.8, "dynamics": "mf"},
  "events": [
    {"t": 1.0, "n": "kick"},           // Uses context velocity
    {"t": 2.0, "n": "snare", "v": 0.7}, // Softer snare
    {"t": 3.0, "n": "kick"},           // Uses context velocity  
    {"t": 4.0, "n": "snare", "v": 0.9}  // Accented backbeat
  ]
}
```

### Jazz Comping Pattern
```json
{
  "version": "2.0",
  "meta": {"title": "Jazz Comping", "tempo": 120},
  "context": {
    "chord": "Cmaj7",
    "octave": 4,
    "velocity": 0.6,
    "dynamics": "mp",
    "articulation": "legato"
  },
  "events": [
    {"t": 1.0, "d": 3, "dur": 0.5},      // Shell voicing, context velocity
    {"t": 1.0, "d": 7, "dur": 0.5},
    {"t": 2.5, "d": 1, "dur": 0.5, "v": 0.8}, // Emphasized comp
    {"t": 2.5, "d": 5, "dur": 0.5, "v": 0.8},
    {"t": 4.0, "d": 3, "dur": 1.0},      // Back to context velocity
    {"t": 4.0, "d": 7, "dur": 1.0}
  ]
}
```

### Walking Bass Line
```json
{
  "version": "2.0",
  "meta": {"title": "Walking Bass", "tempo": 120},
  "context": {"octave": 2, "articulation": "legato"},
  "patterns": {
    "walk": {
      "events": [
        {"t": 1.0, "d": 1, "dur": 1.0},
        {"t": 2.0, "d": 3, "dur": 1.0},
        {"t": 3.0, "d": 5, "dur": 1.0},
        {"t": 4.0, "d": 7, "dur": 1.0}
      ]
    }
  },
  "sections": {
    "progression": {
      "layers": {
        "bass": {
          "pattern": "walk",
          "variations": {
            "bar1": {"context": {"chord": "C"}},
            "bar2": {"context": {"chord": "Am"}},
            "bar3": {"context": {"chord": "F"}},
            "bar4": {"context": {"chord": "G"}}
          }
        }
      }
    }
  }
}
```

### Full Song Structure
```json
{
  "version": "2.0",
  "meta": {
    "title": "Jazz Standard",
    "tempo": 120,
    "style": "swing"
  },
  "context": {
    "key": "C major",
    "velocity": 0.7,
    "dynamics": "mf"
  },
  "patterns": {
    "swingDrums": {
      "events": [
        {"t": 1.0, "n": "kick", "v": 0.8},
        {"t": 1.33, "n": "hihat", "v": 0.4},
        {"t": 2.0, "n": "snare", "v": 0.7},
        {"t": 2.67, "n": "hihat", "v": 0.4},
        {"t": 3.0, "n": "kick", "v": 0.6},
        {"t": 3.33, "n": "hihat", "v": 0.4},
        {"t": 4.0, "n": "snare", "v": 0.7},
        {"t": 4.67, "n": "hihat", "v": 0.4}
      ]
    },
    "walkingBass": {
      "context": {"octave": 2},
      "events": [
        {"t": 1.0, "d": 1, "dur": 1.0},
        {"t": 2.0, "d": 3, "dur": 1.0},
        {"t": 3.0, "d": 5, "dur": 1.0},
        {"t": 4.0, "d": 7, "dur": 1.0}
      ]
    },
    "pianoComping": {
      "context": {"octave": 4},
      "events": [
        {"t": 1.5, "d": 3, "dur": 0.5, "v": 0.6},
        {"t": 1.5, "d": 7, "dur": 0.5, "v": 0.6},
        {"t": 3.0, "d": 1, "dur": 0.5, "v": 0.7},
        {"t": 3.0, "d": 5, "dur": 0.5, "v": 0.7}
      ]
    }
  },
  "sections": {
    "verse": {
      "context": {"chordProgression": ["C", "Am", "F", "G"]},
      "layers": {
        "drums": {"pattern": "swingDrums"},
        "bass": {
          "pattern": "walkingBass",
          "variations": {
            "bar2": {"context": {"chord": "Am"}},
            "bar3": {"context": {"chord": "F"}},
            "bar4": {"context": {"chord": "G"}}
          }
        },
        "piano": {
          "pattern": "pianoComping",
          "variations": {
            "bar2": {"context": {"chord": "Am"}},
            "bar3": {"context": {"chord": "F"}},
            "bar4": {"context": {"chord": "G"}}
          }
        },
        "melody": {
          "context": {"octave": 5, "velocity": 0.8},
          "events": [
            {"t": 1.5, "n": "E", "dur": 1.5, "dynamics": "f"},
            {"t": 3.0, "n": "D", "dur": 1.0, "dynamics": "mf"},
            {"t": 4.0, "n": "C", "dur": 1.0, "dynamics": "mf"}
          ]
        }
      }
    },
    "chorus": {
      "context": {
        "chordProgression": ["F", "C", "G", "Am"],
        "velocity": 0.9,
        "dynamics": "f"
      },
      "layers": {
        "drums": {"pattern": "swingDrums"},
        "bass": {"pattern": "walkingBass"},
        "piano": {"pattern": "pianoComping"}
      }
    }
  },
  "arrangement": ["verse", "chorus", "verse", "chorus"]
}
```

