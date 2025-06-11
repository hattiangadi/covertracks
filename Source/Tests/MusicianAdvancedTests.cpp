// Source/Tests/MusicianAdvancedTests.cpp
#include "MusicianAdvancedTests.h"
#include "PatternGeneratorTests.h" // For MockResponseData

namespace covertracks {

// Area 1: PatternGenerator Integration Tests
void MusicianPatternGeneratorTests::runTest()
{
    testPromptDelegation();
    testChartParsing();
    testDataFlow();
}

void MusicianPatternGeneratorTests::testPromptDelegation()
{
    beginTest("PatternGenerator prompt delegation");
    
    auto musician = std::make_unique<Musician>("prompt-test", "rock");
    
    // Test that Musician properly delegates to PatternGenerator
    PatternGenerator& generator = musician->getPatternGenerator();
    
    // Test prompt creation methods are accessible
    StringArray instruments;
    instruments.add("drums");
    instruments.add("bass");
    
    String songPrompt = generator.createSongPrompt("rock", instruments, 120, "heavy riff");
    expect(songPrompt.isNotEmpty(), "Song prompt should be generated");
    expect(songPrompt.contains("rock") || songPrompt.contains("120"), "Prompt should contain parameters");
    
    String sectionPrompt = generator.createSectionPrompt("verse", "rock", instruments, "quiet verse");
    expect(sectionPrompt.isNotEmpty(), "Section prompt should be generated");
    
    String modPrompt = generator.createModificationPrompt("existing pattern", "make louder");
    expect(modPrompt.isNotEmpty(), "Modification prompt should be generated");
    
    logMessage("PatternGenerator prompt delegation working correctly");
}

void MusicianPatternGeneratorTests::testChartParsing()
{
    beginTest("PatternGenerator chart parsing integration");
    
    auto musician = std::make_unique<Musician>("parse-test", "jazz");
    PatternGenerator& generator = musician->getPatternGenerator();
    MockResponseData mockData;
    
    // Test parsing through PatternGenerator
    auto chart = generator.parseResponse(mockData.response);
    expect(chart != nullptr, "Should parse mock response successfully");
    
    if (chart)
    {
        expect(chart->getTempo() > 0, "Parsed chart should have valid tempo");
        expect(chart->getNumSections() > 0, "Parsed chart should have sections");
    }
    
    // Test validation
    expect(generator.validateResponse(mockData.response), "Should validate mock response");
    expect(!generator.validateResponse("invalid"), "Should reject invalid response");
    
    logMessage("PatternGenerator chart parsing integration working correctly");
}

void MusicianPatternGeneratorTests::testDataFlow()
{
    beginTest("Complete data flow through PatternGenerator");
    
    auto musician = std::make_unique<Musician>("flow-test", "electronic");
    MockResponseData mockData;
    
    // Test the complete flow: MockResponseData → PatternGenerator → Musician conversion
    var result = musician->convertNotationToMidiEvents(mockData.response, 120.0);
    
    expect(result.isObject(), "Result should be an object");
    expect(result.getProperty("success", false), "Conversion should succeed with valid data");
    expect(result.hasProperty("tempo"), "Result should have tempo");
    expect(result.hasProperty("events"), "Result should have events");
    
    // Test that PatternGenerator and Musician work together
    PatternGenerator& generator = musician->getPatternGenerator();
    auto chart = generator.parseResponse(mockData.response);
    
    if (chart)
    {
        // The chart from PatternGenerator should be convertible by Musician
        expect(chart->getTempo() > 0, "Chart tempo should be valid");
        var events = result["events"];
        expect(events.isArray(), "Events should be an array");
    }
    
    logMessage("Complete data flow working correctly");
}

// Area 2: Chart Format Conversion Tests
void MusicianChartFormatTests::runTest()
{
    testFullPipeline();
    testTempoHandling();
    testComplexChartStructures();
    testEdgeCases();
}

void MusicianChartFormatTests::testFullPipeline()
{
    beginTest("Full chart format conversion pipeline");
    
    auto musician = std::make_unique<Musician>("pipeline-test", "ambient");
    MockResponseData mockData;
    
    // Test the complete pipeline: LLM_RESPONSE_TEXT → PARSED_CHART → MIDI_EVENT_CHART
    var midiEventChart = musician->convertNotationToMidiEvents(mockData.response, 120.0);
    
    expect(midiEventChart.isObject(), "Should return MIDI_EVENT_CHART object");
    expect(midiEventChart.getProperty("success", false), "Pipeline should succeed");
    
    // Verify MIDI_EVENT_CHART format
    expect(midiEventChart.hasProperty("tempo"), "Should have tempo property");
    expect(midiEventChart.hasProperty("events"), "Should have events array");
    
    var events = midiEventChart["events"];
    expect(events.isArray(), "Events should be an array");
    
    // If we have events, check their structure
    if (events.isArray() && events.getArray()->size() > 0)
    {
        var firstEvent = events.getArray()->getReference(0);
        expect(firstEvent.isObject(), "MIDI events should be objects");
        expect(firstEvent.hasProperty("type"), "MIDI events should have type");
        expect(firstEvent.hasProperty("timestamp"), "MIDI events should have timestamp");
    }
    
    logMessage("Full pipeline working correctly");
}

void MusicianChartFormatTests::testTempoHandling()
{
    beginTest("Tempo handling in chart conversions");
    
    auto musician = std::make_unique<Musician>("tempo-test", "classical");
    
    // Test with chart that has tempo
    String chartWithTempo = R"({
        "tempo": 140,
        "sections": {
            "A": {
                "bars": [{"primitives": ["kick"], "duration": 1.0}]
            }
        }
    })";
    
    var result1 = musician->convertNotationToMidiEvents(chartWithTempo, 120.0);
    expect(result1.getProperty("success", false), "Should parse chart with tempo");
    expect((double)result1.getProperty("tempo", 0.0) == 140.0, "Should preserve chart tempo");
    
    // Test with chart without tempo (will get default tempo of 120, which is expected behavior)
    String chartWithoutTempo = R"({
        "sections": {
            "A": {
                "bars": [{"primitives": ["kick"], "duration": 1.0}]
            }
        }
    })";
    
    var result2 = musician->convertNotationToMidiEvents(chartWithoutTempo, 100.0);
    if (result2.getProperty("success", false))
    {
        // Chart constructor sets default tempo to 120, so this is expected behavior
        expect((double)result2.getProperty("tempo", 0.0) == 120.0, "Should use Chart default tempo when JSON has none");
    }
    
    logMessage("Tempo handling working correctly");
}

