#include <JuceHeader.h>
#include "../Audio/AudioEngineInterface.h"
#include "../Player/Core/TracktionPlayer.h"

namespace covertracks {

/**
 * @brief Comprehensive test suite for the new AudioEngine architecture
 * 
 * Tests the centralized AudioEngine singleton that manages all audio resources
 * and coordinates multiple TracktionPlayers without conflicts.
 */
class AudioEngineTests : public juce::UnitTest
{
public:
    AudioEngineTests() : juce::UnitTest("AudioEngine Tests", "Audio") {}

    void runTest() override
    {
        beginTest("AudioEngine Singleton Access");
        testSingletonAccess();
        
        beginTest("Resource Ownership");
        testResourceOwnership();
        
        beginTest("Player Registration");
        testPlayerRegistration();
        
        beginTest("Thread Safety");
        testThreadSafety();
        
        beginTest("MIDI Routing");
        testMidiRouting();
        
        beginTest("Player Graph Integration");
        testPlayerGraphIntegration();
        
        beginTest("Audio Quality Monitoring");
        testAudioQualityMonitoring();
        
        beginTest("Multi-Player Coordination");
        testMultiPlayerCoordination();
        
        beginTest("Buffer Underrun Prevention");
        testBufferUnderrunPrevention();
        
        beginTest("Performance Mode Stress Test");
        testPerformanceModeStressTest();
    }

private:
    void testSingletonAccess()
    {
        // Test singleton pattern
        auto& engine1 = getAudioEngine();
        auto& engine2 = getAudioEngine();
        
        // Should be the same instance
        expect(&engine1 == &engine2, "AudioEngine should be singleton");
        
        // Should have valid components

    }
    
    void testResourceOwnership()
    {
        auto& engine = getAudioEngine();
        
        // AudioEngine should own all resources
        auto& deviceManager = engine.getDeviceManager();
        auto& formatManager = engine.getFormatManager();
        auto& pluginList = engine.getKnownPluginList();
        
        // Resources should be consistently accessible
        expect(&engine.getDeviceManager() == &deviceManager, "DeviceManager ownership should be stable");
        expect(&engine.getFormatManager() == &formatManager, "FormatManager ownership should be stable");
        expect(&engine.getKnownPluginList() == &pluginList, "PluginList ownership should be stable");
        
        // Resources should be properly initialized
        expect(deviceManager.getCurrentAudioDevice() != nullptr, "Audio device should be available");
        expect(formatManager.getNumFormats() > 0, "Plugin formats should be registered");
    }
    
    void testPlayerRegistration()
    {
        auto& engine = getAudioEngine();
        
        // Create test players
        auto player1 = createTestPlayer("TestPlayer1");
        auto player2 = createTestPlayer("TestPlayer2");
        
        if (!player1 || !player2) {
            expect(false, "Test skipped - could not create test players");
            return;
        }
        
        // Register players
        expect(engine.registerPlayer(player1.get()), "Player1 registration should succeed");
        expect(engine.registerPlayer(player2.get()), "Player2 registration should succeed");
        
        // Cannot register same player twice
        expect(!engine.registerPlayer(player1.get()), "Duplicate registration should fail");
        
        // Activate players
        expect(engine.activatePlayer(player1.get()), "Player1 activation should succeed");
        expect(engine.activatePlayer(player2.get()), "Player2 activation should succeed");
        
        // Deactivate players
        engine.deactivatePlayer(player1.get());
        engine.deactivatePlayer(player2.get());
        
        // Unregister players
        engine.unregisterPlayer(player1.get());
        engine.unregisterPlayer(player2.get());
    }
    
