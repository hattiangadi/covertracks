#include <JuceHeader.h>
#include "../Audio/AudioEngineInterface.h"
#include "../Player/Core/Player.h"
#include "../Player/Core/PlayerManager.h"
#include "../Musician/Core/Performance.h"

namespace covertracks {

/**
 * @brief Performance mode validation tests for the new AudioEngine architecture
 * 
 * These tests validate that the AudioEngine singleton properly handles the scenarios
 * that previously caused clicking and audio quality issues in performance mode.
 */
class PerformanceModeTests : public juce::UnitTest
{
public:
    PerformanceModeTests() : juce::UnitTest("Performance Mode Tests", "Performance") {}

    void runTest() override
    {
        beginTest("Ensemble Performance Scenario");
        testEnsemblePerformanceScenario();
        
        beginTest("Simultaneous Pattern Playback");
        testSimultaneousPatternPlayback();
        
        beginTest("Buffer Underrun Prevention");
        testBufferUnderrunPrevention();
        
        beginTest("CPU Usage Monitoring");
        testCpuUsageMonitoring();
        
        beginTest("Multi-Player Resource Coordination");
        testMultiPlayerResourceCoordination();
        
        beginTest("Performance Mode Stress Test");
        testPerformanceModeStressTest();
        
        beginTest("Audio Quality Under Load");
        testAudioQualityUnderLoad();
    }

private:
    void testEnsemblePerformanceScenario()
    {
        auto& audioEngine = getAudioEngine();
        
        // Create a typical ensemble: drums, bass, guitar, piano, strings, vocals
        std::vector<std::unique_ptr<Player>> ensemble;
        std::vector<String> instruments = {"Drums", "Bass", "Guitar", "Piano", "Strings", "Vocals"};
        
        for (const auto& instrumentName : instruments)
        {
            auto player = std::make_unique<Player>(
                "test_" + instrumentName.toLowerCase(),
                instrumentName,
                audioEngine.getFormatManager(),
                audioEngine.getKnownPluginList()
            );
            
            expect(player != nullptr, "Should create " + instrumentName + " player");
            if (player)
            {
                player->initialize();
                player->prepareForPlayback();
                ensemble.push_back(std::move(player));
            }
        }
        
        expect(ensemble.size() == instruments.size(), "Should create all ensemble players");
        
        // Register all players with AudioEngine
        for (auto& player : ensemble)
        {
            bool registered = audioEngine.registerPlayer(player.get());
            expect(registered, "Should register " + player->getMetadata().name + " with AudioEngine");
        }
        
        // Activate all players simultaneously (the scenario that caused clicking)
        for (auto& player : ensemble)
        {
            bool activated = audioEngine.activatePlayer(player.get());
            expect(activated, "Should activate " + player->getMetadata().name);
        }
        
        // Verify audio quality remains good with full ensemble
        expect(audioEngine.isAudioQualityGood(), "Audio quality should remain good with full ensemble");
        
        // Check CPU usage is reasonable
        double cpuUsage = audioEngine.getCpuUsage();
        expect(cpuUsage < 0.8, "CPU usage should be reasonable with full ensemble: " + String(cpuUsage));
        
        // Clean up
        for (auto& player : ensemble)
        {
            audioEngine.deactivatePlayer(player.get());
            audioEngine.unregisterPlayer(player.get());
        }
        
        logMessage("Ensemble performance scenario completed successfully");
    }
    
