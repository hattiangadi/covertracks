// Source/Musician/Core/PatternParser.cpp
#include "PatternParser.h"
#include "MusicPatternSystem.h"
#include <iostream>
#include <sstream>
#include <regex>

namespace covertracks {

PatternParser::PatternParser(MusicPatternSystem& system)
    : patternSystem(system)
{
}

std::shared_ptr<Song> PatternParser::parseSong(const String& notation)
{
    // Use the notation parameter to create a song
    return patternSystem.createSongFromNotation(notation);
}

std::shared_ptr<Section> PatternParser::parseSection(const String& notation)
{
    // Use the notation parameter to create a section
    return patternSystem.createSectionFromNotation(notation);
}

std::shared_ptr<Phrase> PatternParser::parsePhrase(const String& notation)
{
    // Use the notation parameter to create a phrase
    return patternSystem.createPhraseFromNotation(notation);
}

std::shared_ptr<Bar> PatternParser::parseBar(const String& notation)
{
    // Use the notation parameter to create a bar
    return patternSystem.createBarFromNotation(notation);
}

std::shared_ptr<Song> PatternParser::parseLLMResponse(const String& response)
{
    // Extract song notation from the LLM response
    String songNotation = extractSongNotation(response);
    
    if (songNotation.isEmpty())
    {
        // If no song notation found, try to extract a section
        String sectionNotation = extractSectionNotation(response);
        
        if (!sectionNotation.isEmpty())
        {
            // Create a song with just this section
            auto section = parseSection(sectionNotation);
            auto song = std::make_shared<Song>();
            song->setTitle("Generated Song");
            song->addSection(section);
            return song;
        }
        
        // If no section found, try to extract a phrase
        String phraseNotation = extractPhraseNotation(response);
        
        if (!phraseNotation.isEmpty())
        {
            // Create a song with a section containing just this phrase
            auto phrase = parsePhrase(phraseNotation);
            auto section = std::make_shared<Section>();
            section->setName("Generated Section");
            section->addPhrase(phrase);
            
            auto song = std::make_shared<Song>();
            song->setTitle("Generated Song");
            song->addSection(section);
            return song;
        }
        
        // If no phrase found, try to extract a bar
        String barNotation = extractBarNotation(response);
        
        if (!barNotation.isEmpty())
        {
            // Create a song with a section containing a phrase with just this bar
            auto bar = parseBar(barNotation);
            auto phrase = std::make_shared<Phrase>();
            phrase->setName("Generated Phrase");
            phrase->addBar(bar);
            
            auto section = std::make_shared<Section>();
            section->setName("Generated Section");
            section->addPhrase(phrase);
            
            auto song = std::make_shared<Song>();
            song->setTitle("Generated Song");
            song->addSection(section);
            return song;
        }
        
        // If nothing found, return an empty song
        std::cout << "No pattern notation found in LLM response" << std::endl;
        return std::make_shared<Song>();
    }
    
    // Parse the song notation
    return parseSong(songNotation);
}

String PatternParser::extractSongNotation(const String& text)
{
    // Look for a song pattern that starts with [SONG: and ends with ]
    int songStart = text.indexOf("[SONG:");
    if (songStart >= 0)
    {
        int nestedLevel = 1;
        int currentPos = songStart + 6; // Start after [SONG:
        
        while (currentPos < text.length())
        {
            if (text[currentPos] == '[')
                nestedLevel++;
            else if (text[currentPos] == ']')
            {
                nestedLevel--;
                if (nestedLevel == 0)
                {
                    // Found the end of the song notation
                    return text.substring(songStart, currentPos + 1);
                }
            }
            
            currentPos++;
        }
    }
    
    // Alternative format: [SONG with no colon
    songStart = text.indexOf("[SONG");
    if (songStart >= 0 && (songStart + 5 >= text.length() || !CharacterFunctions::isLetter(text[songStart + 5])))
    {
        int nestedLevel = 1;
        int currentPos = songStart + 5; // Start after [SONG
        
        while (currentPos < text.length())
        {
            if (text[currentPos] == '[')
                nestedLevel++;
            else if (text[currentPos] == ']')
            {
                nestedLevel--;
                if (nestedLevel == 0)
                {
                    // Found the end of the song notation
                    return text.substring(songStart, currentPos + 1);
                }
            }
            
            currentPos++;
        }
    }
    
    // If no [SONG: pattern found, look for a series of sections
    if (text.contains("<") && text.contains(">"))
    {
        // Find the first section
        int firstSection = text.indexOf("<");
        
        // Find the last section
        int lastSection = text.lastIndexOf(">");
        
        if (firstSection >= 0 && lastSection > firstSection)
        {
            // Wrap the sections in a song
            return "[SONG: " + text.substring(firstSection, lastSection + 1) + "]";
        }
    }
    
    return String();
}

String PatternParser::extractSectionNotation(const String& text)
{
    // Look for a section pattern that starts with < and ends with >
    // and contains a colon
    int start = text.indexOf("<");
    
    while (start >= 0)
    {
        int end = text.indexOf(start, ">");
        if (end > start)
        {
            String section = text.substring(start, end + 1);
            if (section.contains(":"))
                return section;
        }
        
        // Try the next occurrence
        start = text.indexOf(start + 1, "<");
    }
    
    return String();
}

String PatternParser::extractPhraseNotation(const String& text)
{
    // Look for a phrase pattern that starts with { and ends with }
    // and contains a colon
    int start = text.indexOf("{");
    
    while (start >= 0)
    {
        int end = text.indexOf(start, "}");
        if (end > start)
        {
            String phrase = text.substring(start, end + 1);
            if (phrase.contains(":"))
                return phrase;
        }
        
        // Try the next occurrence
        start = text.indexOf(start + 1, "{");
    }
    
    return String();
}

String PatternParser::extractBarNotation(const String& text)
{
    // Look for a bar pattern that starts with [ and ends with ]
    int start = text.indexOf("[");
    
    while (start >= 0)
    {
        int end = text.indexOf(start, "]");
        if (end > start)
        {
            return text.substring(start, end + 1);
        }
        
        // Try the next occurrence
        start = text.indexOf(start + 1, "[");
    }
    
    return String();
}

} // namespace covertracks