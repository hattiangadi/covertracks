# Flexible Quantitative Aliases

## The Reality of Musical Structure

Musical forms have **typical patterns** with **individual exceptions**:
- Most bars are 4/4, but some might be 2/4 or 3/4
- Most verses are 16 bars, but intro might be 4 bars
- Most sections follow the pattern, but there might be extensions or cuts

## Default + Override Model

### Aliases Define Defaults
```json
{
  "aliases": {
    "song": {"contains": "sections", "count": "variable"},
    "section": {"contains": "bars", "count": 8, "default": true},
    "bar": {"contains": "beats", "count": 4, "default": true}
  }
}
```

### Individual Containers Override
```json
{
  "song": {
    "intro": {
      "duration": 4,  // Override: 4 bars instead of default 8
      "bars": {"1-4": {...}}
    },
    "verse": {
      // Uses default: 8 bars
      "bars": {"1-8": {...}}
    },
    "chorus": {
      "duration": 12,  // Override: 12 bars instead of default 8
      "bars": {"1-12": {...}}
    },
    "bridge": {
      // Uses default: 8 bars
      "bars": {
        "1-7": {"beats": 4},      // Normal 4/4 bars
        "8": {"beats": 2}         // Override: 2/4 pickup bar
      }
    }
  }
}
```

## Time Signature Variations

### Mixed Meters in Classical
```json
{
  "aliases": {
    "movement": {"contains": "bars", "count": "variable"},
    "bar": {"contains": "beats", "count": 4, "default": true}  // Default 4/4
  },
  
  "movement": {
    "exposition": {
      "bars": {
        "1-16": {"beats": 4},     // 16 bars of 4/4
        "17": {"beats": 3},       // 1 bar of 3/4 
        "18-32": {"beats": 4}     // Back to 4/4
      }
    }
  }
}
```

### Jazz with Pickup Bars
```json
{
  "aliases": {
    "chorus": {"contains": "sections", "count": 4, "pattern": "AABA"},
    "section": {"contains": "bars", "count": 8, "default": true},
    "bar": {"contains": "beats", "count": 4, "default": true}
  },
  
  "chorus": {
    "A": {
      "bars": {
        "1": {"beats": 2},        // 2/4 pickup bar
        "2-8": {"beats": 4}       // Normal 4/4 bars
      }
    },
    "A2": {
      // Uses default: 8 bars of 4/4
    },
    "B": {
      "duration": 10,             // Override: 10-bar B section
      "bars": {"1-10": {...}}
    },
    "A3": {
      // Uses default: 8 bars of 4/4
    }
  }
}
```

## Section Length Variations

### Pop Song with Variable Sections
```json
{
  "aliases": {
    "song": {"contains": "sections", "count": "variable"},
    "verse": {"contains": "bars", "count": 16, "default": true},
    "chorus": {"contains": "bars", "count": 8, "default": true},
    "bridge": {"contains": "bars", "count": 8, "default": true}
  },
  
  "song": {
    "intro": {
      "type": "verse",            // Inherit verse structure
      "duration": 4,              // But only 4 bars
      "bars": {"1-4": {...}}
    },
    "verse1": {
      // Uses default: 16 bars
      "bars": {"1-16": {...}}
    },
    "chorus1": {
      // Uses default: 8 bars
      "bars": {"1-8": {...}}
    },
    "verse2": {
      "duration": 12,             // Shorter verse: 12 bars
      "bars": {"1-12": {...}}
    },
    "chorus2": {
      "duration": 12,             // Extended chorus: 12 bars
      "bars": {"1-12": {...}}
    },
    "outro": {
      "type": "chorus",           // Based on chorus
      "duration": 4,              // But fade out after 4 bars
      "bars": {"1-4": {...}}
    }
  }
}
```

## Electronic Music with Loop Variations

### Different Loop Lengths
```json
{
  "aliases": {
    "track": {"contains": "sections", "count": "variable"},
    "section": {"contains": "loops", "count": "variable"},
    "loop": {"contains": "steps", "count": 16, "default": true},
    "step": {"contains": "triggers", "count": 1, "default": true}
  },
  
  "track": {
    "intro": {
      "loops": {
        "kick": {
          "steps": 8,             // Half-length loop: 8 steps
          "pattern": {"1,5": {"triggers": [{"sample": "kick.wav"}]}}
        }
      }
    },
    "drop": {
      "loops": {
        "kick": {
          // Uses default: 16 steps
          "pattern": {"1,5,9,13": {...}}
        },
        "snare": {
          "steps": 32,            // Double-length loop: 32 steps  
          "pattern": {"9,25": {...}}
        }
      }
    }
  }
}
```

## Duration Calculation Logic

### Smart Duration Calculation
```cpp
double calculateSectionDuration(const MusicalContainer& section, const AliasDefinition& alias) {
    // 1. Check for explicit duration override
    if (section.hasProperty("duration")) {
        return section.getProperty("duration");
    }
    
    // 2. Calculate from actual content
    if (!section.isEmpty()) {
        return analyzeContentDuration(section);
    }
    
    // 3. Use alias default
    if (alias.count.isInt()) {
        double childDuration = getChildDefaultDuration(alias.contains);
        return alias.count * childDuration;
    }
    
    // 4. Fallback to global default
    return alias.defaultDuration;
}
```

### Content Analysis
```cpp
double analyzeContentDuration(const MusicalContainer& container) {
    double maxTime = 0.0;
    
    // Check all bars and find the highest numbered one
    for (const auto& [barName, bar] : container.variations) {
        if (barName.startsWith("bar") || barName.contains("-")) {
            int barNumber = extractHighestBarNumber(barName);
            maxTime = std::max(maxTime, (double)barNumber);
        }
    }
    
    // Check explicit time ranges like "1-16"
    for (const auto& [rangeName, content] : container.variations) {
        auto range = parseRange(rangeName);  // "1-16" -> {1, 16}
        if (range.isValid()) {
            maxTime = std::max(maxTime, (double)range.end);
        }
    }
    
    return maxTime;
}
```

## Benefits of Default + Override

### 1. **Musical Accuracy**
- Captures how musicians actually think: "normal verse but shorter"
- Handles exceptions without breaking the pattern

### 2. **Efficient Notation**
- Most content uses defaults (concise)
- Only exceptions need explicit specification

### 3. **Flexible Structure**
- Can handle irregular forms naturally
- Supports both strict and free forms

### 4. **AI-Friendly**
- AI knows the typical pattern
- Can generate appropriate exceptions
- Understands structural relationships

### 5. **Validation Support**
- Can warn about unusual structures
- Suggest fixes for malformed patterns
- Provide context-aware editing

## Implementation Strategy

### Phase 1: Default Support
```json
{
  "aliases": {
    "verse": {"contains": "bars", "count": 16, "default": true}
  }
}
```

### Phase 2: Override Support  
```json
{
  "intro": {
    "type": "verse",
    "duration": 4
  }
}
```

### Phase 3: Smart Analysis
- Automatic duration detection from content
- Validation against typical patterns
- Suggestions for structural improvements

This approach respects musical reality: patterns exist, but exceptions are normal and meaningful!