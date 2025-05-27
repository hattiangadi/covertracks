#include "MusicPatternSystem.h"
#include <iostream>
#include <sstream>
#include <regex>

namespace covertracks {

//==============================================================================
// Primitive implementation
//==============================================================================

var Primitive::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("midiNote", midiNote);
    obj->setProperty("description", description);
    return var(obj);
}

Primitive Primitive::fromVar(const var& v)
{
    Primitive result;
    
    if (auto* obj = v.getDynamicObject())
    {
        result.name = obj->getProperty("name").toString();
        result.midiNote = obj->getProperty("midiNote");
        result.description = obj->getProperty("description").toString();
    }
    
    return result;
}

//==============================================================================
// PatternEvent implementation
//==============================================================================

var PatternEvent::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty("primitive", primitive);
    obj->setProperty("position", position);
    obj->setProperty("duration", duration);
    obj->setProperty("velocity", velocity);
    return var(obj);
}

PatternEvent PatternEvent::fromVar(const var& v)
{
    PatternEvent result;
    
    if (auto* obj = v.getDynamicObject())
    {
        result.primitive = obj->getProperty("primitive").toString();
        result.position = obj->getProperty("position");
        result.duration = obj->getProperty("duration");
        result.velocity = obj->getProperty("velocity");
    }
    
    return result;
}

//==============================================================================
// Bar implementation
//==============================================================================

var Bar::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    // Store time signature
    obj->setProperty("timeSignatureNumerator", timeSignatureNumerator);
    obj->setProperty("timeSignatureDenominator", timeSignatureDenominator);
    
    // Serialize events
    Array<var> eventsArray;
    for (const auto& event : events)
    {
        DynamicObject* eventObj = new DynamicObject();
        eventObj->setProperty("type", (int)event.type);
        eventObj->setProperty("beat", event.beat);
        eventObj->setProperty("channel", event.channel);
        eventObj->setProperty("noteNumber", event.noteNumber);
        eventObj->setProperty("velocity", event.velocity);
        eventObj->setProperty("duration", event.duration);
        eventObj->setProperty("controllerNumber", event.controllerNumber);
        eventObj->setProperty("value", event.value);
        eventsArray.add(var(eventObj));
    }
    obj->setProperty("events", eventsArray);
    
    return var(obj);
}

// Forward declaration of the cleanBarCache function from MusicPatternComponents.cpp
extern void cleanBarCache();

//==============================================================================
// Phrase implementation
//==============================================================================

var Phrase::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty("name", name);
    
    // Serialize bars
    Array<var> barsArray;
    for (const auto& bar : bars)
    {
        if (bar)
            barsArray.add(bar->toVar());
    }
    obj->setProperty("bars", barsArray);
    
    return var(obj);
}

std::shared_ptr<Phrase> Phrase::fromVar(const var& v)
{
    auto result = std::make_shared<Phrase>();
    
    if (auto* obj = v.getDynamicObject())
    {
        result->setName(obj->getProperty("name").toString());
        
        var barsVar = obj->getProperty("bars");
        if (barsVar.isArray())
        {
            for (const auto& barVar : *barsVar.getArray())
            {
                auto bar = Bar::fromVar(barVar);
                if (bar)
                    result->bars.add(bar);
            }
        }
    }
    
    return result;
}

//==============================================================================
// Section implementation
//==============================================================================

var Section::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty("name", name);
    
    Array<var> phrasesArray;
    for (const auto& phrase : phrases)
    {
        if (phrase)
            phrasesArray.add(phrase->toVar());
    }
    obj->setProperty("phrases", phrasesArray);
    
    return var(obj);
}

