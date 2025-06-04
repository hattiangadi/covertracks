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

std::shared_ptr<Chart> PatternParser::parseChart(const String& jsonText)
{
    DBG("Parsing JSON chart: " + jsonText);
    
    // Parse JSON
    var json = JSON::parse(jsonText);
    if (json.isVoid())
    {
        DBG("Failed to parse JSON");
        return nullptr;
    }
    
    return parseChartFromJson(json);
}

std::shared_ptr<Chart> PatternParser::parseChartFromJson(const var& json)
{
    auto chart = std::make_shared<Chart>();
    
    // Extract basic properties
    if (json.hasProperty("title"))
        chart->setTitle(json["title"].toString());
    
    if (json.hasProperty("tempo"))
        chart->setTempo(json["tempo"]);
    
    DBG("Chart title: " + chart->getTitle() + ", tempo: " + String(chart->getTempo()));
    
    // Extract primitives
    std::map<String, std::map<String, int>> primitives;
    if (json.hasProperty("primitives"))
    {
        var primitivesJson = json["primitives"];
        if (primitivesJson.isObject())
        {
            for (auto& instrumentProp : primitivesJson.getDynamicObject()->getProperties())
            {
                String instrumentName = instrumentProp.name.toString();
                var instrumentPrimitives = instrumentProp.value;
                
                if (instrumentPrimitives.isObject())
                {
                    for (auto& primitiveProp : instrumentPrimitives.getDynamicObject()->getProperties())
                    {
                        String primitiveName = primitiveProp.name.toString();
                        int midiNote = primitiveProp.value;
                        primitives[instrumentName][primitiveName] = midiNote;
                        DBG("Primitive: " + instrumentName + "." + primitiveName + " = " + String(midiNote));
                    }
                }
            }
        }
    }
    
    // Extract chords
    std::map<String, Array<String>> chords;
    if (json.hasProperty("chords"))
    {
        var chordsJson = json["chords"];
        if (chordsJson.isObject())
        {
            for (auto& chordProp : chordsJson.getDynamicObject()->getProperties())
            {
                String chordName = chordProp.name.toString();
                var chordNotes = chordProp.value;
                
                if (chordNotes.isArray())
                {
                    Array<String> noteArray;
                    for (const auto& note : *chordNotes.getArray())
                    {
                        noteArray.add(note.toString());
                    }
                    chords[chordName] = noteArray;
                    String noteList = "";
                    for (int i = 0; i < noteArray.size(); ++i)
                    {
                        if (i > 0) noteList += ", ";
                        noteList += noteArray[i];
                    }
                    DBG("Chord: " + chordName + " = " + noteList);
                }
            }
        }
    }
    
    // Extract patterns
    std::map<String, Array<var>> patterns;
    if (json.hasProperty("patterns"))
    {
        var patternsJson = json["patterns"];
        if (patternsJson.isObject())
        {
            for (auto& patternProp : patternsJson.getDynamicObject()->getProperties())
            {
                String patternName = patternProp.name.toString();
                var patternEvents = patternProp.value;
                
                if (patternEvents.isArray())
                {
                    Array<var> eventArray;
                    for (const auto& event : *patternEvents.getArray())
                    {
                        eventArray.add(event);
                    }
                    patterns[patternName] = eventArray;
                    DBG("Pattern: " + patternName + " with " + String(eventArray.size()) + " events");
                }
            }
        }
    }
    
    // Extract sections
    if (json.hasProperty("sections"))
    {
        var sectionsJson = json["sections"];
        if (sectionsJson.isObject())
        {
            for (auto& sectionProp : sectionsJson.getDynamicObject()->getProperties())
            {
                String sectionName = sectionProp.name.toString();
                var sectionJson = sectionProp.value;
                
                auto section = parseSectionFromJson(sectionJson, primitives, chords, patterns);
                if (section)
                {
                    section->setName(sectionName);
                    chart->addSection(section);
                    DBG("Added section: " + sectionName);
                }
            }
        }
    }
    
    return chart;
}

std::shared_ptr<Section> PatternParser::parseSectionFromJson(const var& sectionJson,
                                                            const std::map<String, std::map<String, int>>& primitives,
                                                            const std::map<String, Array<String>>& chords,
                                                            const std::map<String, Array<var>>& patterns)
{
    auto section = std::make_shared<Section>();
    
    // Handle section references
    if (sectionJson.hasProperty("reference"))
    {
        String referenceName = sectionJson["reference"].toString();
        DBG("Section references: " + referenceName + " (not yet implemented)");
        // TODO: Implement section referencing
    }
    
    // Get number of bars (default 1)
    int numBars = 1;
    if (sectionJson.hasProperty("bars"))
        numBars = sectionJson["bars"];
    
    // Parse layers
    if (sectionJson.hasProperty("layers"))
    {
        var layersJson = sectionJson["layers"];
        if (layersJson.isObject())
        {
            // Create phrases for each layer
            std::map<String, std::shared_ptr<Phrase>> layerPhrases;
            
            for (auto& layerProp : layersJson.getDynamicObject()->getProperties())
            {
                String layerName = layerProp.name.toString();
                var layerJson = layerProp.value;
                
                auto phrase = std::make_shared<Phrase>();
                phrase->setName(layerName);
                
                // Create bars for this layer
                for (int barIndex = 0; barIndex < numBars; ++barIndex)
                {
                    auto bar = std::make_shared<Bar>();
                    
                    Array<var> events;
                    
                    // Check if layer uses a pattern reference
                    if (layerJson.hasProperty("pattern"))
                    {
                        String patternName = layerJson["pattern"].toString();
                        auto patternIt = patterns.find(patternName);
                        if (patternIt != patterns.end())
                        {
                            events = patternIt->second;
                            DBG("Using pattern '" + patternName + "' for layer '" + layerName + "'");
                        }
                    }
                    // Check if layer has direct events
                    else if (layerJson.isArray())
                    {
                        for (const auto& event : *layerJson.getArray())
                        {
                            events.add(event);
                        }
                    }
                    
                    // Parse events into the bar
                    if (events.size() > 0)
                    {
                        auto parsedBar = parseBarFromJson(events, primitives, chords);
                        if (parsedBar)
                        {
                            // Copy events from parsed bar
                            phrase->addBar(parsedBar);
                        }
                    }
                    else
                    {
                        // Add empty bar
                        phrase->addBar(bar);
                    }
                }
                
                layerPhrases[layerName] = phrase;
            }
            
            // For now, add all layer phrases to the section
            // TODO: Implement proper layer merging
            for (auto& layerPair : layerPhrases)
            {
                section->addPhrase(layerPair.second);
                DBG("Added layer phrase: " + layerPair.first);
            }
        }
    }
    // Handle simple event arrays (no layers)
    else if (sectionJson.isArray())
    {
        Array<var> events;
        for (const auto& event : *sectionJson.getArray())
        {
            events.add(event);
        }
        
        auto phrase = std::make_shared<Phrase>();
        phrase->setName("Main");
        
        auto bar = parseBarFromJson(events, primitives, chords);
        if (bar)
        {
            phrase->addBar(bar);
            section->addPhrase(phrase);
        }
    }
    
    return section;
}

