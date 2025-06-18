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
    variables["instrument"] = instruments.size() > 0 ? instruments[0] : "instrument";
    variables["instruments"] = instruments.joinIntoString(", ");
    variables["tempo"] = String(tempo);
    variables["description"] = description;
    variables["character"] = ""; // Default empty
    variables["instrument_role_definition"] = ""; // Default empty
    
    // Process the template
    return processTemplate(templateText, variables);
}

String PromptManager::createMusicGenerationPrompt(const String& style,
                                                const String& character, 
                                                const String& instrument,
                                                const String& instrumentRoleDefinition,
                                                int /*tempo*/, 
                                                const String& description)
{
    // Get the user prompt template
    String userTemplate = getPromptTemplate("music_generation");
    
    // Get instrument definition - note: this needs to be passed a Player object
    // For now using fallback, will be updated when Player is available
    String instrumentDefinition = getInstrumentDefinition(instrument);
    
    // Create variables map
    std::map<String, String> variables;
    variables["style"] = style;
    variables["character"] = character;
    variables["instrument"] = instrument;
    variables["instrument_role_definition"] = instrumentRoleDefinition;
    variables["instrument_definition"] = instrumentDefinition;
    variables["instrument_list"] = ""; // TODO: Get from ensemble context
    variables["other_instrument_roles"] = ""; // TODO: Get from ensemble context  
    variables["song_to_compose"] = description; // Use description as song content for now
    
    // Process the template
    return processTemplate(userTemplate, variables);
}

