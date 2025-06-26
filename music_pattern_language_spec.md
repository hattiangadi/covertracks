# Music Pattern Language - Complete Specification

## Core Structure

```json
{
  "title": "Song Title",
  "tempo": 120,
  "chords": { "C": ["C3", "E3", "G3"] },
  "patterns": { "name": [[beat, note, velocity, duration]] },
  "sections": { "name": { "bars": 4, "layers": {...} } },
  "arrangement": ["section1", "section2", "section1", "outro"]
}
```

## Note Format

**Basic**: `[beat, note, velocity, duration, source]`
- `beat`: Float position in bar (1.0=downbeat, 1.5=eighth, 2.0=beat 2, 3.75=16th before beat 4)
- `note`: "C4", "F#3", {"chord": "C"}, {"tone": 1}, "kick", "kick+snare"
- `velocity`: 0.0-1.0 (optional, default: 0.8)
- `duration`: beats as float (0.25=sixteenth, 0.5=eighth, 1.0=quarter)
- `source`: metadata string (optional, auto-generated)

**Alternative Formats**:
- `[beat, note]` - velocity/duration use defaults
- `[beat, note, velocity]` - duration uses default
- `[beat, "C4", "0.8", "1.5"]` - strings converted to floats

## Note Types

### Fixed Notes
```json
[1.0, "C4", 0.9, 0.5]
[2.5, "F#5", 0.7, 0.25]  // Sharp notation
[3.0, "Bb3", 0.8, 1.0]   // Flat notation
```

### Percussion Primitives
```json
[1.0, "kick", 0.9]        // Duration auto-set to 0.1-0.3
[2.0, "snare", 0.8]
[3.0, "kick+closed_hat"]  // Combined primitives at same position
[4.0, "crash:v127"]       // Inline velocity notation
```

### Chord References  
```json
[1.0, {"chord": "C"}, 0.7, 2.0]
[5.0, {"chord": "Am7"}, 0.6, 4.0]
```

### Chord Tone References
```json
[1.0, {"tone": 1, "octave": 2}, 0.8, 0.5]  // Root in octave 2
[1.5, {"tone": 3}, 0.6, 0.5]               // Third in default octave 4
[2.0, {"tone": 5, "octave": 3}, 0.8, 0.5]  // Fifth in octave 3
[2.5, {"tone": 7}, 0.6, 0.5]               // Seventh
[3.0, {"tone": 9}, 0.7, 0.5]               // Ninth (wraps with octave shift)
```
- `tone`: 1=root, 3=third, 5=fifth, 7=seventh, 9=ninth, etc.
- `octave`: target octave (optional, default: 4)
- Automatically adapts to current chord context
- Tones > chord size wrap around with octave transposition

## Pattern Variations (NEW)

### Basic Variations
```json
"patterns": {
  "basic_beat": [
    [1.0, "kick", 0.9],
    [2.0, "snare", 0.8],
    [3.0, "kick", 0.7],
    [4.0, "snare", 0.8]
  ],
  "fill_1": [[3.5, "snare", 0.7], [3.75, "snare", 0.8], [4.0, "crash", 0.9]]
}
```

### Using Variations in Sections
```json
"sections": {
  "verse": {
    "bars": 8,
    "layers": {
      "drums": {
        "basePattern": "basic_beat",
        "variations": {
          "bar4": "fill_1",           // Reference to pattern
          "bar8": [[3.0, "kick", 0.9], [3.5, "snare", 0.8], [4.0, "crash"]]  // Inline variation
        }
      }
    }
  }
}
```

## Section Types

### Layer-Based Sections
```json
"section": {
  "bars": 4,
  "layers": {
    "harmony": [chord events],
    "melody": [note events],
    "bass": {"pattern": "bassLine"},
    "drums": {
      "basePattern": "basic_beat",
      "variations": { "bar4": "fill" }
    }
  }
}
```

### Multi-Bar Arrays
```json
"layers": {
  "melody": [
    [1.0, "C4", 0.9, 1.0],   // Bar 1, beat 1
    [3.5, "E4", 0.8, 0.5],   // Bar 1, beat 3.5
    [5.0, "F4", 0.9, 1.0],   // Bar 2, beat 1 (beat 5 = bar 2)
    [7.5, "G4", 0.8, 0.5],   // Bar 2, beat 3.5
    [9.0, "A4", 0.9, 2.0]    // Bar 3, beat 1
  ]
}
```

### Pattern References
```json
"layers": {
  "bass": {"pattern": "walkingBass"},
  "drums": {"pattern": "rock_beat"}
}
```

## Time Signatures

### Global Time Signature
```json
{
  "timeSignature": "6/8",
  "tempo": 90
}
```

