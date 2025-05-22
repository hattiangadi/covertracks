// PlayerManager.cpp
#include "PlayerManager.h"
#include "../../Plugins/InternalPlugins.h"

namespace covertracks {

PlayerManager::PlayerManager()
{
    std::cout << "[PlayerManager] Constructor starting..." << std::endl;
    
    // Create and own the plugin format manager
    std::cout << "[PlayerManager] Creating plugin format manager" << std::endl;
    formatManager = std::make_unique<AudioPluginFormatManager>();
    formatManager->addDefaultFormats();
    
    // Create a single internal plugin format that will be owned by the formatManager
    std::cout << "[PlayerManager] Creating internal plugin format" << std::endl;
    auto* internalFormat = new InternalPluginFormat();
    formatManager->addFormat(internalFormat); // formatManager takes ownership
    
    // Store a non-owning pointer to the internal format for our use
    internalPluginFormat = internalFormat;
    
    // Get the internal plugin types but don't store them as copies
    // This avoids the PluginDescription leak
    std::cout << "[PlayerManager] Getting internal plugin types" << std::endl;
    auto types = internalPluginFormat->getAllTypes();
    std::cout << "[PlayerManager] Got " << types.size() << " internal plugin types" << std::endl;
    
    // Create and own the plugin list
    std::cout << "[PlayerManager] Creating known plugin list" << std::endl;
    knownPluginList = std::make_unique<KnownPluginList>();
    
    // Add internal plugin types to the known plugin list
    // Use the types directly from the format to avoid creating copies
    std::cout << "[PlayerManager] Adding internal plugin types to known plugin list" << std::endl;
    for (auto& type : types)
    {
        // Add the type by reference to avoid creating a copy
        knownPluginList->addType(type);
    }
    
    // Create our own AudioEngine instance with enhanced capabilities
    std::cout << "[PlayerManager] Creating AudioEngine" << std::endl;
    audioEngine = std::make_unique<AudioEngine>(*formatManager, *knownPluginList);
    
    std::cout << "[PlayerManager] AudioEngine created with sample rate: " 
              << audioEngine->getSampleRate() << "Hz, buffer size: "
              << audioEngine->getBufferSize() << " samples" << std::endl;
    
    std::cout << "[PlayerManager] Loading all players" << std::endl;
    loadAllPlayers();
    llmApiKey = String();

    std::cout << "[PlayerManager] Constructor complete" << std::endl;
}

PlayerManager::~PlayerManager()
{
    try
    {
        // Log the start of cleanup with detailed address information
        std::cout << "PlayerManager destructor: Starting cleanup of " << players.size() << " players" << std::endl;
        std::cout << "  audioEngine address: " << audioEngine.get() << std::endl;
        std::cout << "  knownPluginList address: " << knownPluginList.get() << std::endl;
        std::cout << "  formatManager address: " << formatManager.get() << std::endl;
        std::cout << "  internalPluginFormat address: " << internalPluginFormat << std::endl;
        
        // Deactivate any active player
        std::cout << "PlayerManager destructor: Deactivating active player" << std::endl;
        if (activePlayer != nullptr)
            deactivatePlayer(activePlayer);
        
        // Clear all players - this will call each Player's destructor
        // which in turn will clean up their Instruments
        std::cout << "PlayerManager destructor: Clearing players array" << std::endl;
        players.clear();
        std::cout << "PlayerManager destructor: Players cleared" << std::endl;
        
        // Explicitly destroy each member in a controlled order with detailed logging
        std::cout << "PlayerManager destructor: Explicitly destroying audioEngine" << std::endl;
        audioEngine.reset();
        std::cout << "PlayerManager destructor: audioEngine destroyed" << std::endl;
        
        std::cout << "PlayerManager destructor: Explicitly destroying knownPluginList" << std::endl;
        knownPluginList.reset();
        std::cout << "PlayerManager destructor: knownPluginList destroyed" << std::endl;
        
        // Before destroying formatManager, log its state
        if (formatManager)
        {
            std::cout << "PlayerManager destructor: formatManager has " << formatManager->getNumFormats() << " formats" << std::endl;
            for (int i = 0; i < formatManager->getNumFormats(); ++i)
            {
                auto* format = formatManager->getFormat(i);
                std::cout << "  Format " << i << ": " << format->getName() << " (address: " << format << ")" << std::endl;
            }
        }
        
        std::cout << "PlayerManager destructor: Explicitly destroying formatManager" << std::endl;
        formatManager.reset();
        std::cout << "PlayerManager destructor: formatManager destroyed" << std::endl;
        
        // We don't reset the static plugin format here, as it should persist
        // Just set our pointer to null
        std::cout << "PlayerManager destructor: Setting internalPluginFormat to nullptr" << std::endl;
        internalPluginFormat = nullptr;
        
        std::cout << "PlayerManager destructor: All resources cleaned up successfully" << std::endl;
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
    
    // Connect to AudioEngine
    if (!player->connectToAudioEngine(audioEngine.get()))
    {
        std::cerr << "Failed to connect player to AudioEngine" << std::endl;
        player->releaseResources();
        return;
    }
    
    // Add player's graph to the main processor graph in AudioEngine
    if (auto* instrument = player->getInstrument())
    {
        if (auto* processorGraph = instrument->getProcessorGraph())
        {
            audioEngine->addPlayerGraph(*processorGraph);
        }
    }
    
    // Set as active player
    activePlayer = player;
    std::cout << "Player " << player->getMetadata().name << " activated" << std::endl;
}

void PlayerManager::deactivatePlayer(Player* player)
{
    if (player == nullptr || player != activePlayer)
        return;
    
    // Remove player's graph from AudioEngine
    if (auto* instrument = player->getInstrument())
    {
        if (auto* processorGraph = instrument->getProcessorGraph())
        {
            // Get the node ID of the player graph in the main processor graph
            auto nodeId = audioEngine->getPlayerGraphNodeId(processorGraph);
            if (nodeId.uid != 0) // Valid node ID
                audioEngine->removePlayerGraph(nodeId);
        }
    }
    
    // Disconnect from AudioEngine
    player->disconnectFromAudioEngine();
    
    // Release resources
    player->releaseResources();
    
    // Clear active player
    activePlayer = nullptr;
    std::cout << "Player " << player->getMetadata().name << " deactivated" << std::endl;
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
        std::cout << "Adding player to manager: " << metadata.name << std::endl;
        players.add(player);
        
        // Notify listeners that a player was added
        sendChangeMessage();
    }
    
    // Explicitly save the graph state to a file
    bool graphSaved = player->saveGraphState();
    if (graphSaved)
    {
        std::cout << "Graph state saved successfully for player: " << metadata.name << std::endl;
    }
    
    // Convert to JSON
    var playerJson = player->toVar();
    
    // Get the file path
    File playerFile = getPlayersDirectory().getChildFile(metadata.name + ".player");
    
    // Save to file
    String jsonStr = JSON::toString(playerJson);
    bool success = playerFile.replaceWithText(jsonStr);
    
    if (success) {
        std::cout << "Saved player to file: " << playerFile.getFullPathName() << std::endl;
        
        // Notify listeners that a player was saved
        sendChangeMessage();
    }
    else {
        std::cout << "Failed to save player to file: " << playerFile.getFullPathName() << std::endl;
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
        std::cout << "Graph state saved successfully for player: " << metadata.name << std::endl;
    }
    
    // Convert to JSON
    var playerJson = playerPtr->toVar();
    
    // Get the file path
    File playerFile = getPlayersDirectory().getChildFile(metadata.name + ".player");
    
    // Save to file
    String jsonStr = JSON::toString(playerJson);
    bool success = playerFile.replaceWithText(jsonStr);
    
    if (success)
    {
        std::cout << "Saved player to file: " << playerFile.getFullPathName() << std::endl;
        
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
        std::cout << "Player ownership transferred to PlayerManager" << std::endl;
    }
    else
    {
        std::cout << "Failed to save player to file: " << playerFile.getFullPathName() << std::endl;
    }
    
    return success;
}

std::unique_ptr<Player> PlayerManager::createPlayer()
{
    // Generate a unique ID
    String id = String::toHexString(Random::getSystemRandom().nextInt());
    
    // Create a new player with default settings, passing the AudioEngine as a raw pointer
    auto player = std::make_unique<Player>(id, "New Player", *formatManager, *knownPluginList, audioEngine.get());
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
        std::cout << "Preparing new player for playback" << std::endl;
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
        // Use the fromVar method to create a player with the shared format manager and plugin list
        return Player::fromVar(json, *formatManager, *knownPluginList, audioEngine.get());
    }
    
