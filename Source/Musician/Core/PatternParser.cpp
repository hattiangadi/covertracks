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
    
    // Extract sections with arrangement support
    if (json.hasProperty("sections"))
    {
        var sectionsJson = json["sections"];
        if (sectionsJson.isObject())
        {
            // First, parse all sections into a map
            std::map<String, std::shared_ptr<Section>> sectionMap;
            
            for (auto& sectionProp : sectionsJson.getDynamicObject()->getProperties())
            {
                String sectionName = sectionProp.name.toString();
                var sectionJson = sectionProp.value;
                
                auto section = parseSectionFromJson(sectionJson, primitives, chords, patterns);
                if (section)
                {
                    section->setName(sectionName);
                    sectionMap[sectionName] = section;
                    DBG("Parsed section: " + sectionName);
                }
            }
            
            // Check for arrangement field
            if (json.hasProperty("arrangement"))
            {
                var arrangementJson = json["arrangement"];
                if (arrangementJson.isArray())
                {
                    // Add sections in arrangement order
                    for (const auto& sectionNameVar : *arrangementJson.getArray())
                    {
                        String sectionName = sectionNameVar.toString();
                        auto sectionIt = sectionMap.find(sectionName);
                        if (sectionIt != sectionMap.end())
                        {
                            // Create a copy of the section for the arrangement
                            auto arrangedSection = sectionIt->second;
                            chart->addSection(arrangedSection);
                            DBG("Added arranged section: " + sectionName);
                        }
                        else
                        {
                            DBG("Warning: Section '" + sectionName + "' in arrangement not found");
                        }
                    }
                }
            }
            else
            {
                // No arrangement - add sections in definition order (fallback)
                for (auto& sectionPair : sectionMap)
                {
                    chart->addSection(sectionPair.second);
                    DBG("Added section (no arrangement): " + sectionPair.first);
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
    
    // Parse layers - merge all layers into a single unified phrase
    if (sectionJson.hasProperty("layers"))
    {
        var layersJson = sectionJson["layers"];
        if (layersJson.isObject())
        {
            // Create a single merged phrase for all layers
            auto mergedPhrase = std::make_shared<Phrase>();
            mergedPhrase->setName("MergedLayers");
            
            // Create merged bars for each bar index
            for (int barIndex = 0; barIndex < numBars; ++barIndex)
            {
                auto mergedBar = std::make_shared<Bar>();
                
                // Collect all events from all layers for this bar
                Array<var> allEvents;
                
                for (auto& layerProp : layersJson.getDynamicObject()->getProperties())
                {
                    String layerName = layerProp.name.toString();
                    var layerJson = layerProp.value;
                    
                    Array<var> layerEvents;
                    
                    // Check if layer uses a pattern reference
                    if (layerJson.hasProperty("pattern"))
                    {
                        String patternName = layerJson["pattern"].toString();
                        auto patternIt = patterns.find(patternName);
                        if (patternIt != patterns.end())
                        {
                            layerEvents = patternIt->second;
                            DBG("Merging pattern '" + patternName + "' from layer '" + layerName + "'");
                        }
                    }
                    // Check if layer has direct events
                    else if (layerJson.isArray())
                    {
                        for (const auto& event : *layerJson.getArray())
                        {
                            layerEvents.add(event);
                        }
                    }
                    
                    // Add all events from this layer to the merged collection
                    for (const auto& event : layerEvents)
                    {
                        allEvents.add(event);
                    }
                    
                    DBG("Added " + String(layerEvents.size()) + " events from layer '" + layerName + "'");
                }
                
                // Parse the merged events into a single bar
                if (allEvents.size() > 0)
                {
                    auto parsedBar = parseBarFromJson(allEvents, primitives, chords);
                    if (parsedBar)
                    {
                        mergedPhrase->addBar(parsedBar);
                        DBG("Created merged bar " + String(barIndex) + " with " + String(allEvents.size()) + " total events");
                    }
                }
                else
                {
                    // Add empty bar if no events
                    mergedPhrase->addBar(mergedBar);
                }
            }
            
            // Add the single merged phrase to the section
            section->addPhrase(mergedPhrase);
            DBG("Added merged phrase with " + String(mergedPhrase->getNumBars()) + " bars");
        }
    }
    // Handle pattern variations (basePattern + variations)
    else if (sectionJson.hasProperty("basePattern") && sectionJson.hasProperty("variations"))
    {
        String basePatternName = sectionJson["basePattern"].toString();
        var variationsJson = sectionJson["variations"];
        
        auto basePatternIt = patterns.find(basePatternName);
        if (basePatternIt != patterns.end())
        {
            Array<var> basePattern = basePatternIt->second;
            
            auto phrase = std::make_shared<Phrase>();
            phrase->setName("VariationPhrase");
            
            // Create bars with variations
            for (int barIndex = 0; barIndex < numBars; ++barIndex)
            {
                Array<var> barEvents = basePattern;  // Start with base pattern
                
                // Check for variation for this bar
                String barKey = "bar" + String(barIndex + 1);  // bar1, bar2, etc.
                if (variationsJson.hasProperty(barKey))
                {
                    var variationVar = variationsJson.getProperty(barKey, var());
                    if (!variationVar.isVoid())
                    {
                        String variationPatternName = variationVar.toString();
                        auto variationIt = patterns.find(variationPatternName);
                        if (variationIt != patterns.end())
                        {
                            barEvents = variationIt->second;  // Use variation pattern instead
                            DBG("Using variation '" + variationPatternName + "' for " + barKey);
                        }
                    }
                }
                
                // Parse the events (base or variation) into the bar
                auto bar = parseBarFromJson(barEvents, primitives, chords);
                if (bar)
                {
                    phrase->addBar(bar);
                }
            }
            
            section->addPhrase(phrase);
            DBG("Added variation phrase with " + String(phrase->getNumBars()) + " bars");
        }
    }
    // Handle multi-bar patterns (T2_MultiBars)
    else if (sectionJson.hasProperty("barPatterns"))
    {
        var barPatternsJson = sectionJson["barPatterns"];
        if (barPatternsJson.isArray())
        {
            auto phrase = std::make_shared<Phrase>();
            phrase->setName("MultiBarPhrase");
            
            // Parse each bar pattern
            for (const auto& barPatternVar : *barPatternsJson.getArray())
            {
                if (barPatternVar.isArray())
                {
                    Array<var> barEvents;
                    for (const auto& event : *barPatternVar.getArray())
                    {
                        barEvents.add(event);
                    }
                    
                    auto bar = parseBarFromJson(barEvents, primitives, chords);
                    if (bar)
                    {
                        phrase->addBar(bar);
                    }
                }
            }
            
            section->addPhrase(phrase);
            DBG("Added multi-bar phrase with " + String(phrase->getNumBars()) + " bars");
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