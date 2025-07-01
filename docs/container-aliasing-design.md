# Container Aliasing Design for Music JSON 2.0

## Problem Statement

The current UniversalMusicParser has type-specific parsing methods (parseLayersInto, parseVariationsInto, etc.) which violates the "universal" principle. The MusicalContainer architecture treats everything uniformly, but the parser doesn't.

## Solution: Semantic Aliasing

### Core Principle

All musical structures are MusicalContainers, but we use semantic aliases to provide clarity:

```
Arrangement → Section → Phrase → Bar → Beat → Event
```

### Implementation Approach

#### 1. Universal Container Parsing

Replace type-specific methods with a single universal method:

```cpp
void parseContainerInto(MusicalContainer& parent, 
                       const juce::var& json,
                       const juce::String& semanticType)
{
    if (!json.isObject()) return;
    
    auto obj = json.getDynamicObject();
    if (!obj) return;
    
    for (const auto& item : obj->getProperties()) {
        MusicalContainer child = parseContainer(item.value);
        
        // Store based on temporal relationship, not type
        if (isSimultaneous(semanticType)) {
            parent.layers[item.name.toString()] = child;
        } else if (isSequential(semanticType)) {
            parent.variations[item.name.toString()] = child;
        } else if (isNamed(semanticType)) {
            parent.patterns[item.name.toString()] = child;
        }
    }
}
```

#### 2. Semantic Type Mapping

```cpp
enum class TemporalRelationship {
    Simultaneous,  // Layers, instruments playing together
    Sequential,    // Variations, sections in order
    Named         // Patterns, reusable templates
};

TemporalRelationship getRelationship(const juce::String& alias) {
    if (alias == "layers" || alias == "instruments" || alias == "parts")
        return TemporalRelationship::Simultaneous;
    
    if (alias == "variations" || alias == "bars" || alias == "sections")
        return TemporalRelationship::Sequential;
        
    if (alias == "patterns" || alias == "templates" || alias == "phrases")
        return TemporalRelationship::Named;
        
    return TemporalRelationship::Sequential; // Default
}
```

#### 3. JSON Structure with Aliases

```json
{
  "version": "2.0",
  "arrangement": {
    "intro": {
      "duration": 8,  // 8 bars
      "phrases": {
        "opening": {
          "bars": {
            "1-4": {
              "beats": {
                "1": {"events": [{"n": "C4", "t": 1.0}]},
                "2": {"events": [{"n": "E4", "t": 1.0}]},
                "3": {"events": [{"n": "G4", "t": 1.0}]},
                "4": {"events": [{"n": "C5", "t": 1.0}]}
              }
            },
            "5-8": {
              "pattern": "opening",  // Reference first 4 bars
              "transpose": 5        // Up a fifth
            }
          }
        }
      }
    },
    "verse": {
      "duration": 16,
      "phrases": {
        "a": {"bars": {"1-8": {...}}},
        "b": {"bars": {"9-16": {...}}}
      }
    }
  }
}
```

### Benefits of Aliasing

#### 1. Semantic Clarity
Musicians understand "verse contains phrases" better than "container contains containers"

#### 2. Universal Implementation
```cpp
// All these use the same underlying parseContainer method
parseContainerInto(root, json["arrangement"], "arrangement");
parseContainerInto(section, json["phrases"], "phrases");
parseContainerInto(phrase, json["bars"], "bars");
```

#### 3. Flexible Duration Specification
```json
{
  "verse": {
    "duration": 16,  // Section spans 16 bars
    "phrases": {
      "a": {"duration": 8},  // Phrase spans 8 bars
      "b": {"duration": 8}
    }
  }
}
```

#### 4. Automatic Expansion
```json
{
  "pattern": {
    "basic_rhythm": {
      "bar": {
        "beats": {
          "1": {"kick": true},
          "2": {"snare": true},
          "3": {"kick": true},
          "4": {"snare": true}
        }
      }
    }
  },
  "verse": {
    "drums": {
      "pattern": "basic_rhythm",
      "repeat": 16  // Automatically expands to 16 bars
    }
  }
}
```

### Solving the Duration Problem

The aliasing system naturally solves the duration issue:

```json
{
  "arrangement": [
    {"section": "intro", "bars": 4},
    {"section": "verse", "bars": 16},
    {"section": "chorus", "bars": 8},
    {"section": "verse", "bars": 16},
    {"section": "chorus", "bars": 8, "repeat": 2}
  ]
}
```

Or with nested specification:
```json
{
  "verse": {
    "bars": 16,  // Total duration
    "structure": {
      "0-4": {"pattern": "verse_a"},
      "4-8": {"pattern": "verse_a", "variation": "high"},
      "8-12": {"pattern": "verse_b"},
      "12-16": {"pattern": "verse_b", "variation": "climax"}
    }
  }
}
```

### Parser Refactoring Plan

#### Phase 1: Add Universal Method
```cpp
// Add alongside existing methods
void parseContainerInto(MusicalContainer& parent, 
                       const juce::var& json,
                       const juce::String& semanticType);
```

#### Phase 2: Migrate Callers
```cpp
// Old
parseLayersInto(container, json["layers"]);
parseSectionsInto(container, json["sections"]);

// New
parseContainerInto(container, json["layers"], "layers");
parseContainerInto(container, json["sections"], "sections");
```

#### Phase 3: Remove Type-Specific Methods
Delete parseLayersInto, parseVariationsInto, etc. once migration is complete.

### Backward Compatibility

The aliasing system maintains full backward compatibility:
- "layers" still creates simultaneous containers
- "variations" still creates sequential containers
- "sections" still creates named containers
- Old JSON files continue to work unchanged

### Future Extensions

The aliasing system makes it trivial to add new semantic types:

```json
{
  "movements": {  // Classical music movements
    "allegro": {...},
    "andante": {...},
    "scherzo": {...},
    "finale": {...}
  },
  
  "stanzas": {  // Song verses with lyrics
    "verse1": {...},
    "verse2": {...},
    "refrain": {...}
  },
  
  "grooves": {  // Drum patterns
    "main": {...},
    "fill": {...},
    "breakdown": {...}
  }
}
```

All these work with the same universal parser - no code changes needed!