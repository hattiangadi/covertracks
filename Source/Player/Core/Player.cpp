#include "Player.h"
#include <iostream>

namespace covertracks {

Player::Player(const juce::String& id,
               const juce::String& name,
               juce::AudioPluginFormatManager& fm,
               juce::KnownPluginList& kpl)
    : playerId(id)
    , playerName(name)
    , formatManager(fm)
    , knownPluginList(kpl)
{
    // Create Instrument - it owns all audio concerns
    instrument = std::make_unique<Instrument>(name + "_Instrument", formatManager, knownPluginList);
    
    // Setup default musician for chart generation
    setupDefaultMusician();
    
    // Initialize default instrument capabilities (can be customized later)
    instrumentCapabilities.type = InstrumentCapabilityType::NoteRange;
    instrumentCapabilities.instrumentType = "general";
    instrumentCapabilities.minNote = 0;
    instrumentCapabilities.maxNote = 127;
    
    // Initialize metadata
    metadata = PlayerMetadata(name, id, "general");
    
    DBG("Player created: " << name << " (ID: " << id << ")");
}

Player::~Player()
{
    shutdown();
    DBG("Player destroyed: " << playerName);
}

// =========================================================================
// State Machine Workflow Methods
// =========================================================================

bool Player::preparePattern(const juce::String& patternJson)
{
    if (patternJson.isEmpty())
    {
        std::cerr << "Player::preparePattern - Empty pattern" << std::endl;
        return false;
    }
    
    // Store the pattern for later conversion
    preparedPatternJson = patternJson;
    
    // Clear any previously converted data
    convertedMidiSequence.reset();
    preparedMidiChart = juce::var();
    
    DBG("Player " << playerName << " prepared pattern for conversion");
    return true;
}

bool Player::convertToMidi()
{
    if (preparedPatternJson.isEmpty())
    {
        std::cerr << "Player::convertToMidi - No pattern prepared" << std::endl;
        return false;
    }
    
    if (!musician)
    {
        std::cerr << "Player::convertToMidi - No musician available" << std::endl;
        return false;
    }
    
    // Use musician to convert pattern to MIDI events
    auto midiResult = musician->convertNotationToMidiEvents(preparedPatternJson, lastTempo);
    
    if (!midiResult.isObject())
    {
        std::cerr << "Player::convertToMidi - Invalid MIDI result from musician" << std::endl;
        return false;
    }
    
    auto* resultObj = midiResult.getDynamicObject();
    if (!resultObj || !resultObj->getProperty("success"))
    {
        std::cerr << "Player::convertToMidi - Musician conversion failed" << std::endl;
        return false;
    }
    
    // Store the converted MIDI chart
    preparedMidiChart = resultObj->getProperty("events");
    lastTempo = resultObj->getProperty("tempo");
    
    if (!preparedMidiChart.isArray())
    {
        std::cerr << "Player::convertToMidi - No events array in result" << std::endl;
        return false;
    }
    
    DBG("Player " << playerName << " converted pattern to " 
        << preparedMidiChart.getArray()->size() << " MIDI events at " << lastTempo << " BPM");
    return true;
}

bool Player::loadPreparedMidi()
{
    if (!preparedMidiChart.isArray() || !instrument)
    {
        std::cerr << "Player::loadPreparedMidi - No MIDI data or instrument" << std::endl;
        return false;
    }
    
    // Convert MIDI chart to sequence for instant playback
    auto sequence = instrument->convertChartToMidiSequence(preparedMidiChart);
    if (!sequence || sequence->getNumEvents() == 0)
    {
        std::cerr << "Player::loadPreparedMidi - Failed to convert to MIDI sequence" << std::endl;
        return false;
    }
    
    // Store the pre-converted sequence
    preloadedSequence = std::move(sequence);
    lastGeneratedChart = preparedMidiChart;
    
    DBG("Player " << playerName << " loaded " 
        << preloadedSequence->getNumEvents() << " MIDI events for instant playback");
    return true;
}

// =========================================================================
// High-Level Orchestration (Musician ↔ Instrument)
// =========================================================================

void Player::generateAndPlay(const juce::var& parameters, 
                                     std::function<void(bool success)> callback)
{
    if (!initialized)
    {
        std::cerr << "Player not initialized" << std::endl;
        if (callback) callback(false);
        return;
    }
    
    // Step 1: Generate chart using Musician
    generateChart(parameters, [this, callback](bool generateSuccess, const juce::var& chart) {
        if (!generateSuccess)
        {
            std::cerr << "Chart generation failed" << std::endl;
            if (callback) callback(false);
            return;
        }
        
        // Step 2: Play chart using Instrument
        bool playSuccess = instrument->playChart(chart);
        if (!playSuccess)
        {
            std::cerr << "Chart playback failed" << std::endl;
        }
        
        if (callback) callback(playSuccess);
    });
}

bool Player::loadChart(const juce::var& chart, double tempo)
{
    if (!initialized || !instrument)
    {
        std::cerr << "Player not initialized" << std::endl;
        return false;
    }
    
    // Store chart for later playback - this pre-converts MIDI for instant playback
    lastGeneratedChart = chart;
    lastTempo = tempo;
    
    // Pre-convert the chart to MIDI sequence for instant playback later
    // This ensures all instruments finish their conversion before any start playing
    auto sequence = instrument->convertChartToMidiSequence(chart);
    if (!sequence || sequence->getNumEvents() == 0)
    {
        std::cerr << "Failed to convert chart to MIDI sequence for loading" << std::endl;
        return false;
    }
    
    // Store the pre-converted sequence
    preloadedSequence = std::move(sequence);
    
    DBG("Player " << playerName << " loaded chart with " 
        << preloadedSequence->getNumEvents() << " MIDI events");
    return true;
}

bool Player::playChart(const juce::var& chart, double tempo)
{
    if (!initialized || !instrument)
    {
        std::cerr << "Player not initialized" << std::endl;
        return false;
    }
    
    // Store chart and delegate to instrument
    lastGeneratedChart = chart;
    return instrument->playChart(chart, tempo);
}

bool Player::playLoadedChart()
{
    if (!initialized || !instrument)
    {
        std::cerr << "Player not initialized" << std::endl;
        return false;
    }
    
    if (!preloadedSequence || preloadedSequence->getNumEvents() == 0)
    {
        std::cerr << "No chart loaded for instant playback" << std::endl;
        return false;
    }
    
    // Play the pre-converted sequence - this should be instant
    bool result = instrument->playMidiSequence(*preloadedSequence, lastTempo);
    
    if (result)
    {
        DBG("Player " << playerName << " playing pre-loaded chart with " 
            << preloadedSequence->getNumEvents() << " events");
    }
    
    return result;
}

void Player::generateChart(const juce::var& parameters,
                                   std::function<void(bool success, const juce::var& chart)> callback)
{
    if (!musician)
    {
        std::cerr << "No musician available for chart generation" << std::endl;
        if (callback) callback(false, juce::var{});
        return;
    }
    
    // Use musician to generate chart
    musician->generateChart(parameters, [this, callback](bool success, const juce::var& chart) {
        if (success)
        {
            lastGeneratedChart = chart;
            DBG("Player " << playerName << " generated chart with " 
                << (chart.isArray() ? chart.getArray()->size() : 0) << " events");
        }
        
        if (callback) callback(success, chart);
    });
}

// =========================================================================
// Audio Delegation (Pass-through to Instrument)
// =========================================================================

bool Player::addPlugin(const juce::PluginDescription& pluginDesc)
{
    if (!instrument)
        return false;
        
    return instrument->addPlugin(pluginDesc);
}

void Player::clearPlugins()
{
    if (instrument)
        instrument->clearPlugins();
}

int Player::getPluginCount() const
{
    return instrument ? instrument->getPluginCount() : 0;
}

juce::StringArray Player::getPluginNames() const
{
    return instrument ? instrument->getPluginNames() : juce::StringArray{};
}

void Player::stop()
{
    if (instrument)
        instrument->stopPlayback();
}

void Player::pause()
{
    if (instrument)
        instrument->pausePlayback();
}

void Player::resume()
{
    if (instrument)
        instrument->resumePlayback();
}

bool Player::isPlaying() const
{
    return instrument ? instrument->isPlaying() : false;
}

bool Player::isPaused() const
{
    return instrument ? instrument->isPaused() : false;
}

void Player::processMidiMessage(const juce::MidiMessage& message)
{
    if (instrument)
        instrument->processMidiMessage(message);
}

// =========================================================================
// Component Access (for advanced use)
// =========================================================================

void Player::setMusician(std::unique_ptr<Musician> newMusician)
{
    musician = std::move(newMusician);
    DBG("Player " << playerName << " musician updated");
}

void Player::setLLMClient(std::shared_ptr<LLMClient> client)
{
    if (musician)
    {
        musician->setLLMClient(client);
        DBG("Player " << playerName << " LLM client set");
    }
}

// =========================================================================
// Persistence (Player State Management)
// =========================================================================

bool Player::saveState()
{
    auto stateFile = getStateFile();
    auto state = exportState();
    
    if (!state)
    {
        std::cerr << "Failed to export player state" << std::endl;
        return false;
    }
    
    // Ensure parent directory exists
    stateFile.getParentDirectory().createDirectory();
    
    // Write XML to file
    if (state->writeTo(stateFile))
    {
        DBG("Player " << playerName << " state saved to: " << stateFile.getFullPathName());
        return true;
    }
    else
    {
        std::cerr << "Failed to write state file: " << stateFile.getFullPathName() << std::endl;
        return false;
    }
}

bool Player::loadState()
{
    auto stateFile = getStateFile();
    
    if (!stateFile.existsAsFile())
    {
        DBG("No state file exists for " << playerName << " at: " << stateFile.getFullPathName());
        return false;
    }
    
    auto stateXml = juce::XmlDocument::parse(stateFile);
    if (!stateXml)
    {
        std::cerr << "Failed to parse state file: " << stateFile.getFullPathName() << std::endl;
        return false;
    }
    
    bool success = importState(*stateXml);
    if (success)
    {
        DBG("Player " << playerName << " state loaded from: " << stateFile.getFullPathName());
    }
    
    return success;
}

std::unique_ptr<juce::XmlElement> Player::exportState() const
{
    auto state = std::make_unique<juce::XmlElement>("PlayerState");
    
    // Player metadata
    state->setAttribute("id", playerId);
    state->setAttribute("name", playerName);
    state->setAttribute("initialized", initialized);
    
    // Instrument state
    if (instrument)
    {
        auto instrumentState = instrument->getPluginState();
        if (instrumentState)
        {
            state->addChildElement(instrumentState.release());
        }
    }
    
    // Musician state
    if (musician)
    {
        // TODO: Add musician state serialization when Musician class supports it
        auto musicianState = state->createNewChildElement("MusicianState");
        musicianState->setAttribute("hasMusician", true);
    }
    
    // Last generated chart
    if (!lastGeneratedChart.isVoid())
    {
        auto chartElement = state->createNewChildElement("LastChart");
        // TODO: Serialize chart data when needed
        chartElement->setAttribute("hasChart", true);
    }
    
    return state;
}

bool Player::importState(const juce::XmlElement& state)
{
    if (state.getTagName() != "PlayerState")
    {
        std::cerr << "Invalid state XML for Player" << std::endl;
        return false;
    }
    
    // Validate player identity
    auto stateId = state.getStringAttribute("id");
    auto stateName = state.getStringAttribute("name");
    
    if (stateId != playerId)
    {
        DBG("Warning: State ID mismatch: " << stateId << " vs " << playerId);
    }
    
    if (stateName != playerName)
    {
        DBG("Updating player name from state: " << stateName);
        playerName = stateName;
    }
    
    // Restore instrument state
    auto* instrumentStateElement = state.getChildByName("InstrumentState");
    if (instrumentStateElement && instrument)
    {
        instrument->restorePluginState(*instrumentStateElement);
    }
    
    // TODO: Restore musician state when supported
    // TODO: Restore last chart when needed
    
    DBG("Player " << playerName << " state imported");
    return true;
}

juce::var Player::toVar() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("id", playerId);
    obj->setProperty("name", playerName);
    obj->setProperty("initialized", initialized);
    obj->setProperty("pluginCount", getPluginCount());
    
