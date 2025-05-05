// PlayerManager.cpp
#include "PlayerManager.h"

PlayerManager::PlayerManager()
{
    loadAllPlayers();
}

PlayerManager::~PlayerManager()
{
}


var PlayerManager::PlayerMetadata::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("description", description);
    obj->setProperty("created", created.toISO8601(true));
    obj->setProperty("lastModified", lastModified.toISO8601(true));
    
    return var(obj);
}

// PlayerCapabilities implementation has been removed

void PlayerManager::PlayerMetadata::fromVar(const var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        name = obj->getProperty("name").toString();
        category = obj->getProperty("category").toString();
        description = obj->getProperty("description").toString();
        
        created = Time::fromISO8601(obj->getProperty("created").toString());
        lastModified = Time::fromISO8601(obj->getProperty("lastModified").toString());
    }
}

// PlayerManager.cpp - Implement player save and load with proper graph state handling
// Update toVar and fromVar methods

var PlayerManager::Player::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("metadata", metadata.toVar());
    
    if (graphState != nullptr)
    {
        // Convert the graph state (which includes plugin states) to base64
        MemoryOutputStream mo;
        graphState->writeTo(mo, {});
        obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
    }
    
    return var(obj);
}

std::unique_ptr<PlayerManager::Player> PlayerManager::Player::fromVar(const var& v)
{
    auto player = std::make_unique<Player>();
    
    if (auto* obj = v.getDynamicObject())
    {
        player->metadata.fromVar(obj->getProperty("metadata"));
        
        auto graphBase64 = obj->getProperty("pluginGraph").toString();
        if (graphBase64.isNotEmpty())
        {
            // Decode the graph state
            MemoryBlock mb;
            mb.fromBase64Encoding(graphBase64);
            player->graphState = parseXML(mb.toString());
        }
    }
    
    return player;
}



Array<PlayerManager::Player*> PlayerManager::getPlayers() const
{
    Array<Player*> result;
    for (auto* player : players)
        result.add(player);
    
    return result;
}

void PlayerManager::addPlayer(std::unique_ptr<Player> player)
{
    // Check if the player already exists in the collection
    bool found = false;
    for (auto* existingPlayer : players)
    {
        if (existingPlayer->metadata.name == player->metadata.name)
        {
            // Update existing player
            existingPlayer->metadata = player->metadata;
            
            if (player->graphState != nullptr)
                existingPlayer->graphState = std::make_unique<XmlElement>(*player->graphState);
            else
                existingPlayer->graphState = nullptr;
            
            found = true;
            break;
        }
    }
    
    // If not found, add it to the collection
    if (!found)
    {
        players.add(player.release());
    }
}

bool PlayerManager::savePlayer(Player* player)
{
    if (player == nullptr)
        return false;
    
    // Update last modified timestamp
    player->metadata.lastModified = Time::getCurrentTime();
    
    // Create JSON object
    var json = player->toVar();
    
    // Generate filename from player name
    String safeName = player->metadata.name.replaceCharacters(" ", "_");
    File file = getPlayersDirectory().getChildFile(safeName + ".player");
    
    // Save the JSON to file
    auto jsonString = JSON::toString(json, true);
    return file.replaceWithText(jsonString);
}

std::unique_ptr<PlayerManager::Player> PlayerManager::createPlayer()
{
    auto player = std::make_unique<Player>();
    
    // Set default values
    player->metadata.name = "New Player";
    player->metadata.category = "Undefined";
    player->metadata.description = "A new player configuration";
    player->metadata.created = Time::getCurrentTime();
    player->metadata.lastModified = player->metadata.created;
    
    // Create a properly structured empty plugin graph state
    auto* filtergraph = new XmlElement("FILTERGRAPH");
    
    // Add necessary attributes for a valid empty graph
    filtergraph->setAttribute("name", "New Graph");
    
    // Add connections element (empty)
    auto* connections = new XmlElement("CONNECTIONS");
    filtergraph->addChildElement(connections);
    
    // Add inputs element
    auto* inputs = new XmlElement("INPUTS");
    filtergraph->addChildElement(inputs);
    
    // Add outputs element
    auto* outputs = new XmlElement("OUTPUTS");
    filtergraph->addChildElement(outputs);
    
    player->graphState = std::unique_ptr<XmlElement>(filtergraph);
    
    return player;
}

std::unique_ptr<PlayerManager::Player> PlayerManager::loadPlayer(const File& file)
{
    if (!file.existsAsFile())
        return nullptr;
    
    auto jsonString = file.loadFileAsString();
    auto json = JSON::parse(jsonString);
    
    return Player::fromVar(json);
}

bool PlayerManager::deletePlayer(Player* player)
{
    if (player == nullptr)
        return false;
    
    // Find the player file
    String safeName = player->metadata.name.replaceCharacters(" ", "_");
    File file = getPlayersDirectory().getChildFile(safeName + ".player");
    
    // Delete the file
    bool fileDeleted = !file.existsAsFile() || file.deleteFile();
    
    // Remove from the array (if it belongs to this manager)
    for (int i = 0; i < players.size(); ++i)
    {
        if (players[i] == player)
        {
            players.remove(i);
            return fileDeleted;
        }
    }
    
    return fileDeleted;
}

File PlayerManager::getPlayersDirectory()
{
    // Get application data directory
    File appDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                      .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                      .getChildFile("Players");
    
    // Create directory if needed
    if (!appDir.exists())
        appDir.createDirectory();
    
    return appDir;
}

void PlayerManager::loadAllPlayers()
{
    // Clear existing players
    players.clear();
    
    // Get all player files
    Array<File> playerFiles;
    getPlayersDirectory().findChildFiles(playerFiles, File::findFiles, false, "*.player");
    
    // Load each player
    for (auto& file : playerFiles)
    {
        if (auto player = loadPlayer(file))
            players.add(player.release());
    }
}