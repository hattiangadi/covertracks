// PlayerManager.cpp
#include "PlayerManager.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../Audio/AudioEngineInterface.h"

namespace covertracks {

PlayerManager::PlayerManager()
{
    // std::cout << "[PlayerManager] Constructor starting..." << std::endl;
    
    // Get singleton AudioEngine which owns the formatManager and knownPluginList
    // std::cout << "[PlayerManager] Getting AudioEngine singleton for plugin setup" << std::endl;
    auto* audioEngine = getAudioEnginePtr();
    auto& formatManager = audioEngine->getFormatManager();
    auto& knownPluginList = audioEngine->getKnownPluginList();
    
    // Create a single internal plugin format that will be owned by the formatManager
    // std::cout << "[PlayerManager] Creating internal plugin format" << std::endl;
    auto* internalFormat = new InternalPluginFormat();
    formatManager.addFormat(internalFormat); // formatManager takes ownership
    
    // Store a non-owning pointer to the internal format for our use
    internalPluginFormat = internalFormat;
    
    // Get the internal plugin types but don't store them as copies
    // This avoids the PluginDescription leak
    // std::cout << "[PlayerManager] Getting internal plugin types" << std::endl;
    auto types = internalPluginFormat->getAllTypes();
    // std::cout << "[PlayerManager] Got " << types.size() << " internal plugin types" << std::endl;
    
    // Add internal plugin types to the known plugin list
    // Use the types directly from the format to avoid creating copies
    // std::cout << "[PlayerManager] Adding internal plugin types to known plugin list" << std::endl;
    for (auto& type : types)
    {
        // Add the type by reference to avoid creating a copy
        knownPluginList.addType(type);
    }
    
    // std::cout << "[PlayerManager] AudioEngine singleton ready with sample rate: " 
              // << audioEngine.getSampleRate() << "Hz, buffer size: "
              // << audioEngine.getBufferSize() << " samples" << std::endl;
    
    // std::cout << "[PlayerManager] Loading all players" << std::endl;
    loadAllPlayers();
    llmApiKey = String();

    // std::cout << "[PlayerManager] Constructor complete" << std::endl;
}

PlayerManager::~PlayerManager()
{
    try
    {
        // Log the start of cleanup with detailed address information
        // std::cout << "PlayerManager destructor: Starting cleanup of " << players.size() << " players" << std::endl;
        // std::cout << "  AudioEngine singleton in use (not owned by PlayerManager)" << std::endl;
        // std::cout << "  internalPluginFormat address: " << internalPluginFormat << std::endl;
        
        // Deactivate any active player
        // std::cout << "PlayerManager destructor: Deactivating active player" << std::endl;
        if (activePlayer != nullptr)
            deactivatePlayer(activePlayer);
        
        // Clear all players - this will call each Player's destructor
        // which in turn will clean up their Instruments
        // std::cout << "PlayerManager destructor: Clearing players array" << std::endl;
        players.clear();
        // std::cout << "PlayerManager destructor: Players cleared" << std::endl;
        
        // AudioEngine, formatManager, and knownPluginList are now owned by singleton
        // No explicit cleanup needed for these resources
        
        // We don't reset the static plugin format here, as it should persist
        // Just set our pointer to null
        // std::cout << "PlayerManager destructor: Setting internalPluginFormat to nullptr" << std::endl;
        internalPluginFormat = nullptr;
        
        // std::cout << "PlayerManager destructor: All resources cleaned up successfully" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION in PlayerManager destructor: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "UNKNOWN EXCEPTION in PlayerManager destructor" << std::endl;
    }
}
void PlayerManager::activatePlayer(Player* player)
{
    if (player == nullptr || player == activePlayer)
        return;
    
    // Deactivate current player if any
    if (activePlayer != nullptr)
        deactivatePlayer(activePlayer);
    
    // Prepare player for playback
    player->prepareForPlayback();
    
    // Connect to AudioEngine singleton
    if (!player->connectToAudioEngine(getAudioEnginePtr()))
    {
        std::cerr << "Failed to connect player to AudioEngine" << std::endl;
        player->releaseResources();
        return;
    }
    
    // Add player's graph to the main processor graph in AudioEngine
    if (auto* instrument = player->getInstrument())
    {
        // TODO: Update to use TracktionInstrument instead of PluginGraph
        // if (auto* processorGraph = instrument->getProcessorGraph())
        {
            // Note: Old PluginGraph integration disabled - now using TracktionPlayer/TracktionInstrument
            // Integrate player's graph into AudioEngine master graph
            if (!getAudioEnginePtr()->addPlayerGraph(player))
            {
                std::cerr << "Warning: Could not integrate player graph for " << player->getMetadata().name << std::endl;
            }
        }
    }
    
    // Set as active player
    activePlayer = player;
    // std::cout << "Player " << player->getMetadata().name << " activated" << std::endl;
}

void PlayerManager::deactivatePlayer(Player* player)
{
    if (player == nullptr || player != activePlayer)
        return;
    
    // Remove player's graph from AudioEngine
    if (auto* instrument = player->getInstrument())
    {
        // TODO: Update to use TracktionInstrument instead of PluginGraph
        // if (auto* processorGraph = instrument->getProcessorGraph())
        {
            // Note: Old PluginGraph removal disabled - now using TracktionPlayer/TracktionInstrument
            // Remove player's graph from AudioEngine master graph
            getAudioEnginePtr()->removePlayerGraph(player);
        }
    }
    
    // Disconnect from AudioEngine
    player->disconnectFromAudioEngine();
    
    // Release resources
    player->releaseResources();
    
    // Clear active player
    activePlayer = nullptr;
    // std::cout << "Player " << player->getMetadata().name << " deactivated" << std::endl;
}

bool PlayerManager::isPlayerActive(Player* player) const
{
    return player != nullptr && player == activePlayer;
}

Array<Player*> PlayerManager::getPlayers() const
{
    Array<Player*> result;
    
    for (auto* player : players)
    {
        result.add(player);
    }
    
    return result;
}

void PlayerManager::addPlayer(std::unique_ptr<Player> player)
{
    if (player != nullptr)
    {
        // Update the last modified time in metadata
        auto& metadata = player->getMetadata();
        metadata.lastModified = Time::getCurrentTime();
        
        // If this is a new player, set the created time
        if (metadata.created == Time())
        {
            metadata.created = metadata.lastModified;
        }
        
        // Add to the list
        players.add(player.release());
    }
}

bool PlayerManager::savePlayer(Player* player)
{
    if (player == nullptr)
    {
        return false;
    }
    
    // Update the last modified time
    auto& metadata = player->getMetadata();
    metadata.lastModified = Time::getCurrentTime();
    
    // Make sure the player is in our list
    bool playerInList = false;
    for (auto* existingPlayer : players)
    {
        if (existingPlayer == player)
        {
            playerInList = true;
            break;
        }
    }
    
    // If the player isn't in our list, add it
    if (!playerInList)
    {
        // std::cout << "Adding player to manager: " << metadata.name << std::endl;
        players.add(player);
        
        // Notify listeners that a player was added
        sendChangeMessage();
    }
    
    // Explicitly save the graph state to a file
    bool graphSaved = player->saveGraphState();
    if (graphSaved)
    {
        // std::cout << "Graph state saved successfully for player: " << metadata.name << std::endl;
    }
    
    // Convert to JSON
    var playerJson = player->toVar();
    
    // Get the file path - use player ID to avoid duplicates when renaming
    File playerFile = getPlayersDirectory().getChildFile(player->getId() + ".player");
    
    // Save to file
    String jsonStr = JSON::toString(playerJson);
    bool success = playerFile.replaceWithText(jsonStr);
    
    if (success) {
        // std::cout << "Saved player to file: " << playerFile.getFullPathName() << std::endl;
        
        // Notify listeners that a player was saved
        sendChangeMessage();
    }
    else {
        // std::cout << "Failed to save player to file: " << playerFile.getFullPathName() << std::endl;
    }
        
    return success;
}

bool PlayerManager::savePlayerAndTakeOwnership(std::unique_ptr<Player> player)
{
    if (player == nullptr)
    {
        return false;
    }
    
    // Get a raw pointer to the player before we move it
    Player* playerPtr = player.get();
    
    // Update the last modified time
    auto& metadata = playerPtr->getMetadata();
    metadata.lastModified = Time::getCurrentTime();
    
    // Explicitly save the graph state to a file
    bool graphSaved = playerPtr->saveGraphState();
    if (graphSaved)
    {
        // std::cout << "Graph state saved successfully for player: " << metadata.name << std::endl;
    }
    
    // Convert to JSON
    var playerJson = playerPtr->toVar();
    
    // Get the file path - use player ID to avoid duplicates when renaming
    File playerFile = getPlayersDirectory().getChildFile(playerPtr->getId() + ".player");
    
    // Save to file
    String jsonStr = JSON::toString(playerJson);
    bool success = playerFile.replaceWithText(jsonStr);
    
    if (success)
    {
        // std::cout << "Saved player to file: " << playerFile.getFullPathName() << std::endl;
        
        // Take ownership of the player and add it to our list
        // This will replace any existing player with the same pointer
        for (int i = 0; i < players.size(); ++i)
        {
            if (players[i] == playerPtr)
            {
                players.remove(i, false); // Remove without deleting
                break;
            }
        }
        
        // Now add the player we own
        players.add(player.release());
        // std::cout << "Player ownership transferred to PlayerManager" << std::endl;
    }
    else
    {
        // std::cout << "Failed to save player to file: " << playerFile.getFullPathName() << std::endl;
    }
    
    return success;
}

std::unique_ptr<Player> PlayerManager::createPlayer()
{
    // Generate a unique ID
    String id = String::toHexString(Random::getSystemRandom().nextInt());
    
    // Create a new player with default settings, using singleton AudioEngine
    auto* audioEngine = getAudioEnginePtr();
    auto player = std::make_unique<Player>(id, "New Player", audioEngine->getFormatManager(), audioEngine->getKnownPluginList());
    player->initialize();
    
    // Set default metadata
    auto& metadata = player->getMetadata();
    metadata.category = "Default";
    metadata.description = "A new player";
    metadata.created = Time::getCurrentTime();
    metadata.lastModified = metadata.created;
    
    // Ensure the player is fully prepared for playback
    if (!player->isReadyForPlayback())
    {
        // std::cout << "Preparing new player for playback" << std::endl;
        player->prepareForPlayback();
    }
    
    return player;
}

std::unique_ptr<Player> PlayerManager::loadPlayer(const File& file)
{
    if (!file.existsAsFile())
        return nullptr;
    
    // Read the file content
    String content = file.loadFileAsString();
    var json;
    Result result = JSON::parse(content, json);
    
    if (result.wasOk() && json.isObject())
    {
        // Use the fromVar method to create a player with the singleton AudioEngine
        auto* audioEngine = getAudioEnginePtr();
        return Player::fromVar(json, audioEngine->getFormatManager(), audioEngine->getKnownPluginList());
    }
    
    return nullptr;
}

bool PlayerManager::deletePlayer(Player* player)
{
    if (player == nullptr)
    {
        return false;
    }
    
    // Get the player file paths - check both old (.player) and new (.json) locations
    auto& metadata = player->getMetadata();
    
    // Primary location: ~/Library/Application Support/CovertTracks/Players/ (use ID)
    File playerFile = getPlayersDirectory().getChildFile(player->getId() + ".player");
    
    // Also check old Documents location (used by previous PlayerEditorComponent saves) - try both name and ID
    File documentsDir = File::getSpecialLocation(File::userDocumentsDirectory);
    File oldDocumentsDir = documentsDir.getChildFile("Covertracks").getChildFile("Players");
    File oldDocumentsFileByName = oldDocumentsDir.getChildFile(metadata.name + ".json");
    File oldDocumentsFileById = oldDocumentsDir.getChildFile(player->getId() + ".json");
    
    // Graph file location
    File graphFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                      .getChildFile("Covertracks")
                      .getChildFile("Graphs")
                      .getChildFile(player->getId() + ".filtergraph");
    
    std::cout << "Deleting player: " << metadata.name << " (ID: " << player->getId() << ")" << std::endl;
    std::cout << "Checking player file: " << playerFile.getFullPathName() << std::endl;
    std::cout << "Checking old documents file (by name): " << oldDocumentsFileByName.getFullPathName() << std::endl;
    std::cout << "Checking old documents file (by ID): " << oldDocumentsFileById.getFullPathName() << std::endl;
    std::cout << "Checking graph file: " << graphFile.getFullPathName() << std::endl;
    
    // Delete the files
    bool playerFileDeleted = true;
    if (playerFile.existsAsFile())
    {
        playerFileDeleted = playerFile.deleteFile();
        std::cout << "Player file deleted: " << (playerFileDeleted ? "yes" : "no") << std::endl;
    }
    
    bool oldDocumentsFileByNameDeleted = true;
    if (oldDocumentsFileByName.existsAsFile())
    {
        oldDocumentsFileByNameDeleted = oldDocumentsFileByName.deleteFile();
        std::cout << "Old documents file (by name) deleted: " << (oldDocumentsFileByNameDeleted ? "yes" : "no") << std::endl;
    }
    
    bool oldDocumentsFileByIdDeleted = true;
    if (oldDocumentsFileById.existsAsFile())
    {
        oldDocumentsFileByIdDeleted = oldDocumentsFileById.deleteFile();
        std::cout << "Old documents file (by ID) deleted: " << (oldDocumentsFileByIdDeleted ? "yes" : "no") << std::endl;
    }
    
    bool graphFileDeleted = true;
    if (graphFile.existsAsFile())
    {
        graphFileDeleted = graphFile.deleteFile();
        std::cout << "Graph file deleted: " << (graphFileDeleted ? "yes" : "no") << std::endl;
    }
    
    // Remove from the list
    players.removeObject(player, true);
    
    // Return true if all existing files were deleted successfully
    return playerFileDeleted && oldDocumentsFileByNameDeleted && oldDocumentsFileByIdDeleted && graphFileDeleted;
}

File PlayerManager::getPlayersDirectory()
{
    File appDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                    .getChildFile("Covertracks");
    
    // Create the directory if it doesn't exist
    appDir.createDirectory();
    
    return appDir.getChildFile("Players");
}

void PlayerManager::loadAllPlayers()
{
    // Clear existing players
    players.clear();
    
    // Get the players directory
    File playersDir = getPlayersDirectory();
    // std::cout << "Loading players with minimal resources from directory: " << playersDir.getFullPathName() << std::endl;
    
    // Create the directory if it doesn't exist
    if (!playersDir.exists())
    {
        // std::cout << "Players directory doesn't exist, creating it." << std::endl;
        playersDir.createDirectory();
        return;
    }
    
    // Load all player files
    Array<File> playerFiles;
    
    // Try both .player and .json extensions since we might have old files
    playersDir.findChildFiles(playerFiles, File::findFiles, false, "*.player");
    // std::cout << "Found " << playerFiles.size() << " .player files" << std::endl;
    
    // Also look for .json files and add them to the array
    Array<File> jsonFiles;
    playersDir.findChildFiles(jsonFiles, File::findFiles, false, "*.json");
    // std::cout << "Found " << jsonFiles.size() << " .json files" << std::endl;
    
    // Add json files to player files
    playerFiles.addArray(jsonFiles);
    
    if (playerFiles.isEmpty())
    {
        // std::cout << "No player files found in directory" << std::endl;
        return;
    }
    
    // Load each player file with minimal resources
    for (auto& file : playerFiles)
    {
        // std::cout << "Loading player with minimal resources from file: " << file.getFileName() << std::endl;
        auto player = loadPlayer(file);
        if (player != nullptr)
        {
            // std::cout << "Successfully loaded player with minimal resources: " << player->getMetadata().name << std::endl;
            addPlayer(std::move(player));
        }
        else
        {
            // std::cout << "Failed to load player from file: " << file.getFileName() << std::endl;
        }
    }
    
    // std::cout << "Loaded " << players.size() << " players with minimal resources in total" << std::endl;
}

bool PlayerManager::playPattern(Player* player, const var& pattern, double tempo)
{
    if (player == nullptr)
    {
        std::cerr << "Cannot play pattern: player is null" << std::endl;
        return false;
    }
    
    // std::cout << "===== PLAYING PATTERN ON PLAYER: " << player->getMetadata().name << " =====" << std::endl;
    
    // Make sure the player is prepared for playback
    if (!player->isReadyForPlayback())
    {
        // std::cout << "Preparing player " << player->getMetadata().name << " for playback" << std::endl;
        player->prepareForPlayback();
    }
    
    // Connect to AudioEngine if not already connected
    if (!player->isConnectedToAudioEngine())
    {
        // std::cout << "Connecting player " << player->getMetadata().name << " to AudioEngine" << std::endl;
        if (!player->connectToAudioEngine(getAudioEnginePtr()))
        {
            std::cerr << "Failed to connect player to AudioEngine" << std::endl;
            return false;
        }
    }
    
    // Ensure the player is activated in the AudioEngine
    if (player != activePlayer)
    {
        // std::cout << "Activating player " << player->getMetadata().name << " in AudioEngine" << std::endl;
        getAudioEnginePtr()->activatePlayer(player);
    }
    
    // Make sure the player's processor is registered with the AudioEngine
    if (auto* instrument = player->getInstrument())
    {
        // TODO: Update to use TracktionInstrument instead of PluginGraph
        // if (auto* processorGraph = instrument->getProcessorGraph())
        {
            // Note: Old PluginGraph registration disabled - now using TracktionPlayer/TracktionInstrument
            // std::cout << "Registering processor for player " << player->getMetadata().name << std::endl;
            // Note: registerProcessor method not available in new AudioEngine
            // Player registration handles this automatically
        }
        // else
        // {
        //     std::cerr << "Player " << player->getMetadata().name << " has no processor graph!" << std::endl;
        //     return false;
        // }
    }
    else
    {
        std::cerr << "Player " << player->getMetadata().name << " has no instrument!" << std::endl;
        return false;
    }
    
    // Load and play the chart
    // std::cout << "Loading and playing chart on player: " << player->getMetadata().name << std::endl;
    
    // Create a chart object with the pattern data
    DynamicObject* chartObj = new DynamicObject();
    chartObj->setProperty("events", pattern);
    chartObj->setProperty("tempo", tempo);
    var chartVar(chartObj);
    
    bool loaded = player->loadChart(chartVar);
    bool result = loaded ? player->play() : false;
    
    if (result)
    {
        // std::cout << "Successfully started pattern playback on player: " << player->getMetadata().name << std::endl;
    }
    else
    {
        std::cerr << "Failed to start pattern playback on player: " << player->getMetadata().name << std::endl;
    }
    
    return result;
}

bool PlayerManager::playPatternOnAllActivePlayers(const var& pattern, double tempo)
{
    // Get all players that are active or ready for playback
    Array<Player*> playersToUse;
    
    for (auto* player : players)
    {
        if (player->isReadyForPlayback() || player == activePlayer)
        {
            playersToUse.add(player);
        }
    }
    
    if (playersToUse.isEmpty())
    {
        std::cerr << "No active players to play pattern on" << std::endl;
        return false;
    }
    
    // Play the pattern on all selected players
    bool anySuccess = false;
    
    for (auto* player : playersToUse)
    {
        if (playPattern(player, pattern, tempo))
        {
            anySuccess = true;
        }
    }
    
    return anySuccess;
}

void PlayerManager::stopPlayback(Player* player)
{
    if (player != nullptr)
    {
        // Stop playback for the specified player
        // std::cout << "Stopping playback for player: " << player->getMetadata().name << std::endl;
        player->stopPlayback();
    }
    else
    {
        // Stop playback for all players
        // std::cout << "Stopping playback for all players" << std::endl;
        for (auto* p : players)
        {
            p->stopPlayback();
        }
    }
}

std::shared_ptr<LLMClient> PlayerManager::getLLMClient(const String& apiKey)
{
    // Create the LLM client if it doesn't exist or if a new API key is provided
    if (llmClient == nullptr || (apiKey.isNotEmpty() && apiKey != llmApiKey))
    {
        String keyToUse = apiKey.isNotEmpty() ? apiKey : llmApiKey;
        
        if (keyToUse.isNotEmpty())
        {
            llmClient = std::make_shared<ClaudeClient>(keyToUse);
            llmApiKey = keyToUse;
        }
    }
    
    return llmClient;
}

void PlayerManager::setLLMApiKey(const String& apiKey)
{
    if (apiKey != llmApiKey)
    {
        llmApiKey = apiKey;
        
        // Create a new client with the updated API key
        if (!llmApiKey.isEmpty())
        {
            llmClient = std::make_shared<ClaudeClient>(llmApiKey);
        }
        else
        {
            llmClient.reset();
        }
        
        // Update all players with the new LLM client
        for (auto* player : players)
        {
            player->setLLMClient(llmClient);
        }
        
        // Notify listeners of the change
        sendChangeMessage();
    }
}

void PlayerManager::generatePatternForPlayer(Player* player, 
                                          const var& parameters,
                                          std::function<void(bool success, const var& pattern)> callback)
{
    if (player == nullptr)
    {
        if (callback)
            callback(false, "No player specified");
        return;
    }
    
    // Ensure the player has a musician
    Musician* musician = player->getMusician();
    
    if (musician == nullptr)
    {
        // Create a musician for the player if it doesn't have one
        player->setMusician(std::make_unique<Musician>(player->getId(), player->getMetadata().category));
        musician = player->getMusician();
    }
    
    // Ensure the musician has an LLM client
    if (llmClient && musician)
    {
        musician->setLLMClient(llmClient);
        
        // Generate the chart
        musician->generateChart(parameters, [player, callback](bool success, const var& chart) {
            // Load the chart if generation succeeded
            if (success)
            {
                // Load the chart into the player
                bool loaded = player->loadChart(chart);
                
                // Optionally auto-play the chart
                if (loaded && chart.hasProperty("command") && chart["command"] == "play_pattern")
                {
                    player->play();
                }
            }
            
            // Call the callback
            if (callback)
                callback(success, chart);
        });
    }
    else
    {
        // No LLM client or musician
        if (callback)
            callback(false, "No LLM client available");
    }
}

void PlayerManager::generatePatternForAllPlayers(const var& parameters,
                                              std::function<void(bool success, const var& pattern)> callback)
{
    // Get all active players
    Array<Player*> activePlayers;
    
    for (auto* player : players)
    {
        if (player->isReadyForPlayback() || player == activePlayer)
        {
            activePlayers.add(player);
        }
    }
    
    if (activePlayers.isEmpty())
    {
        if (callback)
            callback(false, "No active players");
        return;
    }
    
    // Generate patterns for each player
    int playersRemaining = activePlayers.size();
    bool anySuccess = false;
    var combinedPattern;
    
    // Create an empty array to store all events
    Array<var> allEvents;
    
    for (auto* player : activePlayers)
    {
        generatePatternForPlayer(player, parameters, [&](bool success, const var& pattern) {
            if (success)
            {
                anySuccess = true;
                
                // Extract events from the pattern
                if (pattern.hasProperty("events") && pattern["events"].isArray())
                {
                    auto* events = pattern["events"].getArray();
                    for (auto& event : *events)
                    {
                        allEvents.add(event);
                    }
                }
            }
            
            // Decrement the counter
            playersRemaining--;
            
            // If all players have been processed, call the callback
            if (playersRemaining == 0)
            {
                if (anySuccess)
                {
                    // Create a combined pattern
                    DynamicObject* combinedObj = new DynamicObject();
                    combinedObj->setProperty("command", "play_pattern");
                    combinedObj->setProperty("tempo", parameters.getProperty("tempo", 120.0));
                    combinedObj->setProperty("events", var(allEvents));
                    
                    if (callback)
                        callback(true, var(combinedObj));
                }
                else
                {
                    if (callback)
                        callback(false, "Failed to generate patterns for any player");
                }
            }
        });
    }
}


} // namespace covertracks