std::shared_ptr<Bar> PatternParser::parseBarFromJson(const Array<var>& eventsJson,
                                                     const std::map<String, std::map<String, int>>& primitives,
                                                     const std::map<String, Array<String>>& chords)
{
    auto bar = std::make_shared<Bar>();
    
    for (const auto& eventJson : eventsJson)
    {
        if (!eventJson.isObject())
            continue;
            
        // Extract beat position
        double beat = 1.0;
        if (eventJson.hasProperty("beat"))
            beat = eventJson["beat"];
        
        // Extract velocity (0.0-1.0)
        float velocity = 0.8f;
        if (eventJson.hasProperty("velocity"))
            velocity = eventJson["velocity"];
        
        // Extract duration
        float duration = 0.25f;
        if (eventJson.hasProperty("duration"))
            duration = eventJson["duration"];
        
        // Handle primitive events
        if (eventJson.hasProperty("primitive"))
        {
            String primitiveName = eventJson["primitive"].toString();
            
            // Find the primitive in any instrument
            int midiNote = 60; // Default
            int channel = 0;
            
            for (const auto& instrumentPair : primitives)
            {
                auto primitiveIt = instrumentPair.second.find(primitiveName);
                if (primitiveIt != instrumentPair.second.end())
                {
                    midiNote = primitiveIt->second;
                    if (instrumentPair.first == "drums")
                        channel = 9; // Drum channel
                    break;
                }
            }
            
            bar->addNote(static_cast<int>(beat), channel, midiNote, velocity, duration);
            DBG("Added primitive: " + primitiveName + " at beat " + String(beat) + " (MIDI " + String(midiNote) + ")");
        }
        // Handle chord events
        else if (eventJson.hasProperty("chord"))
        {
            String chordName = eventJson["chord"].toString();
            auto chordIt = chords.find(chordName);
            
            if (chordIt != chords.end())
            {
                // Add each note in the chord
                for (const String& noteName : chordIt->second)
                {
                    // Find the note in primitives
                    int midiNote = 60; // Default
                    
                    for (const auto& instrumentPair : primitives)
                    {
                        auto primitiveIt = instrumentPair.second.find(noteName);
                        if (primitiveIt != instrumentPair.second.end())
                        {
                            midiNote = primitiveIt->second;
                            break;
                        }
                    }
                    
                    bar->addNote(static_cast<int>(beat), 0, midiNote, velocity, duration);
                }
                DBG("Added chord: " + chordName + " at beat " + String(beat));
            }
        }
    }
    
    return bar;
}

std::shared_ptr<Chart> PatternParser::parseLLMResponse(const String& response)
{
    // Try to extract JSON from LLM response
    String jsonText = response.trim();
    
    // Look for JSON block markers
    if (jsonText.contains("```json"))
    {
        int startPos = jsonText.indexOf("```json") + 7;
        int endPos = jsonText.indexOf(startPos, "```");
        if (endPos > startPos)
        {
            jsonText = jsonText.substring(startPos, endPos).trim();
        }
    }
    
    return parseChart(jsonText);
}

String PatternParser::extractSongNotation(const String& text)
{
    // Look for JSON objects in the text
    int startPos = text.indexOf("{");
    if (startPos >= 0)
    {
        int braceCount = 0;
        int endPos = startPos;
        
        for (int i = startPos; i < text.length(); ++i)
        {
            if (text[i] == '{') braceCount++;
            else if (text[i] == '}') braceCount--;
            
            if (braceCount == 0)
            {
                endPos = i + 1;
                break;
            }
        }
        
        if (braceCount == 0)
        {
            return text.substring(startPos, endPos);
        }
    }
    
    return String();
}

String PatternParser::extractSectionNotation(const String& text)
{
    // For JSON format, sections are part of the main chart
    return extractSongNotation(text);
}

String PatternParser::extractPhraseNotation(const String& text)
{
    // For JSON format, phrases are part of sections
    return extractSongNotation(text);
}

String PatternParser::extractBarNotation(const String& text)
{
    // For JSON format, bars are part of phrases
    return extractSongNotation(text);
}

} // namespace covertracks