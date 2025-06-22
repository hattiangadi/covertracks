#include "Player.h"
#include "../../Musician/Core/Musician.h"
#include <iostream>

namespace covertracks {

// InstrumentCapabilities stub implementations
int InstrumentCapabilities::findNote(const String& noteName) const {
    // Stub implementation - just return a default note
    (void)noteName; // Suppress unused parameter warning
    return 60; // Middle C
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
var Player::convertPatternStringToChart(const String&, double) { return var{}; }
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
    obj->setProperty("name", metadata.name);
    obj->setProperty("stub", true);
    return var(obj.get());
}

std::unique_ptr<Player> Player::fromVar(const var&, AudioPluginFormatManager& fm, KnownPluginList& kpl) {
    return std::make_unique<Player>("stub", "Stub Player", fm, kpl);
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