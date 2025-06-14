// Source/Musician/Core/PatternGenerator.h
#pragma once

#include <JuceHeader.h>
#include "MusicPatternComponents.h"
#include "../../LLM/Core/LLMClient.h"
#include "../../LLM/Core/PromptManager.h"

namespace covertracks {

// Forward declarations
class MusicPatternSystem;
class PatternParser;
class Player;

/**
 * @brief Pure utility class for pattern generation and parsing
 * 
 * This class provides utilities for creating prompts and parsing LLM responses
 * to generate musical patterns. It does NOT manage threads or async operations.
 * Thread management is handled by the Musician class.
 */
class PatternGenerator {
public:
    /**
     * @brief Construct a new Pattern Generator
     * 
     * @param patternSystem Reference to the MusicPatternSystem
     * @param promptManager Reference to the PromptManager
     */
    PatternGenerator(MusicPatternSystem& patternSystem, PromptManager& promptManager);
    
    /**
     * @brief Destroy the Pattern Generator
     */
    ~PatternGenerator();
    
    /**
     * @brief Create a song generation prompt
     * 
     * @param style The musical style (e.g., "jazz", "rock")
     * @param instruments The instruments to include
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @return String The formatted prompt for the LLM
     */
    String createSongPrompt(const String& style,
                           const StringArray& instruments,
                           int tempo,
                           const String& description);
    
    /**
     * @brief Create a song generation prompt using Player capabilities
     * 
     * @param style The musical style (e.g., "jazz", "rock")
     * @param players Vector of Player pointers to use their capabilities
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @return String The formatted prompt for the LLM
     */
    String createSongPromptWithPlayers(const String& style,
                                     const std::vector<Player*>& players,
                                     int tempo,
                                     const String& description);
    
    /**
     * @brief Create a pattern modification prompt
     * 
     * @param currentPattern The current pattern notation
     * @param modification Description of modifications to make
     * @return String The formatted prompt for the LLM
     */
    String createModificationPrompt(const String& currentPattern,
                                   const String& modification);
    
    /**
     * @brief Create a section generation prompt
     * 
     * @param section The section type (e.g., "verse", "chorus")
     * @param style The musical style
     * @param instruments The instruments to include
     * @param description Additional description or constraints
     * @return String The formatted prompt for the LLM
     */
    String createSectionPrompt(const String& section,
                              const String& style,
                              const StringArray& instruments,
                              const String& description);
    
    /**
     * @brief Parse an LLM response into a Chart
     * 
     * @param response The raw LLM response text
     * @return std::shared_ptr<Chart> The parsed chart (nullptr if parsing failed)
     */
    std::shared_ptr<Chart> parseResponse(const String& response);
    
    /**
     * @brief Validate that a response contains valid pattern notation
     * 
     * @param response The LLM response to validate
     * @return bool True if the response appears to contain valid notation
     */
    bool validateResponse(const String& response);
    
    /**
     * @brief Get the system prompt for pattern generation
     * 
     * @return String The system prompt
     */
    String getSystemPrompt() const { return systemPrompt; }
    
    /**
     * @brief Set the system prompt for pattern generation
     * 
     * @param newSystemPrompt The system prompt
     */
    void setSystemPrompt(const String& newSystemPrompt) { this->systemPrompt = newSystemPrompt; }
    
private:
    // Member variables
    MusicPatternSystem& patternSystem;
    PromptManager& promptManager;
    std::unique_ptr<PatternParser> parser;
    
    // System prompt
    String systemPrompt;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGenerator)
};

} // namespace covertracks