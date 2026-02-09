#include "Performance.h"
#include "../../Player/Core/PlayerManager.h"
#include "../../Player/Core/Player.h"
#include "../../LLM/Core/LLMClient.h"
#include "../../LLM/Core/PromptManager.h"
#include "MusicJson3/LeadSheetConverter.h"
#include <iostream>

namespace covertracks {

Performance::Performance(const juce::String& performanceName)
    : name(performanceName)
{
    // Generate a unique ID for this performance
    juce::Uuid uuid;
    uniqueID = uuid.toString();
}

Performance::~Performance()
{
    // Stop any active playback before cleanup
    stopAll();
    
    // Clear references to break potential reference cycles
    song = nullptr;
    chart = nullptr;
    generatedCharts.clear();
}

// The setSong method is now implemented inline in the header file

juce::var Performance::getGeneratedChart(const juce::String& roleName) const
{
    auto it = generatedCharts.find(roleName);
    if (it != generatedCharts.end())
    {
        return it->second;
    }
    return juce::var();
}

void Performance::setGeneratedChart(const juce::String& roleName, const juce::var& generatedChart)
{
    generatedCharts[roleName] = generatedChart;
}

juce::String Performance::getLLMPattern(const juce::String& roleName) const
{
    auto it = llmPatterns.find(roleName);
    if (it != llmPatterns.end())
    {
        return it->second;
    }
    return "";
}

void Performance::setLLMPattern(const juce::String& roleName, const juce::String& pattern)
{
    llmPatterns[roleName] = pattern;
}

juce::var Performance::getMIDIChart(const juce::String& roleName) const
{
    auto it = midiCharts.find(roleName);
    if (it != midiCharts.end())
    {
        return it->second;
    }
    return juce::var();
}

void Performance::setMIDIChart(const juce::String& roleName, const juce::var& midiChart)
{
    midiCharts[roleName] = midiChart;
}

juce::String Performance::getStatusMessage(const juce::String& roleName) const
{
    auto it = statusMessages.find(roleName);
    if (it != statusMessages.end())
    {
        return it->second;
    }
    return "";
}

void Performance::setStatusMessage(const juce::String& roleName, const juce::String& status)
{
    statusMessages[roleName] = status;
}

void Performance::clearGeneratedCharts()
{
    generatedCharts.clear();
    llmPatterns.clear();
    midiCharts.clear();
    statusMessages.clear();
}

juce::StringArray Performance::getGeneratedChartRoleNames() const
{
    juce::StringArray roleNames;
    for (const auto& pair : generatedCharts)
    {
        roleNames.add(pair.first);
    }
    return roleNames;
}

void Performance::cleanupStaleStatusMessages()
{
    // Remove status messages for roles that have no chart data
    auto statusIt = statusMessages.begin();
    while (statusIt != statusMessages.end())
    {
        const juce::String& roleName = statusIt->first;
        auto chartIt = generatedCharts.find(roleName);
        
        // If no chart exists for this role, remove the status message
        if (chartIt == generatedCharts.end() || chartIt->second.isVoid())
        {
            statusIt = statusMessages.erase(statusIt);
        }
        else
        {
            ++statusIt;
        }
    }
}

juce::Array<MusicianRole> Performance::getEnsemble() const
{
    // Return cached roles if valid
    if (ensembleCacheValid)
    {
        return cachedEnsembleRoles;
    }
    
    // Load ensemble roles and cache them
    const_cast<Performance*>(this)->refreshEnsembleCache();
    return cachedEnsembleRoles;
}

void Performance::refreshEnsembleCache()
{
    cachedEnsembleRoles.clear();
    ensembleCacheValid = false;
    
    if (ensembleName.isEmpty())
    {
        ensembleCacheValid = true;
        return;
    }
    
    // Load the actual ensemble from disk
    juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    for (const auto& file : ensembleFiles)
    {
        // Load the ensemble to check its name
        auto ensemble = EnsembleManager::loadFromFile(file, nullptr); // We don't need PlayerManager just to get roles
        if (ensemble && ensemble->getName() == ensembleName)
        {
            // Found the matching ensemble, get its musician roles but clear player pointers
            // since we're not managing the full ensemble lifecycle here
            auto ensembleRoles = ensemble->getMusicianRoles();
            for (auto role : ensembleRoles)
            {
                // Player will be resolved by PerformanceEditorComponent
                // when it needs to actually work with players
                cachedEnsembleRoles.add(role);
            }
            ensembleCacheValid = true;
            return;
        }
    }
    
    // If no ensemble found, cache is still valid (empty)
    ensembleCacheValid = true;
}

juce::var Performance::toVar() const
{
    juce::DynamicObject* obj = new juce::DynamicObject();
    
    obj->setProperty("id", uniqueID);
    obj->setProperty("name", name);
    obj->setProperty("description", description);
    
    // Save song ID if we have a song
    if (song)
    {
        obj->setProperty("songId", song->getUniqueID());
        obj->setProperty("song", song->getUniqueID()); // For backward compatibility
    }
    
    // Save chart ID if we have a chart
    if (chart)
    {
        obj->setProperty("chartId", chart->getUniqueID());
    }
    
    obj->setProperty("ensembleName", ensembleName);
    obj->setProperty("instructions", instructions);
    
    // Save performance attributes
    obj->setProperty("key", key);
    obj->setProperty("genre", genre);
    obj->setProperty("mood", mood);
    obj->setProperty("style", style);
    obj->setProperty("character", character);
    
    // Save generated charts (legacy support)
    juce::DynamicObject* chartsObj = new juce::DynamicObject();
    for (const auto& pair : generatedCharts)
    {
        if (!pair.second.isVoid())
        {
            // Store the chart directly as a var
            chartsObj->setProperty(pair.first, pair.second);
        }
    }
    obj->setProperty("charts", var(chartsObj));
    
    // Save LLM patterns
    juce::DynamicObject* llmPatternsObj = new juce::DynamicObject();
    for (const auto& pair : llmPatterns)
    {
        if (!pair.second.isEmpty())
        {
            llmPatternsObj->setProperty(pair.first, pair.second);
        }
    }
    obj->setProperty("llmPatterns", var(llmPatternsObj));
    
    // Save MIDI charts
    juce::DynamicObject* midiChartsObj = new juce::DynamicObject();
    for (const auto& pair : midiCharts)
    {
        if (!pair.second.isVoid())
        {
            midiChartsObj->setProperty(pair.first, pair.second);
        }
    }
    obj->setProperty("midiCharts", var(midiChartsObj));
    
    // Save status messages
    juce::DynamicObject* statusObj = new juce::DynamicObject();
    for (const auto& pair : statusMessages)
    {
        statusObj->setProperty(pair.first, pair.second);
    }
    obj->setProperty("status", var(statusObj));
    
    return var(obj);
}

std::shared_ptr<Performance> Performance::fromVar(const juce::var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        auto performance = std::make_shared<Performance>();
        
        // Load basic properties
        if (obj->hasProperty("id"))
        {
            performance->uniqueID = obj->getProperty("id").toString();
        }
        
        if (obj->hasProperty("name"))
        {
            performance->name = obj->getProperty("name").toString();
        }
        
        if (obj->hasProperty("description"))
        {
            performance->description = obj->getProperty("description").toString();
        }
        
        // Note: We don't load the song or chart here, as we need the SongListComponent to find them by ID
        // This will be handled by the PerformanceComponent when loading
        
        if (obj->hasProperty("ensembleName"))
        {
            performance->ensembleName = obj->getProperty("ensembleName").toString();
        }
        
        if (obj->hasProperty("instructions"))
        {
            performance->instructions = obj->getProperty("instructions").toString();
        }
        
        // Load performance attributes
        if (obj->hasProperty("key"))
        {
            performance->key = obj->getProperty("key").toString();
        }
        
        if (obj->hasProperty("genre"))
        {
            performance->genre = obj->getProperty("genre").toString();
        }
        
        if (obj->hasProperty("mood"))
        {
            performance->mood = obj->getProperty("mood").toString();
        }
        
        if (obj->hasProperty("style"))
        {
            performance->style = obj->getProperty("style").toString();
        }
        
        if (obj->hasProperty("character"))
        {
            performance->character = obj->getProperty("character").toString();
        }
        
        // Load generated charts (legacy support)
        if (auto* chartsObj = obj->getProperty("charts").getDynamicObject())
        {
            for (auto& prop : chartsObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                performance->generatedCharts[roleName] = prop.value;
            }
        }
        
        // Load LLM patterns
        if (auto* llmPatternsObj = obj->getProperty("llmPatterns").getDynamicObject())
        {
            for (auto& prop : llmPatternsObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                performance->llmPatterns[roleName] = prop.value.toString();
            }
        }
        
        // Load MIDI charts
        if (auto* midiChartsObj = obj->getProperty("midiCharts").getDynamicObject())
        {
            for (auto& prop : midiChartsObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                performance->midiCharts[roleName] = prop.value;
            }
        }
        
        // Load status messages
        if (auto* statusObj = obj->getProperty("status").getDynamicObject())
        {
            for (auto& prop : statusObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                juce::String status = prop.value.toString();
                
                performance->statusMessages[roleName] = status;
            }
        }
        
        return performance;
    }
    