### Per-Bar Time Signatures
```json
"sections": {
  "complex": {
    "bars": 4,
    "timeSignatures": ["4/4", "3/4", "4/4", "5/4"],
    "layers": {...}
  }
}
```

## Arrangement

### Section Ordering
```json
"arrangement": ["intro", "verse", "chorus", "verse", "chorus", "bridge", "chorus", "outro"]
```
- Sections can be repeated
- Arrangement defines the complete song structure
- Each section reference uses its full bar count

## Advanced Features

### Dynamic Velocity & Duration

**Automatic Duration Assignment**:
- Percussion: 0.1-0.3 seconds (with random variation)
- Bass range (< MIDI 48): 0.6 seconds
- Mid range (48-72): 0.8 seconds  
- High range (> 72): 0.5 seconds
- Non-percussion gets ±20% random variation

**Velocity Patterns**:
```json
"patterns": {
  "dynamic_melody": [
    [1.0, "C4", 0.9, 1.0],    // Forte
    [2.0, "E4", 0.7, 0.5],    // Mezzo-forte
    [2.5, "G4", 0.5, 0.5],    // Mezzo-piano
    [3.0, "C5", 0.3, 1.0]     // Piano
  ]
}
```

### Boundary Handling

**Beat Position Rules**:
- Valid range: 1.0 to (bars * beatsPerBar - 0.01)
- Notes outside boundaries are skipped with warnings
- Durations clamped to prevent bar crossing
- Minimum duration: 0.1 beats

**Multi-Bar Distribution**:
- Events automatically assigned to correct bar based on beat position
- Bar 1: beats 1.0-4.99, Bar 2: beats 5.0-8.99, etc.

### Channel Management

**Automatic Channel Assignment**:
- Drums/Percussion → Channel 10 (MIDI standard)
- All other instruments → Channel 1
- Based on primitive type detection

### Error Handling

**Graceful Fallbacks**:
- Unknown primitives → Middle C (MIDI 60)
- Missing chord definitions → Skipped with warning
- Invalid JSON → Returns null pattern
- Out-of-range notes → Clamped to MIDI 0-127

## Complete Musical Examples

*Each instrument gets its own chart - these examples demonstrate all language features in musical context.*

### Drums - Complete Groove Architecture

```json
{
  "title": "Modern Drum Arrangement",
  "tempo": 120,
  "primitives": {
    "kick": 36, "snare": 38, "ride": 51, "crash": 49,
    "high_tom": 50, "mid_tom": 47, "floor_tom": 43,
    "hihat": 42, "open_hihat": 46
  },
  "patterns": {
    "main_groove": [
      [1.0, "kick", 0.9],                    // Strong downbeat
      [1.5, "hihat", 0.4],                   // Ghost hihat
      [2.0, "snare", 0.8],                   // Backbeat
      [2.5, "kick", 0.7],                    // Syncopated kick
      [3.0, "kick+hihat", 0.8],              // Combined sounds
      [3.5, "hihat", 0.3],                   // Light ghost
      [4.0, "snare", 0.9],                   // Strong backbeat
      [4.5, "kick", 0.6]                     // Setup for next bar
    ],
    "tom_build": [
      [3.0, "high_tom", 0.7],
      [3.25, "high_tom", 0.8],
      [3.5, "mid_tom", 0.8],
      [3.75, "floor_tom", 0.9],
      [4.0, "crash+kick", 1.0]               // Explosive accent
    ],
    "breakdown": [
      [1.0, "kick", 0.9, 0.1],               // Short, punchy notes
      [2.5, "snare", 0.6, 0.1],              // Sparse pattern
      [4.0, "kick", 0.8, 0.1]
    ]
  },
  "sections": {
    "verse": {
      "bars": 8,
      "layers": {
        "drums": {
          "basePattern": "main_groove",       // Consistent foundation
          "variations": {
            "bar4": "tom_build",              // Build tension
            "bar7": "breakdown",              // Dynamic contrast
            "bar8": "tom_build"               // Return to power
          }
        }
      }
    }
  },
  "arrangement": ["verse", "verse"]          // Song structure
}
```

**How it works:** The `basePattern` provides consistent groove while `variations` create dynamics. Combined primitives (`kick+crash`) create powerful accents. Velocity changes (0.3-1.0) control dynamics. Precise timing (1.25, 3.75) creates feel.

### Bass - Adaptive Harmonic Foundation

