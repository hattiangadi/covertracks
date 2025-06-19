# Technical Debt Analysis & Cleanup Plan

## Critical Audio Quality Issues ❗

### Performance Mode Clicking (High Priority)
**Problem**: Significant audio clicking/popping during performance mode playback
**Impact**: Poor user experience, potentially damaging to speakers/headphones
**Root Causes** (likely):
- Buffer underruns during multi-player coordination
- MIDI timing issues with multiple Players
- Audio thread blocking during LLM pattern generation
- Improper audio device buffer management
- Plugin graph processing conflicts

**Target for Stage 2**: AudioEngine refactor must address this issue
**Testing Requirements**:
- Audio quality measurements (THD, clicking detection)
- Multi-player performance stress testing
- Real-time audio processing validation
- Buffer underrun monitoring

## Immediate Cleanup Candidates (High Confidence)

### Dead Code - Safe to Remove ✅
1. **`Source/Temp/ClaudeClient.*`** - Experimental version with different namespace
   - Different API structure, appears abandoned
   - Duplicate of `Source/LLM/Core/ClaudeClient.*`
   - **Action**: Delete entire `/Temp/` directory

2. **`MusicPatternSystem.cpp.new`** - Backup file  
   - Experimental version of existing file
   - **Action**: Remove `.new` backup file

3. **Test JSON files in root** - Development artifacts
   - `chord_tone_example.json`, `twinkle_*.json`, `take_a_train_fixed.json`
   - **Action**: Move to `Resources/TestPatterns/` or delete

4. **`test_json_extraction.cpp`** - Development utility
   - Standalone utility not integrated into build
   - **Action**: Delete or move to utilities folder

### Commented Code - Review & Remove ⚠️
From component analysis, several files have large commented sections:
- **Player.cpp**: Commented out LLM integration methods
- **Various UI components**: Commented experimental features
- **Action**: Manual review to confirm safety, then remove

## Component Decomposition Candidates

### Large UI Components (>1500 lines)
1. **PerformanceEditorComponent.cpp** (2,018 lines)
   - **Decompose into**: Pattern editing, LLM interaction, playback control
   - **Extract**: LLMInteractionPanel, PatternEditingPanel, PlaybackControlPanel

2. **PlayerModeComponent.cpp** (1,832 lines)  
   - **Decompose into**: Player selection, plugin editing, playback control
   - **Extract**: PlayerSelectionPanel, PluginEditorPanel, PlaybackPanel

3. **GraphEditorPanel.cpp** (1,756 lines)
   - **Decompose into**: Graph visualization, plugin management, connection handling
   - **Extract**: GraphVisualizationComponent, PluginManagementPanel

### Complex Core Components (>1200 lines)
4. **Instrument.cpp** (1,631 lines)
   - **Decompose into**: Resource management, audio processing, MIDI handling
   - **Extract**: InstrumentResourceManager, AudioProcessor, MidiHandler

5. **Player.cpp** (1,373 lines)
   - **Review for**: Single responsibility violations
   - **Potential extracts**: ChartManager, StateManager

## Architectural Cleanup

### Duplicate Functionality ⚠️
1. **Pattern Representations** - Multiple overlapping systems:
   - MusicPatternSystem (hierarchical patterns)
   - PatternParser (text-to-MIDI conversion)  
   - Player-internal pattern handling
   - **Action**: Consolidate into unified pattern system

2. **Plugin Graph Management** - Two graphs per Instrument:
   - `pluginGraph` (PluginGraph class)
   - `processorGraph` (AudioProcessorGraph)
   - **Action**: Eliminate duplication, use single graph

3. **Audio Resource Management** - Complex ownership:
   - AudioEngine owns some resources, references others
   - Players hold references without clear contracts
   - **Action**: Clarify ownership model in Stage 2

### Interface Consolidation
4. **LLM Integration Points** - Multiple interfaces:
   - LLMManager (singleton)
   - Player-level LLM clients
   - Performance-level LLM coordination
   - **Action**: Standardize LLM integration pattern

## Cleanup Staging Strategy

### Stage 1 Completion - Safe Removals
**Priority**: High, Low Risk
- [x] Remove `/Temp/` directory  
- [x] Remove `.new` backup files
- [x] Clean up test artifacts in root
- [x] Remove obvious commented code

### Stage 2 - During Audio Rearchitecture  
**Priority**: High, Medium Risk (done during larger changes)
- [ ] Eliminate plugin graph duplication in Instrument
- [ ] Clarify AudioEngine resource ownership
- [ ] Remove old audio initialization patterns

### Stage 3 - Component-by-Component Cleanup
**Priority**: Medium, Medium Risk
- [ ] Decompose large UI components (one per sprint)
- [ ] Consolidate pattern representation systems
- [ ] Standardize LLM integration interfaces

### Stage 4 - Final Polish
**Priority**: Low, Low Risk  
- [ ] Remove any remaining commented code
- [ ] Consolidate utility functions
- [ ] Final architecture review

## Cleanup Guidelines

### Before Removing Code
1. **Search for references** - Ensure no hidden dependencies
2. **Check git history** - Understand why code was added
3. **Review tests** - Ensure no test dependencies
4. **Document removal** - Note what was removed and why

### During Component Decomposition
1. **Extract interfaces first** - Define clear boundaries  
2. **Move functionality gradually** - One responsibility at a time
3. **Maintain tests** - Update tests to work with new structure
4. **Keep rollback ability** - Don't break builds during transition

### Quality Gates
- [ ] **All tests pass** after each cleanup
- [ ] **Build succeeds** with no warnings
- [ ] **No functionality regression** verified by testing
- [ ] **Documentation updated** to reflect changes
- [ ] **Code review** for architectural changes

## Metrics to Track

### Code Quality Metrics
- **Lines of code per component** (target: <800 lines per file)
- **Cyclomatic complexity** (target: <10 per function)
- **Test coverage** (target: >80% for core components)
- **Build warnings** (target: 0 warnings)

### Architecture Health Metrics  
- **Component coupling** (minimize dependencies)
- **Interface clarity** (clear, documented boundaries)
- **Resource ownership** (unambiguous ownership chains)
- **Dead code** (regular scans for unused code)

This analysis provides a structured approach to cleaning up the codebase while maintaining stability and functionality.