std::shared_ptr<Section> Section::fromVar(const var& v)
{
    auto result = std::make_shared<Section>();
    
    if (auto* obj = v.getDynamicObject())
    {
        result->setName(obj->getProperty("name").toString());
        
        var phrasesVar = obj->getProperty("phrases");
        if (phrasesVar.isArray())
        {
            for (const auto& phraseVar : *phrasesVar.getArray())
            {
                auto phrase = Phrase::fromVar(phraseVar);
                if (phrase)
                    result->phrases.add(phrase);
            }
        }
    }
    
    return result;
}

//==============================================================================
//                    if (event)
//                    {
//                        // Clone the event and adjust its timestamp
//                        auto newEvent = event->message;
//                        newEvent.setTimeStamp(newEvent.getTimeStamp() + currentTick);
//                        sequence->addEvent(newEvent);
//                    }
//                }
//                
//                // Update the current position
//                // For simplicity, assume each bar is 4 quarter notes (1 bar in 4/4 time)
//                // In a more complete implementation, this would depend on the time signature
//                double beatsInBar = 4.0 * (bar->getTimeSignatureNumerator() / static_cast<double>(bar->getTimeSignatureDenominator())) * 4.0;
//                currentTick += beatsInBar * ticksPerQuarterNote;
//            }
//        }
//    }
//    
//    // Sort the sequence by timestamp
//    sequence->sort();
//    
//    return sequence;
//}

//==============================================================================
// InstrumentDefinition implementation
//==============================================================================

InstrumentDefinition::InstrumentDefinition(const String& instrumentName)
    : name(instrumentName)
{
    DBG("[InstrumentDefinition] Created: " + name + ", Bar instances: " + String(Bar::getInstanceCount()));
}

void InstrumentDefinition::addPrimitive(const String& primitiveName, const Primitive& primitive)
{
    primitives[primitiveName] = primitive;
    DBG("[InstrumentDefinition] Added primitive: " + primitiveName);
}

void InstrumentDefinition::removePrimitive(const String& primitiveName)
{
    auto it = primitives.find(primitiveName);
    if (it != primitives.end())
    {
        primitives.erase(it);
        DBG("[InstrumentDefinition] Removed primitive: " + primitiveName);
    }
}

void InstrumentDefinition::addPattern(const String& patternName, std::shared_ptr<Bar> pattern)
{
    patterns[patternName] = pattern;
    DBG("[InstrumentDefinition] Added pattern: " + patternName + ", Bar instances: " + String(Bar::getInstanceCount()));
}

void InstrumentDefinition::removePattern(const String& patternName)
{
    auto it = patterns.find(patternName);
    if (it != patterns.end())
    {
        patterns.erase(it);
        DBG("[InstrumentDefinition] Removed pattern: " + patternName + ", Bar instances: " + String(Bar::getInstanceCount()));
    }
}

Primitive InstrumentDefinition::getPrimitive(const String& primitiveName) const
{
    auto it = primitives.find(primitiveName);
    if (it != primitives.end())
        return it->second;
    
    // Return an empty primitive if not found
    return Primitive();
}

std::shared_ptr<Bar> InstrumentDefinition::getPattern(const String& patternName) const
{
    auto it = patterns.find(patternName);
    if (it != patterns.end())
        return it->second;
    
    // Return nullptr if not found
    return nullptr;
}

var InstrumentDefinition::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty("name", name);
    
    // Serialize primitives
    Array<var> primitivesArray;
    for (const auto& primitive : primitives)
    {
        primitivesArray.add(primitive.second.toVar());
    }
    obj->setProperty("primitives", primitivesArray);
    
    // Serialize patterns
    DynamicObject* patternsObj = new DynamicObject();
    for (const auto& pattern : patterns)
    {
        if (pattern.second)
            patternsObj->setProperty(pattern.first, pattern.second->toVar());
    }
    obj->setProperty("patterns", var(patternsObj));
    
    return var(obj);
}

