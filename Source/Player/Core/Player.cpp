#include "Player.h"
#include <iostream>

namespace covertracks {

// PlayerMetadata implementation
var PlayerMetadata::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("description", description);
    obj->setProperty("created", created.toISO8601(true));
    obj->setProperty("lastModified", lastModified.toISO8601(true));
    
    return var(obj);
}

void PlayerMetadata::fromVar(const var& v)
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

// Player implementation
Player::Player(const String& playerId, const String& name,
               AudioPluginFormatManager& fm,
               KnownPluginList& kpl,
               AudioEngine* engine)
    : id(playerId),
      formatManager(fm),
      knownPluginList(kpl),
      audioEngine(engine)
{
    // Initialize metadata
    metadata.name = name;
    metadata.category = "Undefined";
    metadata.description = "A new player";
    metadata.created = Time::getCurrentTime();
    metadata.lastModified = metadata.created;

    std::cout << "Creating Player: " << name << " (ID: " << playerId << ")" 
              << " with " << (audioEngine != nullptr ? "shared" : "dedicated") 
              << " audio engine" << std::endl;
}

Player::~Player()
{
    // Stop playback before destruction
    stopPlayback();
    
    std::cout << "Player destructor: Cleaning up " << metadata.name << " (ID: " << id << ")" << std::endl;
    
    // Explicitly clean up the instrument first
    // This ensures the instrument is destroyed before the player
    if (instrument)
    {
        std::cout << "Player destructor: Explicitly cleaning up instrument for " << metadata.name << std::endl;
        instrument.reset();
    }
    
    std::cout << "Player destructor: Finished cleaning up " << metadata.name << " (ID: " << id << ")" << std::endl;
}

void Player::loadMinimalResources()
{
    // Just load metadata and minimal state
    if (!fullyLoaded)
    {
        // Create instrument if needed (but don't initialize)
        if (instrument == nullptr)
        {
            instrument = std::make_unique<Instrument>(metadata.name, formatManager, knownPluginList);
            
            // Set up pattern finished callback
            instrument->setPatternFinishedCallback([this]() {
                this->handlePatternFinished();
            });
        }
        
        fullyLoaded = true;
        std::cout << "Player " << metadata.name << " minimal resources loaded" << std::endl;
    }
}

void Player::prepareForEditing()
{
    // Prepare for UI editing
    if (!readyForEditing)
    {
        // Make sure minimal resources are loaded
        if (!fullyLoaded)
            loadMinimalResources();
        
        // Prepare instrument for editing
        if (instrument)
        {
            instrument->prepareForEditing();
            
            // Load the graph state if not already loaded
            if (!graphLoaded)
            {
                std::cout << "Loading graph state for editing player: " << metadata.name << std::endl;
                loadGraphStateIfNeeded();
            }
        }
        
        readyForEditing = true;
    }
}

void Player::prepareForPlayback()
{
    // Prepare for audio playback
    if (!readyForPlayback)
    {
        // Make sure minimal resources are loaded
        if (!fullyLoaded)
            loadMinimalResources();
        
        // Load the graph state if not already loaded
        if (!graphLoaded)
        {
            std::cout << "Loading graph state for playback of player: " << metadata.name << std::endl;
            loadGraphStateIfNeeded();
        }
        
        // Connect to AudioEngine if available
        if (audioEngine && !isConnectedToAudioEngine())
            connectToAudioEngine(audioEngine);
        
        // Prepare instrument for playback
        if (instrument)
            instrument->prepareForPlayback();
        
        readyForPlayback = true;
        std::cout << "Player " << metadata.name << " prepared for playback" << std::endl;
    }
}

void Player::releaseResources()
{
    // Release all heavy resources
    if (fullyLoaded)
    {
        // Disconnect from AudioEngine if connected
        if (isConnectedToAudioEngine())
            disconnectFromAudioEngine();
        
        // Release instrument resources
        if (instrument)
            instrument->releaseResources();
        
        readyForEditing = false;
        readyForPlayback = false;
        fullyLoaded = false;
        graphLoaded = false; // Mark graph as unloaded so it will be reloaded when needed
        
        std::cout << "Player " << metadata.name << " resources released" << std::endl;
    }
}

