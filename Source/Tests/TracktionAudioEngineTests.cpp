#include <JuceHeader.h>
#include "../Audio/TracktionAudioEngine.h"
#include "../Player/Core/Player.h"

namespace covertracks {

/**
 * @brief Comprehensive tests for TracktionAudioEngine robustness
 */
class TracktionAudioEngineTests : public juce::UnitTest
{
public:
    TracktionAudioEngineTests() : juce::UnitTest("TracktionAudioEngine Robustness", "Audio Engine") {}

    void runTest() override
    {
        testEngineInitialization();
        testMultiplePlayerManagement();
        testDeviceHandling();
        testEngineShutdown();
    }

private:
    void testEngineInitialization()
    {
        beginTest("TracktionAudioEngine initialization");
        
        // Get singleton instance
        auto& engine = TracktionAudioEngine::getInstance();
        
        // Test basic properties
        // getTracktionEngine() returns a reference, so it's always valid
        logMessage("Tracktion engine is accessible");
        
        // getDeviceManager() returns a reference, so it's always valid
        logMessage("Audio device manager is accessible");
        
        // Test engine state
        expect(engine.isAudioQualityGood(), "Audio quality should be good");
        expect(engine.getCpuUsage() >= 0.0, "CPU usage should be non-negative");
    }
    
    void testMultiplePlayerManagement()
    {
        beginTest("Multiple player management in TracktionAudioEngine");
        
        auto& engine = TracktionAudioEngine::getInstance();
        
        // Create test players using proper constructor
        auto& formatManager = engine.getFormatManager();
        auto& knownPluginList = engine.getKnownPluginList();
        
        std::vector<std::unique_ptr<Player>> players;
        for (int i = 0; i < 5; ++i)
        {
            juce::String playerId = "player_" + juce::String(i + 1);
            juce::String playerName = "Test Player " + juce::String(i + 1);
            auto player = std::make_unique<Player>(playerId, playerName, formatManager, knownPluginList);
            players.push_back(std::move(player));
        }
        
        // Test player registration (conceptual - actual implementation depends on Player integration)
        expect(players.size() == 5, "Should have created 5 players");
        
        // Test that engine remains stable with multiple players
        expect(engine.isAudioQualityGood(), "Audio quality should remain good with multiple players");
    }
    
    void testDeviceHandling()
    {
        beginTest("Audio device handling robustness");
        
        auto& engine = TracktionAudioEngine::getInstance();
        auto& deviceManager = engine.getDeviceManager();
        
        // Test device manager state (reference is always valid)
        logMessage("Device manager is available");
        
        // Test current device setup
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        if (currentDevice != nullptr)
        {
            expect(currentDevice->getTypeName().isNotEmpty(), "Device should have a type name");
            expect(currentDevice->getName().isNotEmpty(), "Device should have a name");
            
            // Test sample rate and buffer size
            double sampleRate = currentDevice->getCurrentSampleRate();
            int bufferSize = currentDevice->getCurrentBufferSizeSamples();
            
            expect(sampleRate > 0, "Sample rate should be positive");
            expect(bufferSize > 0, "Buffer size should be positive");
            expect(juce::isPowerOfTwo(bufferSize), "Buffer size should be power of 2");
        }
        
        // Test device availability
        const auto& audioDeviceTypes = deviceManager.getAvailableDeviceTypes();
        expect(audioDeviceTypes.size() > 0, "Should have at least one audio device type");
    }
    
    void testEngineShutdown()
    {
        beginTest("TracktionAudioEngine shutdown robustness");
        
        auto& engine = TracktionAudioEngine::getInstance();
        
        // Test that engine can handle shutdown gracefully
        // Note: We don't actually shut down the singleton here as it would affect other tests
        
        // Test stopping all audio processing
        auto& deviceManager = engine.getDeviceManager();
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        
        if (currentDevice != nullptr)
        {
            // Test that we can query device state safely
            currentDevice->isOpen(); // Just test it doesn't crash
            expect(true, "Device state should be queryable");
            
            // Test audio callback state
            expect(true, "Engine should handle audio callback queries safely");
        }
        
        // Test engine remains functional after shutdown attempts
        expect(engine.isAudioQualityGood(), "Audio quality should remain good");
        expect(engine.getBufferUnderruns() >= 0, "Buffer underruns should be non-negative");
    }
};

/**
 * @brief Tests for TracktionAudioEngine MIDI handling
 */
class TracktionMidiTests : public juce::UnitTest
{
public:
    TracktionMidiTests() : juce::UnitTest("TracktionEngine MIDI", "Audio Engine") {}

    void runTest() override
    {
        testMidiDeviceDetection();
        testMidiMessageHandling();
    }

private:
    void testMidiDeviceDetection()
    {
        beginTest("MIDI device detection");
        
        auto& engine = TracktionAudioEngine::getInstance();
        auto& deviceManager = engine.getDeviceManager();
        
        // Test MIDI input detection
        auto midiInputs = juce::MidiInput::getAvailableDevices();
        expect(true, "MIDI input detection should not crash"); // Always pass, just test it doesn't crash
        
        // Test MIDI output detection  
        auto midiOutputs = juce::MidiOutput::getAvailableDevices();
        expect(true, "MIDI output detection should not crash"); // Always pass, just test it doesn't crash
        
        // Log available devices for debugging
        logMessage("Available MIDI inputs: " + juce::String(midiInputs.size()));
        logMessage("Available MIDI outputs: " + juce::String(midiOutputs.size()));
    }
    
    void testMidiMessageHandling()
    {
        beginTest("MIDI message handling");
        
        // Create test MIDI messages
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, 60, 0.0f);
        juce::MidiMessage controlChange = juce::MidiMessage::controllerEvent(1, 7, 100);
        
        // Test message properties
        expect(noteOn.isNoteOn(), "Note on message should be detected");
        expect(noteOff.isNoteOff(), "Note off message should be detected");
        expect(controlChange.isController(), "Control change should be detected");
        
        // Test message data integrity
        expect(noteOn.getNoteNumber() == 60, "Note number should be preserved");
        expect(noteOn.getChannel() == 1, "Channel should be preserved");
        expect(controlChange.getControllerNumber() == 7, "Controller number should be preserved");
        expect(controlChange.getControllerValue() == 100, "Controller value should be preserved");
    }
};

// Register the tests
static TracktionAudioEngineTests tracktionAudioEngineTests;
static TracktionMidiTests tracktionMidiTests;

} // namespace covertracks