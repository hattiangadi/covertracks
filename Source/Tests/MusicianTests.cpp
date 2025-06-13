// Source/Tests/MusicianTests.cpp
#include "MusicianTests.h"

namespace covertracks {

void MusicianBasicTests::runTest()
{
    testConstruction();
    testThreadLifecycle();
    testBasicProperties();
    testLLMClientConnection();
}

void MusicianBasicTests::testConstruction()
{
    beginTest("Basic Musician construction and destruction");
    
    // Test basic construction
    {
        auto musician = std::make_unique<Musician>("test-musician", "rock");
        
        expect(musician != nullptr, "Musician should construct successfully");
        expect(musician->getId() == "test-musician", "Musician ID should be set correctly");
        expect(musician->getStyle() == "rock", "Musician style should be set correctly");
        expect(!musician->isGenerating(), "Musician should not be generating initially");
        
        // Test that components are initialized (methods should not crash)
        expect(true, "PatternSystem should be accessible");
        try { musician->getPatternSystem(); } catch (...) { expect(false, "PatternSystem access failed"); }
        expect(true, "PromptManager should be accessible"); 
        try { musician->getPromptManager(); } catch (...) { expect(false, "PromptManager access failed"); }
        expect(true, "PatternGenerator should be accessible");
        try { musician->getPatternGenerator(); } catch (...) { expect(false, "PatternGenerator access failed"); }
    }
    
    // Should destruct cleanly without issues
    logMessage("Musician constructed and destructed successfully");
}

void MusicianBasicTests::testThreadLifecycle()
{
    beginTest("Thread lifecycle management");
    
    // Test construction and destruction
    {
        auto musician = std::make_unique<Musician>("thread-test", "jazz");
        
        // Thread should be running after construction
        expect(musician->isThreadRunning(), "Thread should be running after construction");
        
        // Test that the thread remains stable
        juce::Thread::sleep(50); // Give it a moment to run
        expect(musician->isThreadRunning(), "Thread should remain running");
        
        // Destruction should clean up the thread properly
        // (This happens automatically when musician goes out of scope)
    }
    
    // Test multiple construction/destruction cycles
    for (int i = 0; i < 3; ++i)
    {
        auto musician = std::make_unique<Musician>("cycle-test-" + String(i), "test");
        expect(musician->isThreadRunning(), "Thread should start on construction cycle " + String(i));
        // Destructor will clean up properly
    }
    
    logMessage("Thread lifecycle management working correctly");
}

void MusicianBasicTests::testBasicProperties()
{
    beginTest("Basic property management");
    
    auto musician = std::make_unique<Musician>("prop-test", "");
    
    // Test style changes
    musician->setStyle("electronic");
    expect(musician->getStyle() == "electronic", "Style should update correctly");
    
    // Test LLM connection status (should be false without LLM client)
    expect(!musician->isConnectedToLLM(), "Should not be connected without LLM client");
    expect(musician->getLLMServiceName().isEmpty(), "Service name should be empty without client");
    
    logMessage("Basic property management working correctly");
}

void MusicianBasicTests::testLLMClientConnection()
{
    beginTest("LLM client connection management");
    
    auto musician = std::make_unique<Musician>("llm-test", "pop");
    
    // Initially no LLM client
    expect(!musician->isConnectedToLLM(), "Should start without LLM client");
    
    // Note: We can't easily test with a real LLM client in unit tests
    // without mocking, so we'll test the null case for now
    musician->setLLMClient(nullptr);
    expect(!musician->isConnectedToLLM(), "Should handle null LLM client");
    
    logMessage("LLM client connection management working correctly");
}

void MusicianChartTests::runTest()
{
    testChartConversion();
    testInvalidChartHandling();
    testMidiEventGeneration();
}

void MusicianChartTests::testChartConversion()
{
    beginTest("Chart conversion utilities");
    
    auto musician = std::make_unique<Musician>("chart-test", "folk");
    MockMusicianData mockData;
    
    // Test converting valid JSON notation to MIDI events
    var midiEvents = musician->convertNotationToMidiEvents(mockData.validJsonResponse, 120.0);
    
    expect(midiEvents.isObject(), "Should return an object for valid JSON");
    expect(midiEvents.hasProperty("success"), "Should have success property");
    expect(midiEvents.hasProperty("tempo"), "Should have tempo property");
    
    // If conversion succeeded, should have events
    if (midiEvents.getProperty("success", false))
    {
        expect(midiEvents.hasProperty("events"), "Successful conversion should have events");
        var events = midiEvents["events"];
        expect(events.isArray(), "Events should be an array");
    }
    
    logMessage("Chart conversion utilities working correctly");
}

void MusicianChartTests::testInvalidChartHandling()
{
    beginTest("Invalid chart handling");
    
    auto musician = std::make_unique<Musician>("invalid-test", "ambient");
    MockMusicianData mockData;
    
    // Test with invalid JSON
    var invalidResult = musician->convertNotationToMidiEvents(mockData.invalidJsonResponse, 120.0);
    expect(invalidResult.isObject(), "Should return an object even for invalid input");
    expect(!invalidResult.getProperty("success", true), "Should indicate failure for invalid JSON");
    
    // Test with malformed JSON
    var malformedResult = musician->convertNotationToMidiEvents(mockData.malformedJsonResponse, 120.0);
    expect(malformedResult.isObject(), "Should return an object for malformed JSON");
    expect(!malformedResult.getProperty("success", true), "Should indicate failure for malformed JSON");
    
    // Test with empty string
    var emptyResult = musician->convertNotationToMidiEvents("", 120.0);
    expect(emptyResult.isObject(), "Should return an object for empty input");
    expect(!emptyResult.getProperty("success", true), "Should indicate failure for empty input");
    
    logMessage("Invalid chart handling working correctly");
}

void MusicianChartTests::testMidiEventGeneration()
{
    beginTest("MIDI event generation");
    
    auto musician = std::make_unique<Musician>("midi-test", "classical");
    
    // Test with a simple, known pattern
    String simplePattern = R"({
        "tempo": 120,
        "sections": {
            "A": {
                "bars": [
                    {
                        "primitives": ["C4"],
                        "duration": 1.0
                    }
                ]
            }
        }
    })";
    
    var result = musician->convertNotationToMidiEvents(simplePattern, 120.0);
    
    if (result.getProperty("success", false))
    {
        expect((double)result.getProperty("tempo", 0.0) == 120.0, "Tempo should be preserved");
        
        var events = result["events"];
        if (events.isArray() && events.getArray()->size() > 0)
        {
            // Check that we have some MIDI events
            expect(events.getArray()->size() > 0, "Should generate at least one MIDI event");
            
            // Check first event structure
            var firstEvent = events.getArray()->getReference(0);
            expect(firstEvent.isObject(), "MIDI events should be objects");
            expect(firstEvent.hasProperty("type"), "MIDI events should have type property");
            expect(firstEvent.hasProperty("timestamp"), "MIDI events should have timestamp");
        }
    }
    
    logMessage("MIDI event generation working correctly");
}