    return nullptr;
}

// =========================================================================
// Unified Playback System Implementation
// =========================================================================

bool Performance::play(PlayerManager* playerManager, const juce::StringArray& roleNames)
{
    if (!playerManager)
    {
        std::cerr << "[Performance] No PlayerManager provided for playback" << std::endl;
        return false;
    }
    
    // Stop any current playback
    stopAll();
    
    // Filter roles to play
    auto rolesToPlay = filterRolesToPlay(roleNames);
    if (rolesToPlay.isEmpty())
    {
        std::cerr << "[Performance] No valid roles found to play" << std::endl;
        return false;
    }
    
    std::cout << "[Performance] Starting unified playback for " << rolesToPlay.size() << " roles..." << std::endl;
    
    // Load charts for all roles (Load phase)
    if (!loadChartsForRoles(rolesToPlay, playerManager))
    {
        std::cerr << "[Performance] Failed to load charts for playback" << std::endl;
        return false;
    }
    
    // Start synchronized playback (Play phase)
    if (!startPlaybackForLoadedRoles())
    {
        std::cerr << "[Performance] Failed to start synchronized playback" << std::endl;
        return false;
    }
    
    std::cout << "[Performance] Unified playback started successfully!" << std::endl;
    return true;
}

bool Performance::play(PlayerManager* playerManager, const juce::String& roleName)
{
    juce::StringArray singleRole;
    singleRole.add(roleName);
    return play(playerManager, singleRole);
}