bool Player::connectToAudioEngine(AudioEngine* engine)
{
    if (engine == nullptr)
        return false;
    
    audioEngine = engine;
    
    // Connect instrument to the engine
    if (instrument && !instrument->connectToAudioEngine(engine))
        return false;
    
    // Register the processor graph with the AudioEngine to ensure MIDI messages are routed properly
    if (instrument && instrument->getProcessorGraph() != nullptr)
    {
        // Register the processor with the AudioEngine
        engine->registerProcessor(instrument->getProcessorGraph());
        std::cout << "Registered processor for player " << metadata.name << " in connectToAudioEngine" << std::endl;
    }
    
    // Activate this player in the engine
    engine->activatePlayer(this);
    
    return true;
}

void Player::disconnectFromAudioEngine()
{
    if (audioEngine)
    {
        // Deactivate this player in the engine
        audioEngine->deactivatePlayer(this);
        
        // Disconnect instrument from the engine
        if (instrument)
            instrument->disconnectFromAudioEngine();
        
        audioEngine = nullptr;
    }
}

bool Player::isConnectedToAudioEngine() const
{
    return audioEngine != nullptr && instrument && instrument->isConnectedToAudioEngine();
}

bool Player::isFullyLoaded() const
{
    return fullyLoaded;
}

bool Player::isReadyForPlayback() const
{
    return readyForPlayback;
}

bool Player::isReadyForEditing() const
{
    return readyForEditing;
}

bool Player::initializeMinimal()
{
    // Only create the instrument with minimal initialization
    if (instrument == nullptr)
    {
        instrument = std::make_unique<Instrument>(metadata.name, formatManager, knownPluginList, audioEngine);
        
        // Set up pattern finished callback
        instrument->setPatternFinishedCallback([this]() {
            this->handlePatternFinished();
        });
    }
    
    std::cout << "Player initialized with minimal resources: " << metadata.name << " (ID: " << id << ")" << std::endl;
    
    // Mark as minimally loaded
    fullyLoaded = true;
    
    return true;
}

bool Player::loadGraphStateIfNeeded()
{
    // If graph is already loaded, no need to load it again
    if (graphLoaded)
        return true;
    
    bool success = false;
    
    // Try to load from file first if we have a graph file name
    if (graphFileName.isNotEmpty())
    {
        // Get the application data directory
        File appDataDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                          .getChildFile("Covertracks")
                          .getChildFile("Graphs");
        
        File graphFile = appDataDir.getChildFile(graphFileName);
        std::cout << "Loading graph state from file: " << graphFile.getFullPathName() << std::endl;
        
        if (graphFile.existsAsFile())
        {
            // Parse the XML file
            std::unique_ptr<XmlElement> graphState = parseXML(graphFile);
            
            if (graphState != nullptr && graphState->hasTagName("FILTERGRAPH"))
            {
                // Count nodes in the graph state
                int nodeCount = 0;
                for (auto* node : graphState->getChildWithTagNameIterator("FILTER"))
                {
                    nodeCount++;
                    std::cout << "  Found node in file XML: " << node->getIntAttribute("uid") 
                              << " - " << node->getStringAttribute("name") << std::endl;
                }
                std::cout << "Graph file contains " << nodeCount << " nodes" << std::endl;
                
                std::cout << "Successfully loaded graph state from file" << std::endl;
                success = loadPluginGraph(*graphState);
            }
            else
            {
                std::cerr << "Failed to parse graph file: " << graphFile.getFullPathName() << std::endl;
            }
        }
        else
        {
            std::cerr << "Graph file not found: " << graphFile.getFullPathName() << std::endl;
        }
    }
    // Fall back to embedded graph state if file loading failed
    else if (embeddedGraphState.isNotEmpty() && !success)
    {
        std::cout << "Loading embedded graph state" << std::endl;
        
        // Convert from base64 to XML
        MemoryBlock mb;
        mb.fromBase64Encoding(embeddedGraphState);
        
        std::unique_ptr<XmlElement> graphState = parseXML(mb.toString());
        
        if (graphState != nullptr && graphState->hasTagName("FILTERGRAPH"))
        {
            // Count nodes in the graph state
            int nodeCount = 0;
            for (auto* node : graphState->getChildWithTagNameIterator("FILTER"))
            {
                nodeCount++;
                std::cout << "  Found node in embedded XML: " << node->getIntAttribute("uid") 
                          << " - " << node->getStringAttribute("name") << std::endl;
            }
            std::cout << "Embedded graph contains " << nodeCount << " nodes" << std::endl;
            
            std::cout << "Successfully loaded embedded graph state" << std::endl;
            success = loadPluginGraph(*graphState);
        }
        else
        {
            std::cerr << "Failed to parse embedded graph state" << std::endl;
        }
    }
    // Try to load from the default graph file path as a last resort
    else if (!success)
    {
        File graphFile = getGraphFilePath();
        if (graphFile.existsAsFile())
        {
            std::cout << "Trying to load graph from default path: " << graphFile.getFullPathName() << std::endl;
            
            // Parse the XML file
            std::unique_ptr<XmlElement> graphState = parseXML(graphFile);
            
            if (graphState != nullptr && graphState->hasTagName("FILTERGRAPH"))
            {
                std::cout << "Successfully loaded graph state from default path" << std::endl;
                success = loadPluginGraph(*graphState);
            }
        }
    }
    
    // Mark graph as loaded if successful
    if (success)
    {
        graphLoaded = true;
    }
    
    return success;
}

