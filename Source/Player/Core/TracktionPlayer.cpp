#include "TracktionPlayer.h"
#include <iostream>

namespace covertracks {

TracktionPlayer::TracktionPlayer(const juce::String& id,
                                const juce::String& name,
                                juce::AudioPluginFormatManager& fm,
                                juce::KnownPluginList& kpl)
    : playerId(id)
    , playerName(name)
    , formatManager(fm)
    , knownPluginList(kpl)
{
    // Create TracktionInstrument - it owns all audio concerns
    instrument = std::make_unique<TracktionInstrument>(name + "_Instrument", formatManager, knownPluginList);
    
    // Setup default musician for chart generation
    setupDefaultMusician();
    
    DBG("TracktionPlayer created: " << name << " (ID: " << id << ")");
}

TracktionPlayer::~TracktionPlayer()
{
    shutdown();
    DBG("TracktionPlayer destroyed: " << playerName);
}

// =========================================================================
// High-Level Orchestration (Musician ↔ Instrument)
// =========================================================================

void TracktionPlayer::generateAndPlay(const juce::var& parameters, 
                                     std::function<void(bool success)> callback)
{
    if (!initialized)
    {
        std::cerr << "TracktionPlayer not initialized" << std::endl;
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
        
        // Step 2: Play chart using TracktionInstrument
        bool playSuccess = instrument->playChart(chart);
        if (!playSuccess)
        {
            std::cerr << "Chart playback failed" << std::endl;
        }
        
        if (callback) callback(playSuccess);
    });
}

bool TracktionPlayer::playChart(const juce::var& chart, double tempo)
{
    if (!initialized || !instrument)
    {
        std::cerr << "TracktionPlayer not initialized" << std::endl;
        return false;
    }
    
    // Store chart and delegate to instrument
    lastGeneratedChart = chart;
    return instrument->playChart(chart, tempo);
}

void TracktionPlayer::generateChart(const juce::var& parameters,
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
            DBG("TracktionPlayer " << playerName << " generated chart with " 
                << (chart.isArray() ? chart.getArray()->size() : 0) << " events");
        }
        
        if (callback) callback(success, chart);
    });
}

// =========================================================================
// Audio Delegation (Pass-through to TracktionInstrument)
// =========================================================================

bool TracktionPlayer::addPlugin(const juce::PluginDescription& pluginDesc)
{
    if (!instrument)
        return false;
        
    return instrument->addPlugin(pluginDesc);
}

void TracktionPlayer::clearPlugins()
{
    if (instrument)
        instrument->clearPlugins();
}

int TracktionPlayer::getPluginCount() const
{
    return instrument ? instrument->getPluginCount() : 0;
}

juce::StringArray TracktionPlayer::getPluginNames() const
{
    return instrument ? instrument->getPluginNames() : juce::StringArray{};
}

void TracktionPlayer::stop()
{
    if (instrument)
        instrument->stopPlayback();
}

void TracktionPlayer::pause()
{
    if (instrument)
        instrument->pausePlayback();
}

void TracktionPlayer::resume()
{
    if (instrument)
        instrument->resumePlayback();
}

bool TracktionPlayer::isPlaying() const
{
    return instrument ? instrument->isPlaying() : false;
}

bool TracktionPlayer::isPaused() const
{
    return instrument ? instrument->isPaused() : false;
}

void TracktionPlayer::processMidiMessage(const juce::MidiMessage& message)
{
    if (instrument)
        instrument->processMidiMessage(message);
}

// =========================================================================
// Component Access (for advanced use)
// =========================================================================

void TracktionPlayer::setMusician(std::unique_ptr<Musician> newMusician)
{
    musician = std::move(newMusician);
    DBG("TracktionPlayer " << playerName << " musician updated");
}

void TracktionPlayer::setLLMClient(std::shared_ptr<LLMClient> client)
{
    if (musician)
    {
        musician->setLLMClient(client);
        DBG("TracktionPlayer " << playerName << " LLM client set");
    }
}

// =========================================================================
// Persistence (Player State Management)
// =========================================================================