    // Add plugin names
    auto pluginNames = getPluginNames();
    juce::Array<juce::var> pluginArray;
    for (const auto& name : pluginNames)
    {
        pluginArray.add(name);
    }
    obj->setProperty("plugins", pluginArray);
    
    return juce::var(obj.get());
}

std::unique_ptr<Player> Player::fromVar(const juce::var& data,
                                                         juce::AudioPluginFormatManager& formatManager,
                                                         juce::KnownPluginList& knownPluginList)
{
    if (!data.isObject())
        return nullptr;
        
    auto* obj = data.getDynamicObject();
    if (!obj)
        return nullptr;
        
    auto id = obj->getProperty("id").toString();
    auto name = obj->getProperty("name").toString();
    
    if (id.isEmpty() || name.isEmpty())
        return nullptr;
        
    auto player = std::make_unique<Player>(id, name, formatManager, knownPluginList);
    
    // TODO: Restore additional state when needed
    
    return player;
}

// =========================================================================
// Lifecycle Management
// =========================================================================

bool Player::initialize()
{
    if (initialized)
        return true;
        
    if (!instrument)
    {
        std::cerr << "No instrument available for initialization" << std::endl;
        return false;
    }
    
    // Register instrument with audio engine
    if (!instrument->registerWithAudioEngine())
    {
        std::cerr << "Failed to register instrument with audio engine" << std::endl;
        return false;
    }
    
    initialized = true;
    DBG("Player " << playerName << " initialized successfully");
    return true;
}