InstrumentDefinition InstrumentDefinition::fromVar(const var& v)
{
    InstrumentDefinition result;
    
    if (auto* obj = v.getDynamicObject())
    {
        result.name = obj->getProperty("name").toString();
        
        // Get primitives
        var primitivesVar = obj->getProperty("primitives");
        if (primitivesVar.isArray())
        {
            for (const auto& primitiveVar : *primitivesVar.getArray())
            {
                Primitive prim = Primitive::fromVar(primitiveVar);
                result.primitives[prim.name] = prim;
            }
        }
        
        // Get patterns
        var patternsVar = obj->getProperty("patterns");
        if (auto* patternsObj = patternsVar.getDynamicObject())
        {
            for (const auto& prop : patternsObj->getProperties())
            {
                auto pattern = Bar::fromVar(prop.value);
                if (pattern)
                    result.patterns[prop.name.toString()] = pattern;
            }
        }
    }
    
    return result;
}

//==============================================================================
// MusicPatternSystem implementation
//==============================================================================

MusicPatternSystem::MusicPatternSystem()
{
    DBG("[MusicPatternSystem] Created, Bar instances: " + String(Bar::getInstanceCount()));
    
    // Register any static Bar instances that might be created during initialization
    Bar::printInstanceCount();
    
    initializeDefaultInstruments();
    
    // Check for any new Bar instances after initialization
    Bar::printInstanceCount();
}

MusicPatternSystem::~MusicPatternSystem()
{
    // Log instances before cleanup
    DBG("[MusicPatternSystem] Destroying, instances before cleanup:");
    DBG("  - Bar: " + String(Bar::getInstanceCount()));
    DBG("  - Phrase: " + String(Phrase::getInstanceCount()));
    DBG("  - Section: " + String(Section::getInstanceCount()));
    DBG("  - Chart: " + String(Chart::getInstanceCount()));
    
    // Clear all pattern maps to ensure proper cleanup
    patterns.clear();
    phrases.clear();
    sections.clear();
    charts.clear();
    instrumentDefinitions.clear();
    
    // Log instances after cleanup
    DBG("[MusicPatternSystem] Destroyed, instances after cleanup:");
    DBG("  - Bar: " + String(Bar::getInstanceCount()));
    DBG("  - Phrase: " + String(Phrase::getInstanceCount()));
    DBG("  - Section: " + String(Section::getInstanceCount()));
    DBG("  - Chart: " + String(Chart::getInstanceCount()));
}

void MusicPatternSystem::initializeDefaultInstruments()
{
    DBG("[MusicPatternSystem] Initializing default instruments (without pre-creating patterns)");
    
    // Create a basic drum kit with just primitive definitions
    InstrumentDefinition drums("drums");
    
    // Add drum primitives
    Primitive kick;
    kick.name = "kick";
    kick.midiNote = 36;
    kick.description = "Bass drum";
    drums.addPrimitive("kick", kick);
    
    Primitive snare;
    snare.name = "snare";
    snare.midiNote = 38;
    snare.description = "Snare drum";
    drums.addPrimitive("snare", snare);
    
    Primitive hihat;
    hihat.name = "hihat";
    hihat.midiNote = 42;
    hihat.description = "Hi-hat";
    drums.addPrimitive("hihat", hihat);
    
    // Add the drum kit
    addInstrumentDefinition("drums", drums);
    
    // Create a basic piano
    InstrumentDefinition piano("piano");
    
    // Add piano primitives
    Primitive c3;
    c3.name = "C3";
    c3.midiNote = 48;
    c3.description = "C3 note";
    piano.addPrimitive("C3", c3);
    
    Primitive e3;
    e3.name = "E3";
    e3.midiNote = 52;
    e3.description = "E3 note";
    piano.addPrimitive("E3", e3);
    
    Primitive g3;
    g3.name = "G3";
    g3.midiNote = 55;
    g3.description = "G3 note";
    piano.addPrimitive("G3", g3);
    
    // Add the piano
    addInstrumentDefinition("piano", piano);
    
    DBG("[MusicPatternSystem] Default instruments initialized");
}

