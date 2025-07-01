#pragma once
#include <JuceHeader.h>
#include <memory>
#include "MusicalContainer.h"

namespace covertracks {

/**
 * Universal parser for the recursive grammar system.
 * Replaces all the separate parsers (sections, layers, patterns, variations)
 * with a single recursive parser that handles any JSON structure.
 * 
 * This class doesn't exist yet - we're defining the interface from our TDD tests!
 */
class UniversalMusicParser
{
public:
    // Main parsing entry points
    static std::unique_ptr<MusicalContainer> parseJSON(const juce::String& jsonString);
    static std::unique_ptr<MusicalContainer> parseJSON(const juce::var& jsonVar);
    
    // Parse specific components
    static MusicalContainer parseContainer(const juce::var& containerJson);
    static Context parseContext(const juce::var& contextJson);
    static Event parseEvent(const juce::var& eventJson);
    static std::vector<Event> parseEvents(const juce::var& eventsJson);
    
    // Parse expression features
    static ExpressionMap parseExpression(const juce::var& expressionJson);
    static ProcessorMap parseProcessors(const juce::var& processorsJson);
    static AutomationMap parseAutomation(const juce::var& automationJson);
    static std::map<juce::String, juce::String> parseShortcuts(const juce::var& shortcutsJson);
    
    // Parse advanced features
    static ChordProgression parseChordProgression(const juce::var& chordsJson);
    static AIConstraints parseConstraints(const juce::var& constraintsJson);
    static PerformanceContext parsePerformance(const juce::var& performanceJson);
    
    // Validation
    static juce::Result validateFormat(const juce::var& json);
    static juce::Result validateContainer(const juce::var& container);
    static juce::Result validateEvent(const juce::var& event);
    
    // Version detection
    static juce::String detectVersion(const juce::var& json);
    
    // Error handling
    static juce::String getLastError();
    
private:
    // Temporal relationship types for containers
    enum class TemporalRelationship {
        Simultaneous,  // Layers, instruments (play together)
        Sequential,    // Variations, bars (play one after another)
        Named,         // Patterns, templates (reusable)
        Structure      // Sections, movements (organizational)
    };
    
    // Self-describing alias definition with quantitative relationships
    struct AliasDefinition {
        TemporalRelationship type;
        juce::String contains;          // What this container holds
        juce::var count;               // How many (int, "variable", "free", "infinite", {min,max})
        juce::String pattern;          // Layout pattern ("AABA", "AAB", etc.)
        double defaultDuration;        // Fallback duration if count unavailable
        juce::var metadata;           // Additional properties
        
        AliasDefinition() 
            : type(TemporalRelationship::Sequential), defaultDuration(4.0) {}
        AliasDefinition(TemporalRelationship t, const juce::String& c = {}, const juce::var& cnt = 1) 
            : type(t), contains(c), count(cnt), defaultDuration(4.0) {}
    };
    
    // Dynamic alias registry populated from pattern
    static std::map<juce::String, AliasDefinition> currentAliases;
    
    // Internal parsing methods
    static MusicalContainer parseRecursiveStructure(const juce::var& json);
    static void parseLayersInto(MusicalContainer& container, const juce::var& layersJson);
    static void parseVariationsInto(MusicalContainer& container, const juce::var& variationsJson);
    static void parsePatternsInto(MusicalContainer& container, const juce::var& patternsJson);
    static void parseSectionsInto(MusicalContainer& container, const juce::var& sectionsJson);
    
    // Universal container parsing
    static void parseContainerInto(MusicalContainer& parent, 
                                  const juce::var& containersJson,
                                  const juce::String& semanticType);
    static TemporalRelationship getTemporalRelationship(const juce::String& semanticType);
    
    // Self-describing alias support
    static void parseAliases(const juce::var& aliasesJson);
    static void clearAliases();
    static TemporalRelationship getRelationshipForAlias(const juce::String& alias);
    static bool hasAlias(const juce::String& alias);
    static double calculateContainerDuration(const juce::String& containerType, 
                                           const MusicalContainer& container);
    static int getExpectedChildCount(const juce::String& containerType);
    
    // Expression parsing helpers
    static void parseExpressionInto(Event& event, const juce::var& expressionJson);
    static void parseProcessorsInto(Event& event, const juce::var& processorsJson);
    static void parseAutomationInto(Event& event, const juce::var& automationJson);
    
    // Shorthand expansion
    static EventStream expandShorthand(const juce::var& shorthandJson, 
                                     const std::map<juce::String, juce::String>& shortcuts);
    static std::vector<Event> expandDotNotation(const juce::String& pattern, 
                                               const std::map<juce::String, juce::String>& shortcuts);
    
    // Context resolution
    static Context mergeContexts(const Context& parent, const Context& child);
    
    // Harmony resolution
    static int resolveChordDegree(int degree, const juce::String& chord, int octave = 4);
    static int resolveNoteToMidi(const juce::String& note, int octave = 4);
    
    // Error state
    static juce::String lastError;
    
    // Pattern propagation helpers
    static void propagatePatterns(MusicalContainer& container);
    static void propagatePatternsTo(MusicalContainer& child, const std::map<juce::String, MusicalContainer>& rootPatterns);
};

/**
 * Converter between different format versions
 */
class MusicJsonConverter
{
public:
    // Version detection
    static juce::String detectVersion(const juce::var& json);
    
    // Format conversion
    static juce::var convertToV2(const juce::var& v1Json);
    static juce::var convertToV1(const juce::var& v2Json);
    
    // Validation
    static juce::Result validateV1Format(const juce::var& json);
    static juce::Result validateV2Format(const juce::var& json);
    
private:
    static juce::var convertV1Section(const juce::var& section);
    static juce::var convertV1Pattern(const juce::var& pattern);
    static juce::var convertV2ToV1Structure(const juce::var& container);
};

} // namespace covertracks