```json
{
  "title": "Bass Line with Harmonic Movement",
  "tempo": 120,
  "patterns": {
    "walking_foundation": [
      [1.0, {"tone": 1, "octave": 2}, 0.8, 0.9],    // Root - strong foundation
      [2.0, {"tone": 3, "octave": 2}, 0.7, 0.9],    // Third - harmonic color
      [3.0, {"tone": 5, "octave": 2}, 0.8, 0.9],    // Fifth - stable
      [4.0, {"tone": 1, "octave": 3}, 0.7, 0.9]     // Octave - upward motion
    ],
    "chromatic_transition": [
      [1.0, {"tone": 1, "octave": 2}, 0.8, 0.9],    // Chord tone start
      [2.0, {"tone": 3, "octave": 2}, 0.7, 0.9],
      [3.0, "F#2", 0.6, 0.5],                       // Fixed chromatic note
      [3.5, "G2", 0.7, 0.5],                        // Leading to next chord
      [4.0, {"tone": 1, "octave": 2}, 0.8, 0.9]     // Back to chord tones
    ],
    "funk_pocket": [
      [1.0, {"tone": 1, "octave": 2}, 0.9, 0.2],    // Punchy attack
      [1.75, {"tone": 1, "octave": 2}, 0.4, 0.1],   // Ghost note feel
      [2.25, {"tone": 1, "octave": 3}, 0.8, 0.2],   // Octave pop
      [3.0, {"tone": 1, "octave": 2}, 0.9, 0.3],
      [4.0, {"tone": 5, "octave": 2}, 0.8, 0.5]     // Fifth for resolution
    ]
  },
  "sections": {
    "progression": {
      "bars": 12,
      "layers": {
        "bass": {
          "basePattern": "walking_foundation",        // Harmonic foundation
          "variations": {
            "bar3": "chromatic_transition",           // Smooth voice leading
            "bar6": "chromatic_transition",
            "bar9": "funk_pocket",                    // Rhythmic variation
            "bar11": "chromatic_transition"           // Turnaround
          }
        }
      }
    }
  }
}
```

**How it works:** Chord tones (`{"tone": 1}`) adapt to any harmony while fixed notes (`"F#2"`) provide specific chromatic movement. Different octaves create register variation. Velocity and duration changes create articulation differences (punchy vs. smooth).

### Piano - Harmonic Comping and Voicing

```json
{
  "title": "Piano Comping Styles",
  "tempo": 120,
  "patterns": {
    "shell_voicing": [
      [2.0, {"tone": 3, "octave": 4}, 0.6, 0.4],    // Third in right hand
      [2.0, {"tone": 7, "octave": 3}, 0.5, 0.4],    // Seventh in left hand
      [3.5, {"tone": 5, "octave": 4}, 0.5, 0.3],    // Fifth - lighter touch
      [4.25, {"tone": 1, "octave": 4}, 0.6, 0.3]    // Root anticipation
    ],
    "full_chord_stab": [
      [1.0, {"tone": 1, "octave": 3}, 0.7, 0.5],    // Full chord voicing
      [1.0, {"tone": 3, "octave": 4}, 0.7, 0.5],    // Multiple simultaneous notes
      [1.0, {"tone": 5, "octave": 4}, 0.6, 0.5],    // for rich harmony
      [1.0, {"tone": 7, "octave": 4}, 0.6, 0.5]
    ],
    "latin_montuno": [
      [1.0, {"tone": 1, "octave": 4}, 0.7, 0.2],    // Rhythmic ostinato
      [1.5, {"tone": 3, "octave": 4}, 0.6, 0.2],    // using chord tones
      [2.0, {"tone": 5, "octave": 4}, 0.7, 0.2],
      [2.5, {"tone": 3, "octave": 4}, 0.6, 0.2],
      [3.0, {"tone": 1, "octave": 4}, 0.7, 0.2],
      [3.5, {"tone": 5, "octave": 4}, 0.6, 0.2],
      [4.0, {"tone": 3, "octave": 4}, 0.7, 0.2],
      [4.5, {"tone": 1, "octave": 4}, 0.6, 0.2]
    ]
  },
  "sections": {
    "comping_section": {
      "bars": 8,
      "layers": {
        "piano": {
          "basePattern": "shell_voicing",             // Sparse, supportive
          "variations": {
            "bar2": "full_chord_stab",               // Accent moments
            "bar5": "latin_montuno",                 // Rhythmic drive
            "bar6": "latin_montuno",
            "bar8": "full_chord_stab"                // Strong ending
          }
        }
      }
    }
  }
}
```

**How it works:** Simultaneous notes at same beat create chords. Different octaves separate left/right hand parts. Chord tones automatically voice any progression. Rhythmic variations change the feel from sparse to driving.

### Electric Guitar - Power and Texture

