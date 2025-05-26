// Source/Musician/Core/MusicPatternSystem.h
#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace covertracks {

// Forward declarations
class PatternParser;
class PatternGenerator;

/**
 * @brief Definition of a single note or sound element
 */
struct Primitive {
    String name;
    int midiNote;
    String description;
    
    var toVar() const;
    static Primitive fromVar(const var& v);
};

/**
 * @brief Musical event with timing information
 */
struct PatternEvent {
    String primitive;           // Reference to primitive (e.g., "kick", "C3")
    double position = 0.0;      // Beat position (1.0 = first beat)
    double duration = 0.25;     // Duration in beats
    int velocity = 100;         // MIDI velocity (0-127)
    
    var toVar() const;
    static PatternEvent fromVar(const var& v);
};

/**
 * @brief Instrument definition with primitives and patterns
 */
class InstrumentDefinition {
public:
    InstrumentDefinition(const String& name = "");
    
    // Properties
    String getName() const { return name; }
    
    // Primitive management
    void addPrimitive(const String& name, const Primitive& primitive);
    void removePrimitive(const String& name);
    
    // Pattern management
    void addPattern(const String& name, std::shared_ptr<Bar> pattern);
    void removePattern(const String& name);
    
    // Access methods
    bool hasPrimitive(const String& primitiveName) const { return primitives.find(primitiveName) != primitives.end(); }
    bool hasPattern(const String& patternName) const { return patterns.find(patternName) != patterns.end(); }
    
    Primitive getPrimitive(const String& name) const;
    std::shared_ptr<Bar> getPattern(const String& name) const;
    
    // Serialization
    var toVar() const;
    static InstrumentDefinition fromVar(const var& v);
    
private:
    String name;
    std::map<String, Primitive> primitives;
    std::map<String, std::shared_ptr<Bar>> patterns;
};

/**
 * @brief Central management system for musical patterns
 */
class MusicPatternSystem {
public:
    MusicPatternSystem();
    ~MusicPatternSystem();
    
    // Instrument management
    void addInstrumentDefinition(const String& name, const InstrumentDefinition& definition);
    bool hasInstrumentDefinition(const String& name) const;
    const InstrumentDefinition& getInstrumentDefinition(const String& name) const;
    const std::map<String, InstrumentDefinition>& getAllInstrumentDefinitions() const { return instrumentDefinitions; }
    
    // Pattern reference resolution
    std::shared_ptr<Bar> resolvePatternReference(const String& reference, const String& instrumentName = "");
    
    // Pattern creation helpers
    std::shared_ptr<Bar> createBarFromNotation(const String& notation);
    std::shared_ptr<Phrase> createPhraseFromNotation(const String& notation);
    std::shared_ptr<Section> createSectionFromNotation(const String& notation);
    std::shared_ptr<Chart> createChartFromNotation(const String& notation);
    
    // Pattern management
    void addPattern(const String& id, std::shared_ptr<Bar> pattern) { patterns[id] = pattern; }
    void addPhrase(const String& id, std::shared_ptr<Phrase> phrase) { phrases[id] = phrase; }
    void addSection(const String& id, std::shared_ptr<Section> section) { sections[id] = section; }
    void addChart(const String& id, std::shared_ptr<Chart> chart) { charts[id] = chart; }
    
    bool hasPattern(const String& id) const { return patterns.find(id) != patterns.end(); }
    bool hasPhrase(const String& id) const { return phrases.find(id) != phrases.end(); }
    bool hasSection(const String& id) const { return sections.find(id) != sections.end(); }
    bool hasChart(const String& id) const { return charts.find(id) != charts.end(); }
    
    std::shared_ptr<Bar> getPattern(const String& id) const;
    std::shared_ptr<Phrase> getPhrase(const String& id) const;
    std::shared_ptr<Section> getSection(const String& id) const;
    std::shared_ptr<Chart> getChart(const String& id) const;
    
    // Factory methods for creating MIDI sequences from patterns
    std::unique_ptr<MidiMessageSequence> createMidiSequence(const String& patternId, double tempo = 120.0);
    
    // Serialization
    var toVar() const;
    bool fromVar(const var& v);
    
    // File I/O
    bool saveToFile(const File& file) const;
    bool loadFromFile(const File& file);
    
private:
    // Default instrument definitions
    void initializeDefaultInstruments();
    
    // Storage for instrument definitions
    std::map<String, InstrumentDefinition> instrumentDefinitions;
    
    // Storage for pattern hierarchy
    std::map<String, std::shared_ptr<Bar>> patterns;
    std::map<String, std::shared_ptr<Phrase>> phrases;
    std::map<String, std::shared_ptr<Section>> sections;
    std::map<String, std::shared_ptr<Chart>> charts;
    
    // Parser helper methods
    String extractTimeSignature(const String& barText, String& remainingText);
    Array<PatternEvent> parseEvents(const String& eventsText, const String& timeSignature);
    
    // Timing calculators
    int getBeatsPerBar(const String& timeSignature);
    double getPositionInBeats(const String& positionText, const String& timeSignature);
    
    // Default instrument definition for error cases
    InstrumentDefinition defaultInstrument;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MusicPatternSystem)
};

} // namespace covertracks
