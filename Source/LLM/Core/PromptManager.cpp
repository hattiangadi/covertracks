// Source/LLM/Core/PromptManager.cpp
#include "PromptManager.h"
#include "../../Player/Core/Player.h"

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

String PromptManager::createMusicGenerationPrompt(const String& style,
                                                 const std::vector<Player*>& players,
                                                 int tempo,
                                                 const String& description)
{
    // Get the base template
    String templateText = getPromptTemplate("music_generation");
    
    // Create variables map
    std::map<String, String> variables;
    variables["style"] = style;
    
    // Extract instrument names from players
    StringArray instrumentNames;
    for (const auto* player : players)
    {
        if (player)
        {
            instrumentNames.add(player->getInstrumentCapabilities().instrumentType);
        }
    }
    variables["instruments"] = instrumentNames.joinIntoString(", ");
    variables["tempo"] = String(tempo);
    variables["description"] = description;
    
    // Create instrument definitions from player capabilities
    variables["instrument_definitions"] = createInstrumentDefinitionsFromPlayers(players);
    
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
    // Template for general music generation using JSON format
    promptTemplates["music_generation"] = 
        "You are a professional musician and music pattern generator. Create a {{style}} piece using these instruments: {{instruments}}.\n\n"
        "Target tempo: {{tempo}} BPM\n\n"
        "{{instrument_definitions}}\n\n"
        "Additional instructions: {{description}}\n\n"
        "IMPORTANT: Respond with a complete JSON chart using our hierarchical pattern language that supports:\n"
        "- T1: Basic primitives (individual notes/hits)\n"
        "- T2: Multi-bar patterns using barPatterns arrays\n"
        "- T3: Chord definitions and chord references\n"
        "- T4: Layer merging (multiple parts played simultaneously)\n"
        "- T5: Layers + chords combined\n"
        "- T6: Multi-section songs with arrangements\n"
        "- T7: Pattern references for reusability\n"
        "- T8: Pattern variations (base pattern + bar-specific overrides)\n\n"
        "JSON Format:\n"
        "{\n"
        "  \"title\": \"Song Title\",\n"
        "  \"tempo\": {{tempo}},\n"
        "  \"primitives\": {\n"
        "    \"instrument_name\": {\n"
        "      \"note_name\": midi_number\n"
        "    }\n"
        "  },\n"
        "  \"chords\": {\n"
        "    \"chord_name\": [\"note1\", \"note2\", \"note3\"]\n"
        "  },\n"
        "  \"patterns\": {\n"
        "    \"pattern_name\": [\n"
        "      {\"beat\": 1.0, \"primitive\": \"note_name\"},\n"
        "      {\"beat\": 2.0, \"chord\": \"chord_name\"}\n"
        "    ]\n"
        "  },\n"
        "  \"sections\": {\n"
        "    \"verse\": {\n"
        "      \"bars\": 4,\n"
        "      \"basePattern\": \"pattern_name\",\n"
        "      \"variations\": {\n"
        "        \"bar4\": \"fill_pattern\"\n"
        "      },\n"
        "      \"layers\": {\n"
        "        \"layer1\": {\"pattern\": \"pattern_name\"},\n"
        "        \"layer2\": [{\"beat\": 1.0, \"primitive\": \"note\"}]\n"
        "      }\n"
        "    },\n"
        "    \"chorus\": {\n"
        "      \"bars\": 4,\n"
        "      \"barPatterns\": [\n"
        "        [{\"beat\": 1.0, \"primitive\": \"note1\"}],\n"
        "        [{\"beat\": 1.0, \"primitive\": \"note2\"}]\n"
        "      ]\n"
        "    }\n"
        "  },\n"
        "  \"arrangement\": [\"verse\", \"chorus\", \"verse\", \"chorus\"]\n"
        "}\n\n"
        "ONLY respond with valid JSON. DO NOT add explanations or comments outside the JSON.";
    
    // Template for pattern modification using JSON format
    promptTemplates["pattern_modification"] = 
        "You are a professional music pattern editor. I have a JSON musical chart that I'd like you to modify.\n\n"
        "Current JSON chart:\n"
        "```json\n"
        "{{current_pattern}}\n"
        "```\n\n"
        "Please make the following modifications: {{modification}}\n\n"
        "Modification guidelines:\n"
        "- Maintain the JSON structure and all existing sections unless specifically asked to change them\n"
        "- Use our hierarchical pattern language features (T1-T8) as appropriate\n"
        "- Keep primitives, chords, and patterns consistent with the instrument type\n"
        "- Preserve tempo and arrangement unless specifically requested to change\n\n"
        "Return the ENTIRE modified JSON chart, not just the changes.\n"
        "ONLY respond with valid JSON. DO NOT add explanations or comments outside the JSON.";
    
    // Template for section generation using JSON format
    promptTemplates["section_generation"] = 
        "You are a professional musician and music pattern generator. Create a {{section}} section for a {{style}} piece using these instruments: {{instruments}}.\n\n"
        "Additional instructions: {{description}}\n\n"
        "Create a JSON chart with ONLY the {{section}} section. Include appropriate primitives, and use our hierarchical pattern language features:\n\n"
        "JSON Format:\n"
        "{\n"
        "  \"title\": \"{{section|title_case}}_Section\",\n"
        "  \"tempo\": 120,\n"
        "  \"primitives\": {\n"
        "    \"instrument_name\": {\n"
        "      \"note_name\": midi_number\n"
        "    }\n"
        "  },\n"
        "  \"patterns\": {\n"
        "    \"pattern_name\": [\n"
        "      {\"beat\": 1.0, \"primitive\": \"note_name\"}\n"
        "    ]\n"
        "  },\n"
        "  \"sections\": {\n"
        "    \"{{section}}\": {\n"
        "      \"bars\": 4,\n"
        "      \"layers\": {\n"
        "        \"main\": [{\"beat\": 1.0, \"primitive\": \"note\"}]\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}\n\n"
        "ONLY respond with valid JSON. DO NOT add explanations or comments outside the JSON.";
}