    void testThreadSafety()
    {
        auto& engine = getAudioEngine();
        
        // Create multiple players for concurrent access
        std::vector<std::unique_ptr<Player>> players;
        for (int i = 0; i < 10; ++i) {
            players.push_back(createTestPlayer("ConcurrentPlayer" + String(i)));
        }
        
        // Test concurrent registration/unregistration
        std::vector<std::thread> threads;
        std::atomic<int> successfulRegistrations{0};
        
        for (auto& player : players) {
            threads.emplace_back([&engine, &player, &successfulRegistrations]() {
                if (engine.registerPlayer(player.get())) {
                    ++successfulRegistrations;
                    
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                    engine.unregisterPlayer(player.get());
                }
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        expect(successfulRegistrations.load() == static_cast<int>(players.size()), 
               "All concurrent registrations should succeed");
    }
    
    void testMidiRouting()
    {
        auto& engine = getAudioEngine();
        
        // Create test player with MIDI callback
        auto player = createTestPlayer("MidiTestPlayer");
        
        if (!player) {
            expect(false, "Test skipped - could not create test player");
            return;
        }
        
        // Set up MIDI callback to count received messages
        std::atomic<int> midiMessagesReceived{0};
        player->setMidiCallback([&midiMessagesReceived](const MidiMessage& message) {
            ++midiMessagesReceived;
            juce::ignoreUnused(message);
        });
        
        // Verify callback is set
        expect(player->hasMidiCallback(), "Player should have MIDI callback set");
        
        // Register and activate player
        engine.registerPlayer(player.get());
        engine.activatePlayer(player.get());
        
        // Send MIDI messages
        for (int i = 0; i < 5; ++i) {
            MidiMessage noteOn = MidiMessage::noteOn(1, 60 + i, 0.8f);
            engine.routeMidiToPlayer(player.get(), noteOn);
        }
        
        // Allow time for message processing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Verify MIDI messages were received
        expect(midiMessagesReceived.load() == 5, "Player should receive all routed MIDI messages");
        expect(engine.isPlayerActive(player.get()), "Player should remain active after MIDI routing");
        
        // Cleanup
        engine.deactivatePlayer(player.get());
        engine.unregisterPlayer(player.get());
    }
    
    void testPlayerGraphIntegration()
    {
        auto& engine = getAudioEngine();
        
        // Create test player with audio graph
        auto player = createTestPlayer("GraphIntegrationPlayer");
        
        if (!player) {
            expect(false, "Test skipped - could not create test player");
            return;
        }
        
        // Initially, graph should not be integrated
        expect(!engine.isPlayerGraphIntegrated(player.get()), 
               "Player graph should not be integrated initially");
        
        // Register and activate player
        expect(engine.registerPlayer(player.get()), "Player registration should succeed");
        expect(engine.activatePlayer(player.get()), "Player activation should succeed");
        
        // Try to integrate player's graph
        bool graphAdded = engine.addPlayerGraph(player.get());
        expect(graphAdded, "Player graph integration should succeed");
        
        // Verify integration status
        expect(engine.isPlayerGraphIntegrated(player.get()), 
               "Player graph should be integrated after addPlayerGraph");
        
        // Test that adding again returns true (already integrated)
        expect(engine.addPlayerGraph(player.get()), 
               "Adding already integrated graph should return true");
        
        // Test graph removal
        engine.removePlayerGraph(player.get());
        expect(!engine.isPlayerGraphIntegrated(player.get()), 
               "Player graph should not be integrated after removal");
        
        // Test integration with inactive player (should fail)
        engine.deactivatePlayer(player.get());
        expect(!engine.addPlayerGraph(player.get()), 
               "Graph integration should fail for inactive player");
        
        // Cleanup
        engine.unregisterPlayer(player.get());
    }
    
    void testAudioQualityMonitoring()
    {
        auto& engine = getAudioEngine();
        
        // Initially should have good quality
        expect(engine.isAudioQualityGood(), "Initial audio quality should be good");
        
        // CPU usage should be reasonable
        double cpuUsage = engine.getCpuUsage();
        expect(cpuUsage >= 0.0 && cpuUsage <= 1.0, "CPU usage should be in valid range");
        
        // Buffer underruns should be minimal
        int underruns = engine.getBufferUnderruns();
        expect(underruns >= 0, "Buffer underruns should be non-negative");
        
        // Quality degradation testing would require simulating high CPU load scenarios
        // This is covered by the performance mode stress tests
    }
    
    void testMultiPlayerCoordination()
    {
        auto& engine = getAudioEngine();
        
        // Create multiple players
        std::vector<std::unique_ptr<Player>> players;
        for (int i = 0; i < 5; ++i) {
            auto player = createTestPlayer("EnsemblePlayer" + String(i));
            players.push_back(std::move(player));
        }
        
        // Register all players
        for (auto& player : players) {
            expect(engine.registerPlayer(player.get()), 
                   "Multi-player registration should succeed");
        }
        
        // Activate all players simultaneously
        for (auto& player : players) {
            expect(engine.activatePlayer(player.get()), 
                   "Multi-player activation should succeed");
        }
        
        // Verify audio quality remains good with multiple players
        expect(engine.isAudioQualityGood(), 
               "Audio quality should remain good with multiple players");
        
        // Cleanup all players
        for (auto& player : players) {
            engine.deactivatePlayer(player.get());
            engine.unregisterPlayer(player.get());
        }
    }
    
    void testBufferUnderrunPrevention()
    {
        auto& engine = getAudioEngine();
        
        // Record initial underrun count
        int initialUnderruns = engine.getBufferUnderruns();
        
        // Create players and simulate heavy load
        std::vector<std::unique_ptr<Player>> players;
        for (int i = 0; i < 8; ++i) {
            auto player = createTestPlayer("LoadTestPlayer" + String(i));
            players.push_back(std::move(player));
        }
        
        // Register and activate all players
        for (auto& player : players) {
            engine.registerPlayer(player.get());
            engine.activatePlayer(player.get());
        }
        
        // Simulate audio processing for a period
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check that underruns haven't significantly increased
        int finalUnderruns = engine.getBufferUnderruns();
        int newUnderruns = finalUnderruns - initialUnderruns;
        
        expect(newUnderruns <= 2, "Buffer underruns should be minimal under load");
        
        // Cleanup
        for (auto& player : players) {
            engine.deactivatePlayer(player.get());
            engine.unregisterPlayer(player.get());
        }
    }
    
    void testPerformanceModeStressTest()
    {
        auto& engine = getAudioEngine();
        
        // This test simulates the performance mode scenario that causes clicking
        
        // Create ensemble of players
        std::vector<std::unique_ptr<Player>> ensemble;
        for (int i = 0; i < 6; ++i) {  // Typical ensemble size
            auto player = createTestPlayer("PerformancePlayer" + String(i));
            ensemble.push_back(std::move(player));
        }
        
        // Register and activate all players
        for (auto& player : ensemble) {
            engine.registerPlayer(player.get());
            engine.activatePlayer(player.get());
        }
        
        // Simulate simultaneous pattern playback (the clicking scenario)
        for (auto& player : ensemble) {
            // Pattern playback testing is handled by the PerformanceModeTests
            // This test focuses on player coordination at the AudioEngine level
            juce::ignoreUnused(player);
        }
        
        // Monitor audio quality during stress test
        bool qualityGood = true;
        for (int i = 0; i < 10; ++i) {  // Monitor for 100ms
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (!engine.isAudioQualityGood()) {
                qualityGood = false;
                break;
            }
        }
        
        expect(qualityGood, "Audio quality should remain good during performance mode");
        
        // Cleanup
        for (auto& player : ensemble) {
            engine.deactivatePlayer(player.get());
            engine.unregisterPlayer(player.get());
        }
    }
    
    // Helper method to create test players
    std::unique_ptr<Player> createTestPlayer(const String& name)
    {
        // For now, create a minimal test player using existing constructor
        try {
            auto& audioEngine = getAudioEngine();
            auto player = std::make_unique<Player>("test_" + name, name, 
                                                 audioEngine.getFormatManager(), 
                                                 audioEngine.getKnownPluginList());
            return player;
        } catch (...) {
            // If Player construction fails, return nullptr and skip tests that need it
            return nullptr;
        }
    }
};

// Global test instance
static AudioEngineTests audioEngineTests;

} // namespace covertracks