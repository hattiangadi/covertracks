#include "UniversalMusicParser.h"
#include <memory>

namespace covertracks {

// Static member definition
juce::String UniversalMusicParser::lastError;

std::unique_ptr<MusicalContainer> UniversalMusicParser::parseJSON(const juce::String& jsonString)
{
    juce::var parsedJson;
    auto result = juce::JSON::parse(jsonString, parsedJson);
    
    if (!result.wasOk()) {
        DBG("JSON Parse Error: " << result.getErrorMessage());
        return nullptr;
    }
    
    auto container = std::make_unique<MusicalContainer>();
    
    // Check version
    if (parsedJson.hasProperty("version")) {
        auto version = parsedJson["version"].toString();
        if (version != "2.0") {
            DBG("Unsupported version: " << version);
            return nullptr;
        }
    }
    
    // Parse global context
    if (parsedJson.hasProperty("context")) {
        container->context = parseContext(parsedJson["context"]);
    }
    
    // Parse meta information
    if (parsedJson.hasProperty("meta")) {
        // TODO: Store meta information
    }
    
    // Parse patterns (must be parsed before sections that reference them)
    if (parsedJson.hasProperty("patterns")) {
        parsePatternsInto(*container, parsedJson["patterns"]);
    }
    
    // Parse sections
    if (parsedJson.hasProperty("sections")) {
        parseSectionsInto(*container, parsedJson["sections"]);
    }
    
    // Parse direct events
    if (parsedJson.hasProperty("events")) {
        auto events = parseEvents(parsedJson["events"]);
        for (const auto& event : events) {
            container->addEvent(event);
        }
    }
    
    // Parse layers at root level
    if (parsedJson.hasProperty("layers")) {
        parseLayersInto(*container, parsedJson["layers"]);
    }
    
    // Parse variations at root level
    if (parsedJson.hasProperty("variations")) {
        parseVariationsInto(*container, parsedJson["variations"]);
    }
    
    return container;
}

Context UniversalMusicParser::parseContext(const juce::var& contextJson)
{
    Context context;
    
    if (contextJson.hasProperty("octave"))
        context.octave = contextJson["octave"];
    if (contextJson.hasProperty("velocity")) {
        auto velocityVar = contextJson["velocity"];
        if (velocityVar.isString()) {
            // Convert dynamic strings to velocity values
            juce::String dynamics = velocityVar.toString();
            if (dynamics == "pp") context.velocity = 0.3f;
            else if (dynamics == "p") context.velocity = 0.4f;
            else if (dynamics == "mp") context.velocity = 0.5f;
            else if (dynamics == "mf") context.velocity = 0.6f;
            else if (dynamics == "f") context.velocity = 0.8f;
            else if (dynamics == "ff") context.velocity = 1.0f;
            else context.velocity = 0.6f; // Default to mf
        } else {
            context.velocity = velocityVar;
        }
    }
    if (contextJson.hasProperty("chord"))
        context.chord = contextJson["chord"].toString();
    if (contextJson.hasProperty("key"))
        context.key = contextJson["key"].toString();
    if (contextJson.hasProperty("dynamics"))
        context.dynamics = contextJson["dynamics"].toString();
    if (contextJson.hasProperty("articulation"))
        context.articulation = contextJson["articulation"].toString();
    if (contextJson.hasProperty("complexity"))
        context.complexity = contextJson["complexity"];
    if (contextJson.hasProperty("energy"))
        context.energy = contextJson["energy"];
    if (contextJson.hasProperty("style"))
        context.style = contextJson["style"].toString();
        
    return context;
}

std::vector<Event> UniversalMusicParser::parseEvents(const juce::var& eventsJson)
{
    std::vector<Event> events;
    
    if (!eventsJson.isArray()) return events;
    
    auto eventsArray = eventsJson.getArray();
    if (!eventsArray) return events;
    
    for (const auto& eventJson : *eventsArray) {
        Event event;
        
        // Parse basic event properties
        if (eventJson.hasProperty("t"))
            event.time = eventJson["t"];
        if (eventJson.hasProperty("n"))
            event.note = eventJson["n"].toString();
        if (eventJson.hasProperty("v"))
            event.velocity = eventJson["v"];
        if (eventJson.hasProperty("d"))
            event.degree = eventJson["d"];
        if (eventJson.hasProperty("dur"))
            event.duration = eventJson["dur"];
        if (eventJson.hasProperty("chord"))
            event.chord = eventJson["chord"].toString();
            
        // Parse expression attributes
        if (eventJson.hasProperty("dynamics"))
            event.dynamics = eventJson["dynamics"].toString();
        if (eventJson.hasProperty("articulation"))
            event.articulation = eventJson["articulation"].toString();
            
        // Parse expression map
        if (eventJson.hasProperty("expression")) {
            auto exprJson = eventJson["expression"];
            if (exprJson.isObject()) {
                auto props = exprJson.getDynamicObject();
                if (props) {
                    for (const auto& prop : props->getProperties()) {
                        event.expression[prop.name.toString()] = prop.value;
                    }
                }
            }
        }
        
        // Parse processors
        if (eventJson.hasProperty("processors")) {
            auto procJson = eventJson["processors"];
            if (procJson.isObject()) {
                auto props = procJson.getDynamicObject();
                if (props) {
                    for (const auto& prop : props->getProperties()) {
                        event.processors[prop.name.toString()] = prop.value;
                    }
                }
            }
        }
        
        events.push_back(event);
    }
    
    return events;
}

void UniversalMusicParser::parseLayersInto(MusicalContainer& container, const juce::var& layersJson)
{
    if (!layersJson.isObject()) return;
    
    auto layersObj = layersJson.getDynamicObject();
    if (!layersObj) return;
    
    for (const auto& layer : layersObj->getProperties()) {
        auto& layerContainer = container.addLayer(layer.name.toString());
        layerContainer = parseContainer(layer.value);
    }
}

void UniversalMusicParser::parseVariationsInto(MusicalContainer& container, const juce::var& variationsJson)
{
    if (!variationsJson.isObject()) return;
    
    auto variationsObj = variationsJson.getDynamicObject();
    if (!variationsObj) return;
    
    for (const auto& variation : variationsObj->getProperties()) {
        auto& variationContainer = container.addVariation(variation.name.toString());
        variationContainer = parseContainer(variation.value);
    }
}

void UniversalMusicParser::parsePatternsInto(MusicalContainer& container, const juce::var& patternsJson)
{
    if (!patternsJson.isObject()) return;
    
    auto patternsObj = patternsJson.getDynamicObject();
    if (!patternsObj) return;
    
    for (const auto& pattern : patternsObj->getProperties()) {
        auto& patternContainer = container.addPattern(pattern.name.toString());
        
        // Patterns can be either an array of events or a full container
        if (pattern.value.isArray()) {
            // Simple event array
            auto events = parseEvents(pattern.value);
            for (const auto& event : events) {
                patternContainer.addEvent(event);
            }
        } else {
            // Full container
            patternContainer = parseContainer(pattern.value);
        }
    }
}

void UniversalMusicParser::parseSectionsInto(MusicalContainer& container, const juce::var& sectionsJson)
{
    if (!sectionsJson.isObject()) return;
    
    auto sectionsObj = sectionsJson.getDynamicObject();
    if (!sectionsObj) return;
    
    for (const auto& section : sectionsObj->getProperties()) {
        // Sections are stored in both sections and patterns maps for flexibility
        container.sections[section.name.toString()] = parseContainer(section.value);
    }
}

MusicalContainer UniversalMusicParser::parseContainer(const juce::var& containerJson)
{
    MusicalContainer container;
    
    // Parse context
    if (containerJson.hasProperty("context")) {
        container.context = parseContext(containerJson["context"]);
    }
    
    // Parse pattern reference
    if (containerJson.hasProperty("pattern")) {
        container.setReference(containerJson["pattern"].toString());
    }
    
    // Parse events
    if (containerJson.hasProperty("events")) {
        auto events = parseEvents(containerJson["events"]);
        for (const auto& event : events) {
            container.addEvent(event);
        }
    }
    
    // Parse layers
    if (containerJson.hasProperty("layers")) {
        parseLayersInto(container, containerJson["layers"]);
    }
    
    // Parse variations
    if (containerJson.hasProperty("variations")) {
        parseVariationsInto(container, containerJson["variations"]);
    }
    
    // Parse nested patterns
    if (containerJson.hasProperty("patterns")) {
        parsePatternsInto(container, containerJson["patterns"]);
    }
    
    // Parse nested sections
    if (containerJson.hasProperty("sections")) {
        parseSectionsInto(container, containerJson["sections"]);
    }
    
    // Parse expression features
    if (containerJson.hasProperty("shortcuts")) {
        auto shortcutsJson = containerJson["shortcuts"];
        if (shortcutsJson.isObject()) {
            auto props = shortcutsJson.getDynamicObject();
            if (props) {
                for (const auto& prop : props->getProperties()) {
                    container.shortcuts[prop.name.toString()] = prop.value.toString();
                }
            }
        }
    }
    
    // Parse chord progression
    if (containerJson.hasProperty("chordProgression")) {
        auto chordProgJson = containerJson["chordProgression"];
        if (chordProgJson.isArray()) {
            auto chordsArray = chordProgJson.getArray();
            if (chordsArray) {
                for (const auto& chord : *chordsArray) {
                    container.chordProgression.push_back(chord);
                }
            }
        }
    }
    
    return container;
}

} // namespace covertracks