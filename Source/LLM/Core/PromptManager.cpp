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
                                                const String& description,
                                                const String& skeleton,
                                                const String& melodyReference,
                                                const String& ensembleList)
{
    // Use skeleton-aware template when skeleton is provided, otherwise compact template
    String templateName = skeleton.isNotEmpty() ? "music_generation_skeleton" : "music_generation_compact";
    String userTemplate = getPromptTemplate(templateName);

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
    variables["instrument_list"] = ensembleList;
    variables["other_instrument_roles"] = ""; // TODO: Get from ensemble context
    variables["song_to_compose"] = description;
    variables["skeleton"] = skeleton;
    variables["melody_reference"] = melodyReference;

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
    
    // Compact user prompt template using Music JSON 3.0
    promptTemplates["music_generation_compact"] =
        "Create a {{style}} piece for {{instrument}}.\n"
        "{{instrument_definition}}\n"
        "Role: {{instrument_role_definition}} in ensemble with {{instrument_list}}.\n"
        "Style: {{style}}. {{character}}\n\n"
        "Use Music JSON 3.0 format:\n"
        "```json\n"
        "{\n"
        "  \"version\": \"3.0\",\n"
        "  \"tempo\": 120,\n"
        "  \"context\": {\"mode_system\": \"chord\", \"mode\": \"Cmaj7\", \"velocity\": 0.75},\n"
        "  \"define\": {\n"
        "    \"bass_walk\": {\n"
        "      \"events\": [\n"
        "        {\"degree\": 1, \"beat\": 0, \"duration\": 1},\n"
        "        {\"degree\": 3, \"beat\": 1, \"duration\": 1},\n"
        "        {\"degree\": 5, \"beat\": 2, \"duration\": 1},\n"
        "        {\"degree\": 3, \"beat\": 3, \"duration\": 1}\n"
        "      ]\n"
        "    }\n"
        "  },\n"
        "  \"root\": {\n"
        "    \"mode\": \"sequential\",\n"
        "    \"children\": [\n"
        "      {\"context\": {\"mode\": \"Dm7\"}, \"mode\": \"simultaneous\", \"children\": [\n"
        "        {\"ref\": \"bass_walk\", \"repeat\": 4}\n"
        "      ]},\n"
        "      {\"context\": {\"mode\": \"G7\"}, \"mode\": \"simultaneous\", \"children\": [\n"
        "        {\"ref\": \"bass_walk\", \"repeat\": 4}\n"
        "      ]}\n"
        "    ]\n"
        "  }\n"
        "}\n"
        "```\n\n"
        "Key rules:\n"
        "- \"define\" creates reusable patterns, \"root\" is the arrangement\n"
        "- \"mode\": \"sequential\" = play one after another, \"simultaneous\" = play together\n"
        "- Each child can have its own \"repeat\" count\n"
        "- Use \"degree\" for harmony parts (bass, comp) so they follow chord changes\n"
        "- Use \"note\" (e.g. \"C4\") for melodies with specific pitches, or \"drum\" for percussion\n"
        "- Set \"context\": {\"mode\": \"ChordName\"} per section/bar-group — degrees resolve against it\n"
        "- In \"simultaneous\" containers, ALL siblings must cover the same total duration (e.g. 1-bar bass x8 + 2-bar comp x4 = both 8 bars)\n"
        "- Beat 0 = start of container. Duration in beats (1=quarter, 0.5=eighth)\n\n"
        "IMPORTANT: Keep patterns SHORT (1-2 bars max, 4-8 events each). Use \"repeat\" heavily to fill sections.\n"
        "Chord voicings: use degree arrays like {\"degree\": [1, 3, 5, 7], \"beat\": 0, \"duration\": 4}\n"
        "CRITICAL: Match the song's bar count EXACTLY. If the song has 32 bars, output 32 bars.\n"
        "Create one sequential child per chord change (each measure or group of measures on the same chord).\n"
        "Do NOT collapse or skip measures. Every bar in the song must be accounted for.\n"
        "Do NOT use markdown code fences. Output raw JSON only.\n\n"
        "Song: {{song_to_compose}}\n\n"
        "Respond with JSON only:";
        
    
    // Skeleton-aware template: LLM fills in patterns for a pre-built structure
    promptTemplates["music_generation_skeleton"] =
        "Create a {{style}} part for {{instrument}}.\n"
        "{{instrument_definition}}\n"
        "Role: {{instrument_role_definition}} in ensemble with {{instrument_list}}.\n"
        "Style: {{style}}. {{character}}\n\n"
        "{{song_to_compose}}\n\n"
        "Here is the Music JSON 3.0 skeleton with the song's structure and chord changes.\n"
        "Replace the FILL placeholders in \"define\" with actual degree-based patterns.\n"
        "CRITICAL: Each pattern MUST span exactly 4 beats (beat 0 through beat 3) to fill one bar.\n"
        "The skeleton uses TWO patterns for variety: \"groove\" (main feel) and \"fill\" (contrasting).\n"
        "Make groove and fill RHYTHMICALLY DIFFERENT — vary the rhythms, voicings, and density.\n"
        "Use \"degree\" for harmony/comp parts so they follow chord changes.\n"
        "Use \"note\" (e.g. \"C4\") for MELODY lines with specific pitches — melodies should have stepwise motion, leaps, and rhythmic variety.\n"
        "Chord voicings: use degree arrays like {\"degree\": [1, 3, 5, 7], \"beat\": 0, \"duration\": 4}\n"
        "In \"simultaneous\" containers, ALL siblings must cover the same total duration (4 beats).\n"
        "You may add more patterns to \"define\" and more refs in each section's children.\n"
        "Do NOT change the root structure, chord contexts, or repeat counts.\n"
        "Do NOT use markdown code fences. Output raw JSON only.\n\n"
        "{{skeleton}}\n\n"
        "{{melody_reference}}\n\n"
        "Respond with the complete JSON (skeleton with FILL placeholders replaced by real patterns):";

    // Full user prompt template using Music JSON 3.0
    promptTemplates["music_generation"] =
        "Create a {{style}} piece for {{instrument}}.\n"
        "Instrument definition:\n"
        "{{instrument_definition}}\n"
        "Ensemble role: You're arranging for the {{instrument}} in an ensemble consisting of {{instrument_list}}. {{instrument_role_definition}} {{other_instrument_roles}}.\n"
        "Keep it interesting, including complexity, variation and syncopation, in line with the style.\n\n"
        "Style: {{style}}. {{character}}\n\n"
        "Requirements: Output must be in Music JSON 3.0 format. Respond with ONLY JSON, no explanations.\n\n"
        "# Music JSON 3.0 Specification\n\n"
        "## Core Structure\n"
        "```json\n"
        "{\n"
        "  \"version\": \"3.0\",\n"
        "  \"tempo\": 120,\n"
        "  \"context\": {\"mode_system\": \"chord\", \"mode\": \"Cmaj7\", \"velocity\": 0.75},\n"
        "  \"define\": { ... reusable patterns ... },\n"
        "  \"root\": { ... arrangement tree ... }\n"
        "}\n"
        "```\n\n"
        "## Containers\n"
        "Two modes: `\"sequential\"` (children play one after another) and `\"simultaneous\"` (children play together).\n"
        "Each container can have `\"repeat\": N` — a 1-bar riff with `\"repeat\": 8` plays 8 times independently of siblings.\n\n"
        "```json\n"
        "{\"name\": \"verse\", \"mode\": \"simultaneous\", \"children\": [\n"
        "  {\"ref\": \"bass_walk\", \"repeat\": 8},\n"
        "  {\"ref\": \"melody_verse\"}\n"
        "]}\n"
        "```\n\n"
        "## Events\n"
        "```json\n"
        "{\"note\": \"C4\", \"beat\": 0, \"duration\": 1, \"velocity\": 0.8}\n"
        "```\n"
        "- `beat`: position within container (0 = start). Duration in beats (1=quarter, 0.5=eighth)\n"
        "- `velocity`: 0.0-1.0 (optional, inherits from context)\n\n"
        "## Pitch Types\n"
        "- Note name: `{\"note\": \"C4\"}`, `{\"note\": \"Bb3\"}`, `{\"note\": \"F#5\"}`\n"
        "- Scale degree: `{\"degree\": 1}` — resolves against context mode (chord)\n"
        "- MIDI number: `{\"midi\": 60}`\n"
        "- Drums: `{\"drum\": \"kick\"}`, `{\"drum\": \"snare\"}`, `{\"drum\": \"hihat\"}`\n\n"
        "## Define & Ref\n"
        "`\"define\"` creates reusable patterns. `\"ref\"` references them anywhere in the tree.\n"
        "```json\n"
        "\"define\": {\n"
        "  \"walking\": {\n"
        "    \"events\": [\n"
        "      {\"note\": \"C3\", \"beat\": 0, \"duration\": 1},\n"
        "      {\"note\": \"E3\", \"beat\": 1, \"duration\": 1},\n"
        "      {\"note\": \"G3\", \"beat\": 2, \"duration\": 1},\n"
        "      {\"note\": \"B3\", \"beat\": 3, \"duration\": 1}\n"
        "    ]\n"
        "  }\n"
        "}\n"
        "```\n\n"
        "## Context Inheritance\n"
        "Context cascades down the tree. Set chord at section level, degree events resolve against it.\n"
        "```json\n"
        "{\"name\": \"bridge\", \"context\": {\"mode\": \"Dm7\"}, \"children\": [\n"
        "  {\"ref\": \"bass_walk\", \"repeat\": 4}\n"
        "]}\n"
        "```\n\n"
        "## Full Example (degree-based patterns + chord changes)\n"
        "```json\n"
        "{\n"
        "  \"version\": \"3.0\",\n"
        "  \"tempo\": 140,\n"
        "  \"context\": {\"mode_system\": \"chord\", \"mode\": \"BbMaj7\", \"velocity\": 0.75},\n"
        "  \"define\": {\n"
        "    \"bass_walk\": {\n"
        "      \"events\": [\n"
        "        {\"degree\": 1, \"beat\": 0, \"duration\": 1},\n"
        "        {\"degree\": 3, \"beat\": 1, \"duration\": 1},\n"
        "        {\"degree\": 5, \"beat\": 2, \"duration\": 0.5},\n"
        "        {\"degree\": 4, \"beat\": 2.5, \"duration\": 0.5},\n"
        "        {\"degree\": 3, \"beat\": 3, \"duration\": 1}\n"
        "      ]\n"
        "    },\n"
        "    \"comp\": {\n"
        "      \"events\": [\n"
        "        {\"degree\": 1, \"beat\": 0, \"duration\": 2, \"velocity\": 0.5},\n"
        "        {\"degree\": 3, \"beat\": 0, \"duration\": 2, \"velocity\": 0.5},\n"
        "        {\"degree\": 5, \"beat\": 0, \"duration\": 2, \"velocity\": 0.5},\n"
        "        {\"degree\": 1, \"beat\": 2.5, \"duration\": 1.5, \"velocity\": 0.6},\n"
        "        {\"degree\": 3, \"beat\": 2.5, \"duration\": 1.5, \"velocity\": 0.6}\n"
        "      ]\n"
        "    }\n"
        "  },\n"
        "  \"root\": {\n"
        "    \"mode\": \"sequential\",\n"
        "    \"children\": [\n"
        "      {\"name\": \"A\", \"mode\": \"sequential\", \"children\": [\n"
        "        {\"context\": {\"mode\": \"Dm7\"}, \"mode\": \"simultaneous\", \"children\": [\n"
        "          {\"ref\": \"bass_walk\", \"repeat\": 2},\n"
        "          {\"ref\": \"comp\", \"repeat\": 2}\n"
        "        ]},\n"
        "        {\"context\": {\"mode\": \"G7\"}, \"mode\": \"simultaneous\", \"children\": [\n"
        "          {\"ref\": \"bass_walk\", \"repeat\": 2},\n"
        "          {\"ref\": \"comp\", \"repeat\": 2}\n"
        "        ]},\n"
        "        {\"context\": {\"mode\": \"CMaj7\"}, \"mode\": \"simultaneous\", \"children\": [\n"
        "          {\"ref\": \"bass_walk\", \"repeat\": 2},\n"
        "          {\"ref\": \"comp\", \"repeat\": 2}\n"
        "        ]}\n"
        "      ]},\n"
        "      {\"name\": \"B\", \"mode\": \"sequential\", \"children\": [\n"
        "        {\"context\": {\"mode\": \"Fm7\"}, \"mode\": \"simultaneous\", \"children\": [\n"
        "          {\"ref\": \"bass_walk\", \"repeat\": 4},\n"
        "          {\"ref\": \"comp\", \"repeat\": 4}\n"
        "        ]}\n"
        "      ]}\n"
        "    ]\n"
        "  }\n"
        "}\n"
        "```\n"
        "NOTE: Use \"degree\" (not \"note\") for patterns that should follow chord changes. "
        "Set \"context\": {\"mode\": \"ChordName\"} on each section/bar group so the same pattern sounds different over each chord.\n\n"
        "IMPORTANT CONSTRAINTS:\n"
        "- Keep each pattern SHORT: 1-2 bars max, 4-8 events. Use \"repeat\" to fill sections.\n"
        "- In \"simultaneous\" containers, ALL siblings must cover the same total duration (e.g. 1-bar bass x8 + 2-bar comp x4 = both 8 bars).\n"
        "- Chord voicings: use degree arrays like {\"degree\": [1, 3, 5, 7], \"beat\": 0, \"duration\": 4}\n"
        "- Reuse patterns across sections — don't duplicate similar voicings.\n"
        "- CRITICAL: Match the song's bar count EXACTLY. One sequential child per chord change.\n"
        "- Do NOT collapse or skip measures. Every bar must be accounted for.\n"
        "- Do NOT output markdown fences. Raw JSON only.\n\n"
        "Song to compose:\n\n"
        "{{song_to_compose}}";
    
    // Template for pattern modification using Music JSON 3.0
    promptTemplates["pattern_modification"] =
        "You are a professional music pattern editor. I have a Music JSON 3.0 chart to modify.\n\n"
        "Current chart:\n"
        "```json\n"
        "{{current_pattern}}\n"
        "```\n\n"
        "Modifications: {{modification}}\n\n"
        "Guidelines:\n"
        "- Keep \"version\": \"3.0\" and the define/root structure\n"
        "- Use note names (\"note\": \"C4\") or scale degrees (\"degree\": 1) for pitch\n"
        "- Each container can have its own \"repeat\" count\n"
        "- Preserve tempo and arrangement unless asked to change\n\n"
        "Return the ENTIRE modified JSON chart. Respond with ONLY valid JSON.";
    
    // Template for section generation using Music JSON 3.0
    promptTemplates["section_generation"] =
        "Create a {{section}} section for a {{style}} piece using: {{instruments}}.\n\n"
        "Instructions: {{description}}\n\n"
        "Use Music JSON 3.0 format. Define patterns in \"define\", arrange in \"root\".\n"
        "Each part (bass, melody, comp) should be a separate definition with its own repeat count.\n\n"
        "```json\n"
        "{\n"
        "  \"version\": \"3.0\",\n"
        "  \"tempo\": 120,\n"
        "  \"define\": {\n"
        "    \"pattern_name\": {\"events\": [{\"note\": \"C4\", \"beat\": 0, \"duration\": 1}]}\n"
        "  },\n"
        "  \"root\": {\n"
        "    \"name\": \"{{section}}\",\n"
        "    \"mode\": \"simultaneous\",\n"
        "    \"children\": [\n"
        "      {\"ref\": \"pattern_name\", \"repeat\": 4}\n"
        "    ]\n"
        "  }\n"
        "}\n"
        "```\n"
        "Respond with ONLY valid JSON.";
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