void MusicPatternSystem::addInstrumentDefinition(const String& name, const InstrumentDefinition& definition)
{
    instrumentDefinitions[name] = definition;
}

bool MusicPatternSystem::hasInstrumentDefinition(const String& name) const
{
    return instrumentDefinitions.find(name) != instrumentDefinitions.end();
}

std::shared_ptr<Chart> MusicPatternSystem::getChart(const String& id) const
{
    auto it = charts.find(id);
    if (it != charts.end())
    {
        return it->second;
    }
    
    // Return nullptr if chart not found
    return nullptr;
}

std::shared_ptr<Section> MusicPatternSystem::getSection(const String& id) const
{
    auto it = sections.find(id);
    if (it != sections.end())
    {
        return it->second;
    }
    
    // Return nullptr if section not found
    return nullptr;
}

std::shared_ptr<Phrase> MusicPatternSystem::getPhrase(const String& id) const
{
    auto it = phrases.find(id);
    if (it != phrases.end())
    {
        return it->second;
    }
    
    // Return nullptr if phrase not found
    return nullptr;
}

std::shared_ptr<Bar> MusicPatternSystem::getPattern(const String& id) const
{
    auto it = patterns.find(id);
    if (it != patterns.end())
    {
        return it->second;
    }
    
    // Return nullptr if pattern not found
    return nullptr;
}

const InstrumentDefinition& MusicPatternSystem::getInstrumentDefinition(const String& name) const
{
    static InstrumentDefinition emptyDef;
    auto it = instrumentDefinitions.find(name);
    if (it != instrumentDefinitions.end())
        return it->second;
    return emptyDef;
}

std::shared_ptr<Bar> MusicPatternSystem::resolvePatternReference(const String& reference, const String& instrumentName)
{
    // Check if the reference is a direct pattern definition
    if (reference.startsWith("[") && reference.endsWith("]"))
    {
        // This is a direct pattern definition, parse it
        return createBarFromNotation(reference);
    }
    
    // Check if the reference is a named pattern in the specified instrument
    if (hasInstrumentDefinition(instrumentName))
    {
        auto pattern = getInstrumentDefinition(instrumentName).getPattern(reference);
        if (pattern)
            return pattern;
    }
    
    // Check if the reference is a named pattern in any instrument
    for (const auto& instrument : instrumentDefinitions)
    {
        auto pattern = instrument.second.getPattern(reference);
        if (pattern)
            return pattern;
    }
    
    // If we couldn't resolve the reference, return a default empty bar
    return std::make_shared<Bar>();
}

std::shared_ptr<Bar> MusicPatternSystem::createBarFromNotation(const String& notation)
{
    // Parse the notation and create a bar
    auto bar = std::make_shared<Bar>();
    
    // Extract time signature if present
    String timeSignature = "4/4"; // Default time signature
    String eventsText = notation;
    
    // Remove the outer brackets if present
    if (eventsText.startsWith("[") && eventsText.endsWith("]"))
    {
        eventsText = eventsText.substring(1, eventsText.length() - 1).trim();
    }
    
    // Check for time signature at the beginning (e.g., "3/4: [events]")
    if (eventsText.contains(":"))
    {
        int colonPos = eventsText.indexOf(":");
        String possibleTimeSignature = eventsText.substring(0, colonPos).trim();
        
        // Simple validation for time signature format
        if (possibleTimeSignature.contains("/") && 
            possibleTimeSignature.upToFirstOccurrenceOf("/", false, true).containsOnly("0123456789") &&
            possibleTimeSignature.fromFirstOccurrenceOf("/", false, true).containsOnly("0123456789"))
        {
            timeSignature = possibleTimeSignature;
            eventsText = eventsText.substring(colonPos + 1).trim();
            
            // Parse the time signature and set it on the bar
            StringArray parts;
            parts.addTokens(timeSignature, "/", "");
            if (parts.size() == 2)
            {
                int numerator = parts[0].getIntValue();
                int denominator = parts[1].getIntValue();
                if (numerator > 0 && denominator > 0)
                {
                    bar->setTimeSignature(numerator, denominator);
                }
            }
        }
    }
    
    // Parse the events
    Array<PatternEvent> events = parseEvents(eventsText, timeSignature);
    
    // Add the events to the bar
    for (const auto& event : events)
    {
        // In a real implementation, you would map the primitive to a MIDI note
        // and add it to the bar with the appropriate position, duration, etc.
        // For simplicity, we'll just use a default mapping here
        int midiNote = 60; // Middle C
        float velocity = event.velocity > 0 ? event.velocity / 127.0f : 0.8f;
        float duration = event.duration > 0 ? static_cast<float>(event.duration) : 0.25f;
        
        bar->addNote(static_cast<int>(event.position), 0, midiNote, velocity, duration);
    }
    
    return bar;
}

