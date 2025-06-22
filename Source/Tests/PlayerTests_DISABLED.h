// Source/Tests/PlayerTests.h
#pragma once

#include <JuceHeader.h>
#include "../Player/Core/Player.h"
#include "../Player/Core/PlayerManager.h"
#include "../Audio/AudioEngineInterface.h"

namespace covertracks {

/**
 * @brief Test data for Player tests
 */
struct PlayerTestData {
    // Valid MIDI event chart for testing
    String validMidiEventChart = R"({
        "success": true,
        "tempo": 120,
        "events": [
            {
                "type": "note_on",
                "channel": 1,
                "note": 36,
                "velocity": 100,
                "timestamp": 0.0
            },
            {
                "type": "note_off", 
                "channel": 1,
                "note": 36,
                "velocity": 0,
                "timestamp": 0.5
            }
        ]
    })";
    
    // Chart with multiple notes for testing
    String complexMidiEventChart = R"({
        "success": true,
        "tempo": 140,
        "events": [
            {"type": "note_on", "channel": 1, "note": 36, "velocity": 100, "timestamp": 0.0},
            {"type": "note_on", "channel": 1, "note": 38, "velocity": 90, "timestamp": 0.25},
            {"type": "note_off", "channel": 1, "note": 36, "velocity": 0, "timestamp": 0.5},
            {"type": "note_off", "channel": 1, "note": 38, "velocity": 0, "timestamp": 0.75},
            {"type": "note_on", "channel": 1, "note": 42, "velocity": 80, "timestamp": 1.0},
            {"type": "note_off", "channel": 1, "note": 42, "velocity": 0, "timestamp": 1.25}
        ]
    })";
    
    // Invalid chart formats for error testing
    String invalidChart = R"({"invalid": "format"})";
    String malformedJson = R"({"events": [unclosed)";
    String emptyChart = R"({})";
};

/**
 * @brief Basic Player functionality tests
 */
class PlayerBasicTests : public juce::UnitTest {
public:
    PlayerBasicTests() : juce::UnitTest("Player Basic Tests") {}
    
    void runTest() override;
    
private:
    void testConstruction();
    void testInitialization();
    void testMetadata();
    void testInstrumentCapabilities();
};

/**
 * @brief Player resource management tests
 */
class PlayerResourceTests : public juce::UnitTest {
public:
    PlayerResourceTests() : juce::UnitTest("Player Resource Management") {}
    
    void runTest() override;
    
private:
    void testResourceLoading();
    void testResourceRelease();
    void testAudioEngineConnection();
    void testLoadingStates();
};

/**
 * @brief Player chart loading and management tests
 */
class PlayerChartTests : public juce::UnitTest {
public:
    PlayerChartTests() : juce::UnitTest("Player Chart Management") {}
    
    void runTest() override;
    
private:
    void testValidChartLoading();
    void testInvalidChartHandling();
    void testChartRetrieval();
    void testComplexChartLoading();
};

/**
 * @brief Player playback functionality tests
 */
class PlayerPlaybackTests : public juce::UnitTest {
public:
    PlayerPlaybackTests() : juce::UnitTest("Player Playback") {}
    
    void runTest() override;
    
private:
    void testBasicPlayback();
    void testPlaybackStates();
    void testPlaybackControl();
    void testChartFinishedCallback();
};

/**
 * @brief Player async chart generation tests
 */
class PlayerAsyncTests : public juce::UnitTest {
public:
    PlayerAsyncTests() : juce::UnitTest("Player Async Operations") {}
    
    void runTest() override;
    
private:
    void testChartGeneration();
    void testGenerateAndLoad();
    void testAsyncCallbacks();
    void testConcurrentRequests();
};

/**
 * @brief Player performance and reliability tests
 */
class PlayerPerformanceTests : public juce::UnitTest {
public:
    PlayerPerformanceTests() : juce::UnitTest("Player Performance") {}
    
    void runTest() override;
    
private:
    void testRepeatedPlayback();
    void testMemoryStability();
    void testQuickLoadPlay();
    void testResourceCleanup();
};

/**
 * @brief Player error handling and edge case tests
 */
class PlayerErrorTests : public juce::UnitTest {
public:
    PlayerErrorTests() : juce::UnitTest("Player Error Handling") {}
    
    void runTest() override;
    
private:
    void testInvalidChartFormats();
    void testPlaybackWithoutChart();
    void testAudioEngineFailure();
    void testResourceLoadingFailure();
};

/**
 * @brief Player integration tests with other components
 */
class PlayerIntegrationTests : public juce::UnitTest {
public:
    PlayerIntegrationTests() : juce::UnitTest("Player Integration") {}
    
    void runTest() override;
    
private:
    void testMusicianIntegration();
    void testPlayerManagerIntegration();
    void testInstrumentIntegration();
    void testAudioEngineIntegration();
};

/**
 * @brief NEW: Player as AudioProcessor interface tests
 * Tests the core AudioProcessor functionality that Player must implement
 */
class PlayerAudioProcessorTests : public juce::UnitTest {
public:
    PlayerAudioProcessorTests() : juce::UnitTest("Player AudioProcessor Interface") {}
    
    void runTest() override;
    
private:
    // Core AudioProcessor interface tests
    void testAudioProcessorInheritance();
    void testPrepareToPlay();
    void testProcessBlock();
    void testReleaseResources();
    void testMidiAcceptance();
    void testAudioChannelConfiguration();
    
    // Player-specific AudioProcessor behavior
    void testPlaybackState();
    void testRealTimeMidiProcessing();
    void testSampleAccurateTiming();
    void testBufferBasedPlayback();
    void testMidiSequencePlayback();
    
    // Helper methods
    std::unique_ptr<Player> createTestPlayer();
    void simulateAudioProcessing(Player& player, int numSamples, double sampleRate);
    juce::var createTestMidiChart();
};

// Global test instances
extern PlayerBasicTests playerBasicTests;
extern PlayerResourceTests playerResourceTests;
extern PlayerChartTests playerChartTests;
extern PlayerPlaybackTests playerPlaybackTests;
extern PlayerAsyncTests playerAsyncTests;
extern PlayerPerformanceTests playerPerformanceTests;
extern PlayerErrorTests playerErrorTests;
extern PlayerIntegrationTests playerIntegrationTests;
extern PlayerAudioProcessorTests playerAudioProcessorTests;

} // namespace covertracks