#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

namespace covertracks {

// Forward declarations
class Context;
class Event;
class EventStream;
class MusicalContainer;

// Type aliases for expression features (simplified for TDD)
using ExpressionMap = std::map<juce::String, juce::var>;
using ProcessorMap = std::map<juce::String, juce::var>;
using AutomationMap = std::map<juce::String, juce::var>;
using GrooveSettings = juce::var;
using Transform = juce::var;
using ChordProgression = std::vector<juce::var>;
using ProcessorChain = std::vector<juce::var>;
using AIConstraints = juce::var;
using PerformanceContext = juce::var;

/**
 * Context for attribute inheritance through hierarchy
 */
class Context
{
public:
    // Basic musical attributes
    int octave = 4;
    float velocity = 0.8f;
    juce::String chord = "C";
    juce::String key = "C major";
    juce::String dynamics = "mf";
    juce::String articulation = "normal";
    
    // Advanced features
    juce::String chordContext;          // Reference to chord progression
    GrooveSettings groove;              // Rhythmic feel
    juce::String processorChain;        // Effect chain reference
    AutomationMap automation;           // Parameter automation
    
    // AI/Performance
    float complexity = 0.5f;
    float energy = 0.5f;
    juce::String style = "default";
    
    // Context operations
    Context merge(const Context& child) const;
    bool has(const juce::String& attribute) const;
    juce::var get(const juce::String& attribute) const;
};

/**
 * Musical event with full expression support
 */
class Event
{
public:
    // Core event data
    double time = 0.0;                  // Beat position
    juce::String note;                  // Note name, MIDI number, or chord
    float velocity = 0.8f;              // Velocity override
    int degree = 0;                     // Chord degree (0 = not a degree)
    float duration = 0.25f;             // Duration in beats
    juce::String chord;                 // Chord context override
    
    // Expression attributes
    juce::String dynamics;              // pp, p, mf, f, ff
    juce::String articulation;          // staccato, legato, marcato
    ExpressionMap expression;           // Additional expression
    ProcessorMap processors;            // MIDI processors
    AutomationMap automation;           // Parameter automation
    
    // Resolved attributes (filled during rendering)
    int resolvedOctave = 4;
    float resolvedVelocity = 0.8f;
    juce::String resolvedChord = "C";
    int resolvedNote = 60;              // Final MIDI note number
    
    // Constructors
    Event() = default;
    Event(double t, const juce::String& n, float v) : time(t), note(n), velocity(v) {}
    Event(double t, const juce::String& n, float v, int d) : time(t), note(n), velocity(v), degree(d) {}
    
    // Expression queries
    bool hasExpression(const juce::String& type) const;
    bool hasProcessor(const juce::String& type) const;
};

/**
 * Collection of events with timing and query capabilities
 */
class EventStream
{
public:
    EventStream() = default;
    
    void addEvent(const Event& event);
    void merge(const EventStream& other);
    
    size_t size() const;
    bool isEmpty() const;
    
    bool hasEventAt(double time) const;
    Event getEventAt(double time) const;
    Event getEvent(size_t index) const;
    
    std::vector<Event>::iterator begin();
    std::vector<Event>::iterator end();
    std::vector<Event>::const_iterator begin() const;
    std::vector<Event>::const_iterator end() const;
    
private:
    std::vector<Event> events;
};

/**
 * Core recursive container for the unified music grammar.
 * Everything in the music language is a MusicalContainer:
 * - Sections, Layers, Patterns, Variations all use this same structure
 * - Supports infinite recursive nesting
 * - Context inheritance through hierarchy
 * - Unified temporal relationships (layers = simultaneous, variations = sequential)
 * 
 * This class doesn't exist yet - we're defining the interface from our TDD tests!
 */
class MusicalContainer 
{
public:
    MusicalContainer();
    ~MusicalContainer();
    
    // Core recursive structure
    std::vector<Event> events;                              // Direct events
    std::map<juce::String, MusicalContainer> layers;       // Simultaneous sub-containers
    std::map<juce::String, MusicalContainer> variations;   // Sequential sub-containers
    std::map<juce::String, MusicalContainer> patterns;     // Named templates
    std::map<juce::String, MusicalContainer> sections;     // Named sections (alias for patterns)
    
    // Container properties
    Context context;                    // Local context overrides
    juce::String reference;             // Pattern reference
    
    // Expression features (from complete grammar)
    std::map<juce::String, juce::String> shortcuts;        // Shorthand notation
    std::vector<Transform> transforms;                      // Applied transforms
    ChordProgression chordProgression;                      // Chord changes
    std::map<juce::String, ProcessorChain> processorChains; // MIDI effects
    AIConstraints constraints;                              // Generation constraints
    PerformanceContext performance;                         // Performance metadata
    
    // Core interface that tests expect
    EventStream render(const Context& parentContext, int currentBar, int totalBars) const;
    
    // Helper methods for building containers
    void addEvent(const Event& event);
    MusicalContainer& addLayer(const juce::String& name);
    MusicalContainer& addVariation(const juce::String& name);
    MusicalContainer& addPattern(const juce::String& name);
    
    // Query methods
    bool hasLayer(const juce::String& name) const;
    bool hasVariation(const juce::String& name) const;
    bool hasPattern(const juce::String& name) const;
    bool hasSection(const juce::String& name) const;
    
    MusicalContainer& getLayer(const juce::String& name);
    MusicalContainer& getVariation(const juce::String& name);
    MusicalContainer& getPattern(const juce::String& name);
    MusicalContainer& getSection(const juce::String& name);
    
    // Pattern reference
    void setReference(const juce::String& patternName);
    juce::String getReference() const;
    
    // Class identification for tests
    juce::String getClass() const { return "MusicalContainer"; }
    
private:
    // Internal rendering methods
    EventStream renderEvents(const Context& resolvedContext) const;
    EventStream renderLayers(const Context& resolvedContext, int bar, int totalBars) const;
    EventStream renderVariations(const Context& resolvedContext, int bar) const;
    EventStream renderPatterns(const Context& resolvedContext, int bar, int totalBars) const;
    
    // Context resolution
    Context resolveContext(const Context& parentContext) const;
    
    // Pattern resolution
    const MusicalContainer* resolvePattern(const juce::String& patternName) const;
    
    // Expression processing
    void applyExpression(Event& event, const Context& context) const;
    void applyShorthand(EventStream& stream) const;
    void applyTransforms(EventStream& stream) const;
    
    // Harmony resolution
    int resolveChordDegree(int degree, const juce::String& chord, int octave = 4) const;
    
    // Variation matching
    bool matchesBar(const juce::String& varName, int bar) const;
};

} // namespace covertracks