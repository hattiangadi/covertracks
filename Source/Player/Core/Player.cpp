#include "Player.h"
#include "../../Musician/Core/Musician.h"
#include <iostream>

namespace covertracks {

// InstrumentCapabilities implementations
var InstrumentCapabilities::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", (int)type);
    obj->setProperty("instrumentType", instrumentType);
    obj->setProperty("minNote", minNote);
    obj->setProperty("maxNote", maxNote);
    
    // Serialize sound mappings
    juce::DynamicObject::Ptr mappingsObj = new juce::DynamicObject();
    for (const auto& mapping : soundMappings) {
        mappingsObj->setProperty(mapping.first, mapping.second);
    }
    obj->setProperty("soundMappings", juce::var(mappingsObj.get()));
    
    return juce::var(obj.get());
}

void InstrumentCapabilities::fromVar(const var& v) {
    if (!v.isObject()) return;
    
    auto* obj = v.getDynamicObject();
    if (!obj) return;
    
    type = (InstrumentCapabilityType)(int)obj->getProperty("type");
    instrumentType = obj->getProperty("instrumentType").toString();
    minNote = obj->getProperty("minNote");
    maxNote = obj->getProperty("maxNote");
    
    // Deserialize sound mappings
    soundMappings.clear();
    auto mappingsVar = obj->getProperty("soundMappings");
    if (mappingsVar.isObject()) {
        auto* mappingsObj = mappingsVar.getDynamicObject();
        if (mappingsObj) {
            auto& properties = mappingsObj->getProperties();
            for (int i = 0; i < properties.size(); ++i) {
                auto name = properties.getName(i).toString();
                int note = properties.getValueAt(i);
                soundMappings[name] = note;
            }
        }
    }
}

int InstrumentCapabilities::findNote(const String& noteName) const {
    if (type == InstrumentCapabilityType::Mapping) {
        auto it = soundMappings.find(noteName);
        return (it != soundMappings.end()) ? it->second : -1;
    }
    // For range type, just return a default note within range
    return jlimit(minNote, maxNote, 60); // Middle C if in range
}

// PlayerMetadata implementations
var PlayerMetadata::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("description", description);
    obj->setProperty("created", (int64)created.toMilliseconds());
    obj->setProperty("lastModified", (int64)lastModified.toMilliseconds());
    obj->setProperty("capabilities", capabilities.toVar());
    return juce::var(obj.get());
}

void PlayerMetadata::fromVar(const var& v) {
    if (!v.isObject()) return;
    
    auto* obj = v.getDynamicObject();
    if (!obj) return;
    
    name = obj->getProperty("name").toString();
    category = obj->getProperty("category").toString();
    description = obj->getProperty("description").toString();
    created = Time(obj->getProperty("created"));
    lastModified = Time(obj->getProperty("lastModified"));
    capabilities.fromVar(obj->getProperty("capabilities"));
}

// Minimal stub implementations to satisfy linker while we transition to TracktionPlayer

Player::Player(const String& playerId, const String& name, 
               AudioPluginFormatManager& fmgr,
               KnownPluginList& kpl)
    : AudioProcessor(getDefaultBusesProperties()), id(playerId), formatManager(fmgr), knownPluginList(kpl)
{
    metadata.name = name;
    // Create a basic musician for compatibility
    musician = std::make_unique<Musician>(name + "_Musician");
    std::cout << "WARNING: Using stub Player implementation - migrate to TracktionPlayer" << std::endl;
}

Player::~Player() = default;

bool Player::initialize() { return true; }
bool Player::loadPluginGraph(const XmlElement&) { return true; }
bool Player::saveGraphState() { return true; }
std::unique_ptr<XmlElement> Player::getPluginGraphState() { return nullptr; }

te::Track* Player::getTracktionTrack() { return nullptr; }
bool Player::addTracktionPlugin(const juce::PluginDescription&) { return false; }
void Player::clearTracktionPlugins() {}
int Player::getTracktionPluginCount() const { return 0; }

void Player::generateNewChart(const var&, std::function<void(bool)>) {}
bool Player::loadChart(const var&) { return false; }
bool Player::loadChartFromMusician() { return false; }
var Player::convertPatternStringToChart(const String& patternString, double tempo) { 
    if (musician != nullptr)
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
void Player::generateChart(const var&, std::function<void(bool, const var&)> callback) { 
    if (callback) callback(false, var{});
}
void Player::modifyCurrentChart(const String&, std::function<void(bool)>) {}
const var& Player::getCurrentChart() const { static var empty; return empty; }

bool Player::play() { return false; }
void Player::stop() {}
void Player::generateAndLoad(const var&, std::function<void(bool)>) {}
void Player::stopPlayback() {}
void Player::setChartFinishedCallback(std::function<void(const String&)>) {}

void Player::setMidiCallback(std::function<void(const juce::MidiMessage&)>) {}
void Player::clearMidiCallback() {}
bool Player::hasMidiCallback() const { return false; }
void Player::processMidiMessage(const juce::MidiMessage&) {}

var Player::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("metadata", metadata.toVar());
    obj->setProperty("stub", true);
    
    // Include TracktionPlayer state if available
    if (!tracktionPlayerState.isVoid())
    {
        obj->setProperty("tracktionPlayerState", tracktionPlayerState);
        std::cout << "Player::toVar() - Including TracktionPlayer state and capabilities in save data" << std::endl;
    }
    else
    {
        std::cout << "Player::toVar() - No TracktionPlayer state to save, but including capabilities" << std::endl;
    }
    
    return var(obj.get());
}

