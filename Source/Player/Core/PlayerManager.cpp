// PlayerManager.cpp
#include "PlayerManager.h"

namespace covertracks {

PlayerManager::PlayerManager()
{
    loadAllPlayers();
}

PlayerManager::~PlayerManager()
{
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
    
    // Convert to JSON
    var playerJson = player->toVar();
    
    // Get the file path
    File playerFile = getPlayersDirectory().getChildFile(metadata.name + ".json");
    
    // Save to file
    String jsonStr = JSON::toString(playerJson);
    return playerFile.replaceWithText(jsonStr);
}

std::unique_ptr<Player> PlayerManager::createPlayer()
{
    // Generate a unique ID
    String playerId = String::toHexString(Random::getSystemRandom().nextInt());
    
    // Create a new player with default settings
    auto player = std::make_unique<Player>(playerId, "New Player");
    
    // Initialize it
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
    {
        return nullptr;
    }
    
    // Read the file
    auto json = JSON::parse(file);
    
    // Convert to player
    return Player::fromVar(json);
}

bool PlayerManager::deletePlayer(Player* player)
{
    if (player == nullptr)
    {
        return false;
    }
    
    // Get the file path
    File playerFile = getPlayersDirectory().getChildFile(player->getMetadata().name + ".json");
    
    // Delete the file
    bool fileDeleted = true;
    if (playerFile.existsAsFile())
    {
        fileDeleted = playerFile.deleteFile();
    }
    
    // Remove from the list
    players.removeObject(player, true);
    return fileDeleted;
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
    playersDir.createDirectory();
    
    // Load all JSON files
    for (auto& file : playersDir.findChildFiles(File::findFiles, false, "*.json"))
    {
        if (auto player = loadPlayer(file))
        {
            players.add(player.release());
        }
    }
}

} // namespace covertracks
