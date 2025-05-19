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
    
    // Add the internal plugin format
    std::cout << "[PlayerManager] Adding internal plugin format" << std::endl;
    internalPluginFormat = std::make_unique<InternalPluginFormat>();
    formatManager->addFormat(internalPluginFormat.get());
    
    // Store internal plugin types for later use
    std::cout << "[PlayerManager] Storing internal plugin types" << std::endl;
    internalTypes = internalPluginFormat->getAllTypes();
    std::cout << "[PlayerManager] Got " << internalTypes.size() << " internal plugin types" << std::endl;
    
    // Create and own the plugin list
    std::cout << "[PlayerManager] Creating known plugin list" << std::endl;
    knownPluginList = std::make_unique<KnownPluginList>();
    
    // Add internal plugin types to the known plugin list
    std::cout << "[PlayerManager] Adding internal plugin types to known plugin list" << std::endl;
    for (auto& type : internalTypes)
        knownPluginList->addType(type);
    
    // Create our own AudioEngine instance with enhanced capabilities
    std::cout << "[PlayerManager] Creating AudioEngine" << std::endl;
    audioEngine = std::make_unique<AudioEngine>(*formatManager, *knownPluginList);
    
    std::cout << "[PlayerManager] AudioEngine created with sample rate: " 
              << audioEngine->getSampleRate() << "Hz, buffer size: "
              << audioEngine->getBufferSize() << " samples" << std::endl;
    
    std::cout << "[PlayerManager] Loading all players" << std::endl;
    loadAllPlayers();
    std::cout << "[PlayerManager] Constructor complete" << std::endl;
}

PlayerManager::~PlayerManager()
{
    // Log the start of cleanup
    std::cout << "PlayerManager destructor: Starting cleanup of " << players.size() << " players" << std::endl;
    
    // Deactivate any active player
    if (activePlayer != nullptr)
        deactivatePlayer(activePlayer);
    
    // Clear all players - this will call each Player's destructor
    // which in turn will clean up their Instruments
    players.clear();
    
    // The AudioEngine will be destroyed automatically
    std::cout << "PlayerManager destructor: All players cleaned up" << std::endl;
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
    
    if (success)
        std::cout << "Saved player to file: " << playerFile.getFullPathName() << std::endl;
    else
        std::cout << "Failed to save player to file: " << playerFile.getFullPathName() << std::endl;
        
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
    std::cout << "Loading players from directory: " << playersDir.getFullPathName() << std::endl;
    
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
    
    // Load each player file
    for (auto& file : playerFiles)
    {
        std::cout << "Loading player from file: " << file.getFileName() << std::endl;
        auto player = loadPlayer(file);
        if (player != nullptr)
        {
            std::cout << "Successfully loaded player: " << player->getMetadata().name << std::endl;
            addPlayer(std::move(player));
        }
        else
        {
            std::cout << "Failed to load player from file: " << file.getFileName() << std::endl;
        }
    }
    
    std::cout << "Loaded " << players.size() << " players in total" << std::endl;
}

} // namespace covertracks
