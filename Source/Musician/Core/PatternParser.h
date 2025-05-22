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
     * @brief Parse a complete song from notation
     * 
     * @param notation The song notation
     * @return std::shared_ptr<Song> The parsed song
     */
    std::shared_ptr<Song> parseSong(const String& notation);
    
    /**
     * @brief Parse a section from notation
     * 
     * @param notation The section notation
     * @return std::shared_ptr<Section> The parsed section
     */
    std::shared_ptr<Section> parseSection(const String& notation);
    
    /**
     * @brief Parse a phrase from notation
     * 
     * @param notation The phrase notation
     * @return std::shared_ptr<Phrase> The parsed phrase
     */
    std::shared_ptr<Phrase> parsePhrase(const String& notation);
    
    /**
     * @brief Parse a bar from notation
     * 
     * @param notation The bar notation
     * @return std::shared_ptr<Bar> The parsed bar
     */
    std::shared_ptr<Bar> parseBar(const String& notation);
    
    /**
     * @brief Parse an LLM-generated response into a song
     * 
     * @param response The full text response from an LLM
     * @return std::shared_ptr<Song> The parsed song
     */
    std::shared_ptr<Song> parseLLMResponse(const String& response);
    
private:
    MusicPatternSystem& patternSystem;
    
    // Helper functions for parsing notation
    String extractSongNotation(const String& text);
    String extractSectionNotation(const String& text);
    String extractPhraseNotation(const String& text);
    String extractBarNotation(const String& text);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternParser)
};

} // namespace covertracks