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

**Next: STAGE 2 - Audio Engine Rearchitecture**

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
- **Test-first development** (starting Stage 2)
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