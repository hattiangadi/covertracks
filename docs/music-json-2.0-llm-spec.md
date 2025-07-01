# Music JSON 2.0 Specification for LLMs

## Overview

Music JSON 2.0 is a universal musical notation format that supports self-describing structure through aliases. It can represent any musical form across cultures by allowing patterns to define their own terminology and relationships.

## Core Concepts

### 1. Self-Describing Aliases
Patterns define their own musical terminology and structure:

```json
{
  "aliases": {
    "verse": {"type": "sequential", "contains": "bars", "count": 16},
    "chorus": {"type": "sequential", "contains": "bars", "count": 8},
    "bar": {"type": "sequential", "contains": "beats", "count": 4}
  }
}
```

### 2. Temporal Relationships
- **simultaneous**: Elements play together (layers, instruments)
- **sequential**: Elements play one after another (bars, beats, variations)
- **named**: Reusable templates (patterns, motifs)
- **structure**: Organizational sections (verse, chorus, movement)

### 3. Universal Container Types
Any musical concept can be defined through aliases:
- Western: verse, chorus, bridge, bar, beat
- Jazz: chorus, section, turnaround, head, solo
- Classical: movement, exposition, development, phrase
- Electronic: drop, buildup, breakdown, loop, step
- Indian: alap, jor, jhala, tala, matra

## Complete Example: "Autumn Jazz" - Multi-Style Composition