    return nullptr;
}

bool PlayerManager::deletePlayer(Player* player)
{
    if (player == nullptr)
    {
        return false;
    }
    
    // Get the player file path
    auto& metadata = player->getMetadata();
    File playerFile = getPlayersDirectory().getChildFile(metadata.name + ".player");
    
    // Also get the graph file path
    File graphFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                      .getChildFile("Covertracks")
                      .getChildFile("Graphs")
                      .getChildFile(player->getId() + ".filtergraph");
    
    std::cout << "Deleting player: " << metadata.name << " (ID: " << player->getId() << ")" << std::endl;
    std::cout << "Player file: " << playerFile.getFullPathName() << std::endl;
    std::cout << "Graph file: " << graphFile.getFullPathName() << std::endl;
    
    // Delete the files
    bool playerFileDeleted = true;
    if (playerFile.existsAsFile())
    {
        playerFileDeleted = playerFile.deleteFile();
        std::cout << "Player file deleted: " << (playerFileDeleted ? "yes" : "no") << std::endl;
    }
    
    bool graphFileDeleted = true;
    if (graphFile.existsAsFile())
    {
        graphFileDeleted = graphFile.deleteFile();
        std::cout << "Graph file deleted: " << (graphFileDeleted ? "yes" : "no") << std::endl;
    }
    
    // Remove from the list
    players.removeObject(player, true);
    return playerFileDeleted && graphFileDeleted;
}

