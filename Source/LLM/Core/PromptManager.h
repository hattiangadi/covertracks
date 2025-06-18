// Source/LLM/Core/PromptManager.h
#pragma once

#include <JuceHeader.h>

namespace covertracks {

// Forward declarations
class Player;
struct InstrumentCapabilities;

/**
 * @brief Manages prompt templates for LLM interactions
 * 
 * This class provides functionality for creating and managing prompts
 * for various LLM interactions, including music pattern generation.
 */
class PromptManager {
public:
    /**
     * @brief Construct a new Prompt Manager
     */
    PromptManager();
    
    /**
     * @brief Create a prompt for generating a musical pattern
     * 
     * @param style The musical style (e.g., "jazz", "rock")
     * @param instruments The instruments to include
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @return String The generated prompt
     */
    String createMusicGenerationPrompt(const String& style,
                                     const StringArray& instruments,
                                     int tempo,
                                     const String& description);
    
    /**
     * @brief Create a prompt for generating a musical pattern using Player capabilities
     * 
     * @param style The musical style (e.g., "jazz", "rock")
     * @param players The players with their instrument capabilities
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @return String The generated prompt
     */
    String createMusicGenerationPrompt(const String& style,
                                     const std::vector<Player*>& players,
                                     int tempo,
                                     const String& description);
    
    /**
     * @brief Create a prompt for generating a musical pattern with enhanced fields
     * 
     * @param style The musical style (e.g., "jazz", "pop", "rock", "classical")
     * @param character Character description for the piece
     * @param instrument The target instrument
     * @param instrumentRoleDefinition Role definition for the instrument
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @return String The generated prompt
     */
    String createMusicGenerationPrompt(const String& style,
                                     const String& character,
                                     const String& instrument,
                                     const String& instrumentRoleDefinition,
                                     int tempo,
                                     const String& description);
    
    /**
     * @brief Create a prompt for generating a musical pattern with Player capabilities
     * 
     * @param style The musical style (e.g., "jazz", "pop", "rock", "classical")
     * @param character Character description for the piece
     * @param player The player with instrument capabilities
     * @param instrumentRoleDefinition Role definition for the instrument
     * @param tempo The tempo in BPM
     * @param description Additional description or constraints
     * @return String The generated prompt
     */
    String createMusicGenerationPrompt(const String& style,
                                     const String& character,
                                     Player* player,
                                     const String& instrumentRoleDefinition,
                                     int tempo,
                                     const String& description);
    
    /**
     * @brief Create a prompt for modifying an existing pattern
     * 
     * @param currentPattern The current pattern notation
     * @param modification Description of modifications to make
     * @return String The generated prompt
     */
    String createPatternModificationPrompt(const String& currentPattern,
                                         const String& modification);
    
    /**
     * @brief Create a prompt for generating a specific section
     * 
     * @param section The section type (e.g., "verse", "chorus")
     * @param style The musical style
     * @param instruments The instruments to include
     * @param description Additional description or constraints
     * @return String The generated prompt
     */
    String createSectionGenerationPrompt(const String& section,
                                       const String& style,
                                       const StringArray& instruments,
                                       const String& description);
    
    /**
     * @brief Get a prompt template by name
     * 
     * @param templateName The name of the template
     * @return String The template text
     */
    String getPromptTemplate(const String& templateName);
    
    /**
     * @brief Load prompt templates from a file
     * 
     * @param file The file containing prompt templates
     * @return bool True if loading was successful
     */
    bool loadPromptTemplates(const File& file);
    
private:
    // Prompt templates
    std::map<String, String> promptTemplates;
    
    // Instrument definitions (cached for prompts)
    var instrumentDefinitions;
    
    // Initialize default templates
    void initializeDefaultTemplates();
    
    // Helper method to load instrument definitions
    void loadInstrumentDefinitions();
    
    // Helper method to convert Player capabilities to instrument definitions
    String createInstrumentDefinitionsFromPlayers(const std::vector<Player*>& players);
    
    // Helper method to replace template variables
    String processTemplate(const String& templateText, 
                          const std::map<String, String>& variables);
    
    // Helper method to load the compact musical JSON specification
    String getCompactMusicalJsonSpec();
    
    // Helper method to get instrument definition for a specific instrument
    String getInstrumentDefinition(const String& instrument);
    
    // Helper method to get instrument definition from Player capabilities
    String getInstrumentDefinitionFromPlayer(Player* player);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptManager)
};

} // namespace covertracks