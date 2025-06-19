#include "Player.h"
#include <iostream>

namespace covertracks {

// InstrumentCapabilities implementation
int InstrumentCapabilities::findNote(const String& noteName) const
{
    if (type == InstrumentCapabilityType::Mapping)
    {
        // For drums, look up the note name in the sound mappings
        auto it = soundMappings.find(noteName);
        if (it != soundMappings.end())
            return it->second;
            
        // Also try some common aliases
        static std::map<String, String> aliases = {
            {"bassDrum", "kick"},
            {"rideCymbal", "ride"},
            {"hiHat", "hihat"},
            {"closedHiHat", "hihat"},
            {"openHiHat", "openHihat"},
            {"crashCymbal", "crash"}
            // Add more aliases as needed
        };
        
        auto aliasIt = aliases.find(noteName);
        if (aliasIt != aliases.end())
        {
            auto mappingIt = soundMappings.find(aliasIt->second);
            if (mappingIt != soundMappings.end())
                return mappingIt->second;
        }
    }
    
    // Not found
    return -1;
}

var InstrumentCapabilities::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("type", static_cast<int>(type));
    obj->setProperty("instrumentType", instrumentType);
    obj->setProperty("minNote", minNote);
    obj->setProperty("maxNote", maxNote);
    
    // Serialize sound mappings
    DynamicObject* mappingsObj = new DynamicObject();
    for (const auto& mapping : soundMappings)
    {
        mappingsObj->setProperty(mapping.first, mapping.second);
    }
    obj->setProperty("soundMappings", var(mappingsObj));
    
    return var(obj);
}

void InstrumentCapabilities::fromVar(const var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        // Get properties with default values
        auto typeVar = obj->getProperty("type");
        type = typeVar.isVoid() ? InstrumentCapabilityType::NoteRange : 
              static_cast<InstrumentCapabilityType>(static_cast<int>(typeVar));
              
        auto instrumentTypeVar = obj->getProperty("instrumentType");
        instrumentType = instrumentTypeVar.isVoid() ? String() : instrumentTypeVar.toString();
        
        auto minNoteVar = obj->getProperty("minNote");
        minNote = minNoteVar.isVoid() ? 0 : static_cast<int>(minNoteVar);
        
        auto maxNoteVar = obj->getProperty("maxNote");
        maxNote = maxNoteVar.isVoid() ? 127 : static_cast<int>(maxNoteVar);
        
        // Deserialize sound mappings
        soundMappings.clear();
        auto mappingsVar = obj->getProperty("soundMappings");
        if (!mappingsVar.isVoid())
        {
            if (auto* mappingsObj = mappingsVar.getDynamicObject())
            {
                for (auto& prop : mappingsObj->getProperties())
                {
                    soundMappings[prop.name.toString()] = static_cast<int>(prop.value);
                }
            }
        }
    }
}

// PlayerMetadata implementation
var PlayerMetadata::toVar() const
{
    DynamicObject* obj = new DynamicObject();
    
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("description", description);
    obj->setProperty("created", created.toISO8601(true));
    obj->setProperty("lastModified", lastModified.toISO8601(true));
    obj->setProperty("capabilities", capabilities.toVar());
    
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
        
        if (auto capabilitiesVar = obj->getProperty("capabilities"))
        {
            capabilities.fromVar(capabilitiesVar);
        }
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
    metadata.category = "Keyboard"; // Default to Keyboard category
    metadata.description = "A new player";
    metadata.created = Time::getCurrentTime();
    metadata.lastModified = metadata.created;

    // std::cout << "Creating Player: " << name << " (ID: " << playerId << ")" 
    //           << " with " << (audioEngine != nullptr ? "shared" : "dedicated") 
    //           << " audio engine" << std::endl;
}

