// PlayerManager.cpp
#include "PlayerManager.h"

namespace covertracks {

PlayerManager::PlayerManager(AudioPluginFormatManager& fm,
                           KnownPluginList& kpl,
                           AudioDeviceManager& deviceManager)
    : formatManager(fm),
      knownPluginList(kpl)
{
    // Create our own AudioEngine instance
    audioEngine = std::make_unique<AudioEngine>(deviceManager, formatManager, knownPluginList);
    
    std::cout << "Creating PlayerManager with owned AudioEngine" << std::endl;
    std::cout << "PlayerManager using AudioEngine with sample rate: " 
              << audioEngine->getSampleRate() << "Hz, buffer size: "
              << audioEngine->getBufferSize() << " samples" << std::endl;
    
    loadAllPlayers();
}

PlayerManager::~PlayerManager()
{
    // Log the start of cleanup
    std::cout << "PlayerManager destructor: Starting cleanup of " << players.size() << " players" << std::endl;
    
    // Clear all players - this will call each Player's destructor
    // which in turn will clean up their Instruments
    players.clear();
    
    std::cout << "PlayerManager destructor: All players cleaned up" << std::endl;
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
    auto player = std::make_unique<Player>(id, "New Player", formatManager, knownPluginList, audioEngine.get());
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
        return Player::fromVar(json, formatManager, knownPluginList, audioEngine.get());
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