String PromptManager::createMusicGenerationPrompt(const String& style,
                                                const String& character, 
                                                Player* player,
                                                const String& instrumentRoleDefinition,
                                                int /*tempo*/, 
                                                const String& description)
{
    // Use compact template to avoid token limits
    String userTemplate = getPromptTemplate("music_generation_compact");
    
    // Get instrument definition from player capabilities
    String instrumentDefinition = getInstrumentDefinitionFromPlayer(player);
    String instrument = player ? player->getInstrumentCapabilities().instrumentType : "unknown";
    
    // Create variables map
    std::map<String, String> variables;
    variables["style"] = style;
    variables["character"] = character;
    variables["instrument"] = instrument;
    variables["instrument_role_definition"] = instrumentRoleDefinition;
    variables["instrument_definition"] = instrumentDefinition;
    variables["instrument_list"] = ""; // TODO: Get from ensemble context
    variables["other_instrument_roles"] = ""; // TODO: Get from ensemble context  
    variables["song_to_compose"] = description; // Use description as song content for now
    
    // Process the template
    return processTemplate(userTemplate, variables);
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
    // System prompt template (note: actual system prompt comes from LLM client)
    promptTemplates["music_generation_system"] = 
        "You are a professional musician and music pattern generator. You have in depth knowledge of modern western music theory and practice and are capable of creating in genre and cross genre pieces with both catchy licks and sophisticated harmonies and complex rhythms as needed.\n"
        "Use the Instrument definition and the compact musical json spec below to generate music in the style specified for the role specified to play the song specified.";
    
    // Compact user prompt template (shorter version to avoid token limits)
    promptTemplates["music_generation_compact"] = 
        "Create a {{style}} piece for {{instrument}}.\n"
        "{{instrument_definition}}\n"
        "Role: {{instrument_role_definition}} in ensemble with {{instrument_list}}.\n"
        "Style: {{style}}. {{character}}\n\n"
        "Use this JSON format with chord tone references:\n"
        "```json\n"
        "{\n"
        "  \"title\": \"Pattern Name\",\n"
        "  \"tempo\": 120,\n"
        "  \"chords\": {\"C\": [\"C3\", \"E3\", \"G3\"]},\n"
        "  \"patterns\": {\n"
        "    \"main\": [\n"
        "      [1.0, {\"t\": 1, \"o\": 2}, 0.8, 0.5],\n"
        "      [2.0, {\"t\": 5, \"o\": 2}, 0.7, 0.5]\n"
        "    ]\n"
        "  },\n"
        "  \"sections\": {\n"
        "    \"verse\": {\n"
        "      \"bars\": 4,\n"
        "      \"layers\": {\"melody\": {\"pattern\": \"main\"}}\n"
        "    }\n"
        "  },\n"
        "  \"arrangement\": [\"verse\"]\n"
        "}\n"
        "```\n\n"
        "Chord tones: t=1(root),3(third),5(fifth),7(seventh), o=octave. Use [beat, note, velocity, duration] format.\n"
        "Song: {{song_to_compose}}\n\n"
        "Respond with JSON only:";
        
    
    // Full user prompt template  
    promptTemplates["music_generation"] = 
        "Create a {{style}} piece for {{instrument}}.\n"
        "Instrument definition:\n"
        "{{instrument_definition}}\n"
        "Ensemble role: You're arranging for the {{instrument}} in an ensemble consisting of {{instrument_list}}. {{instrument_role_definition}} {{other_instrument_roles}}.\n"
        "Keep it interesting, including complexity, variation and sycopation, and in line with the style, below.\n\n"
        "Style: {{style}}. {{character}}\n\n"
        "Requirements: There should be exactly the same number of bars as the song. Output must be in the Compact json format as described below. Respond with only JSON chart, no explanations or comments.\n\n"
        "# Music Pattern Language - Compact Specification\n\n"
        "## Core Structure\n\n"
        "```json\n"
        "{\n"
        "  \"title\": \"Song Title\",\n"
        "  \"tempo\": 120,\n"
        "  \"chords\": { \"C\": [\"C3\", \"E3\", \"G3\"] },\n"
        "  \"patterns\": { \"name\": [[beat, note, velocity, duration]] },\n"
        "  \"sections\": { \"name\": { \"bars\": 4, \"layers\": {...} } },\n"
        "  \"arrangement\": [\"section1\", \"section2\"]\n"
        "}\n"
        "```\n\n"
        "## Note Format\n\n"
        "**Basic**: `[beat, note, velocity, duration]`\n"
        "- `beat`: 1.0=downbeat, 1.5=eighth, 2.0=beat 2\n"
        "- `note`: \"C4\", \"F#3\", {\"chord\": \"C\"}, {\"tone\": 1}\n"
        "- `velocity`: 0.0-1.0 (optional, default: 0.8)\n"
        "- `duration`: beats (0.25=sixteenth, 0.5=eighth, 1.0=quarter)\n\n"
        "## Note Types\n\n"
        "### Fixed Notes\n"
        "```json\n"
        "[1.0, \"C4\", 0.9, 0.5]\n"
        "```\n\n"
        "### Chord References\n"
        "```json\n"
        "[1.0, {\"chord\": \"C\"}, 0.7, 2.0]\n"
        "```\n\n"
        "### **NEW: Chord Tone References**\n"
        "```json\n"
        "[1.0, {\"tone\": 1, \"octave\": 2}, 0.8, 0.5]\n"
        "```\n"
        "- `tone`: 1=root, 3=third, 5=fifth, 7=seventh, etc.\n"
        "- `octave`: target octave (optional, default: 4)\n"
        "- **Automatically adapts to chord context**\n\n"
        "## Section Types\n\n"
        "### Layers (Recommended)\n"
        "```json\n"
        "\"section\": {\n"
        "  \"bars\": 4,\n"
        "  \"layers\": {\n"
        "    \"harmony\": [chord events],\n"
        "    \"melody\": [note events],\n"
        "    \"bass\": {\"pattern\": \"bassLine\"}\n"
        "  }\n"
        "}\n"
        "```\n\n"
        "### Pattern References\n"
        "```json\n"
        "\"layers\": {\n"
        "  \"bass\": {\"pattern\": \"walkingBass\"}\n"
        "}\n"
        "```\n\n"
        "### Multi-bar Direct Arrays\n"
        "```json\n"
        "\"layers\": {\n"
        "  \"melody\": [\n"
        "    [1.0, \"C4\", 0.9, 1.0],  // Bar 1\n"
        "    [5.0, \"F4\", 0.9, 1.0]   // Bar 2 (beat 5 = bar 2, beat 1)\n"
        "  ]\n"
        "}\n"
        "```\n\n"
        "## Chord Tone Examples\n\n"
        "### Adaptive Walking Bass\n"
        "```json\n"
        "\"patterns\": {\n"
        "  \"walkingBass\": [\n"
        "    [1.0, {\"tone\": 1, \"octave\": 2}, 0.8, 0.5],  // Root\n"
        "    [1.5, {\"tone\": 5, \"octave\": 2}, 0.6, 0.5],  // Fifth\n"
        "    [2.0, {\"tone\": 3, \"octave\": 2}, 0.8, 0.5],  // Third\n"
        "    [2.5, {\"tone\": 1, \"octave\": 2}, 0.6, 0.5]   // Root\n"
        "  ]\n"
        "}\n"
        "```\n\n"
        "**This pattern automatically becomes:**\n"
        "- Over C: C2, G2, E2, C2\n"
        "- Over F: F2, C3, A2, F2\n"
        "- Over Am: A2, E3, C3, A2\n\n"
        "### Harmonic Comping\n"
        "```json\n"
        "\"patterns\": {\n"
        "  \"comping\": [\n"
        "    [1.0, {\"tone\": 1, \"octave\": 4}, 0.7, 0.5],\n"
        "    [1.75, {\"tone\": 3, \"octave\": 4}, 0.6, 0.25],\n"
        "    [2.25, {\"tone\": 5, \"octave\": 4}, 0.7, 0.5]\n"
        "  ]\n"
        "}\n"
        "```\n\n"
        "## Complete Example\n\n"
        "```json\n"
        "{\n"
        "  \"title\": \"Jazz Standard\",\n"
        "  \"tempo\": 120,\n"
        "  \"chords\": {\n"
        "    \"C\": [\"C3\", \"E3\", \"G3\"],\n"
        "    \"F\": [\"F3\", \"A3\", \"C4\"],\n"
        "    \"G\": [\"G3\", \"B3\", \"D4\"]\n"
        "  },\n"
        "  \"patterns\": {\n"
        "    \"walkingBass\": [\n"
        "      [1.0, {\"tone\": 1, \"octave\": 2}, 0.8, 0.5],\n"
        "      [1.5, {\"tone\": 3, \"octave\": 2}, 0.6, 0.5],\n"
        "      [2.0, {\"tone\": 5, \"octave\": 2}, 0.8, 0.5],\n"
        "      [2.5, {\"tone\": 3, \"octave\": 2}, 0.6, 0.5]\n"
        "    ]\n"
        "  },\n"
        "  \"sections\": {\n"
        "    \"verse\": {\n"
        "      \"bars\": 4,\n"
        "      \"layers\": {\n"
        "        \"chords\": [\n"
        "          [1.0, {\"chord\": \"C\"}, 0.7, 4.0],\n"
        "          [5.0, {\"chord\": \"F\"}, 0.7, 4.0],\n"
        "          [9.0, {\"chord\": \"G\"}, 0.7, 4.0],\n"
        "          [13.0, {\"chord\": \"C\"}, 0.7, 4.0]\n"
        "        ],\n"
        "        \"bass\": {\"pattern\": \"walkingBass\"}\n"
        "      }\n"
        "    }\n"
        "  },\n"
        "  \"arrangement\": [\"verse\"]\n"
        "}\n"
        "```\n\n"
        "## Key Benefits\n\n"
        "- **Chord Tones**: One pattern works with any chord progression\n"
        "- **Compactness**: Minimal JSON for maximum musical expression\n"
        "- **Flexibility**: Mix fixed notes, chords, and harmonic references\n"
        "- **Legibility**: Clear musical relationships and structure\n\n"
        "## Chord Context Resolution\n\n"
        "Chord tone references automatically find the most recent `{\"chord\": \"name\"}` event at or before the current beat to resolve tone numbers to actual pitches.\n\n"
        "Song to compose:\n\n"
        "{{song_to_compose}}";
    
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
        "- Use chord tone references {\"tone\": 1-7, \"octave\": 2-5} for adaptive harmony\n"
        "- Keep patterns consistent with the instrument capabilities shown in the original chart\n"
        "- Preserve tempo and arrangement unless specifically requested to change\n\n"
        "Return the ENTIRE modified JSON chart, not just the changes.\n"
        "ONLY respond with valid JSON. DO NOT add explanations or comments outside the JSON.";
    
    // Template for section generation using JSON format
    promptTemplates["section_generation"] = 
        "You are a professional musician and music pattern generator. Create a {{section}} section for a {{style}} piece using these instruments: {{instruments}}.\n\n"
        "Additional instructions: {{description}}\n\n"
        "Create a COMPACT JSON chart with ONLY the {{section}} section using chord tone references:\n\n"
        "COMPACT JSON Format:\n"
        "{\n"
        "  \"title\": \"{{section|title_case}}_Section\",\n"
        "  \"tempo\": 120,\n"
        "  \"chords\": {\n"
        "    \"I\": [\"note1\", \"note2\", \"note3\"]\n"
        "  },\n"
        "  \"patterns\": {\n"
        "    \"main\": [\n"
        "      [1.0, {\"tone\": 1}, 0.8, 0.5],\n"
        "      [2.0, {\"tone\": 5}, 0.7, 0.5]\n"
        "    ]\n"
        "  },\n"
        "  \"sections\": {\n"
        "    \"{{section}}\": {\n"
        "      \"bars\": 4,\n"
        "      \"layers\": {\n"
        "        \"harmony\": {\"pattern\": \"main\"}\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}\n\n"
        "Use chord tone references for adaptive patterns. Keep it CONCISE.\n"
        "ONLY respond with valid JSON. DO NOT add explanations.";
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
    String instrumentDefs = "Available Instrument Primitives:\n";
    
    for (const auto* player : players)
    {
        if (!player) continue;
        
        const auto& capabilities = player->getInstrumentCapabilities();
        instrumentDefs += capabilities.instrumentType + ": ";
        
        if (capabilities.type == InstrumentCapabilityType::Mapping)
        {
            // For mapped instruments, list just the primitive names
            StringArray soundNames;
            for (const auto& mapping : capabilities.soundMappings)
            {
                soundNames.add(mapping.first);
            }
            instrumentDefs += soundNames.joinIntoString(", ");
        }
        else if (capabilities.type == InstrumentCapabilityType::NoteRange)
        {
            // For note range instruments, show example notes within range
            if (capabilities.instrumentType == "piano")
            {
                instrumentDefs += "C2, D2, E2, F2, G2, A2, B2, C3, D3, E3, F3, G3, A3, B3, C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5 (MIDI " + String(capabilities.minNote) + "-" + String(capabilities.maxNote) + ")";
            }
            else if (capabilities.instrumentType == "bass")
            {
                instrumentDefs += "E1, F1, G1, A1, B1, C2, D2, E2, F2, G2, A2, B2, C3, D3, E3, F3, G3 (MIDI " + String(capabilities.minNote) + "-" + String(capabilities.maxNote) + ")";
            }
            else if (capabilities.instrumentType == "guitar")
            {
                instrumentDefs += "E2, F2, G2, A2, B2, C3, D3, E3, F3, G3, A3, B3, C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5, A5, B5, C6, D6, E6 (MIDI " + String(capabilities.minNote) + "-" + String(capabilities.maxNote) + ")";
            }
            else
            {
                instrumentDefs += "MIDI notes " + String(capabilities.minNote) + " to " + String(capabilities.maxNote);
            }
        }
        
        instrumentDefs += "\n";
    }
    
    return instrumentDefs;
}

