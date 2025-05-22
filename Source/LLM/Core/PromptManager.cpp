// Source/LLM/Core/PromptManager.cpp
#include "PromptManager.h"

namespace covertracks {

PromptManager::PromptManager()
{
    initializeDefaultTemplates();
    loadInstrumentDefinitions();
}

String PromptManager::createMusicGenerationPrompt(const String& style, 
                                                const StringArray& instruments, 
                                                int tempo, 
                                                const String& description)
{
    // Get the base template
    String templateText = getPromptTemplate("music_generation");
    
    // Create variables map
    std::map<String, String> variables;
    variables["style"] = style;
    variables["instruments"] = instruments.joinIntoString(", ");
    variables["tempo"] = String(tempo);
    variables["description"] = description;
    
    // Add instrument definitions
    String instrumentDefs;
    for (const auto& instrument : instruments)
    {
        // Get the definitions for this instrument
        String lowerInstrument = instrument.toLowerCase();
        const char* lowerInstrumentStr = lowerInstrument.toUTF8();
        if (instrumentDefinitions.hasProperty(lowerInstrumentStr))
        {
            var def = instrumentDefinitions[lowerInstrumentStr];
            instrumentDefs += "- " + instrument + ":\n";
            
            // Add primitives
            if (def.hasProperty("primitives") && def["primitives"].isObject())
            {
                auto* primitives = def["primitives"].getDynamicObject();
                instrumentDefs += "  Primitives: ";
                
                StringArray primitiveNames;
                for (auto& key : primitives->getProperties())
                    primitiveNames.add(key.name.toString());
                
                instrumentDefs += primitiveNames.joinIntoString(", ") + "\n";
            }
            
            // Add patterns
            if (def.hasProperty("patterns") && def["patterns"].isObject())
            {
                auto* patterns = def["patterns"].getDynamicObject();
                instrumentDefs += "  Patterns: ";
                
                StringArray patternInfos;
                for (auto& key : patterns->getProperties())
                {
                    var pattern = patterns->getProperty(key.name);
                    if (pattern.isObject() && pattern.hasProperty("id") && pattern.hasProperty("description"))
                    {
                        patternInfos.add(pattern["id"].toString() + " (" + pattern["description"].toString() + ")");
                    }
                }
                
                instrumentDefs += patternInfos.joinIntoString(", ") + "\n";
            }
            
            instrumentDefs += "\n";
        }
    }
    
    variables["instrument_definitions"] = instrumentDefs;
    
    // Process the template
    return processTemplate(templateText, variables);
}

String PromptManager::createPatternModificationPrompt(const String& currentPattern, 
                                                    const String& modification)
{
    // Get the base template
    String templateText = getPromptTemplate("pattern_modification");
    
    // Create variables map
    std::map<String, String> variables;
    variables["current_pattern"] = currentPattern;
    variables["modification"] = modification;
    
    // Process the template
    return processTemplate(templateText, variables);
}

String PromptManager::createSectionGenerationPrompt(const String& section,
                                                  const String& style,
                                                  const StringArray& instruments,
                                                  const String& description)
{
    // Get the base template
    String templateText = getPromptTemplate("section_generation");
    
    // Create variables map
    std::map<String, String> variables;
    variables["section"] = section;
    variables["style"] = style;
    variables["instruments"] = instruments.joinIntoString(", ");
    variables["description"] = description;
    
    // Process the template
    return processTemplate(templateText, variables);
}

String PromptManager::getPromptTemplate(const String& templateName)
{
    auto it = promptTemplates.find(templateName);
    if (it != promptTemplates.end())
        return it->second;
    
    // Return a default template if not found
    return "Please generate a musical pattern in {{style}} style using these instruments: {{instruments}}.\n\n"
           "Additional instructions: {{description}}";
}

bool PromptManager::loadPromptTemplates(const File& file)
{
    if (!file.existsAsFile())
        return false;
    
    try
    {
        // Read the file content
        String content = file.loadFileAsString();
        
        // Parse as JSON
        var json;
        Result result = JSON::parse(content, json);
        
        if (result.wasOk() && json.isObject())
        {
            // Extract templates
            auto* templates = json.getDynamicObject();
            
            for (auto& prop : templates->getProperties())
            {
                if (prop.value.isString())
                {
                    promptTemplates[prop.name.toString()] = prop.value.toString();
                }
            }
            
            return true;
        }
    }
    catch (...)
    {
        // Handle any exceptions during loading
    }
    
    return false;
}

void PromptManager::initializeDefaultTemplates()
{
    // Template for general music generation
    promptTemplates["music_generation"] = 
        "You are a music pattern generator. Create a {{style}} piece using these instruments: {{instruments}}.\n\n"
        "Target tempo: {{tempo}} BPM\n\n"
        "{{instrument_definitions}}\n\n"
        "Additional instructions: {{description}}\n\n"
        "Please structure your response with a complete song using the hierarchical pattern notation format:\n\n"
        "[SONG:\n"
        "  <INTRO: {pattern} {pattern}>\n"
        "  <VERSE: {pattern} {pattern}>\n"
        "  <CHORUS: {pattern} {pattern}>\n"
        "  <BRIDGE: {pattern} {pattern}>\n"
        "  <OUTRO: {pattern} {pattern}>\n"
        "]\n\n"
        "Each pattern can be a reference to a predefined pattern like {D001} or a new pattern using primitives.\n"
        "For new patterns, use this syntax: {NEW: [1: kick] [2: snare] [3: kick] [4: snare]}\n"
        "You can specify time signatures like [4/4: ...] or [3/4: ...]\n"
        "You can use precise note positioning: [1: kick] [2.5: snare] [3: kick] [4.5: snare]\n\n"
        "ONLY respond with the song pattern, DO NOT add explanations or comments outside the pattern notation.";
    
    // Template for pattern modification
    promptTemplates["pattern_modification"] = 
        "You are a music pattern editor. I have a musical pattern that I'd like you to modify.\n\n"
        "Current pattern:\n"
        "```\n"
        "{{current_pattern}}\n"
        "```\n\n"
        "Please make the following modifications: {{modification}}\n\n"
        "Return the ENTIRE modified pattern in the same format, not just the changes.\n"
        "ONLY respond with the modified pattern, DO NOT add explanations or comments outside the pattern notation.";
    
    // Template for section generation
    promptTemplates["section_generation"] = 
        "You are a music pattern generator. Create a {{section}} section for a {{style}} piece using these instruments: {{instruments}}.\n\n"
        "Additional instructions: {{description}}\n\n"
        "Please structure your response using the hierarchical pattern notation format for just this ONE section:\n\n"
        "<{{section|uppercase}}: {pattern} {pattern}>\n\n"
        "Each pattern can be a reference to a predefined pattern or a new pattern using primitives.\n"
        "For new patterns, use this syntax: {NEW: [1: kick] [2: snare] [3: kick] [4: snare]}\n"
        "ONLY respond with the section pattern, DO NOT add explanations or comments outside the pattern notation.";
}

void PromptManager::loadInstrumentDefinitions()
{
    // Create default instrument definitions
    DynamicObject* instruments = new DynamicObject();
    
    // --- DRUMS ---
    DynamicObject* drums = new DynamicObject();
    
    // Drum primitives
    DynamicObject* drumPrimitives = new DynamicObject();
    drumPrimitives->setProperty("kick", "Bass drum (36)");
    drumPrimitives->setProperty("snare", "Snare drum (38)");
    drumPrimitives->setProperty("closed_hat", "Closed hi-hat (42)");
    drumPrimitives->setProperty("open_hat", "Open hi-hat (46)");
    drumPrimitives->setProperty("ride", "Ride cymbal (51)");
    drumPrimitives->setProperty("crash", "Crash cymbal (49)");
    drumPrimitives->setProperty("tom_h", "High tom (50)");
    drumPrimitives->setProperty("tom_m", "Mid tom (47)");
    drumPrimitives->setProperty("tom_l", "Low tom (45)");
    drums->setProperty("primitives", var(drumPrimitives));
    
    // Drum patterns
    DynamicObject* drumPatterns = new DynamicObject();
    
    DynamicObject* basicRock = new DynamicObject();
    basicRock->setProperty("id", "D001");
    basicRock->setProperty("description", "Basic rock beat");
    drumPatterns->setProperty("basic_rock", var(basicRock));
    
    DynamicObject* simpleSwing = new DynamicObject();
    simpleSwing->setProperty("id", "D002");
    simpleSwing->setProperty("description", "Simple swing beat");
    drumPatterns->setProperty("simple_swing", var(simpleSwing));
    
    DynamicObject* funkGroove = new DynamicObject();
    funkGroove->setProperty("id", "D003");
    funkGroove->setProperty("description", "Funk groove");
    drumPatterns->setProperty("funk_groove", var(funkGroove));
    
    drums->setProperty("patterns", var(drumPatterns));
    
    // --- BASS ---
    DynamicObject* bass = new DynamicObject();
    
    // Bass primitives
    DynamicObject* bassPrimitives = new DynamicObject();
    bassPrimitives->setProperty("C2", "C note (36)");
    bassPrimitives->setProperty("D2", "D note (38)");
    bassPrimitives->setProperty("E2", "E note (40)");
    bassPrimitives->setProperty("F2", "F note (41)");
    bassPrimitives->setProperty("G2", "G note (43)");
    bassPrimitives->setProperty("A2", "A note (45)");
    bassPrimitives->setProperty("B2", "B note (47)");
    bassPrimitives->setProperty("C3", "C note (48)");
    bass->setProperty("primitives", var(bassPrimitives));
    
    // Bass patterns
    DynamicObject* bassPatterns = new DynamicObject();
    
    DynamicObject* walkingBass = new DynamicObject();
    walkingBass->setProperty("id", "B001");
    walkingBass->setProperty("description", "Walking bass");
    bassPatterns->setProperty("walking_bass", var(walkingBass));
    
    DynamicObject* octavePattern = new DynamicObject();
    octavePattern->setProperty("id", "B002");
    octavePattern->setProperty("description", "Octave pattern");
    bassPatterns->setProperty("octave_pattern", var(octavePattern));
    
    DynamicObject* funkBass = new DynamicObject();
    funkBass->setProperty("id", "B003");
    funkBass->setProperty("description", "Funk syncopation");
    bassPatterns->setProperty("funk_syncopation", var(funkBass));
    
    bass->setProperty("patterns", var(bassPatterns));
    
    // --- KEYS ---
    DynamicObject* keys = new DynamicObject();
    
    // Keys primitives
    DynamicObject* keysPrimitives = new DynamicObject();
    keysPrimitives->setProperty("C3", "C note (48)");
    keysPrimitives->setProperty("D3", "D note (50)");
    keysPrimitives->setProperty("E3", "E note (52)");
    keysPrimitives->setProperty("F3", "F note (53)");
    keysPrimitives->setProperty("G3", "G note (55)");
    keysPrimitives->setProperty("A3", "A note (57)");
    keysPrimitives->setProperty("B3", "B note (59)");
    keysPrimitives->setProperty("C4", "C note (60)");
    keys->setProperty("primitives", var(keysPrimitives));
    
    // Keys patterns
    DynamicObject* keysPatterns = new DynamicObject();
    
    DynamicObject* simpleComp = new DynamicObject();
    simpleComp->setProperty("id", "K001");
    simpleComp->setProperty("description", "Simple comp");
    keysPatterns->setProperty("simple_comp", var(simpleComp));
    
    DynamicObject* balladArp = new DynamicObject();
    balladArp->setProperty("id", "K002");
    balladArp->setProperty("description", "Ballad arpeggios");
    keysPatterns->setProperty("ballad_arpeggios", var(balladArp));
    
    DynamicObject* funkStabs = new DynamicObject();
    funkStabs->setProperty("id", "K003");
    funkStabs->setProperty("description", "Funk stabs");
    keysPatterns->setProperty("funk_stabs", var(funkStabs));
    
    keys->setProperty("patterns", var(keysPatterns));
    
    // --- GUITAR ---
    DynamicObject* guitar = new DynamicObject();
    
    // Guitar primitives
    DynamicObject* guitarPrimitives = new DynamicObject();
    guitarPrimitives->setProperty("E2", "E note (40)");
    guitarPrimitives->setProperty("A2", "A note (45)");
    guitarPrimitives->setProperty("D3", "D note (50)");
    guitarPrimitives->setProperty("G3", "G note (55)");
    guitarPrimitives->setProperty("B3", "B note (59)");
    guitarPrimitives->setProperty("E4", "E note (64)");
    guitar->setProperty("primitives", var(guitarPrimitives));
    
    // Guitar patterns
    DynamicObject* guitarPatterns = new DynamicObject();
    
    DynamicObject* strumPattern = new DynamicObject();
    strumPattern->setProperty("id", "G001");
    strumPattern->setProperty("description", "Basic strum pattern");
    guitarPatterns->setProperty("strum_pattern", var(strumPattern));
    
    DynamicObject* arpeggioPattern = new DynamicObject();
    arpeggioPattern->setProperty("id", "G002");
    arpeggioPattern->setProperty("description", "Arpeggio pattern");
    guitarPatterns->setProperty("arpeggio_pattern", var(arpeggioPattern));
    
    DynamicObject* powerChords = new DynamicObject();
    powerChords->setProperty("id", "G003");
    powerChords->setProperty("description", "Power chords");
    guitarPatterns->setProperty("power_chords", var(powerChords));
    
    guitar->setProperty("patterns", var(guitarPatterns));
    
    // Add all instruments to the main object
    instruments->setProperty("drums", var(drums));
    instruments->setProperty("bass", var(bass));
    instruments->setProperty("keys", var(keys));
    instruments->setProperty("guitar", var(guitar));
    
    // Store the instrument definitions
    instrumentDefinitions = var(instruments); 
}

// Helper method to replace template variables
String PromptManager::processTemplate(const String& templateText, 
                                     const std::map<String, String>& variables)
{
    String result = templateText;
    
    // Replace all variables
    for (const auto& var : variables)
    {
        result = result.replace("{{" + var.first + "}}", var.second);
        
        // Support for uppercase transformation
        result = result.replace("{{" + var.first + "|uppercase}}", var.second.toUpperCase());
    }
    
    return result;
}

} //namespace covertracks