bool Performance::playWithEnsemble(PlayerManager* playerManager, const juce::Array<MusicianRole>& ensembleRoles, const juce::StringArray& roleNames)
{
    if (!playerManager)
    {
        std::cerr << "[Performance] No PlayerManager provided for playback" << std::endl;
        return false;
    }
    
    // Stop any current playback
    stopAll();
    
    // Filter roles to play using provided ensemble
    auto rolesToPlay = filterRolesToPlay(ensembleRoles, roleNames);
    if (rolesToPlay.isEmpty())
    {
        std::cerr << "[Performance] No valid roles found to play" << std::endl;
        return false;
    }
    
    std::cout << "[Performance] Starting unified playback for " << rolesToPlay.size() << " roles..." << std::endl;
    
    // Load charts for all roles (Load phase)
    if (!loadChartsForRoles(rolesToPlay, playerManager))
    {
        std::cerr << "[Performance] Failed to load charts for playback" << std::endl;
        return false;
    }
    
    // Start synchronized playback (Play phase)
    if (!startPlaybackForLoadedRoles())
    {
        std::cerr << "[Performance] Failed to start synchronized playback" << std::endl;
        return false;
    }
    
    std::cout << "[Performance] Unified playback started successfully!" << std::endl;
    return true;
}

int Performance::loadCharts(PlayerManager* playerManager, const juce::StringArray& roleNames)
{
    if (!playerManager)
    {
        std::cerr << "[Performance] No PlayerManager provided for loading" << std::endl;
        return 0;
    }
    
    // Filter roles to load
    auto rolesToLoad = filterRolesToPlay(roleNames);
    if (rolesToLoad.isEmpty())
    {
        std::cerr << "[Performance] No valid roles found to load" << std::endl;
        return 0;
    }
    
    std::cout << "[Performance] Loading charts for " << rolesToLoad.size() << " roles..." << std::endl;
    
    // Load charts without starting playback
    loadChartsForRoles(rolesToLoad, playerManager);
    
    // Count successfully loaded roles
    int loaded = 0;
    for (const auto& role : rolesToLoad)
    {
        if (activePlayerRefs.find(role.getName()) != activePlayerRefs.end())
        {
            loaded++;
        }
    }
    
    std::cout << "[Performance] Loaded " << loaded << " of " << rolesToLoad.size() << " charts" << std::endl;
    return loaded;
}

