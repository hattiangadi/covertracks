#include "UniversalMusicParser.h"
#include <memory>

namespace covertracks {

// Static member definitions
juce::String UniversalMusicParser::lastError;
std::map<juce::String, UniversalMusicParser::AliasDefinition> UniversalMusicParser::currentAliases;

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
    
    // Parse aliases first (they guide the rest of the parsing)
    clearAliases();
    if (parsedJson.hasProperty("aliases")) {
        parseAliases(parsedJson["aliases"]);
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
        DBG("Parsing root patterns");
        parsePatternsInto(*container, parsedJson["patterns"]);
        DBG("Root patterns count after parsing: " << container->patterns.size());
    }
    
    // Parse sections (and propagate root patterns to them)
    if (parsedJson.hasProperty("sections")) {
        parseSectionsInto(*container, parsedJson["sections"]);
        // Propagate root patterns to all child containers
        propagatePatterns(*container);
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
    
    // Parse aliased container types using universal method
    // First check defined aliases, then fall back to known types
    for (const auto& aliasEntry : currentAliases) {
        const juce::String& aliasName = aliasEntry.first;
        if (parsedJson.hasProperty(aliasName)) {
            DBG("Found root-level aliased container: " << aliasName);
            parseContainerInto(*container, parsedJson.getProperty(aliasName, {}), aliasName);
        }
    }
    
    // Also check hardcoded aliased types for backward compatibility
    const juce::StringArray aliasedTypes = {
        "movements", "stanzas", "phrases", "bars", "beats",
        "instruments", "parts", "voices", "grooves"
    };
    
    for (int i = 0; i < aliasedTypes.size(); ++i) {
        const juce::String& alias = aliasedTypes[i];
        if (parsedJson.hasProperty(alias) && !hasAlias(alias)) {
            // Only parse if not already handled by defined aliases
            parseContainerInto(*container, parsedJson.getProperty(alias, {}), alias);
        }
    }
    
    // Parse arrangement field
    if (parsedJson.hasProperty("arrangement")) {
        auto arrangementJson = parsedJson["arrangement"];
        if (arrangementJson.isArray()) {
            auto arrangementArray = arrangementJson.getArray();
            if (arrangementArray) {
                for (const auto& sectionName : *arrangementArray) {
                    container->arrangement.push_back(sectionName.toString());
                    DBG("Arrangement: added section " << sectionName.toString());
                }
                DBG("Arrangement: total sections " << container->arrangement.size());
            }
        }
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
            event.time = (double)eventJson["t"] - 1.0;  // Convert musical beat (1-based) to MIDI timestamp (0-based)
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
    
    // Parse aliased container types - check defined aliases first
    for (const auto& aliasEntry : currentAliases) {
        const juce::String& aliasName = aliasEntry.first;
        if (containerJson.hasProperty(aliasName)) {
            parseContainerInto(container, containerJson.getProperty(aliasName, {}), aliasName);
        }
    }
    
    // Parse hardcoded aliased container types for backward compatibility
    const juce::StringArray aliasedTypes = {
        "movements", "stanzas", "phrases", "bars", "beats",
        "instruments", "parts", "voices", "grooves", "motifs"
    };
    
    for (int i = 0; i < aliasedTypes.size(); ++i) {
        const juce::String& alias = aliasedTypes[i];
        if (containerJson.hasProperty(alias) && !hasAlias(alias)) {
            parseContainerInto(container, containerJson.getProperty(alias, {}), alias);
        }
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

void UniversalMusicParser::propagatePatterns(MusicalContainer& container)
{
    // Recursively propagate root patterns to all child containers
    for (auto& [sectionName, section] : container.sections) {
        propagatePatternsTo(section, container.patterns);
    }
    for (auto& [layerName, layer] : container.layers) {
        propagatePatternsTo(layer, container.patterns);
    }
    for (auto& [varName, variation] : container.variations) {
        propagatePatternsTo(variation, container.patterns);
    }
}

void UniversalMusicParser::propagatePatternsTo(MusicalContainer& child, const std::map<juce::String, MusicalContainer>& rootPatterns)
{
    // Copy root patterns to child (if child doesn't already have the pattern)
    for (const auto& [patternName, pattern] : rootPatterns) {
        if (child.patterns.find(patternName) == child.patterns.end()) {
            child.patterns[patternName] = pattern;
        }
    }
    
    // Recursively propagate to grandchildren
    for (auto& [sectionName, section] : child.sections) {
        propagatePatternsTo(section, rootPatterns);
    }
    for (auto& [layerName, layer] : child.layers) {
        propagatePatternsTo(layer, rootPatterns);
    }
    for (auto& [varName, variation] : child.variations) {
        propagatePatternsTo(variation, rootPatterns);
    }
}

void UniversalMusicParser::parseContainerInto(MusicalContainer& parent, 
                                              const juce::var& containersJson,
                                              const juce::String& semanticType)
{
    if (!containersJson.isObject()) return;
    
    auto containersObj = containersJson.getDynamicObject();
    if (!containersObj) return;
    
    // Determine temporal relationship - check aliases first, then defaults
    TemporalRelationship relationship = getRelationshipForAlias(semanticType);
    
    DBG("Parsing container type: " << semanticType << " with relationship: " << (int)relationship
        << " (" << containersObj->getProperties().size() << " children)");
    
    for (const auto& item : containersObj->getProperties()) {
        juce::String itemName = item.name.toString();
        
        // Check if this is a range notation like "1-4", "5-8", etc.
        if (itemName.contains("-") && semanticType == "bars") {
            // Parse range notation
            int dashIndex = itemName.indexOfChar('-');
            if (dashIndex > 0 && dashIndex < itemName.length() - 1) {
                juce::String startStr = itemName.substring(0, dashIndex);
                juce::String endStr = itemName.substring(dashIndex + 1);
                
                if (startStr.containsOnly("0123456789") && endStr.containsOnly("0123456789")) {
                    int startBar = startStr.getIntValue();
                    int endBar = endStr.getIntValue();
                    
                    DBG("Expanding range " << itemName << " to individual bars " << startBar << "-" << endBar);
                    
                    // Expand range to individual bars
                    for (int barNum = startBar; barNum <= endBar; barNum++) {
                        MusicalContainer child = parseContainer(item.value);
                        
                        // Add alias information to child context if we have it
                        if (hasAlias(semanticType)) {
                            auto aliasIt = currentAliases.find(semanticType);
                            if (aliasIt != currentAliases.end()) {
                                double expectedDuration = calculateContainerDuration(semanticType, child);
                                if (expectedDuration > 0) {
                                    child.context.complexity = expectedDuration;
                                }
                            }
                        }
                        
                        juce::String barName = juce::String(barNum);
                        parent.variations[barName] = child;
                        DBG("  Expanded to variations: " << barName);
                    }
                    continue; // Skip normal processing for this item
                }
            }
        }
        
        // Normal processing for non-range items
        MusicalContainer child = parseContainer(item.value);
        
        // Add alias information to child context if we have it
        if (hasAlias(semanticType)) {
            auto aliasIt = currentAliases.find(semanticType);
            if (aliasIt != currentAliases.end()) {
                // Store expected duration based on alias
                double expectedDuration = calculateContainerDuration(semanticType, child);
                if (expectedDuration > 0) {
                    child.context.complexity = expectedDuration; // Temporary storage
                }
            }
        }
        
        // Store in appropriate map based on temporal relationship
        switch (relationship) {
            case TemporalRelationship::Simultaneous:
                parent.layers[itemName] = child;
                DBG("  Added to layers: " << itemName);
                break;
                
            case TemporalRelationship::Sequential:
                parent.variations[itemName] = child;
                DBG("  Added to variations: " << itemName);
                break;
                
            case TemporalRelationship::Named:
                parent.patterns[itemName] = child;
                DBG("  Added to patterns: " << itemName);
                break;
                
            case TemporalRelationship::Structure:
                parent.sections[itemName] = child;
                DBG("  Added to sections: " << itemName);
                break;
        }
    }
}

UniversalMusicParser::TemporalRelationship 
UniversalMusicParser::getTemporalRelationship(const juce::String& semanticType)
{
    // Simultaneous containers (play together)
    if (semanticType == "layers" || semanticType == "instruments" || 
        semanticType == "parts" || semanticType == "voices") {
        return TemporalRelationship::Simultaneous;
    }
    
    // Sequential containers (play one after another)  
    if (semanticType == "variations" || semanticType == "bars" || 
        semanticType == "beats" || semanticType == "measures") {
        return TemporalRelationship::Sequential;
    }
    
    // Named templates (reusable patterns)
    if (semanticType == "patterns" || semanticType == "templates" || 
        semanticType == "phrases" || semanticType == "motifs") {
        return TemporalRelationship::Named;
    }
    
    // Structural containers (sections of a piece)
    if (semanticType == "sections" || semanticType == "movements" || 
        semanticType == "arrangement" || semanticType == "structure") {
        return TemporalRelationship::Structure;
    }
    
    // Default to sequential for unknown types
    return TemporalRelationship::Sequential;
}

void UniversalMusicParser::parseAliases(const juce::var& aliasesJson)
{
    if (!aliasesJson.isObject()) return;
    
    auto aliasesObj = aliasesJson.getDynamicObject();
    if (!aliasesObj) return;
    
    DBG("Parsing aliases: " << aliasesObj->getProperties().size() << " definitions");
    
    for (const auto& alias : aliasesObj->getProperties()) {
        juce::String aliasName = alias.name.toString();
        juce::var aliasData = alias.value;
        
        if (!aliasData.isObject()) continue;
        
        AliasDefinition definition;
        
        // Parse temporal relationship type
        if (aliasData.hasProperty("type")) {
            juce::String typeStr = aliasData["type"].toString();
            if (typeStr == "simultaneous") definition.type = TemporalRelationship::Simultaneous;
            else if (typeStr == "sequential") definition.type = TemporalRelationship::Sequential;
            else if (typeStr == "named") definition.type = TemporalRelationship::Named;
            else if (typeStr == "structure") definition.type = TemporalRelationship::Structure;
            else definition.type = TemporalRelationship::Sequential; // Default
        }
        
        // Parse what this container contains
        if (aliasData.hasProperty("contains")) {
            definition.contains = aliasData["contains"].toString();
        }
        
        // Parse count specification
        if (aliasData.hasProperty("count")) {
            definition.count = aliasData["count"];
        }
        
        // Parse pattern specification
        if (aliasData.hasProperty("pattern")) {
            definition.pattern = aliasData["pattern"].toString();
        }
        
        // Parse default duration
        if (aliasData.hasProperty("defaultDuration")) {
            definition.defaultDuration = aliasData["defaultDuration"];
        }
        
        // Store additional metadata
        definition.metadata = aliasData;
        
        currentAliases[aliasName] = definition;
        
        DBG("Registered alias: " << aliasName << " -> " << definition.contains 
            << " (" << (int)definition.type << ", count: " << definition.count.toString() << ")");
    }
}

void UniversalMusicParser::clearAliases()
{
    currentAliases.clear();
}

UniversalMusicParser::TemporalRelationship 
UniversalMusicParser::getRelationshipForAlias(const juce::String& alias)
{
    auto it = currentAliases.find(alias);
    if (it != currentAliases.end()) {
        return it->second.type;
    }
    
    // Fall back to hardcoded defaults
    return getTemporalRelationship(alias);
}

bool UniversalMusicParser::hasAlias(const juce::String& alias)
{
    return currentAliases.find(alias) != currentAliases.end();
}

double UniversalMusicParser::calculateContainerDuration(const juce::String& containerType, 
                                                       const MusicalContainer& container)
{
    // Check for explicit duration override
    if (container.context.has("duration")) {
        return (double)container.context.get("duration");
    }
    
    // Use alias definition if available
    auto it = currentAliases.find(containerType);
    if (it != currentAliases.end()) {
        const AliasDefinition& alias = it->second;
        
        if (alias.count.isInt()) {
            int childCount = alias.count;
            double childDuration = 4.0; // Default beat duration
            
            // Try to get child duration from alias
            if (!alias.contains.isEmpty()) {
                auto childIt = currentAliases.find(alias.contains);
                if (childIt != currentAliases.end() && childIt->second.count.isInt()) {
                    childDuration = (double)childIt->second.count;
                }
            }
            
            return childCount * childDuration;
        }
        
        return alias.defaultDuration;
    }
    
    // Fallback: analyze actual content or use default
    return 4.0;
}

int UniversalMusicParser::getExpectedChildCount(const juce::String& containerType)
{
    auto it = currentAliases.find(containerType);
    if (it != currentAliases.end()) {
        if (it->second.count.isInt()) {
            return it->second.count;
        }
    }
    
    return -1; // Variable/unknown count
}

} // namespace covertracks