String PromptManager::getCompactMusicalJsonSpec()
{
    // This method is no longer used since the spec is embedded directly in the prompt template
    // Keeping for backward compatibility
    return "Compact Musical JSON Specification embedded in prompt template";
}

String PromptManager::getInstrumentDefinition(const String& instrument)
{
    String lowerInstrument = instrument.toLowerCase();
    
    if (instrumentDefinitions.hasProperty(Identifier(lowerInstrument)))
    {
        var def = instrumentDefinitions[Identifier(lowerInstrument)];
        String instrumentDef = instrument + ":\n";
        
        // Add primitives
        if (def.hasProperty("primitives") && def["primitives"].isObject())
        {
            auto* primitives = def["primitives"].getDynamicObject();
            instrumentDef += "Available notes/sounds: ";
            
            StringArray primitiveNames;
            for (auto& key : primitives->getProperties())
                primitiveNames.add(key.name.toString());
            
            instrumentDef += primitiveNames.joinIntoString(", ") + "\n";
        }
        
        // Add patterns
        if (def.hasProperty("patterns") && def["patterns"].isObject())
        {
            auto* patterns = def["patterns"].getDynamicObject();
            instrumentDef += "Common patterns: ";
            
            StringArray patternInfos;
            for (auto& key : patterns->getProperties())
            {
                var pattern = patterns->getProperty(key.name);
                if (pattern.isObject() && pattern.hasProperty("description"))
                {
                    patternInfos.add(pattern["description"].toString());
                }
            }
            
            instrumentDef += patternInfos.joinIntoString(", ") + "\n";
        }
        
        return instrumentDef;
    }
    
    // Fallback for unknown instruments
    return instrument + ": Standard instrument capabilities";
}