bool Player::initialize()
{
    // Create instrument if needed
    if (instrument == nullptr)
    {
        instrument = std::make_unique<Instrument>(metadata.name, formatManager, knownPluginList, audioEngine);
        
        // Set up pattern finished callback
        instrument->setPatternFinishedCallback([this]() {
            this->handlePatternFinished();
        });
    }
    
    // Prepare the instrument for editing
    instrument->prepareForEditing();
    bool instrumentInitialized = true; // Assume initialization succeeded
    
    // Load the graph state if needed
    loadGraphStateIfNeeded();
    
    // Temporarily commented out Musician initialization for compilation
    /*
    // Create musician if needed
    if (musician == nullptr)
    {
        musician = std::make_unique<Musician>(id, metadata.category);
    }
    
    // Set up the musician to use the instrument for playback
    if (musician != nullptr && instrument != nullptr)
    {
        // Create a function that forwards play commands to the instrument
        musician->setPlayerInterface([this](const std::string& jsonCommand) -> std::string {
            try {
                // Parse the JSON command
                json command = json::parse(jsonCommand);
                
                // Check the command type
                if (command.contains("command"))
                {
                    std::string commandType = command["command"];
                    
                    if (commandType == "play_pattern")
                    {
                        // Extract parameters
                        json events = command["events"];
                        double tempo = 120.0;
                        
                        if (command.contains("tempo") && command["tempo"].is_number())
                        {
                            tempo = command["tempo"];
                        }
                        
                        // Play the pattern on the instrument
                        bool success = instrument->playMIDIPattern(events, tempo);
                        
                        if (success)
                        {
                            return "{\"status\": \"success\", \"message\": \"Pattern playback started\"}";
                        }
                        else
                        {
                            return "{\"status\": \"error\", \"message\": \"Failed to start pattern playback\"}";
                        }
                    }
                    else if (commandType == "stop_player")
                    {
                        // Stop playback
                        instrument->stopPlayback();
                        return "{\"status\": \"success\", \"message\": \"Playback stopped\"}";
                    }
                    else if (commandType == "create_player")
                    {
                        // Player is already created, just acknowledge
                        return "{\"status\": \"success\", \"message\": \"Player already exists\"}";
                    }
                    else
                    {
                        return "{\"status\": \"error\", \"message\": \"Unknown command\"}";
                    }
                }
                else
                {
                    return "{\"status\": \"error\", \"message\": \"Missing command field\"}";
                }
            }
            catch (const json::exception& e)
            {
                return "{\"status\": \"error\", \"message\": \"Invalid JSON format: " + std::string(e.what()) + "\"}";
            }
            catch (const std::exception& e)
            {
                return "{\"status\": \"error\", \"message\": \"" + std::string(e.what()) + "\"}";
            }
        });
    }
    */
    
    std::cout << "Player fully initialized: " << metadata.name << " (ID: " << id << ")" << std::endl;
    
    return instrumentInitialized;
}