File PlayerManager::getPlayersDirectory()
{
    File appDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                    .getChildFile("CovertTracks");
    
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
    std::cout << "Loading players with minimal resources from directory: " << playersDir.getFullPathName() << std::endl;
    
    // Create the directory if it doesn't exist
    if (!playersDir.exists())
    {
        std::cout << "Players directory doesn't exist, creating it." << std::endl;
        playersDir.createDirectory();
        return;
    }
    
    // Load all player files
    Array<File> playerFiles;
    
    // Try both .player and .json extensions since we might have old files
    playersDir.findChildFiles(playerFiles, File::findFiles, false, "*.player");
    std::cout << "Found " << playerFiles.size() << " .player files" << std::endl;
    
    // Also look for .json files and add them to the array
    Array<File> jsonFiles;
    playersDir.findChildFiles(jsonFiles, File::findFiles, false, "*.json");
    std::cout << "Found " << jsonFiles.size() << " .json files" << std::endl;
    
    // Add json files to player files
    playerFiles.addArray(jsonFiles);
    
    if (playerFiles.isEmpty())
    {
        std::cout << "No player files found in directory" << std::endl;
        return;
    }
    
    // Load each player file with minimal resources
    for (auto& file : playerFiles)
    {
        std::cout << "Loading player with minimal resources from file: " << file.getFileName() << std::endl;
        auto player = loadPlayer(file);
        if (player != nullptr)
        {
            std::cout << "Successfully loaded player with minimal resources: " << player->getMetadata().name << std::endl;
            addPlayer(std::move(player));
        }
        else
        {
            std::cout << "Failed to load player from file: " << file.getFileName() << std::endl;
        }
    }
    
    std::cout << "Loaded " << players.size() << " players with minimal resources in total" << std::endl;
}

bool PlayerManager::playPattern(Player* player, const var& pattern, double tempo)
{
    if (player == nullptr)
    {
        std::cerr << "Cannot play pattern: player is null" << std::endl;
        return false;
    }
    
    std::cout << "===== PLAYING PATTERN ON PLAYER: " << player->getMetadata().name << " =====" << std::endl;
    
    // Make sure the player is prepared for playback
    if (!player->isReadyForPlayback())
    {
        std::cout << "Preparing player " << player->getMetadata().name << " for playback" << std::endl;
        player->prepareForPlayback();
    }
    
    // Connect to AudioEngine if not already connected
    if (!player->isConnectedToAudioEngine())
    {
        std::cout << "Connecting player " << player->getMetadata().name << " to AudioEngine" << std::endl;
        if (!player->connectToAudioEngine(audioEngine.get()))
        {
            std::cerr << "Failed to connect player to AudioEngine" << std::endl;
            return false;
        }
    }
    
    // Ensure the player is activated in the AudioEngine
    if (player != activePlayer)
    {
        std::cout << "Activating player " << player->getMetadata().name << " in AudioEngine" << std::endl;
        audioEngine->activatePlayer(player);
    }
    
    // Make sure the player's processor is registered with the AudioEngine
    if (auto* instrument = player->getInstrument())
    {
        if (auto* processorGraph = instrument->getProcessorGraph())
        {
            std::cout << "Registering processor for player " << player->getMetadata().name << std::endl;
            audioEngine->registerProcessor(processorGraph);
        }
        else
        {
            std::cerr << "Player " << player->getMetadata().name << " has no processor graph!" << std::endl;
            return false;
        }
    }
    else
    {
        std::cerr << "Player " << player->getMetadata().name << " has no instrument!" << std::endl;
        return false;
    }
    
    // Play the pattern
    std::cout << "Playing pattern on player: " << player->getMetadata().name << std::endl;
    bool result = player->playPattern(pattern, tempo);
    
    if (result)
    {
        std::cout << "Successfully started pattern playback on player: " << player->getMetadata().name << std::endl;
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
        std::cout << "Stopping playback for player: " << player->getMetadata().name << std::endl;
        player->stopPlayback();
    }
    else
    {
        // Stop playback for all players
        std::cout << "Stopping playback for all players" << std::endl;
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
        
        // Generate the pattern
        musician->generatePattern(parameters, [player, callback](bool success, const var& pattern) {
            // Store the pattern
            if (success)
            {
                // The pattern is already stored in the musician
                
                // Play the pattern on the player
                if (pattern.hasProperty("command") && pattern["command"] == "play_pattern")
                {
                    player->playPattern(pattern.getProperty("events", var()), 
                                      pattern.getProperty("tempo", 120.0));
                }
            }
            
            // Call the callback
            if (callback)
                callback(success, pattern);
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
