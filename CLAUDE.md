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

**STAGE 2: Audio Engine Rearchitecture** 🎵 *IN PROGRESS*
- [ ] Document new single AudioEngine design  
- [ ] Create AudioEngine tests (TDD approach)
- [ ] Implement centralized AudioEngine
- [ ] Migrate Players to shared AudioEngine
- [ ] Remove old per-Player AudioEngine code  
- [ ] **CRITICAL**: Fix performance mode clicking issues

**Next: STAGE 3 - Component Consolidation**

## Architecture Quick Reference
- **[Audio System](docs/audio-architecture.md)**: AudioEngine singleton, Player/Instrument chain
- **[Component Inventory](docs/component-inventory.md)**: What exists now, active vs dead code
- **[All Docs](docs/README.md)**: Complete architecture documentation

## Current Focus Areas

### Known Issues (Stage 1 Analysis)
1. **🔥 CRITICAL: Performance mode clicking** - Audio quality issues during multi-player playback
2. **AudioEngine ownership confusion** - Complex resource sharing patterns
3. **Large UI components** - PerformanceEditorComponent (2,018 lines), PlayerModeComponent (1,832 lines)  
4. **Potential dead code** - `/Temp/ClaudeClient.*`, experimental pattern files (✅ cleaned up)
5. **Plugin graph duplication** - Each Instrument has both pluginGraph and processorGraph

### Component Boundaries (for testing/cleanup)
- **Audio**: AudioEngine + Instrument audio processing 
- **Player**: Player + Musician + LLM integration
- **Performance**: EnsembleManager + Performance coordination
- **UI**: Large components need decomposition
- **Plugins**: PluginGraph + state management

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
- **Audio issues**: Include docs/audio-architecture.md context
- **Component cleanup**: Reference docs/component-inventory.md  
- **Testing**: Follow docs/testing-strategy.md (when created)
- **Large changes**: Review against current stage goals

## Important Instructions
Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly requested by the User.