```json
{
  "version": "2.0",
  "meta": {
    "title": "Autumn Jazz - Comprehensive Demo",
    "composer": "AI Assistant",
    "style": "Jazz with Classical and Electronic elements",
    "tempo": 120,
    "timeSignature": "4/4",
    "key": "Bb major"
  },
  
  "context": {
    "velocity": 0.75,
    "dynamics": "mf",
    "articulation": "legato"
  },
  
  "aliases": {
    "song": {
      "type": "structure",
      "contains": "sections",
      "count": "variable",
      "description": "Complete jazz composition"
    },
    "verse": {
      "type": "sequential", 
      "contains": "bars",
      "count": 32,
      "pattern": "AABA",
      "description": "32-bar jazz verse form"
    },
    "chorus": {
      "type": "sequential",
      "contains": "bars", 
      "count": 32,
      "description": "Solo chorus over verse changes"
    },
    "bridge": {
      "type": "sequential",
      "contains": "bars",
      "count": 8,
      "description": "Contrasting middle section"
    },
    "bar": {
      "type": "sequential",
      "contains": "beats",
      "count": 4,
      "description": "4/4 time signature"
    },
    "beat": {
      "type": "simultaneous",
      "contains": "subdivisions", 
      "count": 4,
      "description": "Quarter note beat with swing feel"
    }
  },
  
  "patterns": {
    "swingBass": {
      "context": {"octave": 2, "velocity": 0.8, "articulation": "staccato"},
      "events": [
        {"t": 1.0, "d": 1, "dur": 0.75},
        {"t": 2.5, "d": 5, "dur": 0.5},
        {"t": 3.0, "d": 1, "dur": 0.75},
        {"t": 4.5, "d": 5, "dur": 0.5}
      ]
    },
    
    "jazzComping": {
      "context": {"octave": 3, "velocity": 0.6, "dynamics": "mp"},
      "events": [
        {"t": 1.5, "d": 3, "dur": 0.5},
        {"t": 1.5, "d": 7, "dur": 0.5},
        {"t": 2.5, "d": 1, "dur": 0.5},
        {"t": 2.5, "d": 5, "dur": 0.5},
        {"t": 4.0, "d": 3, "dur": 0.25},
        {"t": 4.0, "d": 7, "dur": 0.25}
      ]
    },
    
    "classicalArpeggio": {
      "context": {"octave": 4, "velocity": 0.7, "articulation": "legato"},
      "events": [
        {"t": 1.0, "d": 1, "dur": 0.25},
        {"t": 1.25, "d": 3, "dur": 0.25},
        {"t": 1.5, "d": 5, "dur": 0.25},
        {"t": 1.75, "d": 8, "dur": 0.25},
        {"t": 2.0, "d": 5, "dur": 0.25},
        {"t": 2.25, "d": 3, "dur": 0.25},
        {"t": 2.5, "d": 1, "dur": 0.25},
        {"t": 2.75, "d": 3, "dur": 0.25}
      ]
    },
    
    "electronicRiff": {
      "context": {"octave": 5, "velocity": 0.9, "dynamics": "f"},
      "events": [
        {"t": 1.0, "n": "Bb5", "dur": 0.125, "expression": {"filter": "lowpass", "cutoff": 800}},
        {"t": 1.125, "n": "C6", "dur": 0.125, "expression": {"filter": "lowpass", "cutoff": 1200}},
        {"t": 1.25, "n": "D6", "dur": 0.25, "expression": {"filter": "lowpass", "cutoff": 1600}},
        {"t": 1.5, "n": "F6", "dur": 0.5, "expression": {"filter": "lowpass", "cutoff": 2000}}
      ]
    }
  },
  
  "sections": {
    "intro": {
      "context": {"velocity": 0.65, "dynamics": "mp"},
      "layers": {
        "piano": {
          "bars": {
            "1-4": {
              "pattern": "classicalArpeggio",
              "context": {"chord": "BbMaj7"}
            }
          }
        },
        "bass": {
          "bars": {
            "3-4": {
              "pattern": "swingBass", 
              "context": {"chord": "BbMaj7"}
            }
          }
        }
      }
    },
    
    "head": {
      "context": {"velocity": 0.8, "dynamics": "mf"},
      "layers": {
        "melody": {
          "context": {"octave": 4, "velocity": 0.85, "articulation": "legato"},
          "bars": {
            "1": {"events": [{"t": 1.0, "n": "D4", "dur": 1.0}, {"t": 2.0, "n": "F4", "dur": 2.0}]},
            "2": {"events": [{"t": 1.0, "n": "Bb4", "dur": 2.0}, {"t": 3.0, "n": "G4", "dur": 1.0}]},
            "3": {"events": [{"t": 1.0, "n": "A4", "dur": 1.5}, {"t": 2.5, "n": "F4", "dur": 1.5}]},
            "4": {"events": [{"t": 1.0, "n": "D4", "dur": 4.0}]},
            "5": {"events": [{"t": 1.0, "n": "C4", "dur": 1.0}, {"t": 2.0, "n": "Eb4", "dur": 2.0}]},
            "6": {"events": [{"t": 1.0, "n": "Bb4", "dur": 2.0}, {"t": 3.0, "n": "G4", "dur": 1.0}]},
            "7": {"events": [{"t": 1.0, "n": "F4", "dur": 2.0}, {"t": 3.0, "n": "D4", "dur": 1.0}]},
            "8": {"events": [{"t": 1.0, "n": "Bb3", "dur": 4.0}]},
            "9": {"events": [{"t": 1.0, "n": "F4", "dur": 1.0}, {"t": 2.0, "n": "A4", "dur": 2.0}]},
            "10": {"events": [{"t": 1.0, "n": "C5", "dur": 2.0}, {"t": 3.0, "n": "A4", "dur": 1.0}]},
            "11": {"events": [{"t": 1.0, "n": "Bb4", "dur": 1.5}, {"t": 2.5, "n": "G4", "dur": 1.5}]},
            "12": {"events": [{"t": 1.0, "n": "F4", "dur": 4.0}]},
            "13": {"events": [{"t": 1.0, "n": "D4", "dur": 1.0}, {"t": 2.0, "n": "F4", "dur": 2.0}]},
            "14": {"events": [{"t": 1.0, "n": "Bb4", "dur": 2.0}, {"t": 3.0, "n": "G4", "dur": 1.0}]},
            "15": {"events": [{"t": 1.0, "n": "A4", "dur": 1.5}, {"t": 2.5, "n": "F4", "dur": 1.5}]},
            "16": {"events": [{"t": 1.0, "n": "Bb4", "dur": 4.0}]}
          }
        },
        "comping": {
          "bars": {
            "1-8": {"pattern": "jazzComping", "context": {"chord": "BbMaj7"}},
            "9-12": {"pattern": "jazzComping", "context": {"chord": "FMaj7"}}, 
            "13-16": {"pattern": "jazzComping", "context": {"chord": "BbMaj7"}}
          }
        },
        "bass": {
          "bars": {
            "1-8": {"pattern": "swingBass", "context": {"chord": "BbMaj7"}},
            "9-12": {"pattern": "swingBass", "context": {"chord": "FMaj7"}},
            "13-16": {"pattern": "swingBass", "context": {"chord": "BbMaj7"}}
          }
        }
      }
    },
    
    "piano_solo": {
      "context": {"velocity": 0.85, "dynamics": "f"},
      "layers": {
        "piano": {
          "context": {"octave": 4, "velocity": 0.9},
          "bars": {
            "1-4": {
              "events": [
                {"t": 1.0, "n": "D4", "dur": 0.5},
                {"t": 1.5, "n": "F4", "dur": 0.5},
                {"t": 2.0, "n": "Bb4", "dur": 0.5},
                {"t": 2.5, "n": "D5", "dur": 0.5},
                {"t": 3.0, "n": "F5", "dur": 1.0},
                {"t": 4.0, "n": "D5", "dur": 1.0}
              ]
            },
            "5-8": {
              "variations": {
                "5": {"events": [{"t": 1.0, "n": "C5", "dur": 0.25}, {"t": 1.25, "n": "Bb4", "dur": 0.25}, {"t": 1.5, "n": "A4", "dur": 0.5}]},
                "6": {"events": [{"t": 1.0, "n": "Bb4", "dur": 2.0}, {"t": 3.0, "n": "F4", "dur": 1.0}]},
                "7": {"events": [{"t": 1.0, "n": "G4", "dur": 0.75}, {"t": 1.75, "n": "A4", "dur": 0.25}, {"t": 2.0, "n": "Bb4", "dur": 1.0}]},
                "8": {"events": [{"t": 1.0, "n": "D4", "dur": 4.0}]}
              }
            }
          }
        },
        "comping": {
          "bars": {
            "1-8": {"pattern": "jazzComping", "context": {"chord": "BbMaj7"}}
          }
        },
        "bass": {
          "bars": {
            "1-8": {"pattern": "swingBass", "context": {"chord": "BbMaj7"}}
          }
        }
      }
    },
    
    "electronic_bridge": {
      "context": {"velocity": 0.9, "dynamics": "f"},
      "layers": {
        "synth": {
          "bars": {
            "1-4": {"pattern": "electronicRiff", "context": {"chord": "GmMaj7"}},
            "5-8": {"pattern": "electronicRiff", "context": {"chord": "CMaj7"}}
          }
        },
        "piano": {
          "bars": {
            "1-8": {"pattern": "classicalArpeggio", "context": {"chord": "GmMaj7", "velocity": 0.5}}
          }
        }
      }
    },
    
    "outro": {
      "context": {"velocity": 0.6, "dynamics": "mp"},
      "layers": {
        "piano": {
          "bars": {
            "1-2": {"pattern": "classicalArpeggio", "context": {"chord": "BbMaj7"}},
            "3-4": {
              "events": [{"t": 1.0, "n": "Bb3", "dur": 8.0}],
              "context": {"dynamics": "pp", "articulation": "fermata"}
            }
          }
        }
      }
    }
  },
  
  "arrangement": ["intro", "head", "piano_solo", "electronic_bridge", "head", "outro"],
  
  "chordProgression": [
    {"bar": 1, "chord": "BbMaj7"},
    {"bar": 9, "chord": "FMaj7"},
    {"bar": 13, "chord": "BbMaj7"}
  ],
  
  "performance": {
    "swing": 0.67,
    "rubato": 0.1,
    "dynamics": {
      "crescendo": {"start": "intro", "end": "electronic_bridge"},
      "diminuendo": {"start": "electronic_bridge", "end": "outro"}
    }
  }
}
```