std::shared_ptr<Phrase> MusicPatternSystem::createPhraseFromNotation(const String& notation)
{
    auto phrase = std::make_shared<Phrase>();
    
    // Remove outer brackets if present
    String content = notation;
    if (content.startsWith("[") && content.endsWith("]"))
    {
        content = content.substring(1, content.length() - 1).trim();
    }
    
    // Check for phrase name
    if (content.contains(":"))
    {
        int colonPos = content.indexOf(":");
        String phraseName = content.substring(0, colonPos).trim();
        
        // Set the phrase name if it's not a time signature
        if (!phraseName.contains("/"))
        {
            phrase->setName(phraseName);
            content = content.substring(colonPos + 1).trim();
        }
    }
    
    // Split the content into bar definitions
    StringArray barDefs;
    
    // Strategy 1: Look for bar definitions enclosed in brackets
    if (content.contains("[") && content.contains("]"))
    {
        int startPos = 0;
        int endPos = 0;
        int nestingLevel = 0;
        
        for (int i = 0; i < content.length(); ++i)
        {
            if (content[i] == '[')
            {
                if (nestingLevel == 0)
                    startPos = i;
                nestingLevel++;
            }
            else if (content[i] == ']')
            {
                nestingLevel--;
                if (nestingLevel == 0)
                {
                    endPos = i;
                    barDefs.add(content.substring(startPos, endPos + 1).trim());
                }
            }
        }
    }
    // Strategy 2: Split by commas or semicolons if no brackets are found
    else
    {
        barDefs.addTokens(content, ",;", "\"[]");
    }
    
    // Create bars from the definitions
    for (const auto& barDef : barDefs)
    {
        if (barDef.trim().isNotEmpty())
        {
            auto bar = createBarFromNotation(barDef.trim());
            if (bar)
                phrase->addBar(bar);
        }
    }
    
    return phrase;
}

std::shared_ptr<Section> MusicPatternSystem::createSectionFromNotation(const String& notation)
{
    auto section = std::make_shared<Section>();
    
    // Remove outer brackets if present
    String content = notation;
    if (content.startsWith("[") && content.endsWith("]"))
    {
        content = content.substring(1, content.length() - 1).trim();
    }
    
    // Check for section name
    if (content.contains(":"))
    {
        int colonPos = content.indexOf(":");
        String sectionName = content.substring(0, colonPos).trim();
        
        // Set the section name if it's not a time signature
        if (!sectionName.contains("/"))
        {
            section->setName(sectionName);
            content = content.substring(colonPos + 1).trim();
        }
    }
    
    // Split the content into phrase definitions
    StringArray phraseDefs;
    
    // Strategy 1: Look for phrase definitions enclosed in brackets
    if (content.contains("[") && content.contains("]"))
    {
        int startPos = 0;
        int endPos = 0;
        int nestingLevel = 0;
        
        for (int i = 0; i < content.length(); ++i)
        {
            if (content[i] == '[')
            {
                if (nestingLevel == 0)
                    startPos = i;
                nestingLevel++;
            }
            else if (content[i] == ']')
            {
                nestingLevel--;
                if (nestingLevel == 0)
                {
                    endPos = i;
                    phraseDefs.add(content.substring(startPos, endPos + 1).trim());
                }
            }
        }
    }
    // Strategy 2: Split by commas or semicolons if no brackets are found
    else
    {
        phraseDefs.addTokens(content, ",;", "\"[]");
    }
    
    // Create phrases from the definitions
    for (const auto& phraseDef : phraseDefs)
    {
        if (phraseDef.trim().isNotEmpty())
        {
            auto phrase = createPhraseFromNotation(phraseDef.trim());
            if (phrase)
                section->addPhrase(phrase);
        }
    }
    
    return section;
}