std::unique_ptr<Player> Player::fromVar(const var& data, AudioPluginFormatManager& fm, KnownPluginList& kpl) {
    if (!data.isObject()) {
        return std::make_unique<Player>("stub", "Stub Player", fm, kpl);
    }
    
    auto* obj = data.getDynamicObject();
    if (!obj) {
        return std::make_unique<Player>("stub", "Stub Player", fm, kpl);
    }
    
    // Extract basic properties
    String playerId = obj->getProperty("id").toString();
    
    if (playerId.isEmpty()) playerId = "restored_player";
    
    // Get player name from metadata if available, otherwise use fallback
    String playerName = "Restored Player";
    auto metadataVar = obj->getProperty("metadata");
    if (metadataVar.isObject()) {
        auto* metadataObj = metadataVar.getDynamicObject();
        if (metadataObj) {
            playerName = metadataObj->getProperty("name").toString();
        }
    }
    if (playerName.isEmpty()) playerName = "Restored Player";
    
    // Create the player
    auto player = std::make_unique<Player>(playerId, playerName, fm, kpl);
    
    // Restore complete metadata (including capabilities)
    if (metadataVar.isObject()) {
        player->metadata.fromVar(metadataVar);
        std::cout << "Player::fromVar() - Restored metadata including capabilities" << std::endl;
    } else {
        // Fallback to old format
        player->metadata.name = obj->getProperty("name").toString();
        player->metadata.category = obj->getProperty("category").toString();
        player->metadata.description = obj->getProperty("description").toString();
        player->metadata.created = Time(obj->getProperty("created"));
        player->metadata.lastModified = Time(obj->getProperty("lastModified"));
        std::cout << "Player::fromVar() - Using fallback metadata format (no capabilities)" << std::endl;
    }
    
    // Restore TracktionPlayer state if available
    auto tracktionState = obj->getProperty("tracktionPlayerState");
    if (!tracktionState.isVoid())
    {
        player->setTracktionPlayerState(tracktionState);
        std::cout << "Player::fromVar() - Restored TracktionPlayer state" << std::endl;
    }
    else
    {
        std::cout << "Player::fromVar() - No TracktionPlayer state found in save data" << std::endl;
    }
    
    return player;
}

// Static capability creation methods
InstrumentCapabilities Player::createDrumCapabilities() {
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::Mapping;
    caps.instrumentType = "drums";
    return caps;
}

InstrumentCapabilities Player::createPianoCapabilities(int minNote, int maxNote) {
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::NoteRange;
    caps.instrumentType = "piano";
    caps.minNote = minNote;
    caps.maxNote = maxNote;
    return caps;
}

InstrumentCapabilities Player::createBassCapabilities(int minNote, int maxNote) {
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::NoteRange;
    caps.instrumentType = "bass";
    caps.minNote = minNote;
    caps.maxNote = maxNote;
    return caps;
}

InstrumentCapabilities Player::createGuitarCapabilities(int minNote, int maxNote) {
    InstrumentCapabilities caps;
    caps.type = InstrumentCapabilityType::NoteRange;
    caps.instrumentType = "guitar";
    caps.minNote = minNote;
    caps.maxNote = maxNote;
    return caps;
}

void Player::setMusician(std::unique_ptr<Musician> newMusician) { 
    musician = std::move(newMusician);
}

void Player::setLLMClient(std::shared_ptr<LLMClient> client) {
    if (musician) {
        musician->setLLMClient(client);
    }
}


// AudioProcessor interface stubs
void Player::prepareToPlay(double, int) {}
void Player::releaseResources() {}
void Player::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) {}
bool Player::isBusesLayoutSupported(const BusesLayout&) const { return true; }
AudioProcessor::BusesProperties Player::getDefaultBusesProperties() { 
    return AudioProcessor::BusesProperties(); 
}
const juce::String Player::getName() const { return metadata.name; }
void Player::startPlayback() {}
bool Player::isPlaying() const { return false; }

bool Player::connectToAudioEngine(AudioEngineImpl*) { return true; }
void Player::disconnectFromAudioEngine() {}
bool Player::isConnectedToAudioEngine() const { return false; }
bool Player::isFullyLoaded() const { return true; }
bool Player::isReadyForPlayback() const { return true; }
bool Player::isReadyForEditing() const { return true; }
void Player::loadMinimalResources() {}
void Player::prepareForEditing() {}
void Player::prepareForPlayback() {}
bool Player::initializeMinimal() { return true; }
bool Player::loadGraphStateIfNeeded() { return true; }

} // namespace covertracks