bool Player::loadPluginGraph(const XmlElement& graphState)
{
    try
    {
        if (instrument == nullptr)
        {
            std::cerr << "Cannot load plugin graph: instrument not initialized" << std::endl;
            return false;
        }
        
        // Check if the XML is valid
        if (!graphState.hasTagName("FILTERGRAPH"))
        {
            std::cerr << "Invalid plugin graph XML for player " << id << ": wrong root tag" << std::endl;
            return false;
        }
        
        // Debug: Count nodes in the graph state
        int nodeCount = 0;
        for (auto* node : graphState.getChildWithTagNameIterator("FILTER"))
        {
            nodeCount++;
            std::cout << "  Found node in XML: " << node->getIntAttribute("uid") 
                      << " - " << node->getStringAttribute("name") << std::endl;
        }
        std::cout << "Player " << metadata.name << " loading graph with " << nodeCount << " nodes" << std::endl;
        
        // Count connections before loading
        int midiConnections = 0;
        int audioConnections = 0;
        
        for (auto* e : graphState.getChildWithTagNameIterator ("CONNECTION"))
        {
            auto srcChannel = e->getIntAttribute ("srcChannel");
            auto dstChannel = e->getIntAttribute ("dstChannel");
            
            // Get connection type if available
            String type = e->hasAttribute("type") ? e->getStringAttribute("type") : "unknown";
            bool isMidi = (type == "midi") || srcChannel == 4096 || dstChannel == 4096;
            
            if (isMidi)
                midiConnections++;
            else
                audioConnections++;
        }
        
        std::cout << "Player " << metadata.name << " loading graph with " 
                  << (midiConnections + audioConnections) << " connections "
                  << "(MIDI: " << midiConnections << ", Audio: " << audioConnections << ")" << std::endl;
        
        // Load the graph
        bool result = instrument->loadPluginGraph(graphState);
        
        if (result)
        {
            std::cout << "Graph loaded successfully for player " << metadata.name << std::endl;
            
            // Save the graph state to a file
            saveGraphState();
        }
        else
            std::cerr << "Failed to load graph for player " << metadata.name << std::endl;
            
        return result;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in loadPluginGraph: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr << "Unknown exception in loadPluginGraph" << std::endl;
        return false;
    }
}

bool Player::saveGraphState()
{
    if (instrument == nullptr)
    {
        std::cerr << "Cannot save graph state: instrument not initialized" << std::endl;
        return false;
    }
    
    auto graphState = instrument->getPluginGraphState();
    if (graphState == nullptr)
    {
        std::cerr << "Cannot save graph state: no graph state available" << std::endl;
        return false;
    }
    
    return savePluginGraphToFile(*graphState);
}