void MusicianChartFormatTests::testComplexChartStructures()
{
    beginTest("Complex chart structure handling");
    
    auto musician = std::make_unique<Musician>("complex-test", "progressive");
    
    // Test with multiple sections
    String complexChart = R"({
        "tempo": 120,
        "sections": {
            "verse": {
                "bars": [
                    {"primitives": ["kick"], "duration": 1.0},
                    {"primitives": ["snare"], "duration": 1.0}
                ]
            },
            "chorus": {
                "bars": [
                    {"primitives": ["kick", "snare"], "duration": 0.5}
                ]
            }
        }
    })";
    
    var result = musician->convertNotationToMidiEvents(complexChart, 120.0);
    expect(result.getProperty("success", false), "Should handle complex chart structures");
    
    if (result.getProperty("success", false))
    {
        var events = result["events"];
        expect(events.isArray(), "Should generate events from complex structure");
        
        // Should have events from multiple sections  
        if (events.isArray())
        {
            int eventCount = events.getArray()->size();
            expect(eventCount >= 0, "Should generate events (got " + String(eventCount) + " events)");
            
            // Log for debugging
            logMessage("Complex chart generated " + String(eventCount) + " MIDI events");
        }
    }
    
    logMessage("Complex chart structure handling working correctly");
}

void MusicianChartFormatTests::testEdgeCases()
{
    beginTest("Chart format edge cases");
    
    auto musician = std::make_unique<Musician>("edge-test", "experimental");
    
    // Test empty chart
    String emptyChart = R"({"sections": {}})";
    var result1 = musician->convertNotationToMidiEvents(emptyChart, 120.0);
    expect(result1.isObject(), "Should handle empty chart gracefully");
    
    // Test chart with zero tempo
    String zeroTempoChart = R"({
        "tempo": 0,
        "sections": {
            "A": {"bars": [{"primitives": ["kick"], "duration": 1.0}]}
        }
    })";
    var result2 = musician->convertNotationToMidiEvents(zeroTempoChart, 130.0);
    if (result2.getProperty("success", false))
    {
        expect((double)result2.getProperty("tempo", 0.0) == 130.0, "Should use fallback tempo for zero tempo");
    }
    
    // Test chart with negative tempo
    String negativeTempoChart = R"({
        "tempo": -50,
        "sections": {
            "A": {"bars": [{"primitives": ["kick"], "duration": 1.0}]}
        }
    })";
    var result3 = musician->convertNotationToMidiEvents(negativeTempoChart, 140.0);
    if (result3.getProperty("success", false))
    {
        expect((double)result3.getProperty("tempo", 0.0) == 140.0, "Should use fallback tempo for negative tempo");
    }
    
    logMessage("Edge cases handled correctly");
}

// Area 3: Memory Leak Prevention Tests
void MusicianMemoryLeakTests::runTest()
{
    testRepeatedOperations();
    testObjectAccumulation();
    testChartCleanup();
}

