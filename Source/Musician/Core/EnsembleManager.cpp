#include "EnsembleManager.h"
#include "PatternParser.h"
#include <iostream>

namespace covertracks {

EnsembleManager::EnsembleManager(PlayerManager* playerManagerPtr, AudioEngineImpl* audioEnginePtr, const juce::String& id)
    : playerManager(playerManagerPtr),
      audioEngine(audioEnginePtr),
      playing(false),
      pendingPatternRequests(0),
      ensembleId(id.isNotEmpty() ? id : juce::Uuid().toString())
{
    // Create the log directory
    logDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("Covertracks")
                    .getChildFile("Logs");
    
    if (!logDirectory.exists()) {
        logDirectory.createDirectory();
    }
    
    // Create the ensembles directory if it doesn't exist
    juce::File ensemblesDir = getEnsemblesDirectory();
    if (!ensemblesDir.exists()) {
        ensemblesDir.createDirectory();
    }
}

EnsembleManager::~EnsembleManager()
{
    stopPlaying();
    
    // Clear all musician roles to release any references to patterns
    musicianRoles.clear();
    
    std::cout << "[EnsembleManager] Destroyed" << std::endl;
}

bool EnsembleManager::addMusicianRole(const juce::String& roleName, const juce::String& instrumentType, Player* player)
{
    // Check if the role already exists
    for (auto& role : musicianRoles) {
        if (role.name == roleName) {
            std::cout << "[EnsembleManager] Role '" << roleName << "' already exists" << std::endl;
            return false;
        }
    }
    
    // Add the role
    musicianRoles.add(MusicianRole(roleName, instrumentType, player));
    std::cout << "[EnsembleManager] Added role '" << roleName << "' with instrument '" << instrumentType << "'" << std::endl;
    return true;
}

bool EnsembleManager::removeMusicianRole(const juce::String& roleName)
{
    // Find the role
    for (int i = 0; i < musicianRoles.size(); ++i) {
        if (musicianRoles[i].name == roleName) {
            musicianRoles.remove(i);
            std::cout << "[EnsembleManager] Removed role '" << roleName << "'" << std::endl;
            return true;
        }
    }
    
    std::cout << "[EnsembleManager] Role '" << roleName << "' not found" << std::endl;
    return false;
}

const juce::Array<MusicianRole>& EnsembleManager::getMusicianRoles() const
{
    return musicianRoles;
}

void EnsembleManager::generateEnsemblePatterns(const juce::String& songDescription, std::function<void(bool)> callback)
{
    if (musicianRoles.isEmpty()) {
        std::cout << "[EnsembleManager] No musician roles defined" << std::endl;
        if (callback) callback(false);
        return;
    }
    
    // Store the song description
    currentSongDescription = songDescription;
    
    // Store the callback
    patternGenerationCallback = callback;
    
    // Reset pending requests counter
    pendingPatternRequests = musicianRoles.size();
    
    // Generate patterns for each role
    juce::StringArray responses;
    
    for (auto& role : musicianRoles) {
        generatePatternForRole(role, songDescription, [this, &responses, songDescription](bool success) {
            // Use the success parameter to log if needed
            if (!success) {
                std::cout << "[EnsembleManager] Failed to generate pattern for a role" << std::endl;
            }
            
            // Decrement the pending requests counter
            pendingPatternRequests--;
            
            // Check if all patterns have been generated
            if (pendingPatternRequests == 0) {
                // Log the conversation
                logEnsembleConversation(songDescription, responses);
                
                // Call the callback
                if (patternGenerationCallback) {
                    patternGenerationCallback(true);
                }
            }
        });
    }
}

bool EnsembleManager::startPlaying()
{
    if (playing) {
        std::cout << "[EnsembleManager] Already playing" << std::endl;
        return false;
    }
    
    if (musicianRoles.isEmpty()) {
        std::cout << "[EnsembleManager] No musician roles defined" << std::endl;
        return false;
    }
    
    // Check if all roles have patterns
    for (auto& role : musicianRoles) {
        if (role.pattern.isEmpty()) {
            std::cout << "[EnsembleManager] Role '" << role.name << "' has no pattern" << std::endl;
            return false;
        }
    }
    
    // Start playing all players
    for (auto& role : musicianRoles) {
        if (role.player) {
            // Ensure the player is connected to the audio engine if we have one
            if (audioEngine != nullptr && !role.player->isConnectedToAudioEngine()) {
                std::cout << "[EnsembleManager] Connecting player '" << role.player->getMetadata().name 
                          << "' to the ensemble audio engine" << std::endl;
                if (!role.player->connectToAudioEngine(audioEngine)) {
                    std::cout << "[EnsembleManager] Failed to connect player '" << role.player->getMetadata().name 
                              << "' to the audio engine" << std::endl;
                }
            }
            
            // TODO: Parse the pattern and set it for the player
            // We need a MusicPatternSystem instance to create a PatternParser
            // For now, just log that we would parse the pattern
            std::cout << "[EnsembleManager] Would parse pattern for role '" << role.name << "'" << std::endl;
            
            // TODO: Set the MIDI sequence for the player
            // role.player->setMidiSequence(midiSequence);
            
            // Start the player
            // role.player->startPlaying();
        }
    }
    
    // Record the start time
    playbackStartTime = juce::Time::getCurrentTime();
    playing = true;
    
    // Start the timer to update playback
    startTimer(100); // Update every 100ms
    
    std::cout << "[EnsembleManager] Started playing ensemble" << std::endl;
    return true;
}

void EnsembleManager::stopPlaying()
{
    if (!playing) {
        return;
    }
    
    // Stop the timer
    stopTimer();
    
    // Stop all players
    for (auto& role : musicianRoles) {
        if (role.player) {
            // role.player->stopPlaying();
        }
    }
    
    playing = false;
    std::cout << "[EnsembleManager] Stopped playing ensemble" << std::endl;
}

bool EnsembleManager::isPlaying() const
{
    return playing;
}

void EnsembleManager::logEnsembleConversation(const juce::String& songDescription, const juce::StringArray& ensembleResponses)
{
    // Create a timestamp
    juce::Time now = juce::Time::getCurrentTime();
    juce::String timestamp = now.formatted("%Y-%m-%d_%H-%M-%S");
    
    // Create the log file
    juce::File logFile = logDirectory.getChildFile("ensemble_" + timestamp + ".log");
    
    // Create the log content
    juce::String logContent = "Song Description: " + songDescription + "\n\n";
    
    for (int i = 0; i < musicianRoles.size(); ++i) {
        const auto& role = musicianRoles[i];
        logContent += "Role: " + role.name + " (" + role.instrument + ")\n";
        
        if (i < ensembleResponses.size()) {
            logContent += "Response:\n" + ensembleResponses[i] + "\n\n";
        }
        
        logContent += "Pattern:\n" + role.pattern + "\n\n";
        logContent += "------------------------------\n\n";
    }
    
    // Write the log file
    logFile.replaceWithText(logContent);
    
    std::cout << "[EnsembleManager] Logged ensemble conversation to " << logFile.getFullPathName() << std::endl;
}

void EnsembleManager::generatePatternForRole(MusicianRole& role, const juce::String& songDescription, std::function<void(bool)> callback)
{
    // Create the prompt
    juce::String prompt = createPromptForRole(role, songDescription);
    
    // Get the LLM client
    auto llmClient = LLMManager::getInstance().getDefaultClient();
    if (!llmClient) {
        std::cout << "[EnsembleManager] No LLM client available" << std::endl;
        if (callback) callback(false);
        return;
    }
    
    // Send the prompt to the LLM
    llmClient->sendMessage(prompt, [this, &role, callback](bool success, const juce::String& response) {
        if (success) {
            // Parse the pattern from the response
            juce::String pattern = parsePatternFromResponse(role, response);
            
            // Store the pattern
            role.pattern = pattern;
            
            std::cout << "[EnsembleManager] Generated pattern for role '" << role.name << "'" << std::endl;
            
            if (callback) callback(true);
        } else {
            std::cout << "[EnsembleManager] Failed to generate pattern for role '" << role.name << "': " << response << std::endl;
            
            if (callback) callback(false);
        }
    });
}

juce::String EnsembleManager::createPromptForRole(const MusicianRole& role, const juce::String& songDescription)
{
    juce::String prompt = "You are the " + role.name + " player in a band, playing " + role.instrument + ".\n\n";
    prompt += "We are creating a song with the following description:\n" + songDescription + "\n\n";
    
    // Add specific instructions based on the role
    if (role.name.containsIgnoreCase("lead")) {
        prompt += "As the lead instrument, create an engaging melody that stands out and drives the song forward. ";
        prompt += "Include a memorable solo section that showcases your technical ability.\n\n";
    } else if (role.name.containsIgnoreCase("rhythm")) {
        prompt += "As a rhythm instrument, create a solid foundation that supports the lead instruments. ";
        prompt += "Focus on chord progressions and rhythmic patterns that establish the groove of the song.\n\n";
    } else if (role.name.containsIgnoreCase("bass")) {
        prompt += "As the bass player, create a bassline that works with the drums to establish the rhythm section. ";
        prompt += "Focus on root notes and rhythmic patterns that complement the overall groove.\n\n";
    } else if (role.name.containsIgnoreCase("drum")) {
        prompt += "As the drummer, create a drum pattern that establishes the rhythm and feel of the song. ";
        prompt += "Include variations for verse, chorus, and bridge sections.\n\n";
    }
    
    // Add instructions for the pattern format
    prompt += "Create a musical pattern using the following format:\n";
    prompt += "- Use standard music notation with measures separated by '|'\n";
    prompt += "- Specify notes with letter names (A, B, C, D, E, F, G) and accidentals (# for sharp, b for flat)\n";
    prompt += "- Include octave numbers after note names (e.g., C4 for middle C)\n";
    prompt += "- Specify note durations (w for whole, h for half, q for quarter, e for eighth, s for sixteenth)\n";
    prompt += "- Use dots for dotted notes (e.g., q. for dotted quarter)\n";
    prompt += "- For chords, list notes in brackets separated by spaces (e.g., [C4 E4 G4])\n";
    prompt += "- Use 'r' for rests with appropriate duration (e.g., rq for quarter rest)\n";
    prompt += "- Include time signature at the beginning (e.g., 4/4)\n";
    prompt += "- Include tempo marking (e.g., 120 BPM)\n";
    prompt += "- Divide the pattern into sections (INTRO, VERSE, CHORUS, BRIDGE, OUTRO)\n\n";
    
    prompt += "Example pattern format:\n";
    prompt += "TEMPO: 120 BPM\n";
    prompt += "TIME: 4/4\n\n";
    prompt += "INTRO:\n";
    prompt += "| C4q E4q G4q C5q | G4h E4q C4q |\n\n";
    prompt += "VERSE:\n";
    prompt += "| [C4 E4 G4]h [A3 C4 E4]h | [F3 A3 C4]h [G3 B3 D4]h |\n\n";
    
    prompt += "Now, create a complete pattern for " + role.name + " playing " + role.instrument + " in this song. ";
    prompt += "Include all sections (intro, verse, chorus, bridge, outro) and make sure the pattern fits the style and mood described.";
    
    return prompt;
}

juce::String EnsembleManager::parsePatternFromResponse(const MusicianRole& /*role*/, const juce::String& llmResponse)
{
    // Extract the pattern from the LLM response
    // This is a simple implementation that assumes the pattern is the entire response
    // In a real implementation, you would want to extract just the pattern part
    
    // Look for pattern markers
    int tempoIndex = llmResponse.indexOf(juce::StringRef("TEMPO:"));
    int timeIndex = llmResponse.indexOf(juce::StringRef("TIME:"));
    int introIndex = llmResponse.indexOf(juce::StringRef("INTRO:"));
    
    // If we found at least one marker, extract from there
    if (tempoIndex >= 0 || timeIndex >= 0 || introIndex >= 0) {
        int startIndex = juce::jmin(
            tempoIndex >= 0 ? tempoIndex : std::numeric_limits<int>::max(),
            timeIndex >= 0 ? timeIndex : std::numeric_limits<int>::max(),
            introIndex >= 0 ? introIndex : std::numeric_limits<int>::max()
        );
        
        return llmResponse.substring(startIndex);
    }
    
    // If no markers found, look for code blocks
    int codeBlockStart = llmResponse.indexOf(juce::StringRef("```"));
    if (codeBlockStart >= 0) {
        int codeBlockEnd = llmResponse.indexOf(codeBlockStart + 3, juce::StringRef("```"));
        if (codeBlockEnd >= 0) {
            return llmResponse.substring(codeBlockStart + 3, codeBlockEnd);
        }
    }
    
    // If all else fails, return the entire response
    return llmResponse;
}

void EnsembleManager::timerCallback()
{
    // This method is called periodically to synchronize playback
    // For now, it's just a placeholder
}

juce::var EnsembleManager::toVar() const
{
    juce::DynamicObject* obj = new juce::DynamicObject();
    
    // Add metadata
    obj->setProperty("id", ensembleId);
    obj->setProperty("name", ensembleName);
    obj->setProperty("description", ensembleDescription);
    obj->setProperty("currentSongDescription", currentSongDescription);
    
    // Add roles
    juce::Array<juce::var> rolesArray;
    for (const auto& role : musicianRoles)
    {
        juce::DynamicObject* roleObj = new juce::DynamicObject();
        roleObj->setProperty("name", role.name);
        roleObj->setProperty("instrument", role.instrument);
        roleObj->setProperty("pattern", role.pattern);
        roleObj->setProperty("level", role.level);
        
        // Store player ID if available
        if (role.player)
        {
            roleObj->setProperty("playerId", role.player->getId());
        }
        
        rolesArray.add(roleObj);
    }
    
    obj->setProperty("roles", rolesArray);
    
    return juce::var(obj);
}

std::unique_ptr<EnsembleManager> EnsembleManager::fromVar(const juce::var& v, PlayerManager* playerManager)
{
    if (!v.isObject())
        return nullptr;
    
    juce::DynamicObject* obj = v.getDynamicObject();
    if (!obj)
        return nullptr;
    
    // Get the ID
    juce::String id = obj->getProperty("id").toString();
    
    // Create the ensemble
    std::unique_ptr<EnsembleManager> ensemble = std::make_unique<EnsembleManager>(playerManager, nullptr, id);
    
    // Set metadata
    ensemble->setName(obj->getProperty("name").toString());
    ensemble->setDescription(obj->getProperty("description").toString());
    ensemble->setCurrentSongDescription(obj->getProperty("currentSongDescription").toString());
    
    // Add roles
    juce::var rolesVar = obj->getProperty("roles");
    if (rolesVar.isArray())
    {
        juce::Array<juce::var>* rolesArray = rolesVar.getArray();
        
        for (const auto& roleVar : *rolesArray)
        {
            if (roleVar.isObject())
            {
                juce::DynamicObject* roleObj = roleVar.getDynamicObject();
                
                juce::String roleName = roleObj->getProperty("name").toString();
                juce::String instrument = roleObj->getProperty("instrument").toString();
                juce::String pattern = roleObj->getProperty("pattern").toString();
                // Get level property with default value of 1.0f if not present
                float level = 1.0f;
                if (roleObj->hasProperty("level")) {
                    level = static_cast<float>(static_cast<double>(roleObj->getProperty("level")));
                }
                
                // Find the player if available
                Player* player = nullptr;
                if (roleObj->hasProperty("playerId") && playerManager)
                {
                    juce::String playerId = roleObj->getProperty("playerId").toString();
                    DBG("EnsembleManager: Looking for player with ID: " << playerId);
                    for (auto* p : playerManager->getPlayers())
                    {
                        DBG("EnsembleManager: Found player with ID: " << p->getId() << " (name: " << p->getMetadata().name << ")");
                        if (p->getId() == playerId)
                        {
                            player = p;
                            DBG("EnsembleManager: Matched player '" << p->getMetadata().name << "' to role '" << roleName << "'");
                            break;
                        }
                    }
                    if (!player)
                    {
                        DBG("EnsembleManager: No player found with ID: " << playerId << " for role: " << roleName);
                    }
                }
                else
                {
                    DBG("EnsembleManager: No playerId property for role: " << roleName << " or no playerManager");
                }
                
                // Add the role
                if (ensemble->addMusicianRole(roleName, instrument, player))
                {
                    // Set the pattern and level
                    for (auto& role : ensemble->musicianRoles)
                    {
                        if (role.name == roleName)
                        {
                            role.pattern = pattern;
                            role.level = level;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return ensemble;
}

bool EnsembleManager::saveToFile(const juce::File& file) const
{
    // Convert to var
    juce::var data = toVar();
    
    // Convert to JSON
    juce::String jsonString = juce::JSON::toString(data);
    
    // Save to file
    return file.replaceWithText(jsonString);
}

std::unique_ptr<EnsembleManager> EnsembleManager::loadFromFile(const juce::File& file, PlayerManager* playerManager)
{
    if (!file.exists())
        return nullptr;
    
    // Read the file
    juce::String jsonString = file.loadFileAsString();
    
    // Parse the JSON
    juce::var data;
    juce::Result result = juce::JSON::parse(jsonString, data);
    
    if (result.failed())
        return nullptr;
    
    // Create the ensemble
    return fromVar(data, playerManager);
}

juce::File EnsembleManager::getEnsemblesDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("Covertracks")
            .getChildFile("Ensembles");
}

juce::Array<juce::File> EnsembleManager::getEnsembleFiles()
{
    juce::Array<juce::File> result;
    
    juce::File dir = getEnsemblesDirectory();
    if (dir.exists())
    {
        dir.findChildFiles(result, juce::File::findFiles, false, "*.ensemble");
    }
    
    return result;
}

} // namespace covertracks
