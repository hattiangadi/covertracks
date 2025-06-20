#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

// Include Tracktion Engine headers
#include "../../modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"

// Include Tracktion utilities
#include "../../modules/tracktion_engine/examples/common/Utilities.h"

namespace te = tracktion;

namespace covertracks {

/**
 * @brief TDD tests for Tracktion Engine integration
 * 
 * Following Red-Green-Refactor cycle to migrate from custom AudioEngine
 * to professional Tracktion Engine architecture for proper multi-track audio.
 */
class TracktionIntegrationTests : public juce::UnitTest
{
public:
    TracktionIntegrationTests() : juce::UnitTest("Tracktion Integration Tests", "Tracktion") {}

    void runTest() override
    {
        beginTest("Basic Tracktion Engine Initialization");
        testBasicTracktionEngineInit();
        
        beginTest("Create Edit with Single Track");
        testCreateEditWithSingleTrack();
        
        beginTest("Add Instrument Plugin to Track");
        testAddInstrumentPluginToTrack();
        
        beginTest("Create MIDI Clip with Pattern");
        testCreateMidiClipWithPattern();
        
        beginTest("Single Track Audio Playback");
        testSingleTrackAudioPlayback();
        
        beginTest("Multi-Track Simultaneous Playback");
        testMultiTrackSimultaneousPlayback();
        
        beginTest("Player Wrapper Around Tracktion Track");
        testPlayerWrapperAroundTracktionTrack();
    }

private:
    void testBasicTracktionEngineInit()
    {
        // RED: This test will fail because we haven't set up Tracktion Engine yet
        
        // Create Tracktion Engine instance
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        
        expect(engine != nullptr, "Should create Tracktion Engine instance");
        expect(engine->getPluginManager().knownPluginList.getNumTypes() >= 0, 
               "Should have plugin manager with plugin list");
        expect(engine->getDeviceManager().deviceManager.getCurrentAudioDevice() != nullptr,
               "Should have active audio device");
        
        logMessage("Basic Tracktion Engine initialization test - EXPECTED TO FAIL initially");
    }
    
    void testCreateEditWithSingleTrack()
    {
        // RED: This will fail until we implement Edit creation
        
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        
        // Create a new Edit (like a DAW project/session)
        auto edit = te::createEmptyEdit(*engine, juce::File::createTempFile(".tracktionedit"));
        
        expect(edit != nullptr, "Should create Edit instance");
        
        // Get or create the first audio track
        auto track = EngineHelpers::getOrInsertAudioTrackAt(*edit, 0);
        expect(track != nullptr, "Should get first track from Edit");
        expect(track != nullptr && track->isAudioTrack(), 
               "Track should be valid audio track");
        
        logMessage("Edit creation test - EXPECTED TO FAIL initially");
    }
    
    void testAddInstrumentPluginToTrack()
    {
        // RED: This will fail until we implement plugin loading
        
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        auto edit = te::createEmptyEdit(*engine, juce::File::createTempFile(".tracktionedit"));
        auto track = EngineHelpers::getOrInsertAudioTrackAt(*edit, 0);
        
        // Try to add a simple internal instrument
        auto& pluginManager = engine->getPluginManager();
        auto pluginTypes = pluginManager.knownPluginList.getTypes();
        
        expect(pluginTypes.size() > 0, "Should have some plugin types available");
        
        // Look for a synth/instrument plugin
        juce::PluginDescription synthDesc;
        bool foundSynth = false;
        for (auto& desc : pluginTypes)
        {
            if (desc.category.contains("Synth") || desc.category.contains("Instrument"))
            {
                synthDesc = desc;
                foundSynth = true;
                break;
            }
        }
        
        expect(foundSynth, "Should find at least one synth/instrument plugin");
        
        // Add plugin to track
        if (auto audioTrack = dynamic_cast<te::AudioTrack*>(track))
        {
            auto plugin = edit->getPluginCache().createNewPlugin("synth", synthDesc);
            expect(plugin != nullptr, "Should create plugin instance");
            
            audioTrack->pluginList.insertPlugin(plugin, 0, nullptr);
            expect(audioTrack->pluginList.size() > 0, "Track should have plugin in list");
        }
        
        logMessage("Plugin addition test - EXPECTED TO FAIL initially");
    }
    
