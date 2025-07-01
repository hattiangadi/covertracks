# Self-Describing Container Aliases in Music JSON 2.0

## Concept

Instead of hardcoding alias mappings in the parser, patterns define their own semantic relationships. This makes the language truly universal and culturally adaptable.

## Example Implementation

### Jazz Pattern with Western Terminology
```json
{
  "version": "2.0",
  "aliases": {
    "arrangement": {"type": "sequential", "contains": "sections"},
    "sections": {"type": "structure", "contains": "phrases"},
    "phrases": {"type": "named", "contains": "bars"},
    "bars": {"type": "sequential", "contains": "beats"},
    "beats": {"type": "sequential", "contains": "events"},
    "instruments": {"type": "simultaneous", "contains": "parts"}
  },
  "arrangement": {
    "intro": {
      "phrases": {
        "opening": {
          "bars": {
            "1": {"beats": {"1": {"events": [{"n": "C4"}]}}}
          }
        }
      }
    }
  }
}
```

### Indian Classical Pattern
```json
{
  "version": "2.0",
  "aliases": {
    "raga": {"type": "structure", "contains": "alankar"},
    "alankar": {"type": "sequential", "contains": "taal"},
    "taal": {"type": "sequential", "contains": "matra"},
    "matra": {"type": "sequential", "contains": "swar"},
    "tanpura": {"type": "simultaneous", "contains": "strings"}
  },
  "raga": {
    "bhairav": {
      "alankar": {
        "aroha": {
          "taal": {
            "teentaal": {
              "matra": {
                "1": {"swar": [{"n": "Sa"}, {"n": "re"}]}
              }
            }
          }
        }
      }
    }
  }
}
```

### Electronic Music Pattern
```json
{
  "version": "2.0",
  "aliases": {
    "track": {"type": "structure", "contains": "patterns"},
    "patterns": {"type": "named", "contains": "loops"},
    "loops": {"type": "sequential", "contains": "steps"},
    "steps": {"type": "sequential", "contains": "triggers"},
    "channels": {"type": "simultaneous", "contains": "voices"}
  },
  "track": {
    "drop": {
      "patterns": {
        "bass": {
          "loops": {
            "main": {
              "steps": {
                "1": {"triggers": [{"n": "C1", "filter": 0.5}]}
              }
            }
          }
        }
      }
    }
  }
}
```

## Implementation Approach

### 1. Parser Changes
```cpp
class UniversalMusicParser {
private:
    // Dynamic alias registry populated from pattern
    std::map<juce::String, AliasDefinition> aliases;
    
    struct AliasDefinition {
        TemporalRelationship type;
        juce::String contains;
        juce::var metadata;  // Additional properties
    };
    
    void parseAliases(const juce::var& aliasesJson) {
        // Populate aliases map from JSON
    }
    
    TemporalRelationship getRelationshipForAlias(const juce::String& alias) {
        auto it = aliases.find(alias);
        if (it != aliases.end()) {
            return it->second.type;
        }
        // Fallback to default behavior
        return getDefaultRelationship(alias);
    }
};
```

### 2. Benefits

#### Universal Adaptability
- Any musical tradition can define its own terminology
- No parser changes needed for new musical cultures
- Patterns are self-documenting

#### Domain-Specific Languages
- Jazz uses "chorus", classical uses "movement"
- Electronic uses "loop", folk uses "verse"
- Each domain feels natural to its practitioners

#### Progressive Enhancement
- Patterns without aliases use default mappings
- Patterns with aliases override defaults
- Backward compatible with existing patterns

#### Learning and Discovery
- Parser can learn new relationships from patterns
- Could build a dictionary of common aliases
- AI could suggest appropriate aliases for genres

### 3. Advanced Features

#### Inheritance
```json
{
  "aliases": {
    "inherit": "jazz-standard",  // Inherit from known template
    "solo": {"type": "sequential", "contains": "choruses"}  // Add custom
  }
}
```

#### Validation Rules
```json
{
  "aliases": {
    "verse": {
      "type": "structure",
      "contains": "lines",
      "constraints": {
        "minCount": 4,
        "maxCount": 8
      }
    }
  }
}
```

#### Semantic Metadata
```json
{
  "aliases": {
    "movement": {
      "type": "structure",
      "contains": "themes",
      "metadata": {
        "typical_duration": "5-10 minutes",
        "tempo_changes": "allowed",
        "key_changes": "expected"
      }
    }
  }
}
```

## Migration Path

### Phase 1: Add Alias Parsing
- Parse "aliases" field if present
- Use aliases to override default mappings
- Fall back to hardcoded defaults

### Phase 2: Template Library
- Create standard alias sets for genres
- Allow patterns to inherit from templates
- Build community-contributed aliases

### Phase 3: Full Self-Description
- Deprecate hardcoded mappings
- Require explicit aliases or template inheritance
- Parser becomes truly universal

## Example: Complete Self-Describing Pattern

```json
{
  "version": "2.0",
  "meta": {
    "title": "Self-Describing Jazz Standard",
    "genre": "jazz",
    "culture": "western"
  },
  
  "aliases": {
    "song": {
      "type": "structure",
      "contains": ["head", "solos", "outro"],
      "description": "Complete jazz performance"
    },
    "head": {
      "type": "structure", 
      "contains": "choruses",
      "repeat": 2,
      "description": "Main melody, played at start and end"
    },
    "chorus": {
      "type": "sequential",
      "contains": "sections",
      "bars": 32,
      "description": "One complete form (AABA or similar)"
    },
    "sections": {
      "type": "named",
      "contains": "bars",
      "patterns": ["A", "B"],
      "description": "8-bar sections"
    },
    "solos": {
      "type": "sequential",
      "contains": "choruses",
      "performers": ["piano", "bass", "drums"],
      "description": "Improvised sections"
    },
    "instruments": {
      "type": "simultaneous",
      "contains": ["piano", "bass", "drums"],
      "description": "Jazz trio arrangement"
    }
  },
  
  "song": {
    "head": {
      "choruses": {
        "1": {
          "sections": {
            "A": {"bars": {"1-8": {"events": [...]}}},
            "A2": {"bars": {"9-16": {"events": [...]}}},
            "B": {"bars": {"17-24": {"events": [...]}}},
            "A3": {"bars": {"25-32": {"events": [...]}}}
          }
        }
      }
    },
    "solos": {
      "piano": {"choruses": {"1": {...}, "2": {...}}},
      "bass": {"choruses": {"1": {...}}}
    },
    "outro": {
      "tag": {"bars": {"1-4": {"repeat": 3}}}
    }
  }
}
```

This approach makes the language infinitely extensible while maintaining structure and validation!