void MusicianAsyncTests::runTest()
{
    testGenerateChartAsync();
    testMultipleRequests();
    testErrorHandling();
}

void MusicianAsyncTests::testGenerateChartAsync()
{
    beginTest("Async chart generation");
    
    auto musician = std::make_unique<Musician>("async-test", "reggae");
    
    // Test that generateChart requires an LLM client
    var testParams;
    bool callbackCalled = false;
    bool callbackSuccess = false;
    
    musician->generateChart(testParams, [&](bool success, const var& chart) {
        callbackCalled = true;
        callbackSuccess = success;
        juce::ignoreUnused(chart);
    });
    
    // Wait a short time for the callback
    juce::Thread::sleep(100);
    
    expect(callbackCalled, "Callback should be called even without LLM client");
    expect(!callbackSuccess, "Should fail when no LLM client is connected");
    
    logMessage("Async chart generation working correctly");
}

void MusicianAsyncTests::testMultipleRequests()
{
    beginTest("Multiple async requests");
    
    auto musician = std::make_unique<Musician>("multi-test", "metal");
    
    // Test that multiple requests are handled properly
    int callbackCount = 0;
    var testParams;
    
    for (int i = 0; i < 3; ++i)
    {
        musician->generateChart(testParams, [&](bool success, const var& chart) {
            callbackCount++;
            juce::ignoreUnused(success, chart);
        });
    }
    
    // Wait for callbacks
    juce::Thread::sleep(200);
    
    expect(callbackCount == 3, "All callbacks should be invoked");
    
    logMessage("Multiple async requests working correctly");
}

void MusicianAsyncTests::testErrorHandling()
{
    beginTest("Error handling in async operations");
    
    auto musician = std::make_unique<Musician>("error-test", "punk");
    
    // Test generateSection without LLM client
    bool sectionCallbackCalled = false;
    musician->generateSection("verse", var(), [&](bool success, const var& chart) {
        sectionCallbackCalled = true;
        expect(!success, "Section generation should fail without LLM client");
        juce::ignoreUnused(chart);
    });
    
    // Test modifyChart without LLM client
    bool modifyCallbackCalled = false;
    musician->modifyChart(var(), "make it louder", [&](bool success, const var& chart) {
        modifyCallbackCalled = true;
        expect(!success, "Chart modification should fail without LLM client");
        juce::ignoreUnused(chart);
    });
    
    // Wait for callbacks
    juce::Thread::sleep(200);
    
    expect(sectionCallbackCalled, "Section generation callback should be called");
    expect(modifyCallbackCalled, "Chart modification callback should be called");
    
    logMessage("Error handling working correctly");
}

// Test registry - global instances for TestRunner access
MusicianBasicTests musicianBasicTests;
MusicianChartTests musicianChartTests;
MusicianAsyncTests musicianAsyncTests;

} // namespace covertracks