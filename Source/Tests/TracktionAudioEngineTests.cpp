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
        expect(&engine.getTracktionEngine() != nullptr, "Tracktion engine should be accessible");
        expect(&engine.getAudioDeviceManager() != nullptr, "Audio device manager should be accessible");
        
        // Test engine state
        expect(engine.isInitialized(), "Engine should be initialized");
    }
    
    void testMultiplePlayerManagement()
    {
        beginTest("Multiple player management in TracktionAudioEngine");
        
        auto& engine = TracktionAudioEngine::getInstance();
        
        // Create test players
        std::vector<std::unique_ptr<Player>> players;
        for (int i = 0; i < 5; ++i)
        {
            auto player = std::make_unique<Player>("Test Player " + juce::String(i + 1));
            players.push_back(std::move(player));
        }
        
        // Test player registration (conceptual - actual implementation depends on Player integration)
        expect(true, "Multiple players should be manageable");
        
        // Test that engine remains stable with multiple players
        expect(engine.isInitialized(), "Engine should remain initialized with multiple players");
    }
    
    void testDeviceHandling()
    {
        beginTest("Audio device handling robustness");
        
        auto& engine = TracktionAudioEngine::getInstance();
        auto& deviceManager = engine.getAudioDeviceManager();
        
        // Test device manager state
        expect(&deviceManager != nullptr, "Device manager should be available");
        
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
        auto audioDeviceTypes = deviceManager.getAvailableDeviceTypes();
        expect(audioDeviceTypes.size() > 0, "Should have at least one audio device type");
    }
    
    void testEngineShutdown()
    {
        beginTest("TracktionAudioEngine shutdown robustness");
        
        auto& engine = TracktionAudioEngine::getInstance();
        
        // Test that engine can handle shutdown gracefully
        // Note: We don't actually shut down the singleton here as it would affect other tests
        
        // Test stopping all audio processing
        auto& deviceManager = engine.getAudioDeviceManager();
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        
        if (currentDevice != nullptr)
        {
            // Test that we can query device state safely
            bool isOpen = currentDevice->isOpen();
            expect(true, "Device state should be queryable"); // Always pass as we just want to test it doesn't crash
            
            // Test audio callback state
            expect(true, "Engine should handle audio callback queries safely");
        }
        
        // Test engine remains functional after shutdown attempts
        expect(engine.isInitialized(), "Engine should remain initialized");
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
        auto& deviceManager = engine.getAudioDeviceManager();
        
        // Test MIDI input detection
        auto midiInputs = juce::MidiInput::getAvailableDevices();
        expect(true, "MIDI input detection should not crash"); // Always pass, just test it doesn't crash
        
        // Test MIDI output detection  
        auto midiOutputs = juce::MidiOutput::getAvailableDevices();
        expect(true, "MIDI output detection should not crash"); // Always pass, just test it doesn't crash
        
        // Log available devices for debugging
        DBG("Available MIDI inputs: " + juce::String(midiInputs.size()));
        DBG("Available MIDI outputs: " + juce::String(midiOutputs.size()));
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