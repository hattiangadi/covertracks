// Source/Musician/Core/PatternGenerator.cpp
#include "PatternGenerator.h"
#include "PatternParser.h"
#include <iostream>

namespace covertracks {

PatternGenerator::PatternGenerator(MusicPatternSystem& system, PromptManager& manager)
    : patternSystem(system),
      promptManager(manager),
      parser(std::make_unique<PatternParser>(system))
{
    // Set default system prompt
    systemPrompt = 
        "You are a music pattern generator. Your role is to create music patterns based on the user's requests. "
        "You'll be given specific instructions about the style, instruments, and structure of the music to create. "
        "Always respond in the specified pattern notation format. Do not include any explanations or comments "
        "outside the requested pattern format.";
}

PatternGenerator::~PatternGenerator() = default;

String PatternGenerator::createSongPrompt(const String& style,
                                         const StringArray& instruments,
                                         int tempo,
                                         const String& description)
{
    return promptManager.createMusicGenerationPrompt(style, instruments, tempo, description);
}

String PatternGenerator::createModificationPrompt(const String& currentPattern,
                                                 const String& modification)
{
    return promptManager.createPatternModificationPrompt(currentPattern, modification);
}

String PatternGenerator::createSectionPrompt(const String& section,
                                            const String& style,
                                            const StringArray& instruments,
                                            const String& description)
{
    return promptManager.createSectionGenerationPrompt(section, style, instruments, description);
}

std::shared_ptr<Chart> PatternGenerator::parseResponse(const String& response)
{
    return parser->parseLLMResponse(response);
}

bool PatternGenerator::validateResponse(const String& response)
{
    // Basic validation - check if response contains typical pattern notation elements
    if (response.isEmpty())
        return false;
    
    // Check for JSON-like structure or pattern notation keywords
    bool hasJsonStructure = response.contains("{") && response.contains("}");
    bool hasPatternElements = response.contains("tempo") || 
                             response.contains("sections") || 
                             response.contains("primitives") ||
                             response.contains("bars") ||
                             response.contains("patterns");
    
    return hasJsonStructure || hasPatternElements;
}

} // namespace covertracks