// Source/Musician/Core/PatternGenerator.cpp
#include "PatternGenerator.h"
#include "PatternParser.h"
// Legacy Player.h include removed - using Player
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

String PatternGenerator::createSongPromptWithPlayers(const String& style,
                                                   const std::vector<Player*>& players,
                                                   int tempo,
                                                   const String& description)
{
    return promptManager.createMusicGenerationPrompt(style, players, tempo, description);
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
    DBG("DEBUG: PatternGenerator::validateResponse called");
    DBG("DEBUG: Response length: " << response.length());
    DBG("DEBUG: Response first 200 chars: " << response.substring(0, 200));
    
    // Basic validation - check if response contains typical pattern notation elements
    if (response.isEmpty())
    {
        DBG("DEBUG: Validation failed - response is empty");
        return false;
    }
    
    // Check for basic pattern elements first
    bool hasPatternElements = response.contains("tempo") || 
                             response.contains("sections") || 
                             response.contains("primitives") ||
                             response.contains("bars") ||
                             response.contains("patterns");
    
    DBG("DEBUG: Has pattern elements: " << (hasPatternElements ? "true" : "false"));
    
    // If no pattern elements, definitely invalid
    if (!hasPatternElements)
        return false;
    
    // If it looks like JSON (has braces and quoted properties), try to parse it
    bool looksLikeJson = response.contains("{") && 
                        (response.contains("\"tempo\"") || response.contains("\"sections\"") || 
                         response.contains("\"primitives\"") || response.contains("\"bars\"") || 
                         response.contains("\"patterns\""));
    
    DBG("DEBUG: Looks like JSON: " << (looksLikeJson ? "true" : "false"));
                        
    if (looksLikeJson)
    {
        // Extract JSON from markdown code blocks if present
        String jsonToParse = response;
        
        // Check if response is wrapped in markdown code blocks
        if (response.contains("```json") && response.contains("```"))
        {
            int startIndex = response.indexOf("```json") + 7; // Skip "```json"
            int endIndex = response.lastIndexOf("```");
            
            if (startIndex < endIndex)
            {
                jsonToParse = response.substring(startIndex, endIndex).trim();
                DBG("DEBUG: Extracted JSON from markdown, length: " << jsonToParse.length());
            }
        }
        
        var parsedJson;
        Result parseResult = JSON::parse(jsonToParse, parsedJson);
        
        DBG("DEBUG: JSON parse result: " << (parseResult.wasOk() ? "success" : "failed"));
        if (parseResult.failed()) {
            DBG("DEBUG: JSON parse error: " << parseResult.getErrorMessage());
        }
        
        // If it has braces but fails to parse as JSON, it's malformed
        if (parseResult.failed())
            return false;
        
        // If JSON parsing succeeds, check if it contains music pattern structure
        if (parsedJson.isObject())
        {
            DBG("DEBUG: Parsed JSON is an object");
            DynamicObject* obj = parsedJson.getDynamicObject();
            if (obj)
            {
                bool hasTempo = obj->hasProperty("tempo");
                bool hasSections = obj->hasProperty("sections");
                bool hasPrimitives = obj->hasProperty("primitives");
                bool hasBars = obj->hasProperty("bars");
                bool hasPatterns = obj->hasProperty("patterns");
                
                DBG("DEBUG: JSON properties - tempo: " << (hasTempo ? "true" : "false") 
                    << ", sections: " << (hasSections ? "true" : "false")
                    << ", primitives: " << (hasPrimitives ? "true" : "false")
                    << ", bars: " << (hasBars ? "true" : "false")
                    << ", patterns: " << (hasPatterns ? "true" : "false"));
                
                bool hasValidStructure = hasTempo || hasSections || hasPrimitives || hasBars || hasPatterns;
                DBG("DEBUG: Has valid structure: " << (hasValidStructure ? "true" : "false"));
                
                return hasValidStructure;
            }
            else
            {
                DBG("DEBUG: Failed to get DynamicObject from parsed JSON");
            }
        }
        else
        {
            DBG("DEBUG: Parsed JSON is not an object");
        }
        
        DBG("DEBUG: Validation failed - returning false");
        return false;
    }
    
    // If it doesn't look like JSON but has pattern elements, accept it
    return true;
}

} // namespace covertracks