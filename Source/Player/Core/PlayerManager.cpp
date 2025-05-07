// PlayerManager.cpp
#include "PlayerManager.h"
#include "../../UI/MainHostWindow.h"  // For getAppProperties

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
    
    // Store the graph file path if it exists
    if (graphFilePath.existsAsFile())
    {
        obj->setProperty("graphFilePath", graphFilePath.getFullPathName());
    }
    
    return var(obj);
}

std::unique_ptr<PlayerManager::Player> PlayerManager::Player::fromVar(const var& v)
{
    auto player = std::make_unique<Player>();
    
    if (auto* obj = v.getDynamicObject())
    {
        player->metadata.fromVar(obj->getProperty("metadata"));
        
        // Restore graph file path if it was saved
        String graphFilePath = obj->getProperty("graphFilePath").toString();
        if (graphFilePath.isNotEmpty())
        {
            File file(graphFilePath);
            if (file.existsAsFile())
            {
                player->graphFilePath = file;
                // Load the graph file
                player->loadGraphFromFile(file);
            }
        }
    }
    
    return player;
}

bool PlayerManager::Player::loadGraphFromFile(const File& file)
{
    if (!file.existsAsFile())
        return false;
        
    // Load the XML file
    std::unique_ptr<XmlElement> xml = parseXML(file);
    if (xml == nullptr)
        return false;
        
    // Store the graph state
    graphState = std::move(xml);
    
    // Update the file path
    graphFilePath = file;
    
    return true;
}

bool PlayerManager::Player::saveGraphToFile()
{
    if (graphFilePath == File() || !graphState)
        return false;
        
    return saveGraphToFile(graphFilePath);
}

bool PlayerManager::Player::saveGraphToFile(const File& file)
{
    if (!graphState)
        return false;
        
    // Save the XML to the file using MemoryBlock for consistency with other parts of the app
    MemoryBlock block;
    MemoryOutputStream stream(block, false);
    graphState->writeTo(stream);
    
    if (file.replaceWithData(block.getData(), block.getSize()))
    {
        // Update the file path
        graphFilePath = file;
        return true;
    }
    
    return false;
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
    
    // Generate a filename from the player name if we don't have one yet
    File playerFile;
    String safeName = player->metadata.name.replaceCharacters(" ", "_");
    playerFile = getPlayersDirectory().getChildFile(safeName + ".player");
    
    // Save the JSON to file
    auto jsonString = JSON::toString(json, true);
    return playerFile.replaceWithText(jsonString);
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
    
    auto player = Player::fromVar(json);
    
    // Store the file path so we can save to the same location later
    if (player != nullptr)
        player->graphFilePath = file;
    
    return player;
}

bool PlayerManager::deletePlayer(Player* player)
{
    if (player == nullptr)
        return false;
    
    // Generate the player file name based on the player name
    String safeName = player->metadata.name.replaceCharacters(" ", "_");
    File playerFile = getPlayersDirectory().getChildFile(safeName + ".player");
    
    // Delete the player file if it exists
    bool fileDeleted = true;
    if (playerFile.existsAsFile())
        fileDeleted = playerFile.deleteFile();
    
    // Note: We do NOT delete the graph file as it might be used by other players
    
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

std::unique_ptr<PlayerManager::Player> PlayerManager::promptForPlayerFile()
{
    // Since we can't use modal dialogs in newer JUCE versions without enabling JUCE_MODAL_LOOPS_PERMITTED,
    // we need to implement this differently
    // In a real implementation, you would use launchAsync with a callback
    
    // For now, just look in the default players directory and load the first player found
    Array<File> playerFiles;
    getPlayersDirectory().findChildFiles(playerFiles, File::findFiles, false, "*.player");
    
    if (!playerFiles.isEmpty())
        return loadPlayer(playerFiles.getFirst());
    
    return nullptr;
}

void PlayerManager::loadAllPlayers()
{
    // Clear existing players
    players.clear();
    
    // We'll still check the legacy directory for backward compatibility
    Array<File> playerFiles;
    getPlayersDirectory().findChildFiles(playerFiles, File::findFiles, false, "*.player");
    
    // Load each player
    for (auto& file : playerFiles)
    {
        if (auto player = loadPlayer(file))
            players.add(player.release());
    }
    
    // In the future, we could maintain a list of recently used player files
    // in the application settings and load those instead
}

bool PlayerManager::setPlayersDirectory(const File& directory)
{
    if (directory.isDirectory() || directory.createDirectory())
    {
        customPlayersDirectory = directory;
        
        // Save the custom directory path in application settings
        if (auto* props = getAppProperties().getUserSettings())
            props->setValue("customPlayersDirectory", directory.getFullPathName());
            
        return true;
    }
    return false;
}

File PlayerManager::getUserSelectedPlayersDirectory()
{
    // Check if we have a custom directory
    if (customPlayersDirectory.isDirectory())
        return customPlayersDirectory;
        
    // Check if one is saved in settings
    if (auto* props = getAppProperties().getUserSettings())
    {
        String path = props->getValue("customPlayersDirectory");
        if (path.isNotEmpty())
        {
            File dir(path);
            if (dir.isDirectory())
                return dir;
        }
    }
    
    // Fall back to default
    return getPlayersDirectory();
}

File PlayerManager::promptForPlayersDirectory()
{
    // Since we can't use modal dialogs in newer JUCE versions without enabling JUCE_MODAL_LOOPS_PERMITTED,
    // we'll just return the default directory
    // In a real implementation, you would use launchAsync with a callback
    
    return getPlayersDirectory();
}