    void testSimultaneousPatternPlayback()
    {
        auto& audioEngine = getAudioEngine();
        
        // Create multiple players for simultaneous playback
        std::vector<std::unique_ptr<Player>> players;
        
        for (int i = 0; i < 4; ++i)
        {
            auto player = std::make_unique<Player>(
                "simul_test_" + String(i),
                "SimulTest" + String(i),
                audioEngine.getFormatManager(),
                audioEngine.getKnownPluginList()
            );
            
            if (player)
            {
                player->initialize();
                player->prepareForPlayback();
                audioEngine.registerPlayer(player.get());
                audioEngine.activatePlayer(player.get());
                players.push_back(std::move(player));
            }
        }
        
        expect(players.size() == 4, "Should create 4 players for simultaneous test");
        
        // Create test patterns for each player
        String testPattern = R"({
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
                },
                {
                    "type": "note_on", 
                    "time": 0.5,
                    "note": 64,
                    "velocity": 80,
                    "channel": 1,
                    "duration": 0.5
                }
            ]
        })";
        
        var pattern;
        Result parseResult = JSON::parse(testPattern, pattern);
        expect(parseResult.wasOk(), "Test pattern should parse correctly");
        
        if (parseResult.wasOk())
        {
            // Record initial buffer underruns
            int initialUnderruns = audioEngine.getBufferUnderruns();
            
            // Load patterns into all players simultaneously
            int loadedCount = 0;
            for (auto& player : players)
            {
                if (player->loadChart(pattern))
                {
                    loadedCount++;
                }
            }
            
            expect(loadedCount == static_cast<int>(players.size()), 
                   "All players should load pattern successfully");
            
            // Start playback on all players simultaneously (the critical test)
            int playingCount = 0;
            for (auto& player : players)
            {
                if (player->play())
                {
                    playingCount++;
                }
            }
            
            expect(playingCount >= 2, "At least some players should start playing");
            
            // Monitor audio quality during simultaneous playback
            bool qualityGood = true;
            for (int i = 0; i < 10; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (!audioEngine.isAudioQualityGood())
                {
                    qualityGood = false;
                    break;
                }
            }
            
            expect(qualityGood, "Audio quality should remain good during simultaneous playback");
            
            // Check that buffer underruns didn't increase significantly
            int finalUnderruns = audioEngine.getBufferUnderruns();
            int newUnderruns = finalUnderruns - initialUnderruns;
            expect(newUnderruns <= 1, "Buffer underruns should be minimal during simultaneous playback");
            
            // Stop all players
            for (auto& player : players)
            {
                player->stop();
            }
        }
        
        // Clean up
        for (auto& player : players)
        {
            audioEngine.deactivatePlayer(player.get());
            audioEngine.unregisterPlayer(player.get());
        }
        
        logMessage("Simultaneous pattern playback test completed");
    }
    
    void testBufferUnderrunPrevention()
    {
        auto& audioEngine = getAudioEngine();
        
        // Record baseline buffer underruns
        int initialUnderruns = audioEngine.getBufferUnderruns();
        
        // Create a heavy load scenario with many players
        std::vector<std::unique_ptr<Player>> heavyLoad;
        
        for (int i = 0; i < 8; ++i)
        {
            auto player = std::make_unique<Player>(
                "load_test_" + String(i),
                "LoadTest" + String(i),
                audioEngine.getFormatManager(),
                audioEngine.getKnownPluginList()
            );
            
            if (player)
            {
                player->initialize();
                player->prepareForPlayback();
                audioEngine.registerPlayer(player.get());
                audioEngine.activatePlayer(player.get());
                heavyLoad.push_back(std::move(player));
            }
        }
        
        expect(heavyLoad.size() >= 6, "Should create significant load with multiple players");
        
        // Simulate heavy audio processing for extended period
        auto startTime = std::chrono::steady_clock::now();
        bool qualityMaintained = true;
        
        while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(200))
        {
            if (!audioEngine.isAudioQualityGood())
            {
                qualityMaintained = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        expect(qualityMaintained, "Audio quality should be maintained under heavy load");
        
        // Check buffer underrun prevention effectiveness
        int finalUnderruns = audioEngine.getBufferUnderruns();
        int newUnderruns = finalUnderruns - initialUnderruns;
        
        expect(newUnderruns <= 3, "Buffer underruns should be well controlled under heavy load");
        
        // Clean up
        for (auto& player : heavyLoad)
        {
            audioEngine.deactivatePlayer(player.get());
            audioEngine.unregisterPlayer(player.get());
        }
        
        logMessage("Buffer underrun prevention test completed - " + String(newUnderruns) + " new underruns");
    }
    
    void testCpuUsageMonitoring()
    {
        auto& audioEngine = getAudioEngine();
        
        // Test CPU usage monitoring accuracy
        double idleCpu = audioEngine.getCpuUsage();
        expect(idleCpu >= 0.0 && idleCpu <= 1.0, "CPU usage should be in valid range when idle");
        
        // Create some load and verify CPU monitoring responds
        std::vector<std::unique_ptr<Player>> cpuTestPlayers;
        
        for (int i = 0; i < 4; ++i)
        {
            auto player = std::make_unique<Player>(
                "cpu_test_" + String(i),
                "CpuTest" + String(i),
                audioEngine.getFormatManager(),
                audioEngine.getKnownPluginList()
            );
            
            if (player)
            {
                player->initialize();
                player->prepareForPlayback();
                audioEngine.registerPlayer(player.get());
                audioEngine.activatePlayer(player.get());
                cpuTestPlayers.push_back(std::move(player));
            }
        }
        
        // Allow some time for CPU usage to be measured under load
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        double loadedCpu = audioEngine.getCpuUsage();
        expect(loadedCpu >= idleCpu, "CPU usage should increase under load");
        expect(loadedCpu <= 1.0, "CPU usage should never exceed 100%");
        
        // Clean up
        for (auto& player : cpuTestPlayers)
        {
            audioEngine.deactivatePlayer(player.get());
            audioEngine.unregisterPlayer(player.get());
        }
        
        logMessage("CPU usage monitoring test completed - Idle: " + String(idleCpu, 3) + 
                  ", Loaded: " + String(loadedCpu, 3));
    }
    
    void testMultiPlayerResourceCoordination()
    {
        auto& audioEngine = getAudioEngine();
        
        // Test that multiple players can coordinate resource access
        auto player1 = std::make_unique<Player>("coord_test_1", "CoordTest1", 
                                               audioEngine.getFormatManager(), 
                                               audioEngine.getKnownPluginList());
        auto player2 = std::make_unique<Player>("coord_test_2", "CoordTest2", 
                                               audioEngine.getFormatManager(), 
                                               audioEngine.getKnownPluginList());
        
        if (player1 && player2)
        {
            player1->initialize();
            player2->initialize();
            player1->prepareForPlayback();
            player2->prepareForPlayback();
            
            // Test concurrent registration
            bool reg1 = audioEngine.registerPlayer(player1.get());
            bool reg2 = audioEngine.registerPlayer(player2.get());
            
            expect(reg1 && reg2, "Both players should register successfully");
            
            // Test concurrent activation
            bool act1 = audioEngine.activatePlayer(player1.get());
            bool act2 = audioEngine.activatePlayer(player2.get());
            
            expect(act1 && act2, "Both players should activate successfully");
            
            // Verify both players are considered active
            expect(audioEngine.isPlayerActive(player1.get()), "Player1 should be active");
            expect(audioEngine.isPlayerActive(player2.get()), "Player2 should be active");
            
            // Test that audio quality remains good with coordinated access
            expect(audioEngine.isAudioQualityGood(), "Audio quality should be good with coordinated players");
            
            // Clean up
            audioEngine.deactivatePlayer(player1.get());
            audioEngine.deactivatePlayer(player2.get());
            audioEngine.unregisterPlayer(player1.get());
            audioEngine.unregisterPlayer(player2.get());
        }
        
        logMessage("Multi-player resource coordination test completed");
    }
    
    void testPerformanceModeStressTest()
    {
        auto& audioEngine = getAudioEngine();
        
        // This is the ultimate test - recreate the exact scenario that caused clicking
        // Create a performance with multiple patterns playing simultaneously
        
        std::vector<std::unique_ptr<Player>> performancePlayers;
        
        // Create 6 players (typical performance ensemble size)
        std::vector<String> roles = {"Lead", "Rhythm", "Bass", "Drums", "Keys", "Pad"};
        
        for (const auto& role : roles)
        {
            auto player = std::make_unique<Player>(
                "perf_" + role.toLowerCase(),
                "Performance" + role,
                audioEngine.getFormatManager(),
                audioEngine.getKnownPluginList()
            );
            
            if (player)
            {
                player->initialize();
                player->prepareForPlayback();
                performancePlayers.push_back(std::move(player));
            }
        }
        
        expect(performancePlayers.size() == roles.size(), "Should create full performance ensemble");
        
        // Register and activate all players (the critical moment)
        for (auto& player : performancePlayers)
        {
            audioEngine.registerPlayer(player.get());
            audioEngine.activatePlayer(player.get());
        }
        
        // Create different patterns for variety (simulating real performance)
        std::vector<String> patterns = {
            R"({"command":"play_pattern","tempo":120,"events":[{"type":"note_on","time":0.0,"note":60,"velocity":80,"channel":1,"duration":0.25}]})",
            R"({"command":"play_pattern","tempo":120,"events":[{"type":"note_on","time":0.0,"note":64,"velocity":70,"channel":2,"duration":0.5}]})",
            R"({"command":"play_pattern","tempo":120,"events":[{"type":"note_on","time":0.0,"note":67,"velocity":90,"channel":3,"duration":0.125}]})"
        };
        
        // Load different patterns into players
        for (size_t i = 0; i < performancePlayers.size(); ++i)
        {
            var pattern;
            String patternStr = patterns[i % patterns.size()];
            if (JSON::parse(patternStr, pattern).wasOk())
            {
                performancePlayers[i]->loadChart(pattern);
            }
        }
        
        // Record pre-performance metrics
        int initialUnderruns = audioEngine.getBufferUnderruns();

        
        // THE CRITICAL TEST: Start all patterns simultaneously
        int successfulStarts = 0;
        for (auto& player : performancePlayers)
        {
            if (player->play())
            {
                successfulStarts++;
            }
        }
        
        expect(successfulStarts >= 4, "Most players should start successfully in performance mode");
        
        // Monitor performance for extended period (simulate real performance duration)
        bool performanceQualityGood = true;
        auto performanceStart = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - performanceStart < std::chrono::milliseconds(500))
        {
            if (!audioEngine.isAudioQualityGood())
            {
                performanceQualityGood = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        
        expect(performanceQualityGood, "Audio quality should remain good throughout performance");
        
        // Check final metrics
        int finalUnderruns = audioEngine.getBufferUnderruns();
        double finalCpuUsage = audioEngine.getCpuUsage();
        
        int performanceUnderruns = finalUnderruns - initialUnderruns;
        expect(performanceUnderruns <= 2, "Performance mode should have minimal buffer underruns");
        
        expect(finalCpuUsage < 0.9, "CPU usage should remain reasonable during performance");
        
        // Stop all players
        for (auto& player : performancePlayers)
        {
            player->stop();
        }
        
        // Clean up
        for (auto& player : performancePlayers)
        {
            audioEngine.deactivatePlayer(player.get());
            audioEngine.unregisterPlayer(player.get());
        }
        
        logMessage("Performance mode stress test completed - " + String(performanceUnderruns) + 
                  " underruns, CPU: " + String(finalCpuUsage, 3));
    }
    
    void testAudioQualityUnderLoad()
    {
        auto& audioEngine = getAudioEngine();
        
        // Test audio quality monitoring under various load conditions
        expect(audioEngine.isAudioQualityGood(), "Audio quality should start good");
        
        std::vector<std::unique_ptr<Player>> qualityTestPlayers;
        
        // Gradually increase load and monitor quality
        for (int loadLevel = 1; loadLevel <= 5; ++loadLevel)
        {
            // Add another player to increase load
            auto player = std::make_unique<Player>(
                "quality_test_" + String(loadLevel),
                "QualityTest" + String(loadLevel),
                audioEngine.getFormatManager(),
                audioEngine.getKnownPluginList()
            );
            
            if (player)
            {
                player->initialize();
                player->prepareForPlayback();
                audioEngine.registerPlayer(player.get());
                audioEngine.activatePlayer(player.get());
                qualityTestPlayers.push_back(std::move(player));
                
                // Allow system to stabilize
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                
                // Check quality at this load level
                bool qualityAtLevel = audioEngine.isAudioQualityGood();
                double cpuAtLevel = audioEngine.getCpuUsage();
                
                logMessage("Load level " + String(loadLevel) + 
                          ": Quality=" + (qualityAtLevel ? "Good" : "Poor") + 
                          ", CPU=" + String(cpuAtLevel, 3));
                
                // Quality should remain good at reasonable load levels
                if (loadLevel <= 3)
                {
                    expect(qualityAtLevel, "Audio quality should be good at load level " + String(loadLevel));
                }
            }
        }
        
        // Clean up
        for (auto& player : qualityTestPlayers)
        {
            audioEngine.deactivatePlayer(player.get());
            audioEngine.unregisterPlayer(player.get());
        }
        
        logMessage("Audio quality under load test completed");
    }
};

// Global test instance
static PerformanceModeTests performanceModeTests;

} // namespace covertracks