Player::~Player()
{
    // Stop playback before destruction
    stopPlayback();
    
    // std::cout << "Player destructor: Cleaning up " << metadata.name << " (ID: " << id << ")" << std::endl;
    
    // Explicitly clean up the instrument first
    // This ensures the instrument is destroyed before the player
    if (instrument)
    {
        // std::cout << "Player destructor: Explicitly cleaning up instrument for " << metadata.name << std::endl;
        instrument.reset();
    }
    
    // std::cout << "Player destructor: Finished cleaning up " << metadata.name << " (ID: " << id << ")" << std::endl;
}

void Player::loadMinimalResources()
{
    // Just load metadata and minimal state
    if (!fullyLoaded)
    {
        // Create instrument if needed (but don't initialize)
        if (instrument == nullptr)
        {
            instrument = std::make_unique<Instrument>(metadata.name, formatManager, knownPluginList, audioEngine);
            
            // Set up chart finished callback
            instrument->setPatternFinishedCallback([this]() {
                this->handleChartFinished();
            });
        }
        
        fullyLoaded = true;
        // std::cout << "Player " << metadata.name << " minimal resources loaded" << std::endl;
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
                // std::cout << "Loading graph state for editing player: " << metadata.name << std::endl;
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
            // std::cout << "Loading graph state for playback of player: " << metadata.name << std::endl;
            loadGraphStateIfNeeded();
        }
        
        // Connect to AudioEngine if available
        if (audioEngine && !isConnectedToAudioEngine())
            connectToAudioEngine(audioEngine);
        
        // Prepare instrument for playback
        if (instrument)
            instrument->prepareForPlayback();
        
        readyForPlayback = true;
        // std::cout << "Player " << metadata.name << " prepared for playback" << std::endl;
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
        
        // std::cout << "Player " << metadata.name << " resources released" << std::endl;
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
        // Register the player with the AudioEngine (new method)
        if (engine->registerPlayer(this)) {
            // std::cout << "Registered player " << metadata.name << " with AudioEngine" << std::endl;
        } else {
            // std::cout << "Player " << metadata.name << " already registered with AudioEngine" << std::endl;
        }
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
    return audioEngine != nullptr;
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
        
        // Set up chart finished callback
        instrument->setPatternFinishedCallback([this]() {
            this->handleChartFinished();
        });
    }
    
    // std::cout << "Player initialized with minimal resources: " << metadata.name << " (ID: " << id << ")" << std::endl;
    
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
        // std::cout << "Loading graph state from file: " << graphFile.getFullPathName() << std::endl;
        
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
                    // std::cout << "  Found node in file XML: " << node->getIntAttribute("uid") 
    //                           << " - " << node->getStringAttribute("name") << std::endl;
                }
                // std::cout << "Graph file contains " << nodeCount << " nodes" << std::endl;
                
                // std::cout << "Successfully loaded graph state from file" << std::endl;
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
        // std::cout << "Loading embedded graph state" << std::endl;
        
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
                // std::cout << "  Found node in embedded XML: " << node->getIntAttribute("uid") 
    //                       << " - " << node->getStringAttribute("name") << std::endl;
            }
            // std::cout << "Embedded graph contains " << nodeCount << " nodes" << std::endl;
            
            // std::cout << "Successfully loaded embedded graph state" << std::endl;
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
            // std::cout << "Trying to load graph from default path: " << graphFile.getFullPathName() << std::endl;
            
            // Parse the XML file
            std::unique_ptr<XmlElement> graphState = parseXML(graphFile);
            
            if (graphState != nullptr && graphState->hasTagName("FILTERGRAPH"))
            {
                // std::cout << "Successfully loaded graph state from default path" << std::endl;
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
        
        // Set up chart finished callback
        instrument->setPatternFinishedCallback([this]() {
            this->handleChartFinished();
        });
    }
    
    // Prepare the instrument for editing
    instrument->prepareForEditing();
    bool instrumentInitialized = true; // Assume initialization succeeded
    
    // Load the graph state if needed
    loadGraphStateIfNeeded();

    // Create musician if needed
    if (musician == nullptr)
    {
        musician = std::make_unique<Musician>(id, metadata.category);
        
        // Set up the musician to use the instrument for playback (done in setMusician)
    }
    
    // std::cout << "Player fully initialized: " << metadata.name << " (ID: " << id << ")" << std::endl;
    
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
            // std::cout << "  Found node in XML: " << node->getIntAttribute("uid") 
    //                   << " - " << node->getStringAttribute("name") << std::endl;
        }
        // std::cout << "Player " << metadata.name << " loading graph with " << nodeCount << " nodes" << std::endl;
        
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
        
        // std::cout << "Player " << metadata.name << " loading graph with " 
    //               // << (midiConnections + audioConnections) << " connections "
                  // << "(MIDI: " << midiConnections << ", Audio: " << audioConnections << ")" << std::endl;
        
        // Load the graph
        bool result = instrument->loadPluginGraph(graphState);
        
        if (result)
        {
            // std::cout << "Graph loaded successfully for player " << metadata.name << std::endl;
            
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
            
            // std::cout << "Saved graph state to file: " << graphFile.getFullPathName() << std::endl;
            
            // Also update the instrument's internal state with this graph
            if (instrument != nullptr)
            {
                // std::cout << "Updating instrument with the saved graph state" << std::endl;
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
        // std::cout << "Instrument is null, trying to load graph from file" << std::endl;
        return loadGraphStateFromFile();
    }
    
    // First try to get the state from the instrument
    auto graphState = instrument->getPluginGraphState();
    
    // If the instrument has no graph state, try to load from file
    if (graphState == nullptr || !graphState->hasTagName("FILTERGRAPH") || 
        graphState->getNumChildElements() == 0)
    {
        // std::cout << "Instrument has no valid graph state, trying to load from file" << std::endl;
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
        // std::cout << "Loading graph state from file: " << graphFile.getFullPathName() << std::endl;
        
        // Parse the XML file
        std::unique_ptr<XmlElement> graphState = parseXML(graphFile);
        
        if (graphState != nullptr && graphState->hasTagName("FILTERGRAPH"))
        {
            // Count nodes in the graph state
            int nodeCount = 0;
            for (auto* node : graphState->getChildWithTagNameIterator("FILTER"))
            {
                nodeCount++;
                // std::cout << "  Found node in file XML: " << node->getIntAttribute("uid") 
    //                       << " - " << node->getStringAttribute("name") << std::endl;
            }
            // std::cout << "Graph file contains " << nodeCount << " nodes" << std::endl;
            
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
            
            // std::cout << "Graph file contains " << (midiConnections + audioConnections) 
    //                   << " connections (MIDI: " << midiConnections 
    //                   << ", Audio: " << audioConnections << ")" << std::endl;
            
            // If we have an instrument, update it with this graph state
            if (instrument != nullptr)
            {
                // std::cout << "Updating instrument with loaded graph state" << std::endl;
                instrument->loadPluginGraph(*graphState);
            }
            
            return graphState;
        }
        else
        {
            std::cerr << "Failed to parse graph file or invalid format: " << graphFile.getFullPathName() << std::endl;
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

void Player::generateNewChart(const var& parameters, std::function<void(bool success)> callback)
{
    if (musician != nullptr)
    {
        // Use the musician to generate the chart
        musician->generateChart(parameters, [this, callback](bool success, const var& chart) {
            if (success)
            {
                // Load the generated chart
                bool loaded = loadChart(chart);
                if (callback)
                    callback(loaded);
            }
            else
            {
                if (callback)
                    callback(false);
            }
        });
    }
    else
    {
        if (callback)
            callback(false);
    }
}

void Player::generateChart(const var& parameters, 
                          std::function<void(bool success, const var& midiEventChart)> callback)
{
    DBG("DEBUG: Player::generateChart called for player: " << getMetadata().name);
    if (!musician)
    {
        DBG("DEBUG: Player::generateChart - No musician available for player: " << getMetadata().name);
        std::cerr << "Player::generateChart - No musician available" << std::endl;
        if (callback)
            callback(false, var());
        return;
    }
    
    DBG("DEBUG: Player delegating to musician->generateChartWithPlayer using Player capabilities");
    // Delegate to the internal musician using Player capabilities
    musician->generateChartWithPlayer(parameters, this, callback);
}

bool Player::loadChart(const var& chart)
{
    if (!chart.isObject())
    {
        std::cerr << "Player::loadChart - Invalid chart format (not an object)" << std::endl;
        return false;
    }
    
    // Store the chart
    currentLoadedChart = chart;
    
    // Extract tempo if available, otherwise use default (120 BPM)
    chartTempo = chart.hasProperty("tempo") ? static_cast<double>(chart["tempo"]) : 120.0;
    
    // Convert chart to MIDI sequence
    return convertChartToMidi(chart);
}

bool Player::loadChartFromMusician()
{
    if (musician != nullptr)
    {
        const var& chart = musician->getLastGeneratedChart();
        return loadChart(chart);
    }
    return false;
}

var Player::convertPatternStringToChart(const String& patternString, double tempo)
{
    if (musician != nullptr)
    {
        // Use the musician's conversion method to convert the pattern string
        // This uses the same code path as Performance Mode
        return musician->convertNotationToMidiEvents(patternString, tempo);
    }
    
    // Return empty result if no musician available
    DynamicObject::Ptr result = new DynamicObject();
    result->setProperty("success", false);
    result->setProperty("tempo", tempo);
    result->setProperty("events", Array<var>());
    return var(result.get());
}

const var& Player::getCurrentChart() const
{
    return currentLoadedChart;
}

void Player::modifyCurrentChart(const String& modifications, std::function<void(bool success)> callback)
{
    if (musician != nullptr)
    {
        musician->modifyChart(currentLoadedChart, modifications, [this, callback](bool success, const var& chart) {
            if (success)
            {
                // Load the modified chart
                bool loaded = loadChart(chart);
                if (callback)
                    callback(loaded);
            }
            else
            {
                if (callback)
                    callback(false);
            }
        });
    }
    else
    {
        if (callback)
            callback(false);
    }
}

bool Player::convertChartToMidi(const var& chart)
{
    if (!chart.isObject())
    {
        std::cerr << "Player::convertChartToMidi - Invalid chart format (not an object)" << std::endl;
        return false;
    }
    
    // Tempo is already extracted in loadChart method - no need to extract again here
    
    // Create a new MIDI message sequence
    auto sequence = std::make_shared<juce::MidiMessageSequence>();
    
    try
    {
        // Extract events array
        var eventsVar = chart;
        
        // If the chart is a full command with an events property, extract it
        if (chart.hasProperty("events"))
        {
            eventsVar = chart["events"];
        }
        
        if (!eventsVar.isArray())
        {
            DBG("DEBUG: Player::convertChartToMidi - events is not an array");
            DBG("DEBUG: eventsVar type: " << (eventsVar.isVoid() ? "void" : 
                                           eventsVar.isString() ? "string" : 
                                           eventsVar.isObject() ? "object" : 
                                           eventsVar.isInt() ? "int" : "unknown"));
            if (eventsVar.isObject()) {
                DBG("DEBUG: eventsVar as object: " << eventsVar.toString());
            }
            std::cerr << "Player::convertChartToMidi - Invalid chart format (events not an array)" << std::endl;
            return false;
        }
        
        // Process each event in the array
        for (int i = 0; i < eventsVar.size(); ++i)
        {
            var event = eventsVar[i];
            
            if (!event.isObject())
                continue;
            
            // Extract common properties
            juce::String type = event.hasProperty("type") ? event["type"].toString() : "";
            int channel = event.hasProperty("channel") ? static_cast<int>(static_cast<double>(event["channel"])) : 1;
            double timestamp = event.hasProperty("timestamp") ? static_cast<double>(event["timestamp"]) : 0.0;
            
            // Ensure channel is in valid range (1-16)
            channel = juce::jlimit(1, 16, channel);
            
            if (type == "note_on")
            {
                // Extract note-specific properties
                int note = event.hasProperty("note") ? static_cast<int>(static_cast<double>(event["note"])) : 60;
                double duration = event.hasProperty("duration") ? static_cast<double>(event["duration"]) : 0.25; // Default quarter note duration
                
                // Handle velocity - Performance Mode uses float (0.0-1.0), Player Mode might use int (0-127)
                float velocity = 0.5f; // Default velocity
                if (event.hasProperty("velocity")) {
                    double velocityValue = static_cast<double>(event["velocity"]);
                    if (velocityValue <= 1.0) {
                        // Performance Mode format: float between 0.0 and 1.0
                        velocity = static_cast<float>(velocityValue);
                    } else {
                        // Player Mode format: int between 0 and 127, convert to float
                        velocity = static_cast<float>(velocityValue) / 127.0f;
                    }
                }
                
                // Ensure note and velocity are in valid ranges
                note = juce::jlimit(0, 127, note);
                velocity = juce::jlimit(0.0f, 1.0f, velocity);
                
                // Apply player level scaling
                velocity = velocity * outputLevel;
                velocity = juce::jlimit(0.0f, 1.0f, velocity); // Re-clamp after scaling
                
                // Create note on message
                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel, note, velocity);
                noteOn.setTimeStamp(timestamp);
                sequence->addEvent(noteOn);
                
                // std::cout << "Added note_on event: channel=" << channel 
    //                       << ", note=" << note 
    //                       << ", velocity=" << velocity 
    //                       << ", timestamp=" << timestamp << std::endl;
                
                // Create corresponding note off message based on duration
                double noteOffTimestamp = timestamp + duration;
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(channel, note, velocity);
                noteOff.setTimeStamp(noteOffTimestamp);
                sequence->addEvent(noteOff);
                
                // std::cout << "Added note_off event: channel=" << channel 
    //                       << ", note=" << note 
    //                       << ", velocity=" << velocity 
    //                       << ", timestamp=" << noteOffTimestamp 
    //                       << ", duration=" << duration << std::endl;
            }
            else if (type == "note_off")
            {
                // Extract note-specific properties
                int note = event.hasProperty("note") ? static_cast<int>(static_cast<double>(event["note"])) : 60;
                
                // Ensure note is in valid range
                note = juce::jlimit(0, 127, note);
                
                // Create note off message
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(channel, note);
                noteOff.setTimeStamp(timestamp);
                sequence->addEvent(noteOff);
            }
            else if (type == "controller")
            {
                // Extract controller-specific properties
                int controllerNumber = event.hasProperty("controller") ? static_cast<int>(static_cast<double>(event["controller"])) : 0;
                int value = event.hasProperty("value") ? static_cast<int>(static_cast<double>(event["value"])) : 0;
                
                // Ensure controller number and value are in valid ranges
                controllerNumber = juce::jlimit(0, 127, controllerNumber);
                value = juce::jlimit(0, 127, value);
                
                // Create controller message
                juce::MidiMessage cc = juce::MidiMessage::controllerEvent(channel, controllerNumber, value);
                cc.setTimeStamp(timestamp);
                sequence->addEvent(cc);
            }
            else if (type == "program_change")
            {
                // Extract program change-specific properties
                int programNumber = event.hasProperty("program") ? static_cast<int>(static_cast<double>(event["program"])) : 0;
                
                // Ensure program number is in valid range
                programNumber = juce::jlimit(0, 127, programNumber);
                
                // Create program change message
                juce::MidiMessage pc = juce::MidiMessage::programChange(channel, programNumber);
                pc.setTimeStamp(timestamp);
                sequence->addEvent(pc);
            }
        }
        
        // Sort the sequence by timestamp
        sequence->sort();
        
        // Add an all notes off message at the end to ensure clean playback
        double endTime = 0.0;
        
        if (sequence->getNumEvents() > 0)
        {
            endTime = sequence->getEndTime() + 0.5; // Add a half-second buffer
        }
        else
        {
            endTime = 1.0; // Default to 1 second if no events
        }
        
        for (int channel = 1; channel <= 16; ++channel)
        {
            juce::MidiMessage allNotesOff = juce::MidiMessage::allNotesOff(channel);
            allNotesOff.setTimeStamp(endTime);
            sequence->addEvent(allNotesOff);
        }
        
        // Re-sort after adding all notes off messages
        sequence->sort();
        
        // Store the MIDI sequence
        currentMidiSequence = sequence;
        
        // std::cout << "Player::convertChartToMidi - Chart converted with " << sequence->getNumEvents() 
                  // << " events, duration: " << sequence->getEndTime() << " seconds" << std::endl;
        
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Player::convertChartToMidi - Error parsing chart: " << e.what() << std::endl;
        return false;
    }
}

bool Player::play()
{
    if (!currentMidiSequence)
    {
        std::cerr << "Player::play - No chart loaded" << std::endl;
        return false;
    }
    
    if (!instrument)
    {
        std::cerr << "Player::play - Instrument not initialized" << std::endl;
        return false;
    }
    
    // Ensure the instrument is ready for playback
    if (instrument->getResourceState() != Instrument::ResourceState::PlaybackReady)
    {
        std::cerr << "Player::play - Instrument not ready for playback, preparing now" << std::endl;
        prepareForPlayback();
    }
    
    // Use the efficient direct MIDI sequence method instead of converting to var/JSON
    bool result = instrument->playMIDISequence(*currentMidiSequence, chartTempo);
    
    if (result)
    {
        // Calculate the duration of the chart
        double duration = currentMidiSequence->getEndTime();
        
        // std::cout << "Player::play - Started playback of chart with " 
                  // << currentMidiSequence->getNumEvents() << " events"
                  // << ", duration: " << duration << " seconds"
                  // << ", tempo: " << chartTempo << " BPM" << std::endl;
        
        // Log the first few events for debugging
        int eventsToLog = std::min(8, currentMidiSequence->getNumEvents());
        for (int i = 0; i < eventsToLog; ++i)
        {
            auto event = currentMidiSequence->getEventPointer(i);
            juce::String eventType = event->message.isNoteOn() ? "Note On" : 
                                    (event->message.isNoteOff() ? "Note Off" : "Other");
            int noteNumber = event->message.isNoteOnOrOff() ? event->message.getNoteNumber() : -1;
            
            // std::cout << "  Event " << i << ": " << eventType 
    //                   << ", Note: " << noteNumber 
    //                   << ", Time: " << event->message.getTimeStamp() << "s" << std::endl;
        }
    }
    else
    {
        std::cerr << "Player::play - Failed to start playback" << std::endl;
    }
    
    return result;
}

void Player::generateAndLoad(const var& parameters, std::function<void(bool success)> callback)
{
    generateNewChart(parameters, callback);
}


void Player::stopPlayback()
{
    if (instrument != nullptr)
    {
        instrument->stopPlayback();
    }
}

void Player::stop()
{
    if (instrument)
    {
        instrument->stopPlayback();
        // std::cout << "Player::stop - Stopped playback" << std::endl;
    }
}

void Player::setChartFinishedCallback(std::function<void(const String&)> callback)
{
    onChartFinished = std::move(callback);
}

void Player::handleChartFinished()
{
    // Call the callback if set
    if (onChartFinished)
    {
        onChartFinished(id);
    }
}

void Player::setMusician(std::unique_ptr<Musician> newMusician)
{
    musician = std::move(newMusician);
    
    // Musician setup is complete - no need for command interface since it was removed
}

void Player::setLLMClient(std::shared_ptr<LLMClient> client)
{
    if (musician)
    {
        musician->setLLMClient(client);
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
                    
                    // std::cout << "Saved graph state to file AND embedded in player data: " << graphFile.getFullPathName() << std::endl;
                }
                else
                {
                    std::cerr << "Failed to open graph file for writing: " << graphFile.getFullPathName() << std::endl;
                    
                    // Fall back to embedding the graph state in the player data
                    MemoryOutputStream mo;
                    graphState->writeTo(mo, {});
                    obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
                    // std::cout << "Embedded graph state in player data as fallback" << std::endl;
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
    
    }
    else
    {
        // Create an empty graph state to avoid crashes
        auto emptyGraph = std::make_unique<XmlElement>("FILTERGRAPH");
        MemoryOutputStream mo;
        emptyGraph->writeTo(mo, {});
        obj->setProperty("pluginGraph", mo.getMemoryBlock().toBase64Encoding());
    }
    
    // Add musician state if available
    if (musician != nullptr)
    {
        obj->setProperty("musician", musician->toVar());
    }
    
    return var(obj);
}

std::unique_ptr<covertracks::Player> covertracks::Player::fromVar(const var& v, AudioPluginFormatManager& fm, KnownPluginList& kpl, AudioEngine* engine)
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
            // std::cout << "Player has graphFile property: " << graphFileName << std::endl;
            
            // Store the graph file name for later loading
            player->setGraphFileName(graphFileName);
        }
        // Store embedded graph state for later loading
        else if (obj->hasProperty("pluginGraph"))
        {
            // std::cout << "Player has pluginGraph property, storing for later loading" << std::endl;
            
            // Store the embedded graph state for later loading
            player->setEmbeddedGraphState(obj->getProperty("pluginGraph").toString());
        }
        else
        {
            // std::cout << "Player has no graph state (neither graphFile nor pluginGraph property)" << std::endl;
        }

        if (obj->hasProperty("musician"))
        {
            auto musicianVar = obj->getProperty("musician");
            if (musicianVar.isObject())
            {
                auto newMusician = Musician::fromVar(musicianVar);
                if (newMusician)
                {
                    player->setMusician(std::move(newMusician));
                }
            }
        }
        else
        {
            // Create a default musician if none exists
            player->setMusician(std::make_unique<Musician>(player->getId(), player->getMetadata().category));
        }
        
        return player;
    }
    
    // If parsing failed, return a default player
    return std::make_unique<Player>(String::toHexString(Random::getSystemRandom().nextInt()), 
                                   "Default Player", fm, kpl, engine);
}