bool TracktionPlayer::saveState()
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
        DBG("TracktionPlayer " << playerName << " state saved to: " << stateFile.getFullPathName());
        return true;
    }
    else
    {
        std::cerr << "Failed to write state file: " << stateFile.getFullPathName() << std::endl;
        return false;
    }
}

bool TracktionPlayer::loadState()
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
        DBG("TracktionPlayer " << playerName << " state loaded from: " << stateFile.getFullPathName());
    }
    
    return success;
}

std::unique_ptr<juce::XmlElement> TracktionPlayer::exportState() const
{
    auto state = std::make_unique<juce::XmlElement>("TracktionPlayerState");
    
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

bool TracktionPlayer::importState(const juce::XmlElement& state)
{
    if (state.getTagName() != "TracktionPlayerState")
    {
        std::cerr << "Invalid state XML for TracktionPlayer" << std::endl;
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
    auto* instrumentStateElement = state.getChildByName("TracktionInstrumentState");
    if (instrumentStateElement && instrument)
    {
        instrument->restorePluginState(*instrumentStateElement);
    }
    
    // TODO: Restore musician state when supported
    // TODO: Restore last chart when needed
    
    DBG("TracktionPlayer " << playerName << " state imported");
    return true;
}

juce::var TracktionPlayer::toVar() const
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

std::unique_ptr<TracktionPlayer> TracktionPlayer::fromVar(const juce::var& data,
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
        
    auto player = std::make_unique<TracktionPlayer>(id, name, formatManager, knownPluginList);
    
    // TODO: Restore additional state when needed
    
    return player;
}

// =========================================================================
// Lifecycle Management
// =========================================================================

bool TracktionPlayer::initialize()
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
    DBG("TracktionPlayer " << playerName << " initialized successfully");
    return true;
}

void TracktionPlayer::shutdown()
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
    DBG("TracktionPlayer " << playerName << " shutdown");
}

// =========================================================================
// Comprehensive State Persistence (NEW)
// =========================================================================

juce::var TracktionPlayer::saveCompleteState() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    // Player metadata
    obj->setProperty("id", playerId);
    obj->setProperty("name", playerName);
    obj->setProperty("initialized", initialized);
    obj->setProperty("version", "1.0");
    obj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    
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

bool TracktionPlayer::loadCompleteState(const juce::var& state)
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
    
    DBG("TracktionPlayer " << playerName << " complete state loaded successfully");
    return true;
}

juce::var TracktionPlayer::savePluginChain() const
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

bool TracktionPlayer::loadPluginChain(const juce::var& pluginChainState)
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

bool TracktionPlayer::exportAsPreset(const juce::File& file) const
{
    auto state = saveCompleteState();
    
    // Add preset metadata
    if (auto* obj = state.getDynamicObject())
    {
        obj->setProperty("type", "TracktionPlayerPreset");
        obj->setProperty("exportedAt", juce::Time::getCurrentTime().toISO8601(true));
        obj->setProperty("application", "Covertracks");
    }
    
    // Save as JSON
    auto json = juce::JSON::toString(state);
    
    if (file.replaceWithText(json))
    {
        DBG("TracktionPlayer preset exported to: " << file.getFullPathName());
        return true;
    }
    else
    {
        std::cerr << "Failed to export preset to: " << file.getFullPathName() << std::endl;
        return false;
    }
}

bool TracktionPlayer::importFromPreset(const juce::File& file)
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
        if (type != "TracktionPlayerPreset")
        {
            std::cerr << "Invalid preset type: " << type << std::endl;
            return false;
        }
    }
    
    if (loadCompleteState(state))
    {
        DBG("TracktionPlayer preset imported from: " << file.getFullPathName());
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

juce::File TracktionPlayer::getStateFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("Covertracks")
                        .getChildFile("Players");
                        
    return appDataDir.getChildFile(playerId + ".tracktionplayer");
}

void TracktionPlayer::setupDefaultMusician()
{
    // Create a basic musician for chart generation
    // TODO: Initialize with proper LLM client when available
    musician = std::make_unique<Musician>(playerName + "_Musician");
    
    DBG("TracktionPlayer " << playerName << " default musician created");
}

} // namespace covertracks