std::shared_ptr<Chart> MusicPatternSystem::createChartFromNotation(const String& notation)
{
    auto chart = std::make_shared<Chart>();
    
    // Remove outer brackets if present
    String content = notation;
    if (content.startsWith("[") && content.endsWith("]"))
    {
        content = content.substring(1, content.length() - 1).trim();
    }
    
    // Check for chart name and tempo
    if (content.contains(":"))
    {
        int colonPos = content.indexOf(":");
        String chartName = content.substring(0, colonPos).trim();
        chart->setTitle(chartName);
        
        // Extract the rest of the content
        content = content.substring(colonPos + 1).trim();
    }
    
    // Look for tempo
    if (content.contains("BPM:"))
    {
        int bpmPos = content.indexOf("BPM:");
        int endBpmPos = content.indexOf("\n");
        if (endBpmPos < bpmPos) endBpmPos = -1;
        if (endBpmPos == -1) endBpmPos = content.length();
        
        String bpmStr = content.substring(bpmPos + 4, endBpmPos).trim();
        int tempo = bpmStr.getIntValue();
        if (tempo > 0)
        {
            chart->setTempo(tempo);
        }
        
        // Remove the BPM line from content
        content = content.substring(0, bpmPos) + content.substring(endBpmPos);
    }
    
    // Split into sections
    StringArray sectionDefs;
    
    // Strategy 1: Look for sections enclosed in brackets
    if (content.contains("{") && content.contains("}"))
    {
        int startPos = 0;
        while (startPos < content.length())
        {
            int openBracket = content.indexOf("{");
            if (openBracket < startPos) openBracket = -1;
            if (openBracket == -1) break;
            
            int closeBracket = content.indexOf("}");
            if (closeBracket < openBracket) closeBracket = -1;
            if (closeBracket == -1) break;
            
            // Get the section name
            String sectionName = content.substring(startPos, openBracket).trim();
            if (sectionName.endsWith(":")) sectionName = sectionName.dropLastCharacters(1).trim();
            
            // Get the section content including brackets
            String sectionContent = content.substring(openBracket, closeBracket + 1);
            
            // Add the section definition
            sectionDefs.add(sectionName + ": " + sectionContent);
            
            // Move to next section
            startPos = closeBracket + 1;
        }
    }
    // Strategy 2: Split by commas or semicolons if no brackets are found
    else
    {
        sectionDefs.addTokens(content, ",;", "\"{}");
    }
    
    // Create sections from the definitions
    for (const auto& sectionDef : sectionDefs)
    {
        if (sectionDef.trim().isNotEmpty())
        {
            auto section = createSectionFromNotation(sectionDef.trim());
            if (section)
                chart->addSection(section);
        }
    }
    
    return chart;
}

