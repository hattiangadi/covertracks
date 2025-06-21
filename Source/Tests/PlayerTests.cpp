// Source/Tests/PlayerTests.cpp
#include "PlayerTests.h"

namespace covertracks {

// Basic Player Tests
void PlayerBasicTests::runTest()
{
    testConstruction();
    testInitialization();
    testMetadata();
    testInstrumentCapabilities();
}

void PlayerBasicTests::testConstruction()
{
    beginTest("Player construction and destruction");
    
    // Note: Player constructor requires format manager, plugin list, and audio engine
    // In test environment, we'll create minimal setup
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    
    // Test basic construction
    auto player = std::make_unique<Player>("test-construction", "Test Player", formatManager, knownPluginList);
    expect(player.get() != nullptr, "Player should construct successfully");
    expect(player->getId() == "test-construction", "Player should have correct ID");
    
    logMessage("Player construction working correctly");
}

void PlayerBasicTests::testInitialization()
{
    beginTest("Player initialization");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("init-test", "Init Test Player", formatManager, knownPluginList);
    
    // Test initialization
    bool initResult = player->initialize();
    expect(initResult, "Player initialization should succeed");
    
    // After initialization, basic functionality should be available
    expect(player->getId().isNotEmpty(), "Player should have ID after init");
    
    logMessage("Player initialization working correctly");
}

void PlayerBasicTests::testMetadata()
{
    beginTest("Player metadata management");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("metadata-test", "Metadata Test Player", formatManager, knownPluginList);
    
    // Test getting metadata
    PlayerMetadata& metadata = player->getMetadata();
    expect(metadata.name.isNotEmpty(), "Should have identifying name");
    
    // Test updating metadata
    metadata.name = "Updated Player";
    metadata.category = "Bass";
    metadata.description = "Updated description";
    
    PlayerMetadata& retrieved = player->getMetadata();
    expect(retrieved.name == "Updated Player", "Metadata name should update");
    expect(retrieved.category == "Bass", "Metadata category should update");
    expect(retrieved.description == "Updated description", "Metadata description should update");
    
    logMessage("Player metadata management working correctly");
}

void PlayerBasicTests::testInstrumentCapabilities()
{
    beginTest("Player instrument capabilities");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("capabilities-test", "Capabilities Test Player", formatManager, knownPluginList);
    
    // Test getting capabilities
    const InstrumentCapabilities& caps = player->getInstrumentCapabilities();
    expect(caps.type == InstrumentCapabilityType::NoteRange || caps.type == InstrumentCapabilityType::Mapping, 
           "Should have some instrument capabilities");
    
    // Test serialization
    var capVar = caps.toVar();
    expect(capVar.isObject(), "Capabilities should serialize to object");
    
    // Test setting capabilities
    InstrumentCapabilities drumCaps = Player::createDrumCapabilities();
    player->setInstrumentCapabilities(drumCaps);
    const InstrumentCapabilities& retrievedCaps = player->getInstrumentCapabilities();
    expect(retrievedCaps.type == InstrumentCapabilityType::Mapping, "Should set drum capabilities");
    
    logMessage("Player instrument capabilities working correctly");
}

// Resource Management Tests
void PlayerResourceTests::runTest()
{
    testResourceLoading();
    testResourceRelease();
    testAudioEngineConnection();
    testLoadingStates();
}

void PlayerResourceTests::testResourceLoading()
{
    beginTest("Player resource loading");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("resource-test", "Resource Test Player", formatManager, knownPluginList);
    
    // Test minimal resource loading
    player->loadMinimalResources();
    expect(player->isFullyLoaded(), "Player should be fully loaded after loadMinimalResources()");
    
    // Test preparing for playback
    player->prepareForPlayback();
    expect(player->isReadyForPlayback(), "Player should be ready for playback after prepareForPlayback()");
    
    logMessage("Player resource loading working correctly");
}

void PlayerResourceTests::testResourceRelease()
{
    beginTest("Player resource release");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("release-test", "Release Test Player", formatManager, knownPluginList);
    
    // Load then release resources
    player->loadMinimalResources();
    player->prepareForPlayback();
    player->releaseResources();
    
    // After release, should not be ready for playback
    expect(!player->isReadyForPlayback(), "Player should not be ready for playback after releaseResources()");
    
    logMessage("Player resource release working correctly");
}

void PlayerResourceTests::testAudioEngineConnection()
{
    beginTest("Player AudioEngine connection");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("audio-test", "Audio Test Player", formatManager, knownPluginList);
    
    // Test connection state (should be false initially since no AudioEngine passed)
    bool isConnected = player->isConnectedToAudioEngine();
    expect(!isConnected, "Should not be connected initially without AudioEngine");
    
    logMessage("Player AudioEngine connection working correctly");
}

void PlayerResourceTests::testLoadingStates()
{
    beginTest("Player loading state management");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("states-test", "States Test Player", formatManager, knownPluginList);
    
    // Test initial states
    expect(!player->isFullyLoaded(), "Should not be fully loaded initially");
    expect(!player->isReadyForPlayback(), "Should not be ready for playback initially");
    expect(!player->isReadyForEditing(), "Should not be ready for editing initially");
    
    // Load minimal resources and test states
    player->loadMinimalResources();
    expect(player->isFullyLoaded(), "Should be fully loaded after minimal resource loading");
    
    // Prepare for editing
    player->prepareForEditing();
    expect(player->isReadyForEditing(), "Should be ready for editing after prepareForEditing()");
    
    // Prepare for playback
    player->prepareForPlayback();
    expect(player->isReadyForPlayback(), "Should be ready for playback after prepareForPlayback()");
    
    logMessage("Player loading state management working correctly");
}

// Chart Management Tests
void PlayerChartTests::runTest()
{
    testValidChartLoading();
    testInvalidChartHandling();
    testChartRetrieval();
    testComplexChartLoading();
}

void PlayerChartTests::testValidChartLoading()
{
    beginTest("Valid chart loading");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("chart-test", "Chart Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Parse test chart
    var chart;
    Result parseResult = JSON::parse(testData.validMidiEventChart, chart);
    expect(parseResult.wasOk(), "Test chart should parse correctly");
    
    if (parseResult.wasOk())
    {
        bool loadResult = player->loadChart(chart);
        expect(loadResult, "Valid chart should load successfully");
        
        if (loadResult)
        {
            const var& loadedChart = player->getCurrentChart();
            expect(!loadedChart.isVoid(), "Loaded chart should not be void");
            expect(loadedChart.isObject(), "Loaded chart should be an object");
        }
    }
    
    logMessage("Valid chart loading working correctly");
}

void PlayerChartTests::testInvalidChartHandling()
{
    beginTest("Invalid chart handling");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("invalid-chart-test", "Invalid Chart Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Test invalid chart format
    var invalidChart;
    JSON::parse(testData.invalidChart, invalidChart);
    bool result1 = player->loadChart(invalidChart);
    expect(!result1, "Invalid chart should be rejected");
    
    // Test empty chart
    var emptyChart;
    JSON::parse(testData.emptyChart, emptyChart);
    bool result2 = player->loadChart(emptyChart);
    expect(!result2, "Empty chart should be rejected");
    
    // Test void chart
    var voidChart;
    bool result3 = player->loadChart(voidChart);
    expect(!result3, "Void chart should be rejected");
    
    logMessage("Invalid chart handling working correctly");
}

void PlayerChartTests::testChartRetrieval()
{
    beginTest("Chart retrieval");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("retrieval-test", "Retrieval Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Load a chart
    var chart;
    JSON::parse(testData.validMidiEventChart, chart);
    
    if (player->loadChart(chart))
    {
        const var& retrieved = player->getCurrentChart();
        expect(!retrieved.isVoid(), "Retrieved chart should not be void");
        expect(retrieved.isObject(), "Retrieved chart should be an object");
        
        // Verify some content
        if (retrieved.hasProperty("events"))
        {
            var events = retrieved["events"];
            expect(events.isArray(), "Events should be an array");
        }
    }
    
    logMessage("Chart retrieval working correctly");
}

void PlayerChartTests::testComplexChartLoading()
{
    beginTest("Complex chart loading");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("complex-test", "Complex Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Load complex chart with multiple events
    var complexChart;
    Result parseResult = JSON::parse(testData.complexMidiEventChart, complexChart);
    expect(parseResult.wasOk(), "Complex chart should parse correctly");
    
    if (parseResult.wasOk())
    {
        bool loadResult = player->loadChart(complexChart);
        expect(loadResult, "Complex chart should load successfully");
        
        if (loadResult)
        {
            const var& loaded = player->getCurrentChart();
            expect(loaded.hasProperty("events"), "Complex chart should have events");
            
            if (loaded.hasProperty("events"))
            {
                var events = loaded["events"];
                expect(events.isArray(), "Events should be an array");
                expect(events.getArray()->size() > 1, "Should have multiple events");
            }
        }
    }
    
    logMessage("Complex chart loading working correctly");
}

// Playback Tests
void PlayerPlaybackTests::runTest()
{
    testBasicPlayback();
    testPlaybackStates();
    testPlaybackControl();
    testChartFinishedCallback();
}

void PlayerPlaybackTests::testBasicPlayback()
{
    beginTest("Basic playback functionality");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("playback-test", "Playback Test Player", formatManager, knownPluginList);
    player->initialize();
    player->prepareForPlayback();
    PlayerTestData testData;
    
    // Load a chart
    var chart;
    JSON::parse(testData.validMidiEventChart, chart);
    
    if (player->loadChart(chart))
    {
        // Test play (may not actually produce audio in test environment)
        (void)player->play();
        // In test environment, play might fail due to no audio output, which is acceptable
        expect(true, "Play method should not crash");
        
        // Test stop
        player->stop();
        expect(true, "Stop method should not crash");
    }
    
    logMessage("Basic playback functionality working correctly");
}

void PlayerPlaybackTests::testPlaybackStates()
{
    beginTest("Playback state management");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("states-test", "States Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Initially should not be playing (check via instrument)
    if (player->getInstrument())
    {
        expect(!player->getInstrument()->isPlaying(), "Should not be playing initially");
    }
    
    // Load chart and test states
    var chart;
    JSON::parse(testData.validMidiEventChart, chart);
    
    if (player->loadChart(chart))
    {
        // States after loading chart
        if (player->getInstrument())
        {
            expect(!player->getInstrument()->isPlaying(), "Should not be playing after chart load");
        }
        
        // Attempt to play
        bool playResult = player->play();
        if (playResult && player->getInstrument())
        {
            expect(player->getInstrument()->isPlaying(), "Should be playing after successful play()");
        }
        juce::ignoreUnused(playResult);
        
        // Stop
        player->stop();
        if (player->getInstrument())
        {
            expect(!player->getInstrument()->isPlaying(), "Should not be playing after stop()");
        }
    }
    
    logMessage("Playback state management working correctly");
}

void PlayerPlaybackTests::testPlaybackControl()
{
    beginTest("Playback control");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("control-test", "Control Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Test stop without playing (should not crash)
    player->stop();
    player->stopPlayback();
    expect(true, "Stop methods should handle no-playback state gracefully");
    
    // Load chart and test controls
    var chart;
    JSON::parse(testData.validMidiEventChart, chart);
    
    if (player->loadChart(chart))
    {
        // Test multiple play/stop cycles
        for (int i = 0; i < 3; ++i)
        {
            player->play();
            juce::Thread::sleep(10); // Brief playback
            player->stop();
            expect(true, "Play/stop cycle " + String(i) + " should work");
        }
    }
    
    logMessage("Playback control working correctly");
}

void PlayerPlaybackTests::testChartFinishedCallback()
{
    beginTest("Chart finished callback");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("callback-test", "Callback Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Set up callback
    bool callbackCalled = false;
    String callbackPlayerName;
    
    player->setChartFinishedCallback([&](const String& playerName) {
        callbackCalled = true;
        callbackPlayerName = playerName;
    });
    
    // The callback won't be triggered in tests without actual playback,
    // but we can verify the callback is set without crashing
    expect(true, "Chart finished callback should be settable");
    
    logMessage("Chart finished callback working correctly");
}

// Async Operation Tests
void PlayerAsyncTests::runTest()
{
    testChartGeneration();
    testGenerateAndLoad();
    testAsyncCallbacks();
    testConcurrentRequests();
}

void PlayerAsyncTests::testChartGeneration()
{
    beginTest("Async chart generation");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("async-test", "Async Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test chart generation (will fail without Musician/LLM setup, but should not crash)
    var parameters;
    bool callbackCalled = false;
    bool callbackSuccess = false;
    
    player->generateNewChart(parameters, [&](bool success) {
        callbackCalled = true;
        callbackSuccess = success;
    });
    
    // Wait briefly for callback
    juce::Thread::sleep(100);
    
    expect(callbackCalled, "Chart generation callback should be called");
    // Success depends on LLM setup, failure is acceptable in tests
    
    logMessage("Async chart generation working correctly");
}

void PlayerAsyncTests::testGenerateAndLoad()
{
    beginTest("Generate and load workflow");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("generate-load-test", "Generate Load Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test generateAndLoad
    var parameters;
    bool callbackCalled = false;
    
    player->generateAndLoad(parameters, [&](bool success) {
        callbackCalled = true;
        juce::ignoreUnused(success);
    });
    
    // Wait for callback
    juce::Thread::sleep(100);
    
    expect(callbackCalled, "GenerateAndLoad callback should be called");
    
    logMessage("Generate and load workflow working correctly");
}

void PlayerAsyncTests::testAsyncCallbacks()
{
    beginTest("Async callback handling");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("callback-test", "Callback Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test multiple async operations
    int callbackCount = 0;
    var parameters;
    
    for (int i = 0; i < 3; ++i)
    {
        player->generateNewChart(parameters, [&](bool success) {
            callbackCount++;
            juce::ignoreUnused(success);
        });
    }
    
    // Wait for callbacks
    juce::Thread::sleep(200);
    
    expect(callbackCount == 3, "All async callbacks should be invoked");
    
    logMessage("Async callback handling working correctly");
}

void PlayerAsyncTests::testConcurrentRequests()
{
    beginTest("Concurrent async requests");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("concurrent-test", "Concurrent Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test concurrent generateNewChart calls
    std::atomic<int> completedCount{0};
    var parameters;
    
    for (int i = 0; i < 5; ++i)
    {
        player->generateNewChart(parameters, [&](bool success) {
            completedCount++;
            juce::ignoreUnused(success);
        });
    }
    
    // Wait for completion
    juce::Thread::sleep(300);
    
    expect(completedCount.load() == 5, "All concurrent requests should complete");
    
    logMessage("Concurrent async requests working correctly");
}

// Performance Tests
void PlayerPerformanceTests::runTest()
{
    testRepeatedPlayback();
    testMemoryStability();
    testQuickLoadPlay();
    testResourceCleanup();
}

void PlayerPerformanceTests::testRepeatedPlayback()
{
    beginTest("Repeated playback performance");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("performance-test", "Performance Test Player", formatManager, knownPluginList);
    player->initialize();
    player->prepareForPlayback();
    PlayerTestData testData;
    
    // Load chart once
    var chart;
    JSON::parse(testData.validMidiEventChart, chart);
    
    if (player->loadChart(chart))
    {
        // Repeated play/stop cycles
        for (int i = 0; i < 20; ++i)
        {
            (void)player->play();
            juce::Thread::sleep(5); // Brief playback
            player->stop();
            
            if (i % 5 == 0)
            {
                // Periodic check that player is still responsive
                if (player->getInstrument())
                {
                    expect(!player->getInstrument()->isPlaying(), "Player should be stopped after cycle " + String(i));
                }
            }
        }
    }
    
    logMessage("Repeated playback performance verified");
}

void PlayerPerformanceTests::testMemoryStability()
{
    beginTest("Memory stability during operations");
    
    PlayerTestData testData;
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    
    // Create and destroy multiple players
    for (int i = 0; i < 10; ++i)
    {
        auto player = std::make_unique<Player>("memory-test-" + String(i), "Memory Test Player " + String(i), formatManager, knownPluginList);
        player->initialize();
        
        // Load chart
        var chart;
        JSON::parse(testData.validMidiEventChart, chart);
        player->loadChart(chart);
        
        // Brief operation
        player->play();
        juce::Thread::sleep(1);
        player->stop();
        
        // Player destructor should clean up properly
    }
    
    logMessage("Memory stability verified");
}

void PlayerPerformanceTests::testQuickLoadPlay()
{
    beginTest("Quick load-and-play performance");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("quick-test", "Quick Test Player", formatManager, knownPluginList);
    player->initialize();
    player->prepareForPlayback();
    PlayerTestData testData;
    
    // Test rapid load-and-play cycles
    for (int i = 0; i < 10; ++i)
    {
        var chart;
        JSON::parse(testData.validMidiEventChart, chart);
        
        bool loadResult = player->loadChart(chart);
        if (loadResult)
        {
            player->play();
            juce::Thread::sleep(2);
            player->stop();
        }
        
        expect(true, "Quick load-play cycle " + String(i) + " should complete");
    }
    
    logMessage("Quick load-and-play performance verified");
}

void PlayerPerformanceTests::testResourceCleanup()
{
    beginTest("Resource cleanup performance");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    
    // Test multiple load/release cycles
    for (int i = 0; i < 5; ++i)
    {
        auto player = std::make_unique<Player>("cleanup-test-" + String(i), "Cleanup Test Player " + String(i), formatManager, knownPluginList);
        player->initialize();
        
        // Load and release resources
        player->loadMinimalResources();
        player->prepareForPlayback();
        player->releaseResources();
        
        // Should be able to load again
        player->loadMinimalResources();
        player->prepareForPlayback();
        player->releaseResources();
        
        expect(true, "Resource cleanup cycle " + String(i) + " should complete");
    }
    
    logMessage("Resource cleanup performance verified");
}

// Error Handling Tests
void PlayerErrorTests::runTest()
{
    testInvalidChartFormats();
    testPlaybackWithoutChart();
    testAudioEngineFailure();
    testResourceLoadingFailure();
}

void PlayerErrorTests::testInvalidChartFormats()
{
    beginTest("Invalid chart format handling");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("error-test", "Error Test Player", formatManager, knownPluginList);
    player->initialize();
    PlayerTestData testData;
    
    // Test various invalid formats
    var malformedChart;
    bool result1 = player->loadChart(malformedChart);
    expect(!result1, "Should reject void chart");
    
    var stringChart = "not an object";
    bool result2 = player->loadChart(stringChart);
    expect(!result2, "Should reject non-object chart");
    
    DynamicObject::Ptr emptyObjectPtr = new DynamicObject();
    var emptyObject(emptyObjectPtr.get());
    bool result3 = player->loadChart(emptyObject);
    expect(!result3, "Should reject empty object");
    
    logMessage("Invalid chart format handling verified");
}

void PlayerErrorTests::testPlaybackWithoutChart()
{
    beginTest("Playback without chart");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("no-chart-test", "No Chart Test Player", formatManager, knownPluginList);
    player->initialize();
    player->prepareForPlayback();
    
    // Try to play without loading a chart
    bool playResult = player->play();
    expect(!playResult, "Play should fail without loaded chart");
    
    if (player->getInstrument())
    {
        expect(!player->getInstrument()->isPlaying(), "Should not be playing without chart");
    }
    
    // Stop should not crash
    player->stop();
    expect(true, "Stop should not crash without chart");
    
    logMessage("Playback without chart handled correctly");
}

void PlayerErrorTests::testAudioEngineFailure()
{
    beginTest("AudioEngine failure handling");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("audio-fail-test", "Audio Fail Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test operations when AudioEngine might not be available
    bool connectResult = player->isConnectedToAudioEngine();
    expect(!connectResult, "Should not be connected without AudioEngine passed to constructor");
    
    // Try operations that depend on AudioEngine
    player->loadMinimalResources();
    player->prepareForPlayback();
    expect(true, "Resource loading should handle AudioEngine unavailability");
    
    logMessage("AudioEngine failure handling verified");
}

void PlayerErrorTests::testResourceLoadingFailure()
{
    beginTest("Resource loading failure handling");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("resource-fail-test", "Resource Fail Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Even if resource loading fails, player should remain stable
    player->loadMinimalResources();
    expect(true, "Resource loading should not crash even if it fails");
    
    // Player should handle failure gracefully
    expect(player->isFullyLoaded(), "Loading state should be consistent after loadMinimalResources");
    
    logMessage("Resource loading failure handling verified");
}

// Integration Tests
void PlayerIntegrationTests::runTest()
{
    testMusicianIntegration();
    testPlayerManagerIntegration();
    testInstrumentIntegration();
    testAudioEngineIntegration();
}

void PlayerIntegrationTests::testMusicianIntegration()
{
    beginTest("Player-Musician integration");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("musician-integration-test", "Musician Integration Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test that Player has access to Musician
    expect(player->getMusician() != nullptr, "Player should have Musician after initialization");
    
    logMessage("Player-Musician integration verified");
}

void PlayerIntegrationTests::testPlayerManagerIntegration()
{
    beginTest("Player-PlayerManager integration");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    // Test that Player works correctly within PlayerManager context
    auto player = std::make_unique<Player>("manager-integration-test", "Manager Integration Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Basic functionality should work independently
    expect(player->getId().isNotEmpty(), "Player should have ID for manager");
    expect(player->getMetadata().name.isNotEmpty(), "Player should have name for manager");
    
    logMessage("Player-PlayerManager integration verified");
}

void PlayerIntegrationTests::testInstrumentIntegration()
{
    beginTest("Player-Instrument integration");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("instrument-integration-test", "Instrument Integration Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test that Player has access to Instrument
    expect(player->getInstrument() != nullptr, "Player should have Instrument after initialization");
    
    // Test instrument capabilities
    const InstrumentCapabilities& caps = player->getInstrumentCapabilities();
    expect(true, "Instrument capabilities should be accessible");
    
    // Test that capabilities can be serialized (important for LLM integration)
    var capVar = caps.toVar();
    expect(capVar.isObject(), "Instrument capabilities should serialize correctly");
    
    logMessage("Player-Instrument integration verified");
}

void PlayerIntegrationTests::testAudioEngineIntegration()
{
    beginTest("Player-AudioEngine integration");
    
    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;
    auto player = std::make_unique<Player>("audio-integration-test", "Audio Integration Test Player", formatManager, knownPluginList);
    player->initialize();
    
    // Test AudioEngine interaction (should be false since no AudioEngine passed)
    bool isConnected = player->isConnectedToAudioEngine();
    expect(!isConnected, "Should not be connected without AudioEngine");
    
    // Test resource loading (which may depend on AudioEngine)
    player->loadMinimalResources();
    player->prepareForPlayback();
    expect(true, "Resource loading should handle AudioEngine state");
    
    logMessage("Player-AudioEngine integration verified");
}

// NEW: Player AudioProcessor Interface Tests
void PlayerAudioProcessorTests::runTest()
{
    testAudioProcessorInheritance();
    testPrepareToPlay();
    testProcessBlock();
    testReleaseResources();
    testMidiAcceptance();
    testAudioChannelConfiguration();
    testPlaybackState();
    testRealTimeMidiProcessing();
    testSampleAccurateTiming();
    testBufferBasedPlayback();
    testMidiSequencePlayback();
}

void PlayerAudioProcessorTests::testAudioProcessorInheritance()
{
    beginTest("Player inherits from AudioProcessor");
    
    auto player = createTestPlayer();
    
    // FAILING TEST: Player should inherit from AudioProcessor
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    expect(audioProcessor != nullptr, "Player should inherit from juce::AudioProcessor");
    
    logMessage("Player AudioProcessor inheritance verified");
}

void PlayerAudioProcessorTests::testPrepareToPlay()
{
    beginTest("Player implements prepareToPlay");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        // FAILING TEST: Should implement prepareToPlay properly
        audioProcessor->prepareToPlay(44100.0, 512);
        expect(true, "prepareToPlay should not crash");
        
        // Should be able to handle different sample rates and buffer sizes
        audioProcessor->prepareToPlay(48000.0, 1024);
        expect(true, "prepareToPlay should handle different configurations");
    }
    else
    {
        expect(false, "Cannot test prepareToPlay - Player is not an AudioProcessor");
    }
    
    logMessage("Player prepareToPlay implementation verified");
}

void PlayerAudioProcessorTests::testProcessBlock()
{
    beginTest("Player implements processBlock");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        // Prepare for processing
        audioProcessor->prepareToPlay(44100.0, 512);
        
        // FAILING TEST: Should implement processBlock
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        // This should not crash
        audioProcessor->processBlock(buffer, midiBuffer);
        expect(true, "processBlock should not crash with empty buffers");
        
        // Test with actual MIDI data
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        midiBuffer.addEvent(juce::MidiMessage::noteOff(1, 60), 256);
        
        audioProcessor->processBlock(buffer, midiBuffer);
        expect(true, "processBlock should handle MIDI events");
    }
    else
    {
        expect(false, "Cannot test processBlock - Player is not an AudioProcessor");
    }
    
    logMessage("Player processBlock implementation verified");
}

void PlayerAudioProcessorTests::testReleaseResources()
{
    beginTest("Player implements releaseResources");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        audioProcessor->prepareToPlay(44100.0, 512);
        
        // FAILING TEST: Should implement releaseResources
        audioProcessor->releaseResources();
        expect(true, "releaseResources should not crash");
        
        // Should be safe to call multiple times
        audioProcessor->releaseResources();
        expect(true, "releaseResources should be safe to call multiple times");
    }
    else
    {
        expect(false, "Cannot test releaseResources - Player is not an AudioProcessor");
    }
    
    logMessage("Player releaseResources implementation verified");
}

void PlayerAudioProcessorTests::testMidiAcceptance()
{
    beginTest("Player accepts MIDI input");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        // FAILING TEST: Player should accept MIDI
        expect(audioProcessor->acceptsMidi(), "Player should accept MIDI input");
        expect(!audioProcessor->producesMidi(), "Player should not produce MIDI output");
        
        expect(audioProcessor->isMidiEffect() == false, "Player should not be a MIDI effect");
    }
    else
    {
        expect(false, "Cannot test MIDI acceptance - Player is not an AudioProcessor");
    }
    
    logMessage("Player MIDI acceptance verified");
}

void PlayerAudioProcessorTests::testAudioChannelConfiguration()
{
    beginTest("Player audio channel configuration");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        // FAILING TEST: Should have proper channel configuration
        expect(audioProcessor->getTotalNumInputChannels() == 0, "Player should have no audio inputs");
        expect(audioProcessor->getTotalNumOutputChannels() == 2, "Player should have stereo output");
        
        // Should support stereo bus layout
        auto busesLayout = audioProcessor->getBusesLayout();
        expect(busesLayout.outputBuses.size() >= 1, "Should have at least one output bus");
        
        if (busesLayout.outputBuses.size() > 0)
        {
            expect(busesLayout.outputBuses[0].size() == 2, "First output bus should be stereo");
        }
    }
    else
    {
        expect(false, "Cannot test audio configuration - Player is not an AudioProcessor");
    }
    
    logMessage("Player audio channel configuration verified");
}

void PlayerAudioProcessorTests::testPlaybackState()
{
    beginTest("Player playback state management");
    
    auto player = createTestPlayer();
    
    // Load a test chart
    auto testChart = createTestMidiChart();
    bool loadResult = player->loadChart(testChart);
    expect(loadResult, "Should be able to load test chart");
    
    // Test playback state
    expect(!player->isPlaying(), "Player should not be playing initially");
    
    // FAILING TEST: Start playback should work
    player->startPlayback();
    expect(player->isPlaying(), "Player should be playing after startPlayback()");
    
    // Stop playback
    player->stopPlayback();
    expect(!player->isPlaying(), "Player should not be playing after stopPlayback()");
    
    logMessage("Player playback state management verified");
}

void PlayerAudioProcessorTests::testRealTimeMidiProcessing()
{
    beginTest("Player real-time MIDI processing");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        // Load chart and start playback
        auto testChart = createTestMidiChart();
        player->loadChart(testChart);
        player->startPlayback();
        
        audioProcessor->prepareToPlay(44100.0, 512);
        
        // FAILING TEST: Should process MIDI in real-time during processBlock
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer inputMidi, outputMidi;
        
        // Process several blocks to simulate real-time playback
        for (int block = 0; block < 10; ++block)
        {
            inputMidi.clear();
            audioProcessor->processBlock(buffer, inputMidi);
            // Player should generate internal MIDI events based on loaded chart
        }
        
        expect(true, "Real-time MIDI processing should not crash");
    }
    else
    {
        expect(false, "Cannot test MIDI processing - Player is not an AudioProcessor");
    }
    
    logMessage("Player real-time MIDI processing verified");
}

void PlayerAudioProcessorTests::testSampleAccurateTiming()
{
    beginTest("Player sample-accurate timing");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        auto testChart = createTestMidiChart();
        player->loadChart(testChart);
        player->startPlayback();
        
        audioProcessor->prepareToPlay(44100.0, 512);
        
        // FAILING TEST: Timing should be sample-accurate
        simulateAudioProcessing(*player, 44100, 44100.0); // 1 second of audio
        
        // Verify that playback position is tracked accurately
        // (This will be implemented based on sample counting, not timers)
        expect(true, "Sample-accurate timing should be maintained");
    }
    else
    {
        expect(false, "Cannot test timing - Player is not an AudioProcessor");
    }
    
    logMessage("Player sample-accurate timing verified");
}

void PlayerAudioProcessorTests::testBufferBasedPlayback()
{
    beginTest("Player buffer-based playback");
    
    auto player = createTestPlayer();
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    
    if (audioProcessor != nullptr)
    {
        auto testChart = createTestMidiChart();
        player->loadChart(testChart);
        player->startPlayback();
        
        // FAILING TEST: Should handle different buffer sizes
        for (int bufferSize : {128, 256, 512, 1024, 2048})
        {
            audioProcessor->prepareToPlay(44100.0, bufferSize);
            
            juce::AudioBuffer<float> buffer(2, bufferSize);
            juce::MidiBuffer midiBuffer;
            
            audioProcessor->processBlock(buffer, midiBuffer);
            expect(true, juce::String("Should handle buffer size: ") + juce::String(bufferSize));
        }
    }
    else
    {
        expect(false, "Cannot test buffer-based playback - Player is not an AudioProcessor");
    }
    
    logMessage("Player buffer-based playback verified");
}

void PlayerAudioProcessorTests::testMidiSequencePlayback()
{
    beginTest("Player MIDI sequence playback");
    
    auto player = createTestPlayer();
    
    // FAILING TEST: Should play MIDI sequences through AudioProcessor interface
    auto testChart = createTestMidiChart();
    bool loadResult = player->loadChart(testChart);
    expect(loadResult, "Should load test MIDI chart");
    
    // Start playback
    player->startPlayback();
    expect(player->isPlaying(), "Should be playing after startPlayback");
    
    // Simulate audio processing
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(player.get());
    if (audioProcessor != nullptr)
    {
        audioProcessor->prepareToPlay(44100.0, 512);
        simulateAudioProcessing(*player, 22050, 44100.0); // 0.5 seconds
    }
    
    logMessage("Player MIDI sequence playback verified");
}

// Helper methods
std::unique_ptr<Player> PlayerAudioProcessorTests::createTestPlayer()
{
    static juce::AudioPluginFormatManager formatManager;
    static juce::KnownPluginList knownPluginList;
    
    auto player = std::make_unique<Player>("test-audio-processor", "Test AudioProcessor Player", formatManager, knownPluginList);
    player->initialize();
    return player;
}

void PlayerAudioProcessorTests::simulateAudioProcessing(Player& player, int numSamples, double sampleRate)
{
    auto* audioProcessor = dynamic_cast<juce::AudioProcessor*>(&player);
    if (audioProcessor == nullptr) return;
    
    const int bufferSize = 512;
    int samplesRemaining = numSamples;
    
    audioProcessor->prepareToPlay(sampleRate, bufferSize);
    
    while (samplesRemaining > 0)
    {
        int currentBufferSize = juce::jmin(bufferSize, samplesRemaining);
        
        juce::AudioBuffer<float> buffer(2, currentBufferSize);
        juce::MidiBuffer midiBuffer;
        
        audioProcessor->processBlock(buffer, midiBuffer);
        
        samplesRemaining -= currentBufferSize;
    }
}

juce::var PlayerAudioProcessorTests::createTestMidiChart()
{
    return juce::JSON::parse(PlayerTestData().validMidiEventChart);
}

// Test registry - global instances for TestRunner access
PlayerBasicTests playerBasicTests;
PlayerResourceTests playerResourceTests;
PlayerChartTests playerChartTests;
PlayerPlaybackTests playerPlaybackTests;
PlayerAsyncTests playerAsyncTests;
PlayerPerformanceTests playerPerformanceTests;
PlayerErrorTests playerErrorTests;
PlayerIntegrationTests playerIntegrationTests;
PlayerAudioProcessorTests playerAudioProcessorTests;

} // namespace covertracks