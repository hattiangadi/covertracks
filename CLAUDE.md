# Covertracks Development Guide

## Build & Run
- Use projucer/xcode to build and run
- Build target: `"Covertracks - App"` scheme

## Current Development Stage
**STAGE 1: Foundation Setup** ✅ *COMPLETE*
- [x] Create docs/ structure and component inventory  
- [x] Document current audio architecture
- [x] Document existing testing framework and strategy
- [x] Define component boundaries for cleanup
- [x] Sprint planning structure
- [x] Remove obvious dead code (Temp/, backup files)
- [x] **BONUS**: Set up complete CMake build for testing/analysis (parallel to Xcode)
- [x] **BONUS**: Set up clang-tidy configuration with CMake compilation database
- [ ] **BONUS**: Install clang-tidy and run first analysis

**STAGE 2: Tracktion Engine Migration** 🎵 *IN PROGRESS*
- [x] Implement TracktionAudioEngine singleton  
- [x] Create working MIDI→Audio test pipeline
- [x] Verify Tracktion built-in plugin integration
- [x] **BREAKTHROUGH**: Confirm Tracktion Engine audio works perfectly
- [ ] **CURRENT**: Migrate to unified Tracktion-only architecture
- [ ] Replace Edit Mode with Tracktion track editing
- [ ] Convert Player/Instrument to use Tracktion tracks
- [ ] Remove custom PluginGraph and AudioProcessorGraph code

**Next: STAGE 3 - Full Tracktion Integration**

## Architecture Quick Reference
- **Audio System**: TracktionAudioEngine singleton with te::Track-per-Player
- **Plugin Management**: Native Tracktion te::Track::pluginList (professional DAW features)
- **Editing Interface**: Unified Tracktion DAW interface (replacing Edit Mode)
- **Performance Mode**: Multiple Tracktion tracks coordinated by EnsembleManager

## Current Focus Areas

### Architecture Status (Post-Tracktion Integration)
1. **✅ SOLVED: Audio pipeline working** - Tracktion Engine provides professional audio quality
2. **✅ COMPLETE: State Machine Workflow** - Clean pattern playback without data passing
3. **📋 IDENTIFIED: Redundant systems to remove:**
   - Custom PluginGraph class (replace with te::Track::pluginList)
   - AudioProcessorGraph usage (replace with Tracktion tracks)  
   - Edit Mode dual-system (merge into Tracktion DAW)
4. **Large UI components** - PerformanceEditorComponent (2,018 lines), PlayerModeComponent (1,832 lines)
5. **🔥 ROOT CAUSE FOUND**: Player/Instrument integration issues, NOT core audio

### NEW: State Machine Workflow for Pattern Playback
**Problem**: Complex data passing between components caused format mismatches and bugs

**Solution**: Each component manages its own state transitions without data passing

#### Workflow Steps:
1. **PlayerModeComponent**: `convertButton` → calls `selectedPlayer->preparePattern(patternJson)`
2. **Player**: Stores pattern internally → calls `musician->convertNotationToMidiEvents()`
3. **PlayerModeComponent**: `loadButton` → calls `selectedPlayer->convertToMidi()`
4. **Player**: Converts pattern internally → calls `instrument->convertChartToMidiSequence()`
5. **PlayerModeComponent**: `playButton` → calls `selectedPlayer->loadPreparedMidi()`
6. **Player**: Loads MIDI internally → stores `preloadedSequence`
7. **PlayerModeComponent**: Final play → calls `selectedPlayer->play()`
8. **Player**: Uses stored sequence → calls `instrument->playMidiSequence()`

#### Benefits:
- **No data format mismatches** - each component uses its preferred format internally
- **No complex conversions** - conversion happens once inside each component
- **Clean interfaces** - simple state transition methods
- **Better error handling** - each step can fail independently
- **Easier debugging** - clear step-by-step progression

#### Important Fix: Editor → Player Mode Resource Conflicts
**Problem Discovered**: Players that were previously used in Editor Mode fail to play in Player Mode due to resource conflicts.

**Root Cause**: Editor Mode takes control of Tracktion tracks/audio resources, leaving them in inconsistent state for Player Mode.

**Solution**: Force clean re-initialization in Player Mode for any previously-used players:
```cpp
// In PlayerModeComponent::playLoadedChart()
selectedPlayer->shutdown();    // Clean up editor-mode resources
selectedPlayer->initialize();  // Fresh initialization for player mode
```

This ensures clean state transitions between Editor Mode and Player Mode.

### New Unified Architecture (Tracktion-Based)
- **Tracktion Core**: TracktionAudioEngine singleton (✅ working)
- **Track-per-Player**: Each Player = One Tracktion track with te::Track::pluginList
- **Plugin Management**: Native Tracktion plugin chains (replace custom graphs)
- **DAW Interface**: Tracktion editing UI (replace Edit Mode)
- **Performance**: EnsembleManager coordinates multiple Tracktion tracks
- **LLM Integration**: Player + Musician + LLM (unchanged)

### Systems to Remove/Replace
- ❌ **Source/Plugins/PluginGraph.cpp** → ✅ te::Track::pluginList
- ❌ **Edit Mode AudioProcessorGraph** → ✅ Tracktion track editing  
- ❌ **Instrument::processorGraph** → ✅ Player as Tracktion track
- ❌ **Dual audio routing** → ✅ Single Tracktion architecture

## Development Standards

### Test-Driven Development (TDD) - MANDATORY
**Follow the Red-Green-Refactor cycle for ALL new features:**

1. **🔴 RED**: Write failing test first
   - Write test that describes the desired behavior
   - Test should fail (feature doesn't exist yet)
   - Commit to verify test actually fails

2. **🟢 GREEN**: Write minimal code to make test pass
   - Implement just enough to make the test pass
   - Don't over-engineer or add extra features
   - Focus on making the test green, nothing more

3. **🔵 REFACTOR**: Clean up code while keeping tests green
   - Improve implementation quality
   - Add proper error handling, thread safety, etc.
   - Ensure all tests still pass after refactoring

**Example TDD Workflow:**
```cpp
// 1. RED: Write failing test
void testMidiCallback() {
    auto player = createTestPlayer();
    expect(!player->hasMidiCallback());  // FAILS - method doesn't exist
}

// 2. GREEN: Minimal implementation  
bool Player::hasMidiCallback() const { return false; }  // Just make it compile

// 3. REFACTOR: Proper implementation
bool Player::hasMidiCallback() const { 
    ScopedLock lock(midiCallbackMutex);
    return midiCallback != nullptr; 
}
```

**❌ NEVER implement features first then add tests**
**✅ ALWAYS write tests first, then implement**

### Other Standards
- **Architecture review** for major changes
- **Component isolation** before refactoring
- **Documentation updates** with code changes

## Context for Claude
When working on specific areas:
- **Audio/Plugin work**: Use proven TracktionAudioEngine + te::Track::pluginList patterns
- **Player integration**: Convert to Tracktion track-based architecture  
- **UI development**: Build on Tracktion DAW interface, eliminate Edit Mode
- **Testing**: Use working audio test patterns from TestsComponent
- **Large changes**: Follow unified Tracktion-only architecture

## Important Instructions
Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly requested by the User.