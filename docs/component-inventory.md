# Component Inventory - Current State

*Analysis as of Stage 1 foundation work*

## Core Architecture (6 main systems)

### 1. Audio System
**Purpose**: Manages all audio resources and processing
- **AudioEngine** (139 lines) - Singleton managing shared audio resources
- **Instrument** (1,631 lines) - JUCE plugin graph wrapper for audio processing
- **PluginGraph** (141 lines) - Plugin hosting and connection management

**Key Pattern**: AudioEngine owned by PlayerManager, shared across all Players

### 2. Player System  
**Purpose**: Unified music player combining audio + AI pattern generation
- **Player** (1,373 lines) - Core component combining Instrument + Musician
- **PlayerManager** (145 lines) - Owns AudioEngine, manages player lifecycle
- **Instrument** - Audio processing via plugin graphs
- **Musician** - Pattern generation and LLM integration

**Key Pattern**: Lazy loading, shared resource references

### 3. Song System
**Purpose**: Lead sheet management and image import
- **Song** - Lead sheet representation (title, tempo, musical structure)
- **SongImageImportComponent** - LLM-powered image-to-structure extraction
- **SongEditorComponent** - Song structure editing interface
- **SongManagerComponent** - Song library management

**Key Distinction**: **Songs** (lead sheets) vs **Charts** (instrument-specific versions)
- **Song**: Generic musical structure, chords, melody, form
- **Chart**: Song adapted for specific instrument with patterns, voicings, etc.

### 4. LLM Integration
**Purpose**: AI-powered music pattern generation and image analysis
- **LLMManager** (91 lines) - Singleton managing AI clients
- **ClaudeClient** - Anthropic API integration  
- **PromptManager** - Template management and conversation history

**Key Pattern**: Centralized prompt management, distributed generation (patterns + image analysis)

### 5. Performance System
**Purpose**: Multi-player ensemble coordination
- **EnsembleManager** (514 lines) - Coordinates multiple players
- **Performance** - Performance definitions and management
- **MusicPatternSystem** (1,211 lines) - Hierarchical pattern structure
- **PatternParser** (961 lines) - Text-to-MIDI conversion

**Key Pattern**: Complex orchestration of multiple players with synchronized timing

### 6. UI System
**Purpose**: User interface and interaction
- **MainHostWindow** (1,182 lines) - Application shell and menu system
- **GraphEditorPanel** (1,756 lines) - Visual plugin editor
- **PlayerModeComponent** (1,832 lines) - Primary player interface
- **PerformanceEditorComponent** (2,018 lines) - Performance editing interface

**Key Pattern**: Large, monolithic UI components with embedded logic

## Resource Ownership Chain

```
PlayerManager
├── AudioEngine (owns shared audio resources)
│   ├── AudioDeviceManager
│   ├── AudioPluginFormatManager  
│   └── KnownPluginList
└── Players[] (references to shared resources)
    ├── Instrument (audio processing)
    ├── Musician (pattern generation)
    └── LLMClient reference
```

## Component Stats
- **Total Files**: 101 (updated after Stage 1 cleanup - removed 2 files)
- **Largest Components**: 
  - PerformanceEditorComponent: 2,018 lines
  - PlayerModeComponent: 1,832 lines  
  - GraphEditorPanel: 1,756 lines
  - Instrument: 1,631 lines
- **Test Coverage**: 7 test files covering major systems

## Active vs Potential Dead Code

### Active (High Confidence)
- All Player/* components - core functionality
- Audio/AudioEngine - singleton audio management
- Main UI components in use
- LLM/Core/* - active AI integration
- Current Performance system

### Removed in Stage 1 Cleanup ✅
- **`/Temp/ClaudeClient.*`** - ~~Duplicate experimental version~~ DELETED
- **`MusicPatternSystem.cpp.new`** - ~~Backup file~~ DELETED  
- **`test_json_extraction.cpp`** - ~~Standalone utility~~ DELETED

### Remaining Cleanup Candidates
- Some overlapping UI components
- Unused test files
- Large commented code sections

### Complex Areas Needing Review
- **Pattern representations** - Multiple overlapping systems
- **Large UI components** - May need decomposition
- **Resource management** - Complex ownership patterns
- **State synchronization** - Multiple components maintaining state