int Performance::loadChartsWithEnsemble(PlayerManager* playerManager, const juce::Array<MusicianRole>& ensembleRoles, const juce::StringArray& roleNames)
{
    if (!playerManager)
    {
        std::cerr << "[Performance] No PlayerManager provided for loading" << std::endl;
        return 0;
    }
    
    // Filter roles to load using provided ensemble
    auto rolesToLoad = filterRolesToPlay(ensembleRoles, roleNames);
    if (rolesToLoad.isEmpty())
    {
        std::cerr << "[Performance] No valid roles found to load" << std::endl;
        return 0;
    }
    
    std::cout << "[Performance] Loading charts for " << rolesToLoad.size() << " roles..." << std::endl;
    
    // Load charts without starting playback
    loadChartsForRoles(rolesToLoad, playerManager);
    
    // Count successfully loaded roles
    int loaded = 0;
    for (const auto& role : rolesToLoad)
    {
        if (activePlayerRefs.find(role.getName()) != activePlayerRefs.end())
        {
            loaded++;
        }
    }
    
    std::cout << "[Performance] Loaded " << loaded << " of " << rolesToLoad.size() << " charts" << std::endl;
    return loaded;
}

void Performance::stopAll()
{
    if (!activePlayerRefs.empty())
    {
        std::cout << "[Performance] Stopping " << activePlayerRefs.size() << " active Players" << std::endl;
        
        for (auto& pair : activePlayerRefs)
        {
            if (pair.second)
            {
                pair.second->stop();
                std::cout << "[Performance] Stopped Player: " << pair.first << std::endl;
            }
        }
        
        activePlayerRefs.clear();
        std::cout << "[Performance] All playback stopped and cleared" << std::endl;
    }
}

bool Performance::isPlaying() const
{
    for (const auto& pair : activePlayerRefs)
    {
        if (pair.second && pair.second->isPlaying())
        {
            return true;
        }
    }
    return false;
}

// =========================================================================
// Helper Methods
// =========================================================================

juce::Array<MusicianRole> Performance::filterRolesToPlay(const juce::StringArray& roleNames)
{
    auto ensemble = getEnsemble();
    juce::Array<MusicianRole> rolesToPlay;
    
    if (roleNames.isEmpty())
    {
        // Play all roles (ensemble playback)
        rolesToPlay = ensemble;
        std::cout << "[Performance] Playing all " << ensemble.size() << " roles in ensemble" << std::endl;
    }
    else
    {
        // Play only specified roles
        for (const auto& role : ensemble)
        {
            if (roleNames.contains(role.getName()))
            {
                rolesToPlay.add(role);
                std::cout << "[Performance] Added role for playback: " << role.getName() << std::endl;
            }
        }
        std::cout << "[Performance] Playing " << rolesToPlay.size() << " of " << roleNames.size() << " requested roles" << std::endl;
    }
    
    return rolesToPlay;
}