```json
{
  "title": "Guitar Rhythm and Lead",
  "tempo": 120,
  "patterns": {
    "power_chord_rhythm": [
      [1.0, {"tone": 1, "octave": 3}, 0.8, 0.5],    // Root of power chord
      [1.0, {"tone": 5, "octave": 3}, 0.8, 0.5],    // Fifth (simultaneous)
      [2.5, {"tone": 1, "octave": 3}, 0.7, 0.2],    // Rhythmic chops
      [2.5, {"tone": 5, "octave": 3}, 0.7, 0.2],
      [3.25, {"tone": 1, "octave": 3}, 0.9, 0.3],
      [3.25, {"tone": 5, "octave": 3}, 0.9, 0.3]
    ],
    "lead_phrase": [
      [1.0, {"tone": 5, "octave": 5}, 0.8, 0.5],    // High register lead
      [1.75, {"tone": 1, "octave": 6}, 0.7, 0.25],  // Scalar movement
      [2.25, {"tone": 7, "octave": 5}, 0.6, 0.25],  // Chord extensions
      [2.75, {"tone": 3, "octave": 5}, 0.8, 1.0],   // Sustained bend
      [4.0, {"tone": 1, "octave": 5}, 0.9, 1.0]     // Resolution
    ],
    "clean_arpeggios": [
      [1.0, {"tone": 1, "octave": 4}, 0.6, 0.25],   // Clean fingerpicked
      [1.25, {"tone": 3, "octave": 4}, 0.5, 0.25],  // arpeggiation
      [1.5, {"tone": 5, "octave": 4}, 0.6, 0.25],
      [1.75, {"tone": 1, "octave": 5}, 0.5, 0.25],
      [2.0, {"tone": 3, "octave": 5}, 0.6, 0.25]
    ]
  },
  "sections": {
    "guitar_arrangement": {
      "bars": 8,
      "layers": {
        "guitar": {
          "basePattern": "power_chord_rhythm",       // Driving rhythm foundation
          "variations": {
            "bar3": "clean_arpeggios",              // Textural contrast
            "bar4": "clean_arpeggios",
            "bar7": "lead_phrase",                  // Solo moment
            "bar8": "lead_phrase"
          }
        }
      }
    }
  }
}
```

**How it works:** Power chords use simultaneous root+fifth. Different octaves create register separation (rhythm vs. lead). Sustained notes (1.0 duration) vs. staccato (0.2) create articulation variety.

## Musical Techniques → Language Features

### **Creating Groove and Feel**
- **Swing feel**: Use triplet subdivisions (1.33, 2.33, 3.33)
- **Ghost notes**: Low velocity (0.3-0.5) for subtle percussion
- **Accents**: High velocity (0.9-1.0) for emphasis
- **Syncopation**: Off-beat timing (1.5, 2.5, 3.5)
- **Combined hits**: `"kick+crash"` for powerful moments

### **Harmonic Movement**
- **Chord-following patterns**: `{"tone": 1}` adapts to any chord
- **Walking bass**: Root→third→fifth→seventh progression
- **Shell voicings**: Third+seventh for jazz piano
- **Power chords**: Root+fifth simultaneously
- **Chromatic approaches**: Fixed notes (`"F#2"`) leading to chord tones

### **Dynamics and Articulation**
- **Forte/piano**: Velocity 0.9 vs 0.3
- **Staccato**: Short duration (0.1-0.2)
- **Legato**: Long duration (0.8-1.0)
- **Ghost notes**: Low velocity + short duration
- **Sustained notes**: Long duration for pads/leads

### **Structural Variety**
- **Consistent groove**: Use `basePattern` for foundation
- **Fills and breaks**: Use `variations` for specific bars
- **Build tension**: Gradually increase velocity/complexity
- **Dynamic contrast**: Alternate dense/sparse patterns
- **Section transitions**: Fills on bar 4, 8, etc.

### **Register and Texture**
- **Bass foundation**: Octave 2-3 for low end
- **Harmonic middle**: Octave 3-4 for chords
- **Melodic lead**: Octave 5-6 for lead lines
- **Simultaneous notes**: Same beat position for chords
- **Voice separation**: Different octaves for left/right hand

## Key Features Summary

1. **Pattern Variations**: Apply different patterns or events to specific bars
2. **Chord Tone References**: Write patterns that adapt to any chord progression
3. **Multi-Bar Support**: Events can span multiple bars with automatic distribution
4. **Time Signature Flexibility**: Global or per-bar time signatures
5. **Arrangement Control**: Define complete song structure
6. **Intelligent Defaults**: Automatic duration and channel assignment
7. **Combined Primitives**: Multiple sounds at same position with `+`
8. **Inline Velocity**: Quick velocity notation with `:v`
9. **Source Tracking**: Automatic metadata for debugging
10. **Graceful Error Handling**: Fallbacks for missing or invalid data

## Implementation Notes

- Beat positions use 1-based indexing (bar starts at beat 1.0)
- All times are in beats, not seconds
- MIDI channel 10 (index 9) reserved for drums
- Pattern variations override base pattern for specified bars
- Chord context determined by most recent chord event
- Events tagged with source metadata for tracking