# Chord Tone References - Language Specification

## Overview

Chord tone references provide a compact way to create harmonic patterns that adapt to chord changes automatically, solving the fixed-position limitation in walking bass and accompaniment patterns.

## Syntax

```json
{"tone": <number>, "octave": <number>}
```

- `tone`: Required. Chord tone number (1-based)
  - 1 = root
  - 3 = third  
  - 5 = fifth
  - 7 = seventh (if defined in chord)
  - Higher numbers cycle through available chord tones
- `octave`: Optional. Target octave (default: 4)

## How It Works

1. **Chord Context**: The parser looks for the most recent `{"chord": "name"}` reference at or before the current beat
2. **Tone Resolution**: Converts tone numbers to actual MIDI notes based on the chord definition
3. **Octave Adjustment**: Places the note in the specified octave
4. **Cycling**: Higher tone numbers (beyond chord size) cycle through available tones with octave shifts

## Examples

### Basic Usage
```json
{
  "chords": {
    "C": ["C3", "E3", "G3"],
    "F": ["F3", "A3", "C4"]
  },
  "sections": {
    "example": {
      "bars": 2,
      "layers": {
        "harmony": [
          [1.0, {"chord": "C"}, 0.7, 2.0],
          [3.0, {"chord": "F"}, 0.7, 2.0]
        ],
        "bass": [
          [1.0, {"tone": 1, "octave": 2}, 0.8, 1.0],  // C2
          [2.0, {"tone": 5, "octave": 2}, 0.8, 1.0],  // G2
          [3.0, {"tone": 1, "octave": 2}, 0.8, 1.0],  // F2
          [4.0, {"tone": 3, "octave": 2}, 0.8, 1.0]   // A2
        ]
      }
    }
  }
}
```

### Walking Bass Pattern
```json
"patterns": {
  "walkingBass": [
    [1.0, {"tone": 1, "octave": 2}, 0.8, 0.5],   // Root
    [1.5, {"tone": 3, "octave": 2}, 0.6, 0.5],   // Third
    [2.0, {"tone": 5, "octave": 2}, 0.8, 0.5],   // Fifth  
    [2.5, {"tone": 3, "octave": 2}, 0.6, 0.5],   // Third
    [3.0, {"tone": 1, "octave": 3}, 0.8, 0.5],   // Root up an octave
    [3.5, {"tone": 5, "octave": 2}, 0.6, 0.5],   // Fifth
    [4.0, {"tone": 3, "octave": 2}, 0.8, 0.5],   // Third
    [4.5, {"tone": 1, "octave": 2}, 0.6, 0.5]    // Root
  ]
}
```

This pattern automatically adapts to any chord progression:
- Over C major: C2, E2, G2, E2, C3, G2, E2, C2
- Over F major: F2, A2, C3, A2, F3, C3, A2, F2  
- Over G major: G2, B2, D3, B2, G3, D3, B2, G2

## Benefits

1. **Compactness**: One pattern works with any chord progression
2. **Musical Coherence**: Always harmonically correct
3. **Implementation Simplicity**: Clear mapping from tone numbers to chord degrees
4. **Legibility**: Easy to understand musical relationships

## Resolution Algorithm

1. **Find Chord Context**: Look backwards from current beat for most recent `{"chord": "name"}`
2. **Get Chord Definition**: Retrieve note array from chords section
3. **Calculate Tone Index**: `(toneNumber - 1) % chordNotes.length`
4. **Parse Base Note**: Convert chord note string to MIDI number
5. **Adjust Octave**: Transpose to requested octave
6. **Handle Extensions**: Add octave shifts for tone numbers beyond chord size

## Error Handling

- **No Chord Context**: Warning logged, note skipped
- **Invalid Chord**: Warning logged, defaults to middle C
- **Invalid Tone Number**: Defaults to root (tone 1)
- **MIDI Range**: Clamps result to 0-127 range

## Comparison with Fixed Notes

### Before (Fixed Position)
```json
"walkingBass": [
  [1.0, "C2", 0.8, 0.5],  // Only works over C chord
  [1.5, "E2", 0.6, 0.5],
  [2.0, "G2", 0.8, 0.5],
  [2.5, "E2", 0.6, 0.5]
]
```

### After (Harmonic Position)  
```json
"walkingBass": [
  [1.0, {"tone": 1, "octave": 2}, 0.8, 0.5],  // Works over any chord
  [1.5, {"tone": 3, "octave": 2}, 0.6, 0.5],
  [2.0, {"tone": 5, "octave": 2}, 0.8, 0.5], 
  [2.5, {"tone": 3, "octave": 2}, 0.6, 0.5]
]
```

The harmonic version automatically adapts to C major, F major, G major, or any other chord while maintaining the same musical relationship pattern.