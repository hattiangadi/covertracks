# Quantitative Container Aliases

## The Core Problem

Current system: "a section contains variations but we don't know how many bars it spans"
Solution: Aliases should define both **semantic relationships** and **quantitative relationships**

## Quantitative Alias Definition

```json
{
  "aliases": {
    "verse": {
      "type": "structure",
      "contains": "bars",
      "count": 8,
      "description": "8-bar verse section"
    },
    "bar": {
      "type": "sequential", 
      "contains": "beats",
      "count": 4,
      "description": "4/4 time signature"
    },
    "beat": {
      "type": "sequential",
      "contains": "subdivisions", 
      "count": 4,
      "description": "16th note subdivisions"
    }
  }
}
```

## Musical Examples

### Jazz Standard (32-bar AABA)
```json
{
  "aliases": {
    "song": {"contains": "chorus", "count": 1},
    "chorus": {"contains": "sections", "count": 4, "pattern": "AABA"},
    "section": {"contains": "bars", "count": 8},
    "bar": {"contains": "beats", "count": 4}
  },
  
  "song": {
    "chorus": {
      "A": {"bars": {"1-8": {...}}},     // 8 bars
      "A2": {"bars": {"9-16": {...}}},   // 8 bars  
      "B": {"bars": {"17-24": {...}}},   // 8 bars
      "A3": {"bars": {"25-32": {...}}}   // 8 bars
    }
  }
}
```

### Blues (12-bar form)
```json
{
  "aliases": {
    "blues": {"contains": "chorus", "count": "infinite"},
    "chorus": {"contains": "sections", "count": 3},
    "section": {"contains": "bars", "count": 4},
    "bar": {"contains": "beats", "count": 4}
  },
  
  "blues": {
    "chorus": {
      "I": {"bars": {"1-4": {"chord": "C7"}}},      // 4 bars
      "IV": {"bars": {"5-8": {"chord": "F7"}}},     // 4 bars
      "V": {"bars": {"9-12": {"chord": "G7-C7"}}}   // 4 bars
    }
  }
}
```

### Classical Sonata
```json
{
  "aliases": {
    "sonata": {"contains": "movements", "count": 3},
    "movement": {"contains": "sections", "count": "variable"},
    "section": {"contains": "themes", "count": "variable"},
    "theme": {"contains": "phrases", "count": 2},
    "phrase": {"contains": "bars", "count": 4}
  }
}
```

### Indian Classical
```json
{
  "aliases": {
    "raga": {"contains": "sections", "count": "variable"},
    "alap": {"contains": "phrases", "count": "free"},
    "gat": {"contains": "cycles", "count": "variable"},
    "cycle": {"contains": "matra", "count": 16},  // Teentaal
    "matra": {"contains": "bol", "count": 1}
  }
}
```

## Count Types

### Fixed Count
```json
"bar": {"contains": "beats", "count": 4}
```

### Variable Count  
```json
"development": {"contains": "phrases", "count": "variable"}
```

### Pattern-Based Count
```json
"chorus": {"contains": "sections", "count": 4, "pattern": "AABA"}
```

### Range Count
```json
"verse": {"contains": "bars", "count": {"min": 8, "max": 16}}
```

### Infinite/Free Count
```json
"improvisation": {"contains": "choruses", "count": "infinite"}
"alap": {"contains": "phrases", "count": "free"}
```

## Implementation: Automatic Duration Calculation

```cpp
class MusicalContainer {
private:
    struct ContainerSpec {
        juce::String contains;
        juce::var count;           // int, "variable", "free", "infinite", {min,max}
        juce::String pattern;      // "AABA", "AAB", etc.
        double defaultDuration;    // fallback duration
    };
    
    double calculateDuration(const ContainerSpec& spec) {
        if (spec.count.isInt()) {
            int childCount = spec.count;
            double childDuration = getChildDuration(spec.contains);
            return childCount * childDuration;
        }
        
        if (spec.count.toString() == "variable") {
            return calculateFromContent();  // Analyze actual content
        }
        
        if (spec.count.toString() == "free") {
            return 0.0;  // No fixed duration
        }
        
        return spec.defaultDuration;
    }
};
```

## Real-World Pattern Examples

### Pop Song with Self-Describing Structure
```json
{
  "version": "2.0",
  "aliases": {
    "song": {"contains": "sections", "count": "variable"},
    "verse": {"contains": "bars", "count": 16},
    "chorus": {"contains": "bars", "count": 8}, 
    "bridge": {"contains": "bars", "count": 8},
    "bar": {"contains": "beats", "count": 4}
  },
  
  "song": {
    "verse1": {"bars": {"1-16": {...}}},      // 16 bars automatically
    "chorus1": {"bars": {"1-8": {...}}},      // 8 bars automatically  
    "verse2": {"bars": {"1-16": {...}}},      // 16 bars automatically
    "chorus2": {"bars": {"1-8": {...}}},      // 8 bars automatically
    "bridge": {"bars": {"1-8": {...}}},       // 8 bars automatically
    "chorus3": {"bars": {"1-8": {...}}}       // 8 bars automatically
  }
}
```

### Electronic Music with Loops
```json
{
  "aliases": {
    "track": {"contains": "sections", "count": "variable"},
    "section": {"contains": "loops", "count": "variable"},
    "loop": {"contains": "steps", "count": 16},     // 16-step sequencer
    "step": {"contains": "triggers", "count": 1},
    "trigger": {"contains": "samples", "count": "variable"}
  },
  
  "track": {
    "intro": {
      "loops": {
        "kick": {"steps": {"1,5,9,13": {"triggers": [{"sample": "kick.wav"}]}}},
        "hat": {"steps": {"3,7,11,15": {"triggers": [{"sample": "hat.wav"}]}}}
      }
    }
  }
}
```

## Benefits

### 1. Solves Duration Problem
- Each container knows how many children it should have
- Automatic duration calculation based on structure
- No ambiguity about section lengths

### 2. Cultural Accuracy
- Indian classical can specify taal cycle counts
- Jazz can specify 32-bar choruses
- Electronic can specify 16-step loops

### 3. Validation
- Parser can validate structure matches aliases
- Warn if content doesn't match expected counts
- Suggest corrections for malformed patterns

### 4. Auto-Generation
- AI can generate content to fill expected structure
- "Generate a verse" knows it needs 16 bars
- "Complete the chorus" knows it needs 8 bars

## Migration Strategy

### Phase 1: Optional Count Specification
```json
{
  "aliases": {
    "verse": {"contains": "bars", "count": 8}  // Optional
  }
}
```

### Phase 2: Count-Aware Rendering
- Use count to expand containers automatically
- Fall back to content-based duration if no count

### Phase 3: Full Quantitative Aliases
- Require count specification for all aliases
- Enable full automatic duration calculation

This transforms the language from "flexible but vague" to "flexible and precise"!