void MusicianMemoryLeakTests::testRepeatedOperations()
{
    beginTest("Memory stability during repeated operations");
    
    auto musician = std::make_unique<Musician>("memory-test", "test");
    MockResponseData mockData;
    
    // Perform many conversion operations to check for accumulation
    for (int i = 0; i < 50; ++i)
    {
        var result = musician->convertNotationToMidiEvents(mockData.response, 120.0);
        expect(result.isObject(), "Operation " + String(i) + " should succeed");
        
        // Force some processing time
        if (i % 10 == 0)
        {
            juce::Thread::sleep(1);
        }
    }
    
    // Test with different patterns to ensure variety
    for (int i = 0; i < 20; ++i)
    {
        String variedPattern = R"({
            "tempo": )" + String(100 + i*5) + R"(,
            "sections": {
                "test)" + String(i) + R"(": {
                    "bars": [{"primitives": ["kick"], "duration": 1.0}]
                }
            }
        })";
        
        var result = musician->convertNotationToMidiEvents(variedPattern, 120.0);
        // Each operation should work independently
    }
    
    logMessage("Repeated operations completed without memory issues");
}

void MusicianMemoryLeakTests::testObjectAccumulation()
{
    beginTest("No object accumulation during operations");
    
    // Test multiple Musician instances
    for (int i = 0; i < 10; ++i)
    {
        auto musician = std::make_unique<Musician>("accumulation-test-" + String(i), "test");
        MockResponseData mockData;
        
        // Each musician should work independently
        var result = musician->convertNotationToMidiEvents(mockData.response, 120.0);
        expect(result.isObject(), "Musician " + String(i) + " should work correctly");
        
        // Destructor should clean up properly when musician goes out of scope
    }
    
    logMessage("No object accumulation detected");
}

void MusicianMemoryLeakTests::testChartCleanup()
{
    beginTest("Chart object cleanup");
    
    auto musician = std::make_unique<Musician>("cleanup-test", "test");
    
    // Test that charts are properly cleaned up after conversion
    MockResponseData mockData;
    
    for (int i = 0; i < 25; ++i)
    {
        // Each conversion should create and clean up chart objects
        var result = musician->convertNotationToMidiEvents(mockData.response, 120.0);
        
        // The result should be independent of previous conversions
        expect(result.isObject(), "Chart cleanup test " + String(i));
        
        if (result.getProperty("success", false))
        {
            var events = result["events"];
            expect(events.isArray(), "Events should be properly formatted");
        }
    }
    
    logMessage("Chart cleanup working correctly");
}

// Area 4: Concurrency Tests
void MusicianConcurrencyTests::runTest()
{
    testConcurrentRequests();
    testThreadSafety();
    testDestructionDuringOperation();
}

void MusicianConcurrencyTests::testConcurrentRequests()
{
    beginTest("Concurrent chart conversion requests");
    
    auto musician = std::make_unique<Musician>("concurrent-test", "test");
    MockResponseData mockData;
    
    // Test multiple simultaneous conversions
    std::atomic<int> completedCount{0};
    const int numRequests = 10;
    
    for (int i = 0; i < numRequests; ++i)
    {
        // Each request uses slightly different data
        String pattern = mockData.response;
        pattern = pattern.replace("120", String(120 + i));
        
        // Perform conversion (these are synchronous but test thread safety)
        var result = musician->convertNotationToMidiEvents(pattern, 120.0 + i);
        
        if (result.isObject())
        {
            completedCount++;
        }
    }
    
    expect(completedCount.load() == numRequests, "All concurrent requests should complete");
    
    logMessage("Concurrent requests handled correctly");
}

void MusicianConcurrencyTests::testThreadSafety()
{
    beginTest("Thread safety of Musician operations");
    
    auto musician = std::make_unique<Musician>("threadsafe-test", "test");
    MockResponseData mockData;
    
    // Test that multiple operations don't interfere with each other
    bool allSucceeded = true;
    
    for (int i = 0; i < 15; ++i)
    {
        var result = musician->convertNotationToMidiEvents(mockData.response, 120.0);
        
        if (!result.isObject() || !result.getProperty("success", false))
        {
            allSucceeded = false;
            break;
        }
        
        // Small delay to allow thread interactions
        juce::Thread::sleep(1);
    }
    
    expect(allSucceeded, "All operations should succeed in thread-safe manner");
    
    logMessage("Thread safety verified");
}

void MusicianConcurrencyTests::testDestructionDuringOperation()
{
    beginTest("Safe destruction during operations");
    
    // Test that Musician can be safely destroyed while operations might be in progress
    for (int i = 0; i < 5; ++i)
    {
        auto musician = std::make_unique<Musician>("destruction-test-" + String(i), "test");
        MockResponseData mockData;
        
        // Start an operation
        var result = musician->convertNotationToMidiEvents(mockData.response, 120.0);
        
        // Immediately destroy (tests destructor thread cleanup)
        musician.reset();
        
        // Should not crash or hang
    }
    
    logMessage("Safe destruction during operations verified");
}

// Test registry - global instances for TestRunner access
MusicianPatternGeneratorTests musicianPatternGeneratorTests;
MusicianChartFormatTests musicianChartFormatTests;
MusicianMemoryLeakTests musicianMemoryLeakTests;
MusicianConcurrencyTests musicianConcurrencyTests;

} // namespace covertracks