void Player::shutdown()
{
    if (!initialized)
        return;
        
    // Stop any playback
    if (instrument)
    {
        instrument->stopPlayback();
        instrument->unregisterFromAudioEngine();
    }
    
    initialized = false;
    DBG("Player " << playerName << " shutdown");
}

// =========================================================================
// Comprehensive State Persistence (NEW)
// =========================================================================

juce::var Player::saveCompleteState() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    // Player metadata
    obj->setProperty("id", playerId);
    obj->setProperty("name", playerName);
    obj->setProperty("initialized", initialized);
    obj->setProperty("version", "1.0");
    obj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    
    // Save PlayerMetadata
    juce::DynamicObject::Ptr metadataObj = new juce::DynamicObject();
    metadataObj->setProperty("name", metadata.name);
    metadataObj->setProperty("id", metadata.id);
    metadataObj->setProperty("instrumentType", metadata.instrumentType);
    metadataObj->setProperty("category", metadata.category);
    metadataObj->setProperty("description", metadata.description);
    metadataObj->setProperty("created", metadata.created.toISO8601(true));
    metadataObj->setProperty("lastModified", metadata.lastModified.toISO8601(true));
    obj->setProperty("metadata", juce::var(metadataObj.get()));
    
    // Save InstrumentCapabilities
    juce::DynamicObject::Ptr capabilitiesObj = new juce::DynamicObject();
    capabilitiesObj->setProperty("type", (int)instrumentCapabilities.type);
    capabilitiesObj->setProperty("instrumentType", instrumentCapabilities.instrumentType);
    capabilitiesObj->setProperty("minNote", instrumentCapabilities.minNote);
    capabilitiesObj->setProperty("maxNote", instrumentCapabilities.maxNote);
    
    // Save sound mappings
    juce::DynamicObject::Ptr mappingsObj = new juce::DynamicObject();
    for (const auto& mapping : instrumentCapabilities.soundMappings)
    {
        mappingsObj->setProperty(mapping.first, mapping.second);
    }
    capabilitiesObj->setProperty("soundMappings", juce::var(mappingsObj.get()));
    obj->setProperty("instrumentCapabilities", juce::var(capabilitiesObj.get()));
    
    // Instrument state (includes complete plugin chain + configurations)
    if (instrument)
    {
        obj->setProperty("instrumentState", instrument->saveInstrumentState());
    }
    
    // Last generated chart
    if (!lastGeneratedChart.isVoid())
    {
        obj->setProperty("lastChart", lastGeneratedChart);
    }
    
    return juce::var(obj.get());
}