String PromptManager::getInstrumentDefinitionFromPlayer(Player* player)
{
    if (!player)
        return "Unknown instrument: No player provided";
    
    const auto& capabilities = player->getInstrumentCapabilities();
    String instrumentDef = capabilities.instrumentType + " capabilities:\n";
    
    if (capabilities.type == InstrumentCapabilityType::NoteRange)
    {
        // For note range instruments (piano, guitar, bass, etc.)
        instrumentDef += "Note range: MIDI " + String(capabilities.minNote) + " to " + String(capabilities.maxNote) + "\n";
        instrumentDef += "Use standard MIDI note names (e.g., \"C4\", \"F#3\", \"Bb2\") - the system will convert these to MIDI numbers automatically.\n";
        
        // Add some example notes for context
        if (capabilities.instrumentType == "piano")
        {
            instrumentDef += "Example notes: C2, C3, C4 (middle C), C5, C6\n";
        }
        else if (capabilities.instrumentType == "bass")
        {
            instrumentDef += "Example notes: E1, A1, D2, G2, C3, F3\n";
        }
        else if (capabilities.instrumentType == "guitar")
        {
            instrumentDef += "Example notes: E2, A2, D3, G3, B3, E4\n";
        }
    }
    else if (capabilities.type == InstrumentCapabilityType::Mapping)
    {
        // For mapped instruments (drums, percussion, etc.)
        instrumentDef += "Available sounds (use these exact names in your patterns):\n";
        
        for (const auto& mapping : capabilities.soundMappings)
        {
            instrumentDef += "- \"" + mapping.first + "\" (MIDI " + String(mapping.second) + ")\n";
        }
        
        instrumentDef += "\nIMPORTANT: Use the sound names (like \"kick\", \"snare\", \"hihat\") NOT the MIDI numbers in your JSON patterns.\n";
    }
    
    return instrumentDef;
}

} //namespace covertracks