Array<PatternEvent> MusicPatternSystem::parseEvents(const String& eventsText, const String& timeSignature)
{
    Array<PatternEvent> events;
    
    // Strategy 1: Parse events in the format [position: primitive]
    if (eventsText.contains("[") && eventsText.contains("]"))
    {
        // Split the events text into individual event definitions
        StringArray parts;
        
        int startPos = 0;
        int endPos = 0;
        int nestingLevel = 0;
        
        for (int i = 0; i < eventsText.length(); ++i)
        {
            if (eventsText[i] == '[')
            {
                if (nestingLevel == 0)
                    startPos = i;
                nestingLevel++;
            }
            else if (eventsText[i] == ']')
            {
                nestingLevel--;
                if (nestingLevel == 0)
                {
                    endPos = i;
                    parts.add(eventsText.substring(startPos, endPos + 1).trim());
                }
            }
        }
        
        // Parse each event
        for (String part : parts)
        {
            part = part.trim();
            if (part.isEmpty())
                continue;
                
            // Add back the bracket if it was removed
            if (!part.startsWith("["))
                part = "[" + part;
                
            // Add back the closing bracket if it was removed
            if (!part.endsWith("]"))
                part = part + "]";
                
            // Now we should have a properly formatted event like [1: kick]
            if (part.startsWith("[") && part.contains(":") && part.endsWith("]"))
            {
                int colonPos = part.indexOf(":");
                String posText = part.substring(1, colonPos).trim();
                String noteText = part.substring(colonPos + 1, part.length() - 1).trim();
                
                double position = getPositionInBeats(posText, timeSignature);
                
                // Split noteText if it contains multiple primitives (e.g., "kick+closed_hat")
                StringArray primitives;
                primitives.addTokens(noteText, "+", "\"");
                
                for (const auto& primitive : primitives)
                {
                    PatternEvent event;
                    event.primitive = primitive.trim();
                    event.position = position;
                    
                    // Check if velocity is specified (e.g., "kick:v80")
                    if (event.primitive.contains(":v"))
                    {
                        int vPos = event.primitive.indexOf(":v");
                        String velText = event.primitive.substring(vPos + 2);
                        event.velocity = velText.getIntValue();
                        event.primitive = event.primitive.substring(0, vPos).trim();
                    }
                    
                    events.add(event);
                }
            }
        }
    }
    // Strategy 2: Simple space-separated list of primitives (evenly distributed)
    else
    {
        StringArray primitives;
        primitives.addTokens(eventsText, " ", "\"");
        
        int beatsPerBar = getBeatsPerBar(timeSignature);
        double beatSpacing = beatsPerBar / static_cast<double>(primitives.size());
        
        for (int i = 0; i < primitives.size(); ++i)
        {
            String primitive = primitives[i].trim();
            if (!primitive.isEmpty())
            {
                PatternEvent event;
                event.primitive = primitive;
                event.position = 1.0 + (i * beatSpacing); // 1-based position
                
                // Check if velocity is specified (e.g., "kick:v80")
                if (event.primitive.contains(":v"))
                {
                    int vPos = event.primitive.indexOf(":v");
                    String velText = event.primitive.substring(vPos + 2);
                    event.velocity = velText.getIntValue();
                    event.primitive = event.primitive.substring(0, vPos).trim();
                }
                
                events.add(event);
            }
        }
    }
    
    return events;
}

int MusicPatternSystem::getBeatsPerBar(const String& timeSignature)
{
    // Parse the time signature (e.g., "4/4", "3/4", etc.)
    if (timeSignature.contains("/"))
    {
        StringArray parts;
        parts.addTokens(timeSignature, "/", "");
        if (parts.size() == 2)
        {
            return parts[0].getIntValue();
        }
    }
    
    // Default to 4 beats per bar (4/4 time)
    return 4;
}

double MusicPatternSystem::getPositionInBeats(const String& positionText, const String&)
{
    // Handle simple beat positions (e.g., "1", "2", "3", "4")
    if (positionText.containsOnly("0123456789"))
    {
        return positionText.getDoubleValue();
    }
    
    // Handle fractional beat positions (e.g., "1.5", "2.5", "3.75")
    if (positionText.containsOnly("0123456789."))
    {
        return positionText.getDoubleValue();
    }
    
    // Default to position 1 (first beat)
    return 1.0;
}

} // namespace covertracks
