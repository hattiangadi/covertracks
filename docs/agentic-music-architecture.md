# Agentic Music Generation Architecture

## Overview

This document outlines a proposed agentic architecture for Covertracks that replaces isolated LLM calls with a coordinated multi-agent system. The architecture emphasizes adaptive, context-aware music generation that learns from user preferences rather than enforcing rigid music theory rules.

## Current State Analysis

### Existing LLM Integration Points

1. **Song Creation from Images** (`Song::processImageForStructure()`)
   - Single LLM call to extract structure from sheet music images
   - Returns JSON with title, tempo, sections, and chords

2. **Instrument Chart Generation** (`Performance::generateChartForRole()`)
   - Single LLM call per instrument role
   - No coordination between instruments
   - No feedback or refinement loops

### Limitations of Current Approach

- **No agent coordination**: Each LLM call operates in isolation
- **No feedback loops**: System doesn't learn from results
- **No context preservation**: Each request starts fresh
- **Single-shot generation**: No iterative refinement
- **No performance metrics**: No tracking of generation quality

## Proposed Agentic Architecture

### Core Components

#### 1. Agent Base Infrastructure

```cpp
class Agent {
    virtual AgentResponse execute(AgentContext& context) = 0;
    virtual bool canHandle(const AgentTask& task) = 0;
    virtual String getRole() const = 0;
    
    // State management
    Memory memory;
    std::vector<Tool*> availableTools;
};

class AgentMemory {
    // Short-term: Current composition context
    MusicContext currentContext;
    
    // Long-term: Style preferences, successful patterns
    StyleDatabase learnedPatterns;
    
    // Working memory: Inter-agent communication
    MessageHistory conversations;
};
```

#### 2. Specialized Agents

**Orchestrator Agent** (Master Coordinator)
- Manages overall composition workflow
- Creates and configures Players/Ensembles
- Optimizes LLM token usage through batching
- Evaluates musical coherence across parts
- Maintains conversation context and musical memory

**Composer Agent** (Musical Director)
- Analyzes lead sheets/images for harmonic structure
- Determines overall arrangement style
- Creates master composition blueprint
- Defines tempo, key, mood, dynamics

**Arranger Agents** (Per-Instrument Specialists)
- Specialized knowledge per instrument type
- Generates idiomatic parts based on composer's blueprint
- Considers player skill level and capabilities
- Ensures parts complement each other

### Communication & Coordination

```cpp
class OrchestrationEngine {
    // Manages agent lifecycle
    void spawnAgent(AgentType type);
    
    // Task distribution logic
    void distributeTask(Task task);
    
    // Result evaluation
    bool evaluateResults(std::vector<AgentResult>);
    
    // Retry/refinement logic
    void refineResults(AgentFeedback feedback);
};
```

## Adaptive Evaluation System

### Context-Aware Evaluation

Rather than using fixed music theory rules, the system adapts its evaluation criteria based on musical style and user preferences.

#### Style-Specific Evaluation Profiles

```cpp
class StyleEvaluationProfile {
    // Different genres have different "good" metrics
    static Profile getPunkProfile() {
        return {
            .keyConsistency = 0.3f,      // Low weight - punk can be atonal
            .harmonicComplexity = -0.2f,  // Negative weight - simpler is better
            .rhythmicDriving = 0.9f,      // High weight - needs energy
            .rawness = 0.8f,              // Distortion/noise is good
            .technicalPerfection = -0.5f  // Imperfection is authentic
        };
    }
    
    static Profile getJazzProfile() {
        return {
            .keyConsistency = 0.1f,       // Low - jazz modulates frequently
            .harmonicComplexity = 0.9f,   // High - complex chords expected
            .rhythmicSwing = 0.7f,        // Swing feel important
            .improvisation = 0.8f,        // Variation valued
            .technicalPerfection = 0.6f   // Competent but not rigid
        };
    }
};
```

### User Preference Learning

#### Explicit Feedback
```cpp
class UserPreferenceTracker {
    // Learn what THIS user actually likes
    void recordFeedback(const Arrangement& arr, UserRating rating) {
        auto features = extractFeatures(arr);
        userModel.update(features, rating);
    }
    
    // Predict if user will like this
    float predictUserSatisfaction(const Arrangement& arr) {
        return userModel.predict(extractFeatures(arr));
    }
};
```

#### Implicit Feedback Signals
```cpp
class ImplicitFeedbackCollector {
    void trackUserBehavior(const Arrangement& arr) {
        // Positive signals
        if (user.playsMultipleTimes(arr)) score += 0.3;
        if (user.savesToFavorites(arr)) score += 0.5;
        if (user.sharesWithOthers(arr)) score += 0.7;
        if (user.adjustsVolumeUp(arr)) score += 0.1;
        
        // Negative signals  
        if (user.stopsEarly(arr)) score -= 0.5;
        if (user.skips(arr)) score -= 0.3;
        if (user.deletesImmediately(arr)) score -= 0.9;
    }
};
```

