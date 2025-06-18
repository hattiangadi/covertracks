# Music Pattern Language - Compact Specification

## Core Structure

```json
{
  "title": "Song Title",
  "tempo": 120,
  "chords": { "C": ["C3", "E3", "G3"] },
  "patterns": { "name": [[beat, note, velocity, duration]] },
  "sections": { "name": { "bars": 4, "layers": {...} } },
  "arrangement": ["section1", "section2"]
}
```

## Note Format

**Basic**: `[beat, note, velocity, duration]`
- `beat`: 1.0=downbeat, 1.5=eighth, 2.0=beat 2
- `note`: "C4", "F#3", {"chord": "C"}, {"tone": 1}
- `velocity`: 0.0-1.0 (optional, default: 0.8)
- `duration`: beats (0.25=sixteenth, 0.5=eighth, 1.0=quarter)

## Note Types

### Fixed Notes
```json
[1.0, "C4", 0.9, 0.5]
```

### Chord References  
```json
[1.0, {"chord": "C"}, 0.7, 2.0]
```

### **NEW: Chord Tone References**
```json
[1.0, {"tone": 1, "octave": 2}, 0.8, 0.5]
```
- `tone`: 1=root, 3=third, 5=fifth, 7=seventh, etc.
- `octave`: target octave (optional, default: 4)
- **Automatically adapts to chord context**

## Section Types

### Layers (Recommended)
```json
"section": {
  "bars": 4,
  "layers": {
    "harmony": [chord events],
    "melody": [note events],
    "bass": {"pattern": "bassLine"}
  }
}
```

### Pattern References
```json
"layers": {
  "bass": {"pattern": "walkingBass"}
}
```

### Multi-bar Direct Arrays
```json
"layers": {
  "melody": [
    [1.0, "C4", 0.9, 1.0],  // Bar 1
    [5.0, "F4", 0.9, 1.0]   // Bar 2 (beat 5 = bar 2, beat 1)
  ]
}
```

## Chord Tone Examples

### Adaptive Walking Bass
```json
"patterns": {
  "walkingBass": [
    [1.0, {"tone": 1, "octave": 2}, 0.8, 0.5],  // Root
    [1.5, {"tone": 5, "octave": 2}, 0.6, 0.5],  // Fifth  
    [2.0, {"tone": 3, "octave": 2}, 0.8, 0.5],  // Third
    [2.5, {"tone": 1, "octave": 2}, 0.6, 0.5]   // Root
  ]
}
```

**This pattern automatically becomes:**
- Over C: C2, G2, E2, C2
- Over F: F2, C3, A2, F2  
- Over Am: A2, E3, C3, A2

### Harmonic Comping
```json
"patterns": {
  "comping": [
    [1.0, {"tone": 1, "octave": 4}, 0.7, 0.5],
    [1.75, {"tone": 3, "octave": 4}, 0.6, 0.25],
    [2.25, {"tone": 5, "octave": 4}, 0.7, 0.5]
  ]
}
```

## Complete Example

```json
{
  "title": "Jazz Standard",
  "tempo": 120,
  "chords": {
    "C": ["C3", "E3", "G3"],
    "F": ["F3", "A3", "C4"],
    "G": ["G3", "B3", "D4"]
  },
  "patterns": {
    "walkingBass": [
      [1.0, {"tone": 1, "octave": 2}, 0.8, 0.5],
      [1.5, {"tone": 3, "octave": 2}, 0.6, 0.5],
      [2.0, {"tone": 5, "octave": 2}, 0.8, 0.5],
      [2.5, {"tone": 3, "octave": 2}, 0.6, 0.5]
    ]
  },
  "sections": {
    "verse": {
      "bars": 4,
      "layers": {
        "chords": [
          [1.0, {"chord": "C"}, 0.7, 4.0],
          [5.0, {"chord": "F"}, 0.7, 4.0],
          [9.0, {"chord": "G"}, 0.7, 4.0],
          [13.0, {"chord": "C"}, 0.7, 4.0]
        ],
        "bass": {"pattern": "walkingBass"}
      }
    }
  },
  "arrangement": ["verse"]
}
```

## Key Benefits

- **Chord Tones**: One pattern works with any chord progression
- **Compactness**: Minimal JSON for maximum musical expression
- **Flexibility**: Mix fixed notes, chords, and harmonic references
- **Legibility**: Clear musical relationships and structure

## Chord Context Resolution

Chord tone references automatically find the most recent `{"chord": "name"}` event at or before the current beat to resolve tone numbers to actual pitches.