## Key Features Demonstrated

### 1. **Self-Describing Structure**
- Custom aliases define the song's formal structure
- Universal terminology that adapts to any musical style
- Quantitative relationships (verse=32 bars, bridge=8 bars)

### 2. **Multi-Style Integration**
- **Jazz**: Swing bass patterns, jazz comping, AABA form
- **Classical**: Arpeggiated accompaniment, dynamic markings
- **Electronic**: Synthesizer riffs with filter modulation

### 3. **Advanced Pattern System**
- Reusable patterns (`swingBass`, `jazzComping`, `classicalArpeggio`)
- Pattern variations with different chord contexts
- Cross-style pattern application

### 4. **Sophisticated Timing**
- Complex rhythmic subdivision (swing eighths, triplets)
- Mixed articulations (legato melody, staccato bass)
- Dynamic tempo and rubato expression

### 5. **Harmonic Sophistication**
- Chord progressions with jazz extensions (BbMaj7, FMaj7)
- Degree-based notation (d: 1, 3, 5) for harmonic flexibility
- Context-aware chord changes

### 6. **Expressive Control**
- Multi-layer dynamics (pp to f)
- Expression maps for electronic processing
- Performance directives (fermata, crescendo)

### 7. **Structural Flexibility**
- Complex arrangement with repeated and varied sections
- Mixed time signatures and pickup bars
- Nested variations within sections