juce::Array<MusicianRole> Performance::filterRolesToPlay(const juce::Array<MusicianRole>& ensembleRoles, const juce::StringArray& roleNames)
{
    juce::Array<MusicianRole> rolesToPlay;
    
    if (roleNames.isEmpty())
    {
        // Play all roles (ensemble playback)
        rolesToPlay = ensembleRoles;
        std::cout << "[Performance] Playing all " << ensembleRoles.size() << " roles in provided ensemble" << std::endl;
    }
    else
    {
        // Play only specified roles
        for (const auto& role : ensembleRoles)
        {
            if (roleNames.contains(role.getName()))
            {
                rolesToPlay.add(role);
                std::cout << "[Performance] Added role for playback: " << role.getName() << std::endl;
            }
        }
        std::cout << "[Performance] Playing " << rolesToPlay.size() << " of " << roleNames.size() << " requested roles" << std::endl;
    }
    
    return rolesToPlay;
}

bool Performance::loadChartsForRoles(const juce::Array<MusicianRole>& roles, PlayerManager* playerManager)
{
    bool anySuccess = false;
    
    for (const auto& role : roles)
    {
        if (!role.player)
        {
            std::cout << "[Performance] No Player available for role: " << role.getName() << std::endl;
            continue;
        }
        
        // Get the prepped MIDI chart
        juce::var midiChart = getMIDIChart(role.getName());
        if (midiChart.isVoid())
        {
            midiChart = getGeneratedChart(role.getName());
        }
        
        if (midiChart.isVoid())
        {
            std::cerr << "[Performance] No chart available for role: '" << role.getName() << "'" << std::endl;

            // Debug: Show what charts are actually available
            std::cout << "[Performance] Available midiCharts keys: ";
            for (const auto& pair : midiCharts)
                std::cout << "'" << pair.first << "' ";
            std::cout << std::endl;

            std::cout << "[Performance] Available generatedCharts keys: ";
            for (const auto& pair : generatedCharts)
                std::cout << "'" << pair.first << "' ";
            std::cout << std::endl;

            continue;
        }
        
        // Use the Player directly from the role
        auto* player = role.player;
        if (!player)
        {
            std::cerr << "[Performance] No Player available for role: " << role.getName() << std::endl;
            continue;
        }
        
        // Extract MIDI events and tempo from chart
        juce::var midiEvents;
        double tempo = 120.0; // Default tempo
        
        if (midiChart.isObject() && midiChart.hasProperty("events"))
        {
            midiEvents = midiChart["events"];
            // Extract tempo if available
            if (midiChart.hasProperty("tempo"))
            {
                tempo = static_cast<double>(midiChart["tempo"]);
                std::cout << "[Performance] Using tempo " << tempo << " BPM for role: " << role.getName() << std::endl;
            }
        }
        else
        {
            midiEvents = midiChart;
        }
        
        // Load chart into Player with correct tempo
        if (player->loadChart(midiEvents, tempo))
        {
            // Store reference to Player (not owned by Performance)
            activePlayerRefs[role.getName()] = player;
            anySuccess = true;
            std::cout << "[Performance] Loaded chart for role: " << role.getName() << " using Player" << std::endl;
        }
        else
        {
            std::cerr << "[Performance] Failed to load chart for role: " << role.getName() << std::endl;
        }
    }
    
    return anySuccess;
}

