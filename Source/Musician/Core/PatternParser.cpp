// Source/Musician/Core/PatternParser.cpp
#include "PatternParser.h"
#include "MusicPatternSystem.h"
// Legacy Player.h include removed - using TracktionPlayer
#include <iostream>
#include <sstream>
#include <regex>

namespace covertracks {

// Store registered instrument capabilities
static std::map<String, InstrumentCapabilities> registeredCapabilities;

void covertracks::PatternParser::registerInstrumentCapabilities(const String& instrumentName, const InstrumentCapabilities& capabilities)
{
    registeredCapabilities[instrumentName] = capabilities;
}

int covertracks::PatternParser::findNoteFromCapabilities(const String& instrumentName, const String& noteName)
{
    auto it = registeredCapabilities.find(instrumentName);
    if (it != registeredCapabilities.end())
    {
        const InstrumentCapabilities& capabilities = it->second;
        return capabilities.findNote(noteName);
    }
    return -1; // Note not found
}

covertracks::PatternParser::PatternParser(MusicPatternSystem& patternSystem)
    : patternSystem(patternSystem)
{
}

std::shared_ptr<covertracks::Chart> covertracks::PatternParser::parseChart(const String& jsonText)
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

std::shared_ptr<covertracks::Chart> covertracks::PatternParser::parseChartFromJson(const var& json)
{
    auto chart = std::make_shared<Chart>();
    
    // Extract basic properties
    if (json.hasProperty("title"))
        chart->setTitle(json["title"].toString());
    
    if (json.hasProperty("tempo"))
        chart->setTempo(json["tempo"]);
    
    DBG("Chart title: " + chart->getTitle() + ", tempo: " + String(chart->getTempo()));
    
    // First, get all primitives from the MusicPatternSystem
    std::map<String, std::map<String, int>> primitives;
    
    // Get all instrument definitions from the pattern system
    auto instrumentDefinitions = patternSystem.getAllInstrumentDefinitions();
    for (const auto& instrumentPair : instrumentDefinitions)
    {
        const String& instrumentName = instrumentPair.first;
        const InstrumentDefinition& instrumentDef = instrumentPair.second;
        
        // Get all primitives for this instrument
        auto instrumentPrimitives = instrumentDef.getPrimitives();
        for (const auto& primitivePair : instrumentPrimitives)
        {
            const String& primitiveName = primitivePair.first;
            const Primitive& primitive = primitivePair.second;
            
            primitives[instrumentName][primitiveName] = primitive.midiNote;
            DBG("System Primitive: " + instrumentName + "." + primitiveName + " = " + String(primitive.midiNote));
        }
    }
    
    // Now add any primitives defined in the JSON (these will override system primitives if there are conflicts)
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
                        DBG("JSON Primitive: " + instrumentName + "." + primitiveName + " = " + String(midiNote));
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

std::shared_ptr<covertracks::Section> covertracks::PatternParser::parseSectionFromJson(const var& sectionJson,
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
        DBG("Parsing layers. Type: " << (layersJson.isObject() ? "Object" : (layersJson.isArray() ? "Array" : "Other")));
        if (layersJson.isObject() && layersJson.getDynamicObject() != nullptr)
        {
            // Create a single merged phrase for all layers
            auto mergedPhrase = std::make_shared<Phrase>();
            mergedPhrase->setName("MergedLayers");
            
            // Get the layers object once before the loop
            auto* layersObj = layersJson.getDynamicObject();
            
            // First, collect all events from all layers with their absolute beat positions
            Array<var> allEventsAllLayers;
            
            for (auto& layerProp : layersObj->getProperties())
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
                        // For multi-bar sections, repeat the pattern for each bar with offset beats
                        for (int barOffset = 0; barOffset < numBars; ++barOffset)
                        {
                            double beatOffset = barOffset * 4.0; // Each bar is 4 beats
                            
                            for (const auto& patternEvent : patternIt->second)
                            {
                                if (patternEvent.isArray() && patternEvent.size() >= 2)
                                {
                                    // Create new event with offset beat position
                                    Array<var> offsetEventArray;
                                    double originalBeat = static_cast<double>(patternEvent[0]);
                                    double offsetBeat = originalBeat + beatOffset;
                                    offsetEventArray.add(var(offsetBeat));
                                    
                                    // Copy other event properties (note, velocity, duration)
                                    for (int i = 1; i < patternEvent.size(); ++i)
                                    {
                                        offsetEventArray.add(patternEvent[i]);
                                    }
                                    
                                    layerEvents.add(var(offsetEventArray));
                                }
                            }
                        }
                        DBG("Repeated pattern '" + patternName + "' " + String(numBars) + " times for layer '" + layerName + "'");
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
                
                // Add all events from this layer with source metadata
                for (const auto& event : layerEvents)
                {
                    // Add source metadata to track where this event came from
                    var eventWithSource = event;
                    if (eventWithSource.isArray() && eventWithSource.size() >= 2)
                    {
                        // Create a new array with source metadata
                        Array<var> eventArray;
                        for (int i = 0; i < eventWithSource.size(); ++i)
                        {
                            eventArray.add(eventWithSource[i]);
                        }
                        // Add source as 5th element (after beat, note, velocity, duration)
                        while (eventArray.size() < 5) eventArray.add(var()); // Fill missing elements
                        eventArray.set(4, "layer:" + layerName);
                        eventWithSource = var(eventArray);
                    }
                    allEventsAllLayers.add(eventWithSource);
                }
                
                DBG("Added " + String(layerEvents.size()) + " events from layer '" + layerName + "'");
            }
            
            // Now distribute events to the correct bars based on beat position
            for (int barIndex = 0; barIndex < numBars; ++barIndex)
            {
                // Calculate beat range for this bar (1-based beats)
                double barStartBeat = (barIndex * 4) + 1.0;  // Bar 0: beats 1-4, Bar 1: beats 5-8, etc.
                double barEndBeat = barStartBeat + 4.0;      // Exclusive end
                
                // Collect events that belong to this bar
                Array<var> barEvents;
                
                for (const auto& event : allEventsAllLayers)
                {
                    if (event.isArray() && event.size() >= 2)
                    {
                        double eventBeat = static_cast<double>(event[0]);
                        
                        // Check if this event belongs to the current bar
                        if (eventBeat >= barStartBeat && eventBeat < barEndBeat)
                        {
                            // Adjust beat position to be relative to this bar (1-4 range)
                            double relativeBeat = eventBeat - barStartBeat + 1.0;
                            
                            // Create new event with adjusted beat
                            Array<var> adjustedEventArray;
                            adjustedEventArray.add(var(relativeBeat));
                            for (int i = 1; i < event.size(); ++i)
                            {
                                adjustedEventArray.add(event[i]);
                            }
                            
                            barEvents.add(var(adjustedEventArray));
                        }
                    }
                }
                
                // Parse the events for this bar
                if (barEvents.size() > 0)
                {
                    auto parsedBar = parseBarFromJson(barEvents, primitives, chords);
                    if (parsedBar)
                    {
                        mergedPhrase->addBar(parsedBar);
                        DBG("Created bar " + String(barIndex) + " with " + String(barEvents.size()) + " events (beats " + String(barStartBeat) + "-" + String(barEndBeat - 0.01) + ")");
                    }
                }
                else
                {
                    // Add empty bar if no events
                    auto emptyBar = std::make_shared<Bar>();
                    mergedPhrase->addBar(emptyBar);
                    DBG("Created empty bar " + String(barIndex) + " (no events in beat range " + String(barStartBeat) + "-" + String(barEndBeat - 0.01) + ")");
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
                String sourceType = "base:" + basePatternName;
                
                // Check for variation for this bar
                String barKey = "bar" + String(barIndex + 1);  // bar1, bar2, etc.
                if (variationsJson.hasProperty(barKey))
                {
                    var variationVar = variationsJson.getProperty(barKey, var());
                    if (!variationVar.isVoid())
                    {
                        if (variationVar.isString())
                        {
                            // Variation is a pattern name reference
                            String variationPatternName = variationVar.toString();
                            auto variationIt = patterns.find(variationPatternName);
                            if (variationIt != patterns.end())
                            {
                                barEvents = variationIt->second;  // Use variation pattern instead
                                sourceType = "variation:" + variationPatternName;
                                DBG("Using variation pattern '" + variationPatternName + "' for " + barKey);
                            }
                        }
                        else if (variationVar.isArray())
                        {
                            // Variation is a direct array of events
                            Array<var> variationEvents;
                            for (const auto& event : *variationVar.getArray())
                            {
                                variationEvents.add(event);
                            }
                            barEvents = variationEvents;  // Use variation events instead
                            sourceType = "variation:direct";
                            DBG("Using direct variation events for " + barKey + " (" + String(variationEvents.size()) + " events)");
                        }
                    }
                }
                
                // Add source metadata to all events in this bar
                Array<var> eventsWithSource;
                for (const auto& event : barEvents)
                {
                    var eventWithSource = event;
                    if (eventWithSource.isArray() && eventWithSource.size() >= 2)
                    {
                        // Create a new array with source metadata
                        Array<var> eventArray;
                        for (int i = 0; i < eventWithSource.size(); ++i)
                        {
                            eventArray.add(eventWithSource[i]);
                        }
                        // Add source as 5th element (after beat, note, velocity, duration)
                        while (eventArray.size() < 5) eventArray.add(var()); // Fill missing elements
                        eventArray.set(4, sourceType);
                        eventWithSource = var(eventArray);
                    }
                    eventsWithSource.add(eventWithSource);
                }
                
                // Parse the events (base or variation) into the bar
                auto bar = parseBarFromJson(eventsWithSource, primitives, chords);
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

std::shared_ptr<covertracks::Bar> covertracks::PatternParser::parseBarFromJson(const Array<var>& eventsJson,
                                                     const std::map<String, std::map<String, int>>& primitives,
                                                     const std::map<String, Array<String>>& chords)
{
    // Use the enhanced version with chord context tracking
    return parseBarFromJsonWithChordContext(eventsJson, primitives, chords);
}

std::shared_ptr<covertracks::Bar> covertracks::PatternParser::parseBarFromJsonWithChordContext(const Array<var>& eventsJson,
                                                     const std::map<String, std::map<String, int>>& primitives,
                                                     const std::map<String, Array<String>>& chords,
                                                     const String& defaultChord)
{
    auto bar = std::make_shared<Bar>();
    
    for (const auto& eventJson : eventsJson)
    {
        // Handle compact array format: [beat, note] or [beat, {chord: "name"}]
        if (!eventJson.isArray() || eventJson.size() < 2)
            continue;
            
        Array<var>* eventArray = eventJson.getArray();
        double beat = (*eventArray)[0];
        var noteData = (*eventArray)[1];
        
        // Default values
        float velocity = 0.8f;
        float duration = 0.25f;
        String source = "pattern"; // Default source
        
        // Optional third element for velocity
        if (eventArray->size() >= 3)
            velocity = (*eventArray)[2];
        
        // Optional fourth element for duration  
        if (eventArray->size() >= 4)
        {
            var durationVar = (*eventArray)[3];
            if (durationVar.isString())
            {
                // Handle string durations like ".5", "1.5"
                String durationStr = durationVar.toString();
                duration = durationStr.getFloatValue();
            }
            else
            {
                // Handle numeric durations
                duration = static_cast<float>(static_cast<double>(durationVar));
            }
        }
            
        // Optional fifth element for source metadata
        if (eventArray->size() >= 5)
            source = (*eventArray)[4].toString();
        
        // Validate timing before adding notes
        // Check if note starts outside the bar boundary (assuming 4/4 time signature: beats 1.0 to just before 5.0)
        if (beat < 1.0 || beat >= 5.0)
        {
            DBG("WARNING: Note at beat " + String(beat, 2) + " is outside bar boundary (1.0-4.99) - skipping note");
            continue; // Skip this note entirely
        }
        
        double noteEndBeat = beat + duration;
        if (noteEndBeat > 5.0) // Check if note extends beyond bar boundary
        {
            double originalDuration = duration;
            duration = jmax(0.1f, 5.0f - static_cast<float>(beat)); // Clamp to bar boundary, minimum 0.1 beats
            DBG("WARNING: Note duration at beat " + String(beat, 2) + " clamped from " + 
                String(originalDuration, 2) + " to " + String(duration, 2) + " beats to stay within bar boundary");
        }
        
        // Handle note data
        if (noteData.isString())
        {
            // Simple note: [1.0, "C4"]
            String noteName = noteData.toString();
            int midiNote = findMidiNoteForPrimitive(noteName, primitives);
            int channel = getChannelForPrimitive(noteName, primitives);
            bar->addNote(static_cast<float>(beat), channel, midiNote, velocity, duration, source);
            DBG("Added note: " + noteName + " at beat " + String(beat) + " (MIDI " + String(midiNote) + ") from " + source);
        }
        else if (noteData.isObject() && noteData.hasProperty("chord"))
        {
            // Chord reference: [1.0, {chord: "C"}]
            String chordName = noteData["chord"].toString();
            auto chordIt = chords.find(chordName);
            
            if (chordIt != chords.end())
            {
                for (const String& noteName : chordIt->second)
                {
                    int midiNote = findMidiNoteForPrimitive(noteName, primitives);
                    int channel = getChannelForPrimitive(noteName, primitives);
                    bar->addNote(static_cast<float>(beat), channel, midiNote, velocity, duration, source);
                }
                DBG("Added chord: " + chordName + " at beat " + String(beat) + " (duration already validated)");
            }
        }
        else if (noteData.isObject() && noteData.hasProperty("tone"))
        {
            // Chord tone reference: [1.0, {tone: 1}]
            int toneNumber = static_cast<int>(static_cast<double>(noteData["tone"]));
            
            // Determine chord context at this beat
            String currentChord = inferCurrentChord(beat, eventsJson, chords);
            if (currentChord.isEmpty())
                currentChord = defaultChord;
            
            if (!currentChord.isEmpty())
            {
                // Optional octave specification (default to 4)
                int octave = 4;
                if (noteData.hasProperty("octave"))
                {
                    octave = static_cast<int>(static_cast<double>(noteData["octave"]));
                }
                
                int midiNote = resolveChordTone(toneNumber, currentChord, chords, octave);
                if (midiNote >= 0)
                {
                    int channel = 0; // Default channel for chord tones
                    bar->addNote(static_cast<float>(beat), channel, midiNote, velocity, duration, source);
                    DBG("Added chord tone " + String(toneNumber) + " of " + currentChord + " at beat " + String(beat) + " (MIDI " + String(midiNote) + ")");
                }
                else
                {
                    DBG("WARNING: Could not resolve tone " + String(toneNumber) + " for chord " + currentChord);
                }
            }
            else
            {
                DBG("WARNING: No chord context for tone reference at beat " + String(beat));
            }
        }
    }
    
    return bar;
}

std::shared_ptr<covertracks::Chart> covertracks::PatternParser::parseLLMResponse(const String& response)
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

String covertracks::PatternParser::extractSongNotation(const String& text)
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

String covertracks::PatternParser::extractSectionNotation(const String& text)
{
    // For JSON format, sections are part of the main chart
    return extractSongNotation(text);
}

String covertracks::PatternParser::extractPhraseNotation(const String& text)
{
    // For JSON format, phrases are part of sections
    return extractSongNotation(text);
}

String covertracks::PatternParser::extractBarNotation(const String& text)
{
    // For JSON format, bars are part of phrases
    return extractSongNotation(text);
}

int covertracks::PatternParser::findMidiNoteForPrimitive(const String& primitiveName, const std::map<String, std::map<String, int>>& primitives)
{
    // First check all instrument primitives
    for (const auto& instrumentPair : primitives)
    {        
        auto primitiveIt = instrumentPair.second.find(primitiveName);
        if (primitiveIt != instrumentPair.second.end())
        {
            return primitiveIt->second;
        }
        
        // If not found directly, check if we have registered capabilities for this instrument
        int noteFromCapabilities = findNoteFromCapabilities(instrumentPair.first, primitiveName);
        if (noteFromCapabilities >= 0)
        {
            return noteFromCapabilities;
        }
    }
    
    // If not found in primitives, try to parse as standard musical note name (e.g., "C4", "G#5", "Bb3")
    int midiNote = parseMusicalNoteName(primitiveName);
    if (midiNote >= 0)
    {
        return midiNote;
    }
    
    // As a last resort, check all registered capabilities regardless of instrument
    for (const auto& capPair : registeredCapabilities)
    {
        int noteFromCapabilities = findNoteFromCapabilities(capPair.first, primitiveName);
        if (noteFromCapabilities >= 0)
        {
            return noteFromCapabilities;
        }
    }
    
    return 60; // Default middle C (not found in primitives or capabilities)
}

int covertracks::PatternParser::getChannelForPrimitive(const String& primitiveName, const std::map<String, std::map<String, int>>& primitives)
{
    // Find which instrument this primitive belongs to
    for (const auto& instrumentPair : primitives)
    {
        auto primitiveIt = instrumentPair.second.find(primitiveName);
        if (primitiveIt != instrumentPair.second.end())
        {
            if (instrumentPair.first == "drums")
                return 9; // Drum channel
            else
                return 0; // Default channel
        }
    }
    return 0; // Default channel
}

int covertracks::PatternParser::parseMusicalNoteName(const String& noteName)
{
    // Parse standard musical note names like "C4", "F#5", "Bb3"
    if (noteName.length() < 2)
        return -1; // Invalid note name
    
    String noteStr = noteName.trim().toUpperCase();
    
    // Extract note name (C, D, E, F, G, A, B)
    char noteLetter = noteStr[0];
    int noteIndex = -1;
    
    switch (noteLetter)
    {
        case 'C': noteIndex = 0; break;
        case 'D': noteIndex = 2; break;
        case 'E': noteIndex = 4; break;
        case 'F': noteIndex = 5; break;
        case 'G': noteIndex = 7; break;
        case 'A': noteIndex = 9; break;
        case 'B': noteIndex = 11; break;
        default: return -1; // Invalid note
    }
    
    // Check for sharp or flat
    int accidentalOffset = 0;
    int octaveStartIndex = 1;
    
    if (noteStr.length() > 1)
    {
        if (noteStr[1] == '#' || noteStr[1] == 'S') // Sharp
        {
            accidentalOffset = 1;
            octaveStartIndex = 2;
        }
        else if (noteStr[1] == 'B' && noteStr.length() > 2 && noteStr[2] != '0') // Flat (but not B0)
        {
            accidentalOffset = -1;
            octaveStartIndex = 2;
        }
    }
    
    // Extract octave number
    if (noteStr.length() <= octaveStartIndex)
        return -1; // No octave specified
    
    String octaveStr = noteStr.substring(octaveStartIndex);
    int octave = octaveStr.getIntValue();
    
    // Validate octave range (MIDI supports 0-10, but typically we use 0-8)
    if (octave < 0 || octave > 10)
        return -1;
    
    // Calculate MIDI note number
    // MIDI note 60 = C4 (Middle C), which means octave 4 starts at MIDI note 60
    // In scientific pitch notation: C0=12, C1=24, C2=36, C3=48, C4=60, etc.
    // Each octave spans 12 semitones
    int midiNote = ((octave + 1) * 12) + noteIndex + accidentalOffset;
    
    // Ensure MIDI note is in valid range (0-127)
    if (midiNote < 0 || midiNote > 127)
        return -1;
    
    return midiNote;
}

int covertracks::PatternParser::resolveChordTone(int toneNumber, const String& chordName, const std::map<String, Array<String>>& chords, int octave)
{
    auto chordIt = chords.find(chordName);
    if (chordIt == chords.end() || chordIt->second.isEmpty())
    {
        DBG("WARNING: Chord '" + chordName + "' not found or empty");
        return -1;
    }
    
    const Array<String>& chordNotes = chordIt->second;
    
    // Tone numbers are 1-based (1=root, 3=third, 5=fifth, etc.)
    // Convert to 0-based index, but handle extensions beyond the basic chord
    int toneIndex = (toneNumber - 1) % chordNotes.size();
    
    // For extensions beyond the chord (e.g., tone 7 in a triad), cycle through the chord notes
    if (toneIndex < 0 || toneIndex >= chordNotes.size())
    {
        toneIndex = 0; // Default to root
    }
    
    String noteName = chordNotes[toneIndex];
    
    // Parse the note to get its MIDI number
    int baseMidiNote = parseMusicalNoteName(noteName);
    if (baseMidiNote < 0)
    {
        DBG("WARNING: Could not parse note '" + noteName + "' from chord " + chordName);
        return 60; // Default to middle C
    }
    
    // Adjust for desired octave
    // parseMusicalNoteName returns the note in its specified octave,
    // so we need to transpose to the requested octave
    int noteOctave = (baseMidiNote / 12) - 1; // Convert MIDI to octave number
    int octaveDifference = octave - noteOctave;
    int targetMidiNote = baseMidiNote + (octaveDifference * 12);
    
    // Handle higher tone numbers by going up octaves
    if (toneNumber > chordNotes.size())
    {
        int extraOctaves = (toneNumber - 1) / chordNotes.size();
        targetMidiNote += extraOctaves * 12;
    }
    
    // Ensure result is in valid MIDI range
    targetMidiNote = jlimit(0, 127, targetMidiNote);
    
    DBG("Resolved tone " + String(toneNumber) + " of chord " + chordName + " (" + noteName + ") to MIDI " + String(targetMidiNote));
    return targetMidiNote;
}

String covertracks::PatternParser::inferCurrentChord(double beat, const Array<var>& eventsJson, const std::map<String, Array<String>>& chords)
{
    String currentChord = "";
    double latestChordBeat = -1.0;
    
    // Look for the most recent chord event at or before this beat
    for (const auto& eventJson : eventsJson)
    {
        if (!eventJson.isArray() || eventJson.size() < 2)
            continue;
            
        Array<var>* eventArray = eventJson.getArray();
        double eventBeat = (*eventArray)[0];
        var noteData = (*eventArray)[1];
        
        // Check if this is a chord reference and it's at or before our target beat
        if (eventBeat <= beat && eventBeat > latestChordBeat && 
            noteData.isObject() && noteData.hasProperty("chord"))
        {
            String chordName = noteData["chord"].toString();
            // Verify this chord exists in our chord definitions
            if (chords.find(chordName) != chords.end())
            {
                currentChord = chordName;
                latestChordBeat = eventBeat;
            }
        }
    }
    
    if (!currentChord.isEmpty())
    {
        DBG("Inferred chord context at beat " + String(beat) + ": " + currentChord + " (from beat " + String(latestChordBeat) + ")");
    }
    
    return currentChord;
}

} // namespace covertracks