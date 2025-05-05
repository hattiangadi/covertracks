// PlayerManager.cpp
#include "PlayerManager.h"

PlayerManager::PlayerManager()
{
    loadAllPlayers();
}

PlayerManager::~PlayerManager()
{
}

var PlayerManager::PlayerCapabilities::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    // Create pitch range object
    DynamicObject* pitchRangeObj = new DynamicObject();
    pitchRangeObj->setProperty("low", pitchRange.getStart());
    pitchRangeObj->setProperty("high", pitchRange.getEnd());
    
    obj->setProperty("pitchRange", var(pitchRangeObj));
    
    // Add techniques as array
    Array<var> techniquesArray;
    for (auto& technique : techniques)
        techniquesArray.add(technique);
    
    obj->setProperty("techniques", var(techniquesArray));
    
    return var(obj);
}

void PlayerManager::PlayerCapabilities::fromVar(const var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        // Parse pitch range
        if (auto* rangeObj = obj->getProperty("pitchRange").getDynamicObject())
        {
            int low = rangeObj->getProperty("low");
            int high = rangeObj->getProperty("high");
            pitchRange = Range<int>(low, high);
        }
        
        // Parse techniques
        techniques.clear();
        if (auto* techniquesArray = obj->getProperty("techniques").getArray())
        {
            for (auto& technique : *techniquesArray)
                techniques.add(technique.toString());
        }
    }
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

var PlayerManager::Player::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("metadata", metadata.toVar());
    obj->setProperty("capabilities", capabilities.toVar());
    
    if (graphState != nullptr)
    {
        MemoryOutputStream mo;
        graphState->writeToStream(mo, {});
        
        // Convert graph state XML to base64 for JSON storage
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
        player->capabilities.fromVar(obj->getProperty("capabilities"));
        
        auto graphBase64 = obj->getProperty("pluginGraph").toString();
        if (graphBase64.isNotEmpty())
        {
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
    
    // Set default capabilities
    player->capabilities.pitchRange = {36, 96}; // MIDI range C2-C8
    player->capabilities.techniques.add("legato");
    
    // Create an empty plugin graph state
    player->graphState = std::make_unique<XmlElement>("FILTERGRAPH");
    
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