bool Player::loadCompleteState(const juce::var& state)
{
    if (!state.isObject())
    {
        std::cerr << "Invalid player state: not an object" << std::endl;
        return false;
    }
    
    auto* obj = state.getDynamicObject();
    if (!obj)
    {
        std::cerr << "Invalid player state: could not get dynamic object" << std::endl;
        return false;
    }
    
    // Validate version (for future compatibility)
    auto version = obj->getProperty("version").toString();
    if (version.isEmpty())
    {
        std::cerr << "Warning: No version in player state, assuming legacy format" << std::endl;
    }
    
    // Load PlayerMetadata
    auto metadataVar = obj->getProperty("metadata");
    if (!metadataVar.isVoid() && metadataVar.isObject())
    {
        auto* metadataObj = metadataVar.getDynamicObject();
        if (metadataObj)
        {
            metadata.name = metadataObj->getProperty("name").toString();
            metadata.id = metadataObj->getProperty("id").toString();
            metadata.instrumentType = metadataObj->getProperty("instrumentType").toString();
            metadata.category = metadataObj->getProperty("category").toString();
            metadata.description = metadataObj->getProperty("description").toString();
            
            auto createdStr = metadataObj->getProperty("created").toString();
            if (createdStr.isNotEmpty())
                metadata.created = juce::Time::fromISO8601(createdStr);
                
            auto lastModifiedStr = metadataObj->getProperty("lastModified").toString();
            if (lastModifiedStr.isNotEmpty())
                metadata.lastModified = juce::Time::fromISO8601(lastModifiedStr);
        }
    }
    
    // Load InstrumentCapabilities
    auto capabilitiesVar = obj->getProperty("instrumentCapabilities");
    if (!capabilitiesVar.isVoid() && capabilitiesVar.isObject())
    {
        auto* capabilitiesObj = capabilitiesVar.getDynamicObject();
        if (capabilitiesObj)
        {
            instrumentCapabilities.type = (InstrumentCapabilityType)(int)capabilitiesObj->getProperty("type");
            instrumentCapabilities.instrumentType = capabilitiesObj->getProperty("instrumentType").toString();
            instrumentCapabilities.minNote = (int)capabilitiesObj->getProperty("minNote");
            instrumentCapabilities.maxNote = (int)capabilitiesObj->getProperty("maxNote");
            
            // Load sound mappings
            auto mappingsVar = capabilitiesObj->getProperty("soundMappings");
            if (!mappingsVar.isVoid() && mappingsVar.isObject())
            {
                auto* mappingsObj = mappingsVar.getDynamicObject();
                if (mappingsObj)
                {
                    instrumentCapabilities.soundMappings.clear();
                    for (const auto& prop : mappingsObj->getProperties())
                    {
                        instrumentCapabilities.soundMappings[prop.name.toString()] = prop.value;
                    }
                }
            }
        }
    }
    
    // Load instrument state
    auto instrumentStateVar = obj->getProperty("instrumentState");
    if (!instrumentStateVar.isVoid() && instrument)
    {
        if (!instrument->loadInstrumentState(instrumentStateVar))
        {
            std::cerr << "Failed to load instrument state" << std::endl;
            return false;
        }
    }
    
    // Load musician state
    auto musicianStateVar = obj->getProperty("musicianState");
    if (!musicianStateVar.isVoid() && musician)
    {
        // TODO: Load musician state when supported
    }
    
    // Load last chart
    auto lastChartVar = obj->getProperty("lastChart");
    if (!lastChartVar.isVoid())
    {
        lastGeneratedChart = lastChartVar;
    }
    
    DBG("Player " << playerName << " complete state loaded successfully");
    return true;
}

