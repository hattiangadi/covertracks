// Source/Musician/Core/PatternParser.h
#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"

namespace covertracks {

// Forward declaration
class MusicPatternSystem;

/**
 * @brief Parses text notation into musical pattern structures
 * 
 * This class provides functionality for parsing the pattern notation
 * described in the Music Pattern System specification.
 */
class PatternParser {
public:
    /**
     * @brief Construct a new Pattern Parser
     * 
     * @param patternSystem Reference to the MusicPatternSystem
     */
    PatternParser(MusicPatternSystem& patternSystem);
    
    /**
     * @brief Parse a complete chart from JSON notation
     * 
     * @param jsonText The JSON chart notation
     * @return std::shared_ptr<Chart> The parsed chart
     */
    std::shared_ptr<Chart> parseChart(const String& jsonText);
    
    /**
     * @brief Parse a chart from JSON object
     * 
     * @param json The JSON object
     * @return std::shared_ptr<Chart> The parsed chart
     */
    std::shared_ptr<Chart> parseChartFromJson(const var& json);
    
    /**
     * @brief Parse a section from JSON
     * 
     * @param sectionJson The section JSON object
     * @param primitives Available primitives
     * @param chords Available chords
     * @param patterns Available patterns
     * @return std::shared_ptr<Section> The parsed section
     */
    std::shared_ptr<Section> parseSectionFromJson(const var& sectionJson, 
                                                  const std::map<String, std::map<String, int>>& primitives,
                                                  const std::map<String, Array<String>>& chords,
                                                  const std::map<String, Array<var>>& patterns);
    
    /**
     * @brief Parse a bar from events JSON
     * 
     * @param eventsJson Array of event objects
     * @param primitives Available primitives
     * @param chords Available chords
     * @return std::shared_ptr<Bar> The parsed bar
     */
    std::shared_ptr<Bar> parseBarFromJson(const Array<var>& eventsJson,
                                          const std::map<String, std::map<String, int>>& primitives,
                                          const std::map<String, Array<String>>& chords);
    
    /**
     * @brief Parse an LLM-generated response into a chart
     * 
     * @param response The full text response from an LLM
     * @return std::shared_ptr<Chart> The parsed chart
     */
    std::shared_ptr<Chart> parseLLMResponse(const String& response);
    
private:
    MusicPatternSystem& patternSystem;
    
    // Helper functions for parsing notation
    String extractSongNotation(const String& text);
    String extractSectionNotation(const String& text);
    String extractPhraseNotation(const String& text);
    String extractBarNotation(const String& text);
    
    // Helper methods for compact format parsing
    int findMidiNoteForPrimitive(const String& primitiveName, const std::map<String, std::map<String, int>>& primitives);
    int getChannelForPrimitive(const String& primitiveName, const std::map<String, std::map<String, int>>& primitives);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternParser)
};

} // namespace covertracks