### Multi-Armed Bandit Approach

The system explores different generation strategies and learns which work best:

```cpp
class AdaptiveOrchestrator {
    // Try different evaluation strategies
    std::vector<EvaluationStrategy> strategies;
    
    Arrangement generate() {
        // Explore: Try different approaches
        auto strategy = selectStrategyWithUCB();
        
        auto arrangement = generateWithStrategy(strategy);
        
        // Get implicit/explicit feedback
        auto userSatisfaction = getUserFeedback(arrangement);
        
        // Update strategy weights
        strategy.updateReward(userSatisfaction);
        
        return arrangement;
    }
};
```

## Integration with Tracktion Engine

### Leveraging Built-in Musical Analysis

The system uses Tracktion's musicality functions as features for evaluation rather than hard rules:

1. **Key Detection** (`determineKeyOfNotes()`)
   - Provides mathematical confidence scores
   - Used as one feature among many

2. **Music Theory Classes** (Chord, Scale)
   - Ensure theoretical correctness when appropriate
   - Weight varies by style

3. **Pattern Analysis** (PatternGenerator)
   - Chord progression tracking
   - Note classification in context

### Custom Evaluation Metrics

```cpp
class MusicEvaluator {
    // Using Tracktion primitives as building blocks
    float evaluateKeyConsistency(te::MidiList& midiData) {
        auto notes = midiData.getNotes();
        auto keyResults = te::determineKeyOfNotes(notes);
        return keyResults[0].correlation;
    }
    
    // Style-aware evaluation
    MusicalScore evaluate(const Arrangement& arr, const Style& style) {
        auto profile = StyleEvaluationProfile::getProfile(style);
        return computeWeightedScore(arr, profile);
    }
};
```

## Implementation Workflow

### Example: Jazz Arrangement from Lead Sheet

```
User: "Create jazz arrangement from lead sheet"
    ↓
Orchestrator: 
  - Analyzes request
  - Loads jazz evaluation profile
  - Spawns Composer agent
    ↓
Composer: 
  - Extracts structure from image
  - Identifies key, tempo, form
  - Creates arrangement blueprint
    ↓
Orchestrator: 
  - Creates ensemble (Piano, Bass, Drums, Sax)
  - Spawns Arranger agents for each
  - Provides blueprint + jazz constraints
    ↓
Arrangers (in parallel): 
  - Generate instrument-specific parts
  - Use jazz idioms and voicings
  - Ensure complementary rhythms
    ↓
Orchestrator: 
  - Evaluates using jazz profile
  - Checks inter-part coherence
  - Requests refinements if needed
    ↓
User Feedback Loop:
  - Present result (or multiple options)
  - Collect implicit/explicit feedback
  - Update preference model
```

## Benefits Over Current System

1. **Unified Musical Vision**: All parts work together coherently
2. **Adaptive Quality Metrics**: What's "good" depends on style and user
3. **Continuous Learning**: System improves with use
4. **Efficient LLM Usage**: Batch requests, avoid redundancy
5. **Iterative Refinement**: Can improve outputs based on evaluation
6. **User-Centric**: Learns individual preferences over time

## Migration Strategy

### Phase 1: Agent Infrastructure
- Implement base Agent class and communication system
- Create Orchestrator with basic workflow management
- Integrate with existing LLMManager

### Phase 2: Evaluation System
- Build style-aware evaluation profiles
- Implement feedback collection (implicit and explicit)
- Create learning system for preferences

### Phase 3: Specialized Agents
- Implement Composer agent (replaces image→song flow)
- Implement Arranger agents (replaces chart generation)
- Add inter-agent communication

### Phase 4: Advanced Features
- Multi-armed bandit strategy selection
- A/B testing with user choice
- Advanced preference modeling

## Key Design Principles

1. **Context Over Rules**: Musical context determines what's "good"
2. **Learning Over Logic**: User preferences trump music theory
3. **Exploration Over Exploitation**: Try different approaches
4. **Feedback Over Perfection**: Iterate based on actual use
5. **Collaboration Over Isolation**: Agents work together

## Conclusion

This agentic architecture transforms Covertracks from a tool that makes isolated LLM calls into an intelligent system that learns what music users actually want to create. By combining Tracktion's musical analysis capabilities with adaptive evaluation and user preference learning, the system can generate increasingly personalized and satisfying musical arrangements across any style or genre.