// Static helper functions for creating instrument capabilities
InstrumentCapabilities Player::createDrumCapabilities()
{
    InstrumentCapabilities capabilities;
    capabilities.type = InstrumentCapabilityType::Mapping;
    capabilities.instrumentType = "drums";
    
    // Standard GM percussion mapping
    capabilities.soundMappings = {
        {"kick", 36},           // Bass Drum 1
        {"snare", 38},          // Acoustic Snare
        {"hihat", 42},          // Closed Hi Hat
        {"openHihat", 46},      // Open Hi-Hat
        {"crash", 49},          // Crash Cymbal 1
        {"ride", 51},           // Ride Cymbal 1
        {"tom1", 50},           // High Tom
        {"tom2", 47},           // Low-Mid Tom
        {"tom3", 43},           // High Floor Tom
        {"rimshot", 37},        // Side Stick
        {"cowbell", 56},        // Cowbell
        {"splash", 55}          // Splash Cymbal
    };
    
    return capabilities;
}

InstrumentCapabilities Player::createPianoCapabilities(int minNote, int maxNote)
{
    InstrumentCapabilities capabilities;
    capabilities.type = InstrumentCapabilityType::NoteRange;
    capabilities.instrumentType = "piano";
    capabilities.minNote = minNote;
    capabilities.maxNote = maxNote;
    
    return capabilities;
}

InstrumentCapabilities Player::createBassCapabilities(int minNote, int maxNote)
{
    InstrumentCapabilities capabilities;
    capabilities.type = InstrumentCapabilityType::NoteRange;
    capabilities.instrumentType = "bass";
    capabilities.minNote = minNote;
    capabilities.maxNote = maxNote;
    
    return capabilities;
}

InstrumentCapabilities Player::createGuitarCapabilities(int minNote, int maxNote)
{
    InstrumentCapabilities capabilities;
    capabilities.type = InstrumentCapabilityType::NoteRange;
    capabilities.instrumentType = "guitar";
    capabilities.minNote = minNote;
    capabilities.maxNote = maxNote;
    
    return capabilities;
}

} // namespace covertracks