bool Performance::startPlaybackForLoadedRoles()
{
    if (activePlayerRefs.empty())
    {
        std::cerr << "[Performance] No loaded Players available for playback" << std::endl;
        return false;
    }
    
    bool anySuccess = false;
    std::cout << "[Performance] Starting synchronized playback for " << activePlayerRefs.size() << " loaded Players..." << std::endl;
    
    // Start all Players simultaneously for synchronized playback
    for (auto& pair : activePlayerRefs)
    {
        if (pair.second->playLoadedChart())
        {
            anySuccess = true;
            std::cout << "[Performance] Started Player: " << pair.first << std::endl;
        }
        else
        {
            std::cerr << "[Performance] Failed to start Player: " << pair.first << std::endl;
        }
    }
    
    return anySuccess;
}

// Legacy method removed - Player is now provided directly in MusicianRole

// =========================================================================
// Chart Generation Methods (NEW: Extracted from UI)
// =========================================================================

bool Performance::generateChartForRole(const juce::String& roleName, 
                                      const MusicianRole& role,
                                      LLMClient* llmClient,
                                      const juce::String& styleOverride,
                                      const juce::String& characterOverride,
                                      std::function<void(bool, const juce::String&)> callback)
{
    if (!llmClient)
    {
        if (callback) callback(false, "No LLM client available");
        return false;
    }
    
    if (!role.player)
    {
        if (callback) callback(false, "No player assigned to role " + roleName);
        return false;
    }
    
    // Check if already generating for this role
    if (generatingRoles.count(roleName) > 0)
    {
        if (callback) callback(false, "Already generating pattern for " + roleName);
        return false;
    }
    
    // Mark as generating
    generatingRoles.insert(roleName);
    setStatusMessage(roleName, "Generating LLM pattern...");
    
    // Use provided overrides or fall back to performance settings
    String currentStyle = styleOverride.isNotEmpty() ? styleOverride : style;
    String currentCharacter = characterOverride.isNotEmpty() ? characterOverride : character;
    
    // Default style if none set
    if (currentStyle.isEmpty()) currentStyle = "jazz";
    
    // Get tempo from song or use default
    int tempo = 120; // Default
    if (song)
    {
        tempo = static_cast<int>(song->getTempo());
    }
    
    // Create comprehensive description for the LLM
    String generationDescription = "Generate a " + role.getInstrumentName() + " chart for the role '" + roleName + "'";

    // Convert lead sheet to 3.0 skeleton if song has structure data
    String skeletonJson;
    String melodyRef;

    if (song)
    {
        generationDescription += " for the song '" + song->getTitle() + "'";

        String songStructure = song->getSourceStructureJson();
        if (songStructure.isNotEmpty())
        {
            auto converted = mj3::LeadSheetConverter::convert(songStructure);
            if (converted.totalBars > 0)
            {
                skeletonJson = converted.skeleton;
                if (converted.melodyReference.isNotEmpty())
                    melodyRef = "Original melody to interpret in your style:\n" + converted.melodyReference;
                generationDescription += " (" + String(converted.totalBars) + " bars)";
            }
            else
            {
                generationDescription += ". Song structure: " + songStructure;
            }
        }
    }

    if (instructions.isNotEmpty())
    {
        generationDescription += ". Performance instructions: " + instructions;
    }

    if (key.isNotEmpty())
    {
        generationDescription += ". Key: " + key;
    }

    if (mood.isNotEmpty())
    {
        generationDescription += ". Mood: " + mood;
    }

    // Build ensemble list from cached roles (excluding current role)
    String ensembleList;
    {
        auto ensemble = getEnsemble();
        juce::StringArray otherRoles;
        for (const auto& r : ensemble)
        {
            if (r.getName() != roleName)
                otherRoles.add(r.getName());
        }
        ensembleList = otherRoles.joinIntoString(", ");
    }

    // Use PromptManager to create the prompt
    PromptManager promptManager;
    String prompt = promptManager.createMusicGenerationPrompt(
        currentStyle,
        currentCharacter,
        role.player,
        roleName, // Use role name as role definition
        tempo,
        generationDescription,
        skeletonJson,
        melodyRef,
        ensembleList
    );
    
    // Send LLM request with callback
    llmClient->sendMessage(prompt, [this, roleName, callback](bool success, const String& response)
    {
        // Ensure we're on the message thread for thread safety
        MessageManager::callAsync([this, roleName, success, response, callback]()
        {
            // Clear the generating flag
            generatingRoles.erase(roleName);
            
            if (success)
            {
                // Validate response isn't too large
                if (response.length() > 100000) // 100KB limit
                {
                    setStatusMessage(roleName, "Response too large");
                    if (callback) callback(false, "LLM response too large");
                    return;
                }
                
                // Extract JSON from response (handle markdown formatting)
                String jsonContent = response.trim();
                
                // Handle markdown code blocks
                if (jsonContent.startsWith("```json") || jsonContent.startsWith("```"))
                {
                    int startIndex = jsonContent.indexOf("\n");
                    if (startIndex >= 0)
                    {
                        jsonContent = jsonContent.substring(startIndex + 1);
                        int endIndex = jsonContent.lastIndexOf("```");
                        if (endIndex >= 0)
                        {
                            jsonContent = jsonContent.substring(0, endIndex);
                        }
                    }
                    jsonContent = jsonContent.trim();
                }
                
                // Find JSON object boundaries if not markdown
                if (!jsonContent.startsWith("{"))
                {
                    int jsonStart = jsonContent.indexOf("{");
                    if (jsonStart >= 0)
                    {
                        jsonContent = jsonContent.substring(jsonStart);
                    }
                }
                
                if (!jsonContent.endsWith("}"))
                {
                    int jsonEnd = jsonContent.lastIndexOf("}");
                    if (jsonEnd >= 0)
                    {
                        jsonContent = jsonContent.substring(0, jsonEnd + 1);
                    }
                }
                
                // Final validation
                if (!jsonContent.startsWith("{") || !jsonContent.endsWith("}"))
                {
                    setStatusMessage(roleName, "Invalid JSON");
                    if (callback) callback(false, "Invalid JSON response");
                    return;
                }
                
                // Store the extracted JSON pattern
                setLLMPattern(roleName, jsonContent);
                setStatusMessage(roleName, "LLM pattern ready");
                if (callback) callback(true, "LLM pattern generated successfully");
            }
            else
            {
                setStatusMessage(roleName, "Generation Failed");
                if (callback) callback(false, "Failed to generate LLM pattern");
            }
        });
    });
    
    return true;
}