void PromptManager::loadInstrumentDefinitions()
{
    // Create default instrument definitions matching our integration test primitives
    DynamicObject* instruments = new DynamicObject();
    
    // --- DRUMS --- (from Integration_Drum_Chart)
    DynamicObject* drums = new DynamicObject();
    
    // Drum primitives - match integration test exactly
    DynamicObject* drumPrimitives = new DynamicObject();
    drumPrimitives->setProperty("kick", "Kick drum (36)");
    drumPrimitives->setProperty("snare", "Snare drum (38)");
    drumPrimitives->setProperty("hihat", "Hi-hat (42)");
    drumPrimitives->setProperty("openHihat", "Open hi-hat (46)");
    drumPrimitives->setProperty("crash", "Crash cymbal (49)");
    drumPrimitives->setProperty("ride", "Ride cymbal (51)");
    drumPrimitives->setProperty("tom1", "High tom (47)");
    drumPrimitives->setProperty("tom2", "Mid tom (45)");
    drumPrimitives->setProperty("tom3", "Low tom (43)");
    drumPrimitives->setProperty("rimshot", "Rimshot (37)");
    drumPrimitives->setProperty("cowbell", "Cowbell (56)");
    drumPrimitives->setProperty("splash", "Splash cymbal (55)");
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
    
    // --- BASS --- (from Integration_Bass_Chart)
    DynamicObject* bass = new DynamicObject();
    
    // Bass primitives - match integration test exactly
    DynamicObject* bassPrimitives = new DynamicObject();
    bassPrimitives->setProperty("E1", "E note (28)");
    bassPrimitives->setProperty("F1", "F note (29)");
    bassPrimitives->setProperty("Fs1", "F# note (30)");
    bassPrimitives->setProperty("G1", "G note (31)");
    bassPrimitives->setProperty("Gs1", "G# note (32)");
    bassPrimitives->setProperty("A1", "A note (33)");
    bassPrimitives->setProperty("As1", "A# note (34)");
    bassPrimitives->setProperty("B1", "B note (35)");
    bassPrimitives->setProperty("C2", "C note (36)");
    bassPrimitives->setProperty("Cs2", "C# note (37)");
    bassPrimitives->setProperty("D2", "D note (38)");
    bassPrimitives->setProperty("Ds2", "D# note (39)");
    bassPrimitives->setProperty("E2", "E note (40)");
    bassPrimitives->setProperty("F2", "F note (41)");
    bassPrimitives->setProperty("Fs2", "F# note (42)");
    bassPrimitives->setProperty("G2", "G note (43)");
    bassPrimitives->setProperty("Gs2", "G# note (44)");
    bassPrimitives->setProperty("A2", "A note (45)");
    bassPrimitives->setProperty("As2", "A# note (46)");
    bassPrimitives->setProperty("B2", "B note (47)");
    bassPrimitives->setProperty("C3", "C note (48)");
    bassPrimitives->setProperty("Cs3", "C# note (49)");
    bassPrimitives->setProperty("D3", "D note (50)");
    bassPrimitives->setProperty("Ds3", "D# note (51)");
    bassPrimitives->setProperty("E3", "E note (52)");
    bassPrimitives->setProperty("F3", "F note (53)");
    bassPrimitives->setProperty("Fs3", "F# note (54)");
    bassPrimitives->setProperty("G3", "G note (55)");
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
    
    // --- PIANO --- (from Integration_Piano_Chart)
    DynamicObject* piano = new DynamicObject();
    
    // Piano primitives - match integration test exactly
    DynamicObject* pianoPrimitives = new DynamicObject();
    pianoPrimitives->setProperty("C2", "C note (36)");
    pianoPrimitives->setProperty("D2", "D note (38)");
    pianoPrimitives->setProperty("E2", "E note (40)");
    pianoPrimitives->setProperty("F2", "F note (41)");
    pianoPrimitives->setProperty("G2", "G note (43)");
    pianoPrimitives->setProperty("A2", "A note (45)");
    pianoPrimitives->setProperty("B2", "B note (47)");
    pianoPrimitives->setProperty("C3", "C note (48)");
    pianoPrimitives->setProperty("D3", "D note (50)");
    pianoPrimitives->setProperty("E3", "E note (52)");
    pianoPrimitives->setProperty("F3", "F note (53)");
    pianoPrimitives->setProperty("G3", "G note (55)");
    pianoPrimitives->setProperty("A3", "A note (57)");
    pianoPrimitives->setProperty("B3", "B note (59)");
    pianoPrimitives->setProperty("C4", "C note (60)");
    pianoPrimitives->setProperty("D4", "D note (62)");
    pianoPrimitives->setProperty("E4", "E note (64)");
    pianoPrimitives->setProperty("F4", "F note (65)");
    pianoPrimitives->setProperty("G4", "G note (67)");
    pianoPrimitives->setProperty("A4", "A note (69)");
    pianoPrimitives->setProperty("B4", "B note (71)");
    pianoPrimitives->setProperty("C5", "C note (72)");
    pianoPrimitives->setProperty("D5", "D note (74)");
    pianoPrimitives->setProperty("E5", "E note (76)");
    pianoPrimitives->setProperty("F5", "F note (77)");
    pianoPrimitives->setProperty("G5", "G note (79)");
    piano->setProperty("primitives", var(pianoPrimitives));
    
    // Piano patterns 
    DynamicObject* pianoPatterns = new DynamicObject();
    
    DynamicObject* leftHandBass = new DynamicObject();
    leftHandBass->setProperty("id", "P001");
    leftHandBass->setProperty("description", "Left hand bass pattern");
    pianoPatterns->setProperty("leftHandBass", var(leftHandBass));
    
    DynamicObject* rightHandMelody = new DynamicObject();
    rightHandMelody->setProperty("id", "P002");
    rightHandMelody->setProperty("description", "Right hand melody");
    pianoPatterns->setProperty("rightHandMelody", var(rightHandMelody));
    
    DynamicObject* arpeggioPattern = new DynamicObject();
    arpeggioPattern->setProperty("id", "P003");
    arpeggioPattern->setProperty("description", "Arpeggio pattern");
    pianoPatterns->setProperty("arpeggioPattern", var(arpeggioPattern));
    
    piano->setProperty("patterns", var(pianoPatterns));
    
    // --- GUITAR --- (from Integration_Guitar_Chart)
    DynamicObject* guitar = new DynamicObject();
    
    // Guitar primitives - match integration test exactly
    DynamicObject* guitarPrimitives = new DynamicObject();
    guitarPrimitives->setProperty("E2", "E note (40)");
    guitarPrimitives->setProperty("A2", "A note (45)");
    guitarPrimitives->setProperty("D3", "D note (50)");
    guitarPrimitives->setProperty("G3", "G note (55)");
    guitarPrimitives->setProperty("B3", "B note (59)");
    guitarPrimitives->setProperty("E4", "E note (64)");
    guitarPrimitives->setProperty("F2", "F note (41)");
    guitarPrimitives->setProperty("Fs2", "F# note (42)");
    guitarPrimitives->setProperty("G2", "G note (43)");
    guitarPrimitives->setProperty("Gs2", "G# note (44)");
    guitarPrimitives->setProperty("As2", "A# note (46)");
    guitarPrimitives->setProperty("C3", "C note (48)");
    guitarPrimitives->setProperty("Cs3", "C# note (49)");
    guitarPrimitives->setProperty("Ds3", "D# note (51)");
    guitarPrimitives->setProperty("F3", "F note (53)");
    guitarPrimitives->setProperty("Fs3", "F# note (54)");
    guitarPrimitives->setProperty("Gs3", "G# note (56)");
    guitarPrimitives->setProperty("A3", "A note (57)");
    guitarPrimitives->setProperty("As3", "A# note (58)");
    guitarPrimitives->setProperty("C4", "C note (60)");
    guitarPrimitives->setProperty("Cs4", "C# note (61)");
    guitarPrimitives->setProperty("D4", "D note (62)");
    guitarPrimitives->setProperty("Ds4", "D# note (63)");
    guitarPrimitives->setProperty("F4", "F note (65)");
    guitar->setProperty("primitives", var(guitarPrimitives));
    
    // Guitar patterns
    DynamicObject* guitarPatterns = new DynamicObject();
    
    DynamicObject* strummingPattern = new DynamicObject();
    strummingPattern->setProperty("id", "G001");
    strummingPattern->setProperty("description", "Strumming pattern");
    guitarPatterns->setProperty("strummingPattern", var(strummingPattern));
    
    DynamicObject* bassLine = new DynamicObject();
    bassLine->setProperty("id", "G002");
    bassLine->setProperty("description", "Bass line on guitar");
    guitarPatterns->setProperty("bassLine", var(bassLine));
    
    DynamicObject* fingerpicking = new DynamicObject();
    fingerpicking->setProperty("id", "G003");
    fingerpicking->setProperty("description", "Fingerpicking pattern");
    guitarPatterns->setProperty("fingerpicking", var(fingerpicking));
    
    guitar->setProperty("patterns", var(guitarPatterns));
    
    // Add all instruments to the main object
    instruments->setProperty("drums", var(drums));
    instruments->setProperty("bass", var(bass));
    instruments->setProperty("piano", var(piano));
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

String PromptManager::createInstrumentDefinitionsFromPlayers(const std::vector<Player*>& players)
{
    String instrumentDefs;
    
    for (const auto* player : players)
    {
        if (!player) continue;
        
        const auto& capabilities = player->getInstrumentCapabilities();
        instrumentDefs += "- " + capabilities.instrumentType + ":\n";
        
        if (capabilities.type == InstrumentCapabilityType::Mapping)
        {
            // For mapped instruments (like drums), list the available sounds
            instrumentDefs += "  Available sounds: ";
            StringArray soundNames;
            for (const auto& mapping : capabilities.soundMappings)
            {
                soundNames.add(mapping.first + " (" + String(mapping.second) + ")");
            }
            instrumentDefs += soundNames.joinIntoString(", ") + "\n";
        }
        else if (capabilities.type == InstrumentCapabilityType::NoteRange)
        {
            // For note range instruments, specify the range
            instrumentDefs += "  Note range: MIDI " + String(capabilities.minNote) + 
                            " to " + String(capabilities.maxNote) + "\n";
        }
        
        instrumentDefs += "\n";
    }
    
    return instrumentDefs;
}

} //namespace covertracks