juce::var Player::savePluginChain() const
{
    if (!instrument)
        return juce::var();
        
    auto instrumentState = instrument->saveInstrumentState();
    if (auto* obj = instrumentState.getDynamicObject())
    {
        return obj->getProperty("pluginChain");
    }
    
    return juce::var();
}

bool Player::loadPluginChain(const juce::var& pluginChainState)
{
    if (!instrument)
    {
        std::cerr << "No instrument available for loading plugin chain" << std::endl;
        return false;
    }
    
    // Create a minimal instrument state with just the plugin chain
    juce::DynamicObject::Ptr tempState = new juce::DynamicObject();
    tempState->setProperty("name", instrument->getName());
    tempState->setProperty("pluginChain", pluginChainState);
    
    return instrument->loadInstrumentState(juce::var(tempState.get()));
}

bool Player::exportAsPreset(const juce::File& file) const
{
    auto state = saveCompleteState();
    
    // Add preset metadata
    if (auto* obj = state.getDynamicObject())
    {
        obj->setProperty("type", "PlayerPreset");
        obj->setProperty("exportedAt", juce::Time::getCurrentTime().toISO8601(true));
        obj->setProperty("application", "Covertracks");
    }
    
    // Save as JSON
    auto json = juce::JSON::toString(state);
    
    if (file.replaceWithText(json))
    {
        DBG("Player preset exported to: " << file.getFullPathName());
        return true;
    }
    else
    {
        std::cerr << "Failed to export preset to: " << file.getFullPathName() << std::endl;
        return false;
    }
}

bool Player::importFromPreset(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        std::cerr << "Preset file does not exist: " << file.getFullPathName() << std::endl;
        return false;
    }
    
    auto jsonText = file.loadFileAsString();
    if (jsonText.isEmpty())
    {
        std::cerr << "Could not load preset file: " << file.getFullPathName() << std::endl;
        return false;
    }
    
    auto state = juce::JSON::parse(jsonText);
    if (state.isVoid())
    {
        std::cerr << "Invalid JSON in preset file: " << file.getFullPathName() << std::endl;
        return false;
    }
    
    // Validate preset type
    if (auto* obj = state.getDynamicObject())
    {
        auto type = obj->getProperty("type").toString();
        if (type != "PlayerPreset")
        {
            std::cerr << "Invalid preset type: " << type << std::endl;
            return false;
        }
    }
    
    if (loadCompleteState(state))
    {
        DBG("Player preset imported from: " << file.getFullPathName());
        return true;
    }
    else
    {
        std::cerr << "Failed to load preset state from: " << file.getFullPathName() << std::endl;
        return false;
    }
}

