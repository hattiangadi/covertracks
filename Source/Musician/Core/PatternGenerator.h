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

/**
 * @brief Generates musical patterns using an LLM
 * 
 * This class coordinates interaction with an LLM to generate musical patterns
 * in the notation format defined in the Music Pattern System specification.
 */
class PatternGenerator : public Thread {
public:
    /**
     * @brief Construct a new Pattern Generator
     * 
     * @param patternSystem Reference to the MusicPatternSystem
     * @param llmClient Shared pointer to the LLMClient
     * @param promptManager Reference to the PromptManager
     */
    PatternGenerator(MusicPatternSystem& patternSystem,
                    std::shared_ptr<LLMClient> llmClient,
                    PromptManager& promptManager);
    
    /**
     * @brief Destroy the Pattern Generator
     */
    ~PatternGenerator() override;
    
    /**
     * @brief Callback for pattern generation completion
     * 
     * @param success Whether the generation was successful
     * @param song The generated song (nullptr if generation failed)
     * @param response The raw response from the LLM
     */
    using PatternGenerationCallback = std::function<void(bool success, 
                                                       std::shared_ptr<Song> song, 
                                                       const String& response)>;
    
    /**
     * @brief Generate a song based on style and instruments
     * 
     * @param style The musical style (e.g., "jazz", "rock")
     * @param instruments The instruments to include
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @param callback Callback to be invoked when generation completes
     */
    void generateSong(const String& style,
                     const StringArray& instruments,
                     int tempo,
                     const String& description,
                     PatternGenerationCallback callback);
    
    /**
     * @brief Modify an existing pattern
     * 
     * @param currentPattern The current pattern notation
     * @param modification Description of modifications to make
     * @param callback Callback to be invoked when generation completes
     */
    void modifyPattern(const String& currentPattern,
                      const String& modification,
                      PatternGenerationCallback callback);
    
    /**
     * @brief Generate a specific section for a song
     * 
     * @param section The section type (e.g., "verse", "chorus")
     * @param style The musical style
     * @param instruments The instruments to include
     * @param description Additional description or constraints
     * @param callback Callback to be invoked when generation completes
     */
    void generateSection(const String& section,
                        const String& style,
                        const StringArray& instruments,
                        const String& description,
                        PatternGenerationCallback callback);
    
    /**
     * @brief Check if generation is in progress
     * 
     * @return true if generation is in progress
     */
    bool isGenerating() const { return generating; }
    
    /**
     * @brief Get the conversation history
     * 
     * @return Array<std::pair<String, String>> The conversation history (user, assistant)
     */
    Array<std::pair<String, String>> getConversationHistory() const { return conversationHistory; }
    
    /**
     * @brief Clear the conversation history
     */
    void clearConversationHistory() { conversationHistory.clear(); }
    
    /**
     * @brief Set an initial system prompt for the conversation
     * 
     * @param systemPrompt The system prompt
     */
    void setSystemPrompt(const String& newSystemPrompt) { this->systemPrompt = newSystemPrompt; }
    
private:
    // Thread interface
    void run() override;
    
    // Member variables
    MusicPatternSystem& patternSystem;
    std::shared_ptr<LLMClient> llmClient;
    PromptManager& promptManager;
    std::unique_ptr<PatternParser> parser;
    
    // State variables
    bool generating;
    String currentPrompt;
    PatternGenerationCallback currentCallback;
    
    // Conversation history
    String systemPrompt;
    Array<std::pair<String, String>> conversationHistory;
    
    // Queue management
    WaitableEvent waitEvent;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGenerator)
};

} // namespace covertracks