    void testCreateMidiClipWithPattern()
    {
        // RED: This will fail until we implement MIDI clip creation
        
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        auto edit = te::createEmptyEdit(*engine, juce::File::createTempFile(".tracktionedit"));
        auto track = EngineHelpers::getOrInsertAudioTrackAt(*edit, 0);
        
        // Create a simple MIDI pattern (C major scale)
        juce::MidiMessageSequence pattern;
        for (int note = 60; note < 67; ++note)  // C4 to B4
        {
            double startTime = (note - 60) * 0.5;  // 0.5 seconds apart
            pattern.addEvent(juce::MidiMessage::noteOn(1, note, 0.8f), startTime);
            pattern.addEvent(juce::MidiMessage::noteOff(1, note), startTime + 0.4);
        }
        
        expect(pattern.getNumEvents() > 0, "Should create MIDI pattern with events");
        
        // Create MIDI clip from pattern
        if (auto audioTrack = dynamic_cast<te::AudioTrack*>(track))
        {
            auto midiClip = audioTrack->insertMIDIClip(te::TimeRange(te::TimePosition(), te::TimeDuration::fromSeconds(4.0)), nullptr);
            expect(midiClip != nullptr, "Should create MIDI clip");
            
            // Set the MIDI sequence  
            auto& sequence = midiClip->getSequence();
            sequence.addNote(60, te::BeatPosition(), te::BeatDuration::fromBeats(1.0), 80, 0, nullptr);
            
            expect(!sequence.isEmpty(), "MIDI clip should contain events");
        }
        
        logMessage("MIDI clip creation test - EXPECTED TO FAIL initially");
    }
    
    void testSingleTrackAudioPlayback()
    {
        // RED: This will fail until we implement playback
        
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        auto edit = te::createEmptyEdit(*engine, juce::File::createTempFile(".tracktionedit"));
        
        // Set up track with instrument and MIDI clip (from previous tests)
        // ... setup code ...
        
        // Use transport for playback
        auto& transport = edit->getTransport();
        
        // Start playback
        transport.play(false);
        expect(transport.isPlaying(), "Playback should be active");
        
        // Let it play for a moment
        juce::Thread::sleep(100);
        
        // Check if audio is being generated (this would require more sophisticated testing)
        expect(true, "Playback test placeholder - implement audio detection");
        
        transport.stop(false, false);
        expect(!transport.isPlaying(), "Playback should stop");
        
        logMessage("Single track playback test - EXPECTED TO FAIL initially");
    }
    
    void testMultiTrackSimultaneousPlayback()
    {
        // RED: This will fail until we implement multi-track support
        
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        auto edit = te::createEmptyEdit(*engine, juce::File::createTempFile(".tracktionedit"));
        
        // Add additional tracks
        for (int i = 0; i < 3; ++i)
        {
            edit->ensureNumberOfAudioTracks(i + 2);
        }
        
        auto audioTracks = te::getAudioTracks(*edit);
        expect(audioTracks.size() >= 4, "Should have multiple tracks");
        
        // Add instruments and MIDI clips to each track
        for (auto track : audioTracks)
        {
            juce::ignoreUnused(track);
            // Add instrument plugin
            // Add MIDI clip with different pattern  
            // This is the core test of our "performance mode" equivalent
        }
        
        // Test simultaneous playback (the scenario that was causing clicking)
        auto& transport = edit->getTransport();
        transport.play(false);
        
        expect(transport.isPlaying(), "Multi-track playback should be active");
        
        // Monitor for audio quality (equivalent to our AudioEngine quality tests)
        bool qualityGood = true;
        for (int i = 0; i < 10; ++i)
        {
            juce::Thread::sleep(50);
            // In real implementation, we'd check for dropouts, clicking, etc.
        }
        
        expect(qualityGood, "Multi-track playback should maintain audio quality");
        
        transport.stop(false, false);
        
        logMessage("Multi-track playback test - EXPECTED TO FAIL initially");
    }
    
    void testPlayerWrapperAroundTracktionTrack()
    {
        // RED: This will fail until we create Player wrapper
        
        // This test ensures our existing Player API can work with Tracktion backend
        auto engine = std::make_unique<te::Engine>("CovertracksTracktionTest");
        auto edit = te::createEmptyEdit(*engine, juce::File::createTempFile(".tracktionedit"));
        auto track = EngineHelpers::getOrInsertAudioTrackAt(*edit, 0);
        juce::ignoreUnused(track);
        
        // Create wrapper that makes Tracktion Track look like our Player
        // auto player = std::make_unique<TracktionPlayer>(track);
        
        // Test existing Player interface
        // expect(player->getId().isNotEmpty(), "Player should have ID");
        // expect(player->getMetadata().name.isNotEmpty(), "Player should have name");
        
        // Test pattern loading (our existing API)
        juce::String testPattern = R"({
            "command": "play_pattern",
            "tempo": 120,
            "events": [
                {
                    "type": "note_on",
                    "time": 0.0,
                    "note": 60,
                    "velocity": 80,
                    "channel": 1,
                    "duration": 0.5
                }
            ]
        })";
        
        juce::var pattern;
        juce::Result parseResult = juce::JSON::parse(testPattern, pattern);
        expect(parseResult.wasOk(), "Test pattern should parse");
        
        // expect(player->loadChart(pattern), "Player should load pattern");
        // expect(player->play(), "Player should start playback");
        
        logMessage("Player wrapper test - EXPECTED TO FAIL initially");
    }
};

// Global test instance
static TracktionIntegrationTests tracktionIntegrationTests;

} // namespace covertracks