// =========================================================================
// Helper Methods
// =========================================================================

juce::File Player::getStateFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("Covertracks")
                        .getChildFile("Players");
                        
    return appDataDir.getChildFile(playerId + ".tracktionplayer");
}

juce::var Player::convertPatternStringToChart(const juce::String& patternString, double tempo)
{
    if (musician)
    {
        // Use the musician's conversion method (same as working Player_OLD.cpp)
        return musician->convertNotationToMidiEvents(patternString, tempo);
    }
    
    // Return failure result if no musician available
    juce::DynamicObject::Ptr result = new juce::DynamicObject();
    result->setProperty("success", false);
    result->setProperty("tempo", tempo);
    result->setProperty("events", juce::Array<juce::var>());
    return juce::var(result.get());
}

const InstrumentCapabilities& Player::getInstrumentCapabilities() const
{
    return instrumentCapabilities;
}

void Player::setInstrumentCapabilities(const InstrumentCapabilities& capabilities)
{
    instrumentCapabilities = capabilities;
}

PlayerMetadata Player::getMetadata() const
{
    return metadata;
}

PlayerMetadata& Player::getMetadata()
{
    return metadata;
}

void Player::setMetadata(const PlayerMetadata& newMetadata)
{
    metadata = newMetadata;
}

bool Player::play()
{
    return playLoadedChart();
}

// =========================================================================
// Legacy Player Compatibility Methods (for PlayerEditorComponent)
// =========================================================================

void Player::prepareForEditing()
{
    // Initialize if not already done
    if (!initialized)
    {
        initialize();
    }
}

std::unique_ptr<juce::XmlElement> Player::getPluginGraphState() const
{
    // Delegate to instrument to get plugin state
    if (instrument)
    {
        return instrument->getPluginState();
    }
    return std::make_unique<juce::XmlElement>("PluginGraph");
}

bool Player::isConnectedToAudioEngine() const
{
    return initialized;
}

void Player::disconnectFromAudioEngine()
{
    // Delegate to instrument
    if (instrument)
    {
        instrument->unregisterFromAudioEngine();
    }
}

void Player::releaseResources()
{
    if (instrument)
    {
        instrument->unregisterFromAudioEngine();
    }
}

void Player::loadPluginGraph(const juce::XmlElement& graphXml)
{
    // Delegate to instrument
    if (instrument)
    {
        instrument->restorePluginState(graphXml);
    }
}

void Player::setPlayerState(const juce::var& state)
{
    storedPlayerState = state;
}

juce::var Player::getPlayerState() const
{
    return storedPlayerState;
}

bool Player::hasPlayerState() const
{
    return !storedPlayerState.isVoid();
}

InstrumentCapabilities Player::createDrumCapabilities()
{
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::Mapping;
    caps.instrumentType = "drums";
    caps.soundMappings = {
        {"kick", 36}, {"snare", 38}, {"hihat", 42}, {"openHihat", 46},
        {"crash", 49}, {"ride", 51}, {"rimshot", 37}
    };
    return caps;
}

InstrumentCapabilities Player::createPianoCapabilities()
{
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::NoteRange;
    caps.instrumentType = "piano";
    caps.minNote = 21;  // A0
    caps.maxNote = 108; // C8
    return caps;
}

InstrumentCapabilities Player::createBassCapabilities()
{
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::NoteRange;
    caps.instrumentType = "bass";
    caps.minNote = 28;  // E1
    caps.maxNote = 67;  // G4
    return caps;
}

InstrumentCapabilities Player::createGuitarCapabilities()
{
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::NoteRange;
    caps.instrumentType = "guitar";
    caps.minNote = 40;  // E2
    caps.maxNote = 84;  // C6
    return caps;
}

void Player::setLevel(float level)
{
    // Store level in metadata for now (legacy compatibility)
    // In a real implementation, this might adjust the instrument's volume
    (void)level; // Suppress unused parameter warning
}

void Player::setupDefaultMusician()
{
    // Create a basic musician for chart generation
    // TODO: Initialize with proper LLM client when available
    musician = std::make_unique<Musician>(playerName + "_Musician");
    
    DBG("Player " << playerName << " default musician created");
}

} // namespace covertracks
