#!/usr/bin/env python3

import json
import subprocess
import tempfile
import os

# The comprehensive Autumn Jazz pattern from the spec
autumn_jazz = {
  "version": "2.0",
  "meta": {
    "title": "Autumn Jazz - Comprehensive Demo",
    "composer": "AI Assistant",
    "style": "Jazz with Classical and Electronic elements",
    "tempo": 120
  },
  
  "context": {
    "velocity": 0.75,
    "dynamics": "mf"
  },
  
  "aliases": {
    "verse": {
      "type": "sequential", 
      "contains": "bars",
      "count": 32
    },
    "chorus": {
      "type": "sequential",
      "contains": "bars", 
      "count": 32
    },
    "bridge": {
      "type": "sequential",
      "contains": "bars",
      "count": 8
    },
    "bar": {
      "type": "sequential",
      "contains": "beats",
      "count": 4
    }
  },
  
  "patterns": {
    "swingBass": {
      "context": {"octave": 2, "velocity": 0.8},
      "events": [
        {"t": 1.0, "d": 1, "dur": 0.75},
        {"t": 2.5, "d": 5, "dur": 0.5}
      ]
    },
    
    "jazzComping": {
      "context": {"octave": 3, "velocity": 0.6},
      "events": [
        {"t": 1.5, "d": 3, "dur": 0.5},
        {"t": 1.5, "d": 7, "dur": 0.5}
      ]
    }
  },
  
  "sections": {
    "intro": {
      "context": {"velocity": 0.65},
      "bars": {
        "1": {"events": [{"t": 1.0, "n": "C4", "dur": 1.0}]},
        "2": {"events": [{"t": 1.0, "n": "D4", "dur": 1.0}]},
        "3": {"events": [{"t": 1.0, "n": "E4", "dur": 1.0}]},
        "4": {"events": [{"t": 1.0, "n": "F4", "dur": 1.0}]}
      }
    },
    
    "head": {
      "context": {"velocity": 0.8},
      "layers": {
        "melody": {
          "bars": {
            "1": {"events": [{"t": 1.0, "n": "G4", "dur": 1.0}]},
            "2": {"events": [{"t": 1.0, "n": "A4", "dur": 1.0}]},
            "3": {"events": [{"t": 1.0, "n": "B4", "dur": 1.0}]},
            "4": {"events": [{"t": 1.0, "n": "C5", "dur": 1.0}]}
          }
        },
        "comping": {
          "bars": {
            "1-4": {"pattern": "jazzComping", "context": {"chord": "BbMaj7"}}
          }
        }
      }
    },
    
    "outro": {
      "bars": {
        "1": {"events": [{"t": 1.0, "n": "Bb3", "dur": 4.0}]}
      }
    }
  },
  
  "arrangement": ["intro", "head", "outro"]
}

# Write to temp file and test with Covertracks
with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
    json.dump(autumn_jazz, f, indent=2)
    temp_file = f.name

try:
    # Run the test (this is a placeholder - you'd need to implement actual testing)
    print(f"Testing Autumn Jazz pattern...")
    print(f"Written to: {temp_file}")
    print("Pattern has:")
    print(f"- {len(autumn_jazz['sections'])} sections")
    print(f"- {len(autumn_jazz['patterns'])} patterns") 
    print(f"- {len(autumn_jazz['arrangement'])} arrangement items")
    
    # Calculate expected notes
    intro_bars = 4  # 4 bars, 1 note each = 4 notes
    head_melody_bars = 4  # 4 bars, 1 note each = 4 notes  
    head_comping_bars = 4  # 1 pattern with 2 notes per bar = 8 notes
    outro_bars = 1  # 1 bar, 1 note = 1 note
    
    expected_notes = intro_bars + head_melody_bars + head_comping_bars + outro_bars
    print(f"Expected approximately {expected_notes} notes")
    
finally:
    # Clean up
    os.unlink(temp_file)