bool Performance::prepareChartForRole(const juce::String& roleName, 
                                     const MusicianRole& role,
                                     double tempoOverride)
{
    if (!role.player)
    {
        setStatusMessage(roleName, "No player available");
        return false;
    }
    
    // Check if LLM pattern exists
    String llmPattern = getLLMPattern(roleName);
    if (llmPattern.isEmpty())
    {
        setStatusMessage(roleName, "No LLM pattern to prep");
        return false;
    }
    
    // Get tempo from override, song, or default
    double tempo = 120.0; // Default
    if (tempoOverride > 0.0)
    {
        tempo = tempoOverride;
    }
    else if (song)
    {
        tempo = song->getTempo();
    }
    
    // Apply the role's level setting to the player
    role.player->setLevel(role.level);
    
    // Convert LLM pattern to MIDI chart
    auto midiChart = role.player->convertPatternStringToChart(llmPattern, tempo);
    
    if (midiChart.hasProperty("success") && static_cast<bool>(midiChart["success"]))
    {
        setMIDIChart(roleName, midiChart);
        setStatusMessage(roleName, "MIDI chart ready");
        return true;
    }
    else
    {
        setStatusMessage(roleName, "Prep failed");
        return false;
    }
}

bool Performance::isRoleGenerating(const juce::String& roleName) const
{
    return generatingRoles.count(roleName) > 0;
}

} // namespace covertracks