## Bar Numbering Rules

### **CONTAINER-LOCAL BAR NUMBERING** (CORRECT)
Bar numbers restart at 1 within each container that contains bars:

```json
{
  "sections": {
    "intro": {
      "bars": {
        "1": {...}, "2": {...}, "3": {...}, "4": {...}  // Bars 1-4 within intro
      }
    },
    "head": {
      "bars": {
        "1": {...}, "2": {...}, "3": {...}, "4": {...}  // Bars 1-4 within head (NOT 5-8!)
      }
    },
    "outro": {
      "bars": {
        "1": {...}  // Bar 1 within outro (NOT bar 9!)
      }
    }
  },
  "arrangement": ["intro", "head", "outro"]  // Arrangement defines the sequence
}

// Also applies to other containers with bars:
{
  "layers": {
    "melody": {
      "bars": {
        "1": {...}, "2": {...}  // Bars 1-2 within melody layer
      }
    },
    "bass": {
      "bars": {
        "1": {...}, "2": {...}  // Bars 1-2 within bass layer (separate from melody)
      }
    }
  }
}
```

### **GLOBAL SEQUENTIAL NUMBERING** ❌ (INCORRECT)
**NEVER** use sequential numbering across the entire song:

```json
{
  "sections": {
    "intro": {
      "bars": {
        "1": {...}, "2": {...}, "3": {...}, "4": {...}  // ❌ WRONG
      }
    },
    "head": {
      "bars": {
        "5": {...}, "6": {...}, "7": {...}, "8": {...}  // ❌ WRONG - should be 1-4
      }
    },
    "outro": {
      "bars": {
        "9": {...}  // ❌ WRONG - should be 1
      }
    }
  }
}
```

### Why Container-Local Numbering?

1. **Musical Logic**: Musicians think "bar 3 of the verse" or "bar 2 of the melody layer", not "bar 19 of the song"
2. **Reusability**: Containers can be rearranged without renumbering bars
3. **Pattern References**: Ranges like "1-4" work within each container's context
4. **Readability**: Clearer structure and easier debugging
5. **Flexibility**: Works for sections, layers, variations, or any container with bars

## LLM Guidelines for Music JSON 2.0

### When Creating Musical Content:

1. **Start with Aliases**: Define the musical terminology and structure first
2. **Use Patterns**: Create reusable musical elements before building sections
3. **Layer Thoughtfully**: Use simultaneous layers for ensemble texture
4. **Sequence Logically**: Use sequential containers for temporal progression
5. **Context Inheritance**: Let child elements inherit and override parent context
6. **Cultural Adaptation**: Adjust aliases and terminology for different musical traditions
7. **Container-Local Bar Numbering**: Always restart bar numbers at 1 within each container

### Best Practices:

1. **Realistic Timing**: Use musically meaningful beat divisions
2. **Proper Voice Leading**: Connect chord tones smoothly in harmonic progressions
3. **Dynamic Expression**: Vary velocity, dynamics, and articulation for musicality
4. **Structural Balance**: Create proportional sections with logical transitions
5. **Pattern Variation**: Reuse patterns with contextual modifications for coherence
6. **Bar Numbering**: Never use global sequential numbering - always container-local

### Common Mistakes to Avoid:

1. **❌ Global Bar Numbers**: Don't number bars 1-32 across multiple containers
2. **❌ Missing Arrangement**: Always define the arrangement field to sequence sections
3. **❌ Range Confusion**: "1-4" means bars 1-4 within the current container context
4. **❌ Pattern Scope**: Patterns are global, containers are local - don't mix these concepts

This system enables AI to generate sophisticated, culturally-appropriate musical content across any genre or tradition while maintaining technical precision and expressive depth.