bool Player::savePluginGraphToFile(const XmlElement& graphState)
{
    try
    {
        // Save the graph state to a file
        File graphFile = getGraphFilePath();
        
        // Make sure the directory exists
        graphFile.getParentDirectory().createDirectory();
        
        // Write the XML to the file
        FileOutputStream fos(graphFile);
        if (fos.openedOk())
        {
            graphState.writeTo(fos);
            fos.flush();
            
            std::cout << "Saved graph state to file: " << graphFile.getFullPathName() << std::endl;
            
            // Also update the instrument's internal state with this graph
            if (instrument != nullptr)
            {
                std::cout << "Updating instrument with the saved graph state" << std::endl;
                instrument->loadPluginGraph(graphState);
            }
            
            return true;
        }
        else
        {
            std::cerr << "Failed to open graph file for writing: " << graphFile.getFullPathName() << std::endl;
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception when saving graph state: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cerr << "Unknown exception when saving graph state" << std::endl;
        return false;
    }
}

std::unique_ptr<XmlElement> Player::getPluginGraphState()
{
    if (instrument == nullptr)
    {
        std::cout << "Instrument is null, trying to load graph from file" << std::endl;
        return loadGraphStateFromFile();
    }
    
    // First try to get the state from the instrument
    auto graphState = instrument->getPluginGraphState();
    
    // If the instrument has no graph state, try to load from file
    if (graphState == nullptr || !graphState->hasTagName("FILTERGRAPH") || 
        graphState->getNumChildElements() == 0)
    {
        std::cout << "Instrument has no valid graph state, trying to load from file" << std::endl;
        return loadGraphStateFromFile();
    }
    
    return graphState;
}

std::unique_ptr<XmlElement> Player::loadGraphStateFromFile()
{
    // Try to load the graph state from file
    File graphFile = getGraphFilePath();
    
    if (graphFile.existsAsFile())
    {
        std::cout << "Loading graph state from file: " << graphFile.getFullPathName() << std::endl;
        
        // Parse the XML file
        std::unique_ptr<XmlElement> graphState = parseXML(graphFile);
        
        if (graphState != nullptr && graphState->hasTagName("FILTERGRAPH"))
        {
            // Count nodes in the graph state
            int nodeCount = 0;
            for (auto* node : graphState->getChildWithTagNameIterator("FILTER"))
            {
                nodeCount++;
                std::cout << "  Found node in file XML: " << node->getIntAttribute("uid") 
                          << " - " << node->getStringAttribute("name") << std::endl;
            }
            std::cout << "Graph file contains " << nodeCount << " nodes" << std::endl;
            
            // Count connections
            int midiConnections = 0;
            int audioConnections = 0;
            
            for (auto* e : graphState->getChildWithTagNameIterator("CONNECTION"))
            {
                auto srcChannel = e->getIntAttribute("srcChannel");
                bool isMidi = (srcChannel == 4096);
                
                if (isMidi)
                    midiConnections++;
                else
                    audioConnections++;
            }
            
            std::cout << "Graph file contains " << (midiConnections + audioConnections) 
                      << " connections (MIDI: " << midiConnections 
                      << ", Audio: " << audioConnections << ")" << std::endl;
            
            // If we have an instrument, update it with this graph state
            if (instrument != nullptr)
            {
                std::cout << "Updating instrument with loaded graph state" << std::endl;
                instrument->loadPluginGraph(*graphState);
            }
            
            return graphState;
        }
        else
        {
            std::cerr << "Failed to parse graph file or invalid format: " 
                      << graphFile.getFullPathName() << std::endl;
        }
    }
    else
    {
        std::cerr << "Graph file not found: " << graphFile.getFullPathName() << std::endl;
    }
    
    return nullptr;
}

// Temporarily commented out for compilation
/*
void Player::setLLMClient(std::shared_ptr<LLMClient> client)
{
    if (musician != nullptr)
    {
        musician->setLLMClient(client);
    }
}
*/

// Updated to use var instead of json and stubbed implementation
var Player::generatePattern(const var& /*parameters*/)
{
    // Create a simple empty pattern for testing
    DynamicObject* errorObj = new DynamicObject();
    errorObj->setProperty("error", "Musician functionality is stubbed out");
    
    // Store the last generated pattern
    lastGeneratedPattern = var(errorObj);
    
    return lastGeneratedPattern;
}

bool Player::playPattern(const var& pattern, double tempo)
{
    if (instrument == nullptr)
    {
        std::cerr << "Cannot play pattern: instrument not initialized" << std::endl;
        return false;
    }
    
    // Extract the events array if the pattern is a full command
    var eventsToPlay = pattern;
    
    if (pattern.hasProperty("command") && pattern["command"].toString() == "play_pattern" &&
        pattern.hasProperty("events"))
    {
        eventsToPlay = pattern["events"];
    }
    
    // Use tempo from pattern if available
    double tempoToUse = tempo;
    if (pattern.hasProperty("tempo") && pattern["tempo"].isDouble())
    {
        tempoToUse = static_cast<double>(pattern["tempo"]);
    }
    
    // Play the pattern using the instrument
    return instrument->playMIDIPattern(eventsToPlay, tempoToUse);
}

bool Player::generateAndPlay(const var& parameters)
{
    // Generate the pattern
    var pattern = generatePattern(parameters);
    
    // Check for errors
    if (pattern.hasProperty("error"))
    {
        return false;
    }
    
    // Extract tempo from parameters
    double tempo = 120.0;
    if (parameters.hasProperty("tempo") && parameters["tempo"].isDouble())
    {
        tempo = static_cast<double>(parameters["tempo"]);
    }
    
    // Play the pattern
    return playPattern(pattern, tempo);
}

void Player::stopPlayback()
{
    if (instrument != nullptr)
    {
        instrument->stopPlayback();
    }
}

void Player::setPatternFinishedCallback(std::function<void(const String&)> callback)
{
    onPatternFinished = std::move(callback);
}

void Player::handlePatternFinished()
{
    // Call the callback if set
    if (onPatternFinished)
    {
        onPatternFinished(id);
    }
}

var Player::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    // Add ID to ensure it's preserved
    obj->setProperty("id", id);
    
    // Add metadata
    obj->setProperty("metadata", metadata.toVar());
    
    // Add plugin graph state if available
    if (instrument != nullptr)
    {
        auto graphState = instrument->getPluginGraphState();
        if (graphState != nullptr)
        {
            try
{
            // Save the graph state to a separate file in the same directory as player files
            File graphFile = getGraphFilePath();
            graphFile.deleteFile();
            
            // Make sure the directory exists
            graphFile.getParentDirectory().createDirectory();
            
            // Write the XML to the file
            FileOutputStream fos(graphFile);
            if (fos.openedOk())
            {
                graphState->writeTo(fos);
                fos.flush();
                
                // Store the filename in the player data
                obj->setProperty("graphFile", graphFile.getFileName());
                
                // ALSO embed the graph state in the player data as a backup
                // This ensures we always have a graph state even if the file reference is lost
                MemoryOutputStream mo;
                graphState->writeTo(mo, {});
                obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
                
                std::cout << "Saved graph state to file AND embedded in player data: " << graphFile.getFullPathName() << std::endl;
            }
            else
            {
                std::cerr << "Failed to open graph file for writing: " << graphFile.getFullPathName() << std::endl;
                
                // Fall back to embedding the graph state in the player data
                MemoryOutputStream mo;
                graphState->writeTo(mo, {});
                obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
                std::cout << "Embedded graph state in player data as fallback" << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception when serializing graph state: " << e.what() << std::endl;
            
            // Create an empty graph state to avoid crashes
            auto emptyGraph = std::make_unique<XmlElement>("FILTERGRAPH");
            MemoryOutputStream mo;
            emptyGraph->writeTo(mo, {});
            obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
        }
    }
    else
    {
        // Create an empty graph state to avoid crashes
        auto emptyGraph = std::make_unique<XmlElement>("FILTERGRAPH");
        MemoryOutputStream mo;
        emptyGraph->writeTo(mo, {});
        obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
    }
}
    
    // Add musician state if available
    // This is a placeholder for future implementation
    // obj->setProperty("musicianState", ...);
    
    return var(obj);
}

std::unique_ptr<Player> Player::fromVar(const var& v, AudioPluginFormatManager& fm, KnownPluginList& kpl, AudioEngine* engine)
{
    if (v.isObject())
    {
        auto* obj = v.getDynamicObject();
        
        if (obj == nullptr)
            return nullptr;
        
        // Extract player ID
        String playerId;
        if (obj->hasProperty("id"))
        {
            playerId = obj->getProperty("id").toString();
        }
        else
        {
            // Generate a random ID if none is provided
            playerId = String::toHexString(Random::getSystemRandom().nextInt());
        }
        
        // Extract player name
        String name = "Unnamed Player";
        if (obj->hasProperty("name"))
        {
            name = obj->getProperty("name").toString();
        }
        
        // Create the player with the provided format manager and plugin list
        auto player = std::make_unique<Player>(playerId, name, fm, kpl, engine);
        
        // Load metadata
        if (obj->hasProperty("metadata"))
            player->metadata.fromVar(obj->getProperty("metadata"));
        
        // Only initialize the player with minimal resources at startup
        player->initializeMinimal();
        
        // Store graph file information for later loading
        if (obj->hasProperty("graphFile"))
        {
            String graphFileName = obj->getProperty("graphFile").toString();
            std::cout << "Player has graphFile property: " << graphFileName << std::endl;
            
            // Store the graph file name for later loading
            player->setGraphFileName(graphFileName);
        }
        // Store embedded graph state for later loading
        else if (obj->hasProperty("pluginGraph"))
        {
            std::cout << "Player has pluginGraph property, storing for later loading" << std::endl;
            
            // Store the embedded graph state for later loading
            player->setEmbeddedGraphState(obj->getProperty("pluginGraph").toString());
        }
        else
        {
            std::cout << "Player has no graph state (neither graphFile nor pluginGraph property)" << std::endl;
        }
        
        return player;
    }
    
    // If parsing failed, return a default player
    return std::make_unique<Player>(String::toHexString(Random::getSystemRandom().nextInt()), 
                                   "Default Player", fm, kpl, engine);
}

} // namespace covertracks