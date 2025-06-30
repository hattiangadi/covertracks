# Unified Recursive Grammar for Music JSON 2.0

## Core Insight: Everything is a MusicalContainer

Based on the structural analysis that:
- **Sections ≈ Patterns**: Named event containers that can be referenced and varied
- **Layers ≈ Variations**: Collections with temporal relationships (simultaneous vs sequential)

We can unify the entire music language into a **single recursive data structure**.

## The MusicalContainer Grammar

```bnf
MusicalContainer ::= {
  "events"?: EventList,           // Direct events (optional)
  "layers"?: ContainerMap,        // Simultaneous sub-containers  
  "variations"?: ContainerMap,    // Sequential sub-containers (time-based)
  "patterns"?: ContainerMap,      // Named sub-containers (reusable)
  "context"?: Context,            // Inherited attributes
  "reference"?: String            // Reference to named pattern
}

ContainerMap ::= { Name: MusicalContainer }
EventList ::= Event[]
Event ::= { "t": Number, "n": Note, ... }
Context ::= { "velocity": Value, "octave": Number, ... }
```

## Key Properties

### 1. **Recursive Nesting**
Every `MusicalContainer` can contain other `MusicalContainer`s at any depth:

```json
{
  "layers": {
    "drums": {
      "patterns": {
        "basic": {
          "variations": {
            "bar4": {
              "layers": {
                "kick": {"events": [...]},
                "snare": {"events": [...]}
              }
            }
          }
        }
      }
    }
  }
}
```

### 2. **Temporal Relationships**
- **`layers`**: All play simultaneously (parallel composition)
- **`variations`**: Replace each other based on time/conditions (sequential composition)
- **`patterns`**: Named templates for reuse (reference composition)

### 3. **Context Inheritance**
Every container inherits and can override parent context:
```json
{
  "context": {"velocity": "mf", "octave": 4},
  "layers": {
    "bass": {
      "context": {"octave": 2},        // Overrides parent octave
      "variations": {
        "bar8": {
          "context": {"velocity": "f"}  // Overrides parent velocity
        }
      }
    }
  }
}
```

## Unified Implementation

### Single Core Class
```cpp
class MusicalContainer {
public:
    // Core data
    std::vector<Event> events;
    std::map<String, MusicalContainer> layers;      // Simultaneous
    std::map<String, MusicalContainer> variations;  // Sequential  
    std::map<String, MusicalContainer> patterns;    // Named templates
    Context context;
    String reference;  // Pattern reference
    
    // Single render method handles ALL cases
    EventStream render(const Context& parentContext, int currentBar, int totalBars);
    
private:
    // Unified processing
    EventStream renderEvents(const Context& resolvedContext);
    EventStream renderLayers(const Context& resolvedContext, int bar, int totalBars);
    EventStream renderVariations(const Context& resolvedContext, int bar);
    EventStream resolveReference(const Context& resolvedContext, int bar, int totalBars);
};
```

### Universal Parser
```cpp
class UniversalMusicParser {
public:
    // Single parse method for any JSON structure
    MusicalContainer parse(const juce::var& json);
    
private:
    // Recursive parsing - same logic for all containers
    MusicalContainer parseContainer(const juce::var& containerJson);
    Context parseContext(const juce::var& contextJson);
    std::vector<Event> parseEvents(const juce::var& eventsJson);
};
```

## Expressive Power Examples

### 1. **Nested Arrangement Structure**
```json
{
  "song": {
    "layers": {
      "structure": {
        "variations": {
          "verse1": {"reference": "verse"},
          "chorus1": {"reference": "chorus"},
          "verse2": {
            "reference": "verse",
            "layers": {
              "harmony": {"reference": "verseHarmony"}
            }
          }
        }
      }
    }
  },
  "patterns": {
    "verse": {
      "layers": {
        "drums": {"reference": "basicRock"},
        "bass": {"reference": "walkingBass"},
        "guitar": {
          "variations": {
            "bar1-2": {"reference": "chordStrum"},
            "bar3-4": {"reference": "walkDown"}
          }
        }
      }
    }
  }
}
```

### 2. **Dynamic Layer Switching**
```json
{
  "drums": {
    "layers": {
      "backbone": {"reference": "steadyBeat"},
      "dynamics": {
        "variations": {
          "quiet": {"context": {"velocity": "p"}},
          "building": {
            "layers": {
              "fills": {"reference": "buildingFills"},
              "backbone": {"context": {"velocity": "mf"}}
            }
          },
          "climax": {"context": {"velocity": "ff"}}
        }
      }
    }
  }
}
```

### 3. **Conditional Variations**
```json
{
  "solo": {
    "variations": {
      "beginner": {"reference": "simpleSolo"},
      "intermediate": {
        "layers": {
          "melody": {"reference": "complexMelody"},
          "backing": {"reference": "backing"}
        }
      },
      "advanced": {
        "layers": {
          "melody": {
            "variations": {
              "phrase1": {"reference": "fastRuns"},
              "phrase2": {"reference": "jazzLicks"}
            }
          },
          "harmony": {"reference": "complexChords"}
        }
      }
    }
  }
}
```

## Rendering Algorithm

### Unified Temporal Resolution
```cpp
EventStream MusicalContainer::render(const Context& parentContext, int currentBar, int totalBars) {
    // 1. Resolve context (merge with parent)
    Context resolvedContext = parentContext.merge(this->context);
    
    // 2. Handle pattern reference
    if (!reference.empty()) {
        return resolveReference(resolvedContext, currentBar, totalBars);
    }
    
    // 3. Collect all event streams
    EventStream result;
    
    // 4. Direct events
    result.merge(renderEvents(resolvedContext));
    
    // 5. Simultaneous layers (all play together)
    result.merge(renderLayers(resolvedContext, currentBar, totalBars));
    
    // 6. Sequential variations (choose one based on currentBar)
    result.merge(renderVariations(resolvedContext, currentBar));
    
    return result;
}
```

### Variation Selection Logic
```cpp
EventStream MusicalContainer::renderVariations(const Context& context, int currentBar) {
    // Find matching variation for current bar
    for (auto& [varName, varContainer] : variations) {
        if (matchesBar(varName, currentBar)) {
            return varContainer.render(context, currentBar, totalBars);
        }
    }
    return EventStream();  // No variation for this bar
}

bool matchesBar(const String& varName, int bar) {
    // Support various formats:
    // "bar4" -> bar == 4
    // "bar1-4" -> bar >= 1 && bar <= 4  
    // "even" -> bar % 2 == 0
    // "last" -> bar == totalBars
}
```

## Benefits of Unified Grammar

### 1. **Massive Code Reduction**
- **Before**: Separate parsers for sections, layers, patterns, variations
- **After**: Single recursive `parseContainer()` method

### 2. **Unlimited Nesting**
```json
// This becomes possible:
"drums": {
  "layers": {
    "kit1": {
      "variations": {
        "verse": {
          "layers": {
            "kick": {
              "variations": {
                "accent": {"events": [...]}
              }
            }
          }
        }
      }
    }
  }
}
```

### 3. **Natural Composition**
- Mix and match any containers at any level
- Patterns can contain layers, variations can contain patterns
- Context flows naturally through any structure

### 4. **LLM-Friendly**
- Consistent structure at every level
- AI can generate deep, complex structures following simple rules
- No special cases or different syntaxes

### 5. **Musical Expressiveness**
- Model complex arrangements with nested structure
- Dynamic switching between different complexity levels
- Conditional variations based on performance context

## Migration Path

### Phase 1: Core Implementation
```cpp
// Implement MusicalContainer with basic temporal logic
class MusicalContainer {
    EventStream render(Context, int bar, int totalBars);
};
```

### Phase 2: Parser Unification
```cpp
// Replace all existing parsers with single recursive parser
MusicalContainer UniversalMusicParser::parseContainer(const var& json);
```

### Phase 3: Advanced Features
- Conditional variation selection
- Complex temporal patterns
- Performance-aware rendering

This unified grammar eliminates code duplication while exponentially increasing expressive power through recursive composition!