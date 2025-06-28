// Source/Tests/PatternParsingStressTests.cpp
#include <JuceHeader.h>
#include "../Musician/Core/PatternParser.h"
#include "../Musician/Core/MusicPatternSystem.h"
#include <atomic>
#include <thread>
#include <vector>

namespace covertracks {

/**
 * Stress tests for pattern parsing system - tests the parser under extreme conditions
 * These tests ensure the system is robust enough for production use
 */
class PatternParsingStressTests : public juce::UnitTest
{
public:
    PatternParsingStressTests() : juce::UnitTest("Pattern Parsing Stress Tests", "Music Pattern System") {}

    void runTest() override
    {
        testMassivePatternGeneration();
        testDeepNestingStress();
        testConcurrentAccessStress();
        testMemoryPressureHandling();
        testExtremeEdgeCases();
        testRandomPatternGeneration();
        testLongRunningParsing();
        testResourceExhaustionRecovery();
    }

private:
    void testMassivePatternGeneration()
    {
        beginTest("Massive pattern generation stress test");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Generate a massive pattern with thousands of events
        String massivePattern = R"({
            "title": "Massive Pattern",
            "tempo": 120,
            "primitives": {
                "instruments": {
                    "C4": 60, "D4": 62, "E4": 64, "F4": 65, "G4": 67, "A4": 69, "B4": 71,
                    "C5": 72, "D5": 74, "E5": 76, "F5": 77, "G5": 79, "A5": 81, "B5": 83
                }
            },
            "sections": {
                "massive": {
                    "bars": 200,
                    "layers": {
                        "layer1": [)";
        
        // Generate 5000 events across 200 bars
        const int totalEvents = 5000;
        const int totalBars = 200;
        StringArray notes = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5", "A5", "B5"};
        Random random;
        
        for (int i = 0; i < totalEvents; ++i)
        {
            double time = 1.0 + (i * totalBars * 4.0) / totalEvents; // Spread across all bars
            String note = notes[random.nextInt(notes.size())];
            float velocity = 0.5f + random.nextFloat() * 0.5f;
            float duration = 0.25f + random.nextFloat() * 0.75f;
            
            massivePattern += "[" + String(time, 3) + ", \"" + note + "\", " + 
                             String(velocity, 2) + ", " + String(duration, 2) + "]";
            if (i < totalEvents - 1) massivePattern += ",";
        }
        
        massivePattern += R"(
                        ]
                    }
                }
            }
        })";
        
        logMessage("Generated pattern with " + String(totalEvents) + " events across " + 
                  String(totalBars) + " bars (" + String(massivePattern.length()) + " characters)");
        
        double startTime = Time::getMillisecondCounterHiRes();
        auto chart = parser.parseChart(massivePattern);
        double parseTime = Time::getMillisecondCounterHiRes() - startTime;
        
        expect(chart != nullptr, "Massive pattern should parse successfully");
        expect(parseTime < 30000.0, "Should parse massive pattern in under 30 seconds");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Massive phrase should exist");
                expect(phrase->getNumBars() == totalBars, "Should have correct number of bars");
                
                // Test random bar access
                int randomBar = random.nextInt(totalBars);
                auto bar = phrase->getBar(randomBar);
                expect(bar != nullptr, "Random bar access should work");
                
                if (bar)
                {
                    auto midi = bar->toMidiSequence();
                    logMessage("Random bar " + String(randomBar) + " has " + 
                              String(midi->getNumEvents()) + " MIDI events");
                }
            }
        }
        
        logMessage("Massive pattern parsed in " + String(parseTime, 0) + "ms");
        logMessage("PASS: Massive pattern generation stress test completed");
    }
    
    void testDeepNestingStress()
    {
        beginTest("Deep nesting stress test");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Create deeply nested pattern references
        String deepNesting = R"({
            "title": "Deep Nesting Stress",
            "tempo": 120,
            "patterns": {)";
        
        // Create 50 levels of pattern nesting
        const int nestingLevels = 50;
        for (int i = 0; i < nestingLevels; ++i)
        {
            deepNesting += "\"level" + String(i) + "\": ";
            if (i == 0)
            {
                deepNesting += "[[1.0, \"C4\"], [2.0, \"E4\"]]";
            }
            else
            {
                deepNesting += "[[1.0, {\"pattern\": \"level" + String(i-1) + "\"}]]";
            }
            if (i < nestingLevels - 1) deepNesting += ",";
        }
        
        deepNesting += R"(
            },
            "sections": {
                "nested": {
                    "bars": 4,
                    "layers": {
                        "deepLayer": {
                            "basePattern": "level)" + String(nestingLevels-1) + R"(",
                            "variations": {
                                "bar2": {
                                    "basePattern": "level)" + String(nestingLevels-10) + R"(",
                                    "variations": {
                                        "beat3": "level)" + String(nestingLevels-20) + R"("
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        double startTime = Time::getMillisecondCounterHiRes();
        auto chart = parser.parseChart(deepNesting);
        double parseTime = Time::getMillisecondCounterHiRes() - startTime;
        
        expect(chart != nullptr, "Deep nesting should not break parser");
        expect(parseTime < 10000.0, "Deep nesting should parse within 10 seconds");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        expect(midi->getNumEvents() > 0, "Deep nesting should produce MIDI events");
                        logMessage("Deep nesting result: " + String(midi->getNumEvents()) + " events");
                    }
                }
            }
        }
        
        logMessage("Deep nesting (" + String(nestingLevels) + " levels) parsed in " + 
                  String(parseTime, 0) + "ms");
        logMessage("PASS: Deep nesting stress test completed");
    }
    
    void testConcurrentAccessStress()
    {
        beginTest("Concurrent access stress test");
        
        const int numThreads = 8;
        const int operationsPerThread = 50;
        std::atomic<int> successCount{0};
        std::atomic<int> failureCount{0};
        
        String testPattern = R"({
            "title": "Concurrent Stress Test",
            "tempo": 120,
            "sections": {
                "test": {
                    "bars": 4,
                    "layers": {
                        "melody": [[1.0, "C4"], [2.0, "E4"], [3.0, "G4"], [4.0, "C4"]],
                        "harmony": [[1.0, "C3"], [2.0, "E3"], [3.0, "G3"], [4.0, "C3"]]
                    }
                }
            }
        })";
        
        std::vector<std::thread> threads;
        
        for (int threadId = 0; threadId < numThreads; ++threadId)
        {
            threads.emplace_back([&, threadId]() {
                MusicPatternSystem patternSystem;
                PatternParser parser(patternSystem);
                
                for (int op = 0; op < operationsPerThread; ++op)
                {
                    try
                    {
                        auto chart = parser.parseChart(testPattern);
                        if (chart && chart->getNumSections() > 0)
                        {
                            auto section = chart->getSection(0);
                            if (section && section->getNumPhrases() > 0)
                            {
                                auto phrase = section->getPhrase(0);
                                if (phrase && phrase->getNumBars() > 0)
                                {
                                    // Access random bars to stress test
                                    Random random;
                                    int randomBar = random.nextInt(phrase->getNumBars());
                                    auto bar = phrase->getBar(randomBar);
                                    if (bar)
                                    {
                                        auto midi = bar->toMidiSequence();
                                        if (midi->getNumEvents() > 0)
                                        {
                                            ++successCount;
                                        }
                                        else
                                        {
                                            ++failureCount;
                                        }
                                    }
                                    else
                                    {
                                        ++failureCount;
                                    }
                                }
                                else
                                {
                                    ++failureCount;
                                }
                            }
                            else
                            {
                                ++failureCount;
                            }
                        }
                        else
                        {
                            ++failureCount;
                        }
                        
                        // Small delay to increase chance of race conditions
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    catch (...)
                    {
                        ++failureCount;
                    }
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads)
        {
            thread.join();
        }
        
        int totalOperations = numThreads * operationsPerThread;
        int totalProcessed = successCount + failureCount;
        
        expect(totalProcessed == totalOperations, 
               "Should process all operations: " + String(totalProcessed) + "/" + String(totalOperations));
        expect(failureCount < totalOperations / 10, 
               "Should have low failure rate: " + String(failureCount) + "/" + String(totalOperations));
        
        logMessage("Concurrent stress: " + String(successCount) + " successes, " + 
                  String(failureCount) + " failures from " + String(totalOperations) + " operations");
        
        logMessage("PASS: Concurrent access stress test completed");
    }
    
    void testMemoryPressureHandling()
    {
        beginTest("Memory pressure handling");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Create patterns that would use significant memory
        const int numLargePatterns = 20;
        Array<std::shared_ptr<Chart>> charts;
        
        for (int i = 0; i < numLargePatterns; ++i)
        {
            String largePattern = R"({
                "title": "Memory Pressure Test )" + String(i) + R"(",
                "tempo": 120,
                "sections": {
                    "large": {
                        "bars": 50,
                        "layers": {)";
            
            // Add multiple layers with many events
            for (int layer = 0; layer < 10; ++layer)
            {
                largePattern += "\"layer" + String(layer) + "\": [";
                for (int event = 0; event < 200; ++event)
                {
                    double time = 1.0 + event * 0.25;
                    largePattern += "[" + String(time, 2) + ", \"C4\", 0.7, 0.25]";
                    if (event < 199) largePattern += ",";
                }
                largePattern += "]";
                if (layer < 9) largePattern += ",";
            }
            
            largePattern += R"(
                        }
                    }
                }
            })";
            
            auto chart = parser.parseChart(largePattern);
            if (chart)
            {
                charts.add(chart);
                logMessage("Created memory pressure chart " + String(i) + 
                          " (estimated " + String(largePattern.length() / 1024) + "KB)");
            }
            else
            {
                logMessage("Failed to create chart " + String(i) + " - possible memory pressure");
            }
        }
        
        expect(charts.size() >= numLargePatterns / 2, 
               "Should create at least half the charts under memory pressure");
        
        // Test access to all charts
        int accessibleCharts = 0;
        for (auto& chart : charts)
        {
            if (chart && chart->getNumSections() > 0)
            {
                auto section = chart->getSection(0);
                if (section && section->getNumPhrases() > 0)
                {
                    ++accessibleCharts;
                }
            }
        }
        
        expect(accessibleCharts == charts.size(), 
               "All created charts should remain accessible");
        
        logMessage("Memory pressure test: " + String(charts.size()) + " charts created, " + 
                  String(accessibleCharts) + " accessible");
        
        // Clear charts to test cleanup
        charts.clear();
        
        logMessage("PASS: Memory pressure handling test completed");
    }
    
    void testExtremeEdgeCases()
    {
        beginTest("Extreme edge cases");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test various extreme inputs
        StringArray extremeCases = {
            // Empty sections
            R"({"title": "Empty", "sections": {}})",
            
            // Single note
            R"({"title": "Single", "sections": {"main": [[1.0, "C4"]]}})",
            
            // Extreme timing values
            R"({"title": "Extreme Time", "sections": {"main": [[0.001, "C4"], [999999.999, "D4"]]}})",
            
            // Extreme velocities
            R"({"title": "Extreme Velocity", "sections": {"main": [[1.0, "C4", 0.0], [2.0, "D4", 1.0]]}})",
            
            // Extreme durations
            R"({"title": "Extreme Duration", "sections": {"main": [[1.0, "C4", 0.8, 0.001], [2.0, "D4", 0.8, 1000.0]]}})",
            
            // Very long section names
            R"({"title": "Long Names", "sections": {")" + String::repeatedString("VeryLongSectionName", 100) + R"(": [[1.0, "C4"]]}})",
            
            // Many empty layers
            R"({"title": "Empty Layers", "sections": {"main": {"layers": {"layer1": [], "layer2": [], "layer3": []}}}})",
            
            // Circular references (should be handled gracefully)
            R"({"title": "Circular", "patterns": {"a": [{"pattern": "b"}], "b": [{"pattern": "a"}]}, "sections": {"main": {"pattern": "a"}}})"
        };
        
        for (int i = 0; i < extremeCases.size(); ++i)
        {
            const auto& testCase = extremeCases[i];
            
            try
            {
                auto chart = parser.parseChart(testCase);
                // Should not crash, regardless of whether it succeeds or fails
                
                if (chart)
                {
                    logMessage("Extreme case " + String(i) + ": Parsed successfully");
                    
                    // Basic validation
                    expect(chart->getTitle().isNotEmpty(), "Should have a title");
                    
                    if (chart->getNumSections() > 0)
                    {
                        auto section = chart->getSection(0);
                        if (section && section->getNumPhrases() > 0)
                        {
                            auto phrase = section->getPhrase(0);
                            if (phrase && phrase->getNumBars() > 0)
                            {
                                auto bar = phrase->getBar(0);
                                if (bar)
                                {
                                    auto midi = bar->toMidiSequence();
                                    // Just verify we can get MIDI sequence without crashing
                                    logMessage("Extreme case " + String(i) + ": " + 
                                              String(midi->getNumEvents()) + " MIDI events");
                                }
                            }
                        }
                    }
                }
                else
                {
                    logMessage("Extreme case " + String(i) + ": Gracefully rejected");
                }
            }
            catch (...)
            {
                logMessage("Extreme case " + String(i) + ": Exception caught (should not happen)");
                expect(false, "Parser should not throw exceptions for extreme cases");
            }
        }
        
        logMessage("PASS: Extreme edge cases test completed");
    }
    
    void testRandomPatternGeneration()
    {
        beginTest("Random pattern generation stress");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        Random random;
        
        const int numRandomPatterns = 100;
        int successCount = 0;
        int failureCount = 0;
        
        for (int i = 0; i < numRandomPatterns; ++i)
        {
            // Generate semi-random but valid JSON patterns
            String randomPattern = R"({
                "title": "Random )" + String(i) + R"(",
                "tempo": )" + String(60 + random.nextInt(120)) + R"(",
                "sections": {
                    "main": {
                        "bars": )" + String(1 + random.nextInt(8)) + R"(",
                        "layers": {
                            "melody": [)";
            
            int numEvents = 5 + random.nextInt(20);
            for (int e = 0; e < numEvents; ++e)
            {
                double time = 1.0 + random.nextDouble() * 16.0;
                String note = "C" + String(3 + random.nextInt(3));
                float velocity = 0.3f + random.nextFloat() * 0.7f;
                float duration = 0.1f + random.nextFloat() * 2.0f;
                
                randomPattern += "[" + String(time, 2) + ", \"" + note + "\", " + 
                                String(velocity, 2) + ", " + String(duration, 2) + "]";
                if (e < numEvents - 1) randomPattern += ",";
            }
            
            randomPattern += R"(
                            ]
                        }
                    }
                }
            })";
            
            auto chart = parser.parseChart(randomPattern);
            if (chart && chart->getNumSections() > 0)
            {
                ++successCount;
            }
            else
            {
                ++failureCount;
            }
        }
        
        double successRate = (double)successCount / numRandomPatterns;
        expect(successRate >= 0.8, "Should have high success rate for random valid patterns");
        
        logMessage("Random pattern generation: " + String(successCount) + "/" + 
                  String(numRandomPatterns) + " succeeded (" + 
                  String(successRate * 100.0, 1) + "%)");
        
        logMessage("PASS: Random pattern generation stress test completed");
    }
    
    void testLongRunningParsing()
    {
        beginTest("Long-running parsing stress");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Run parsing operations continuously for a period
        const double testDurationSeconds = 5.0;
        double startTime = Time::getMillisecondCounterHiRes();
        double endTime = startTime + testDurationSeconds * 1000.0;
        
        String testPattern = R"({
            "title": "Long Running Test",
            "tempo": 120,
            "sections": {
                "test": {
                    "bars": 10,
                    "layers": {
                        "layer1": [[1.0, "C4"], [2.0, "E4"], [3.0, "G4"], [4.0, "C4"]],
                        "layer2": [[1.5, "E3"], [2.5, "G3"], [3.5, "B3"], [4.5, "E3"]],
                        "layer3": [[2.0, "G2"], [4.0, "C3"]]
                    }
                }
            }
        })";
        
        int operationCount = 0;
        int successCount = 0;
        
        while (Time::getMillisecondCounterHiRes() < endTime)
        {
            auto chart = parser.parseChart(testPattern);
            ++operationCount;
            
            if (chart && chart->getNumSections() > 0)
            {
                ++successCount;
            }
            
            // Small delay to prevent excessive CPU usage
            Thread::sleep(1);
        }
        
        double actualDuration = (Time::getMillisecondCounterHiRes() - startTime) / 1000.0;
        double operationsPerSecond = operationCount / actualDuration;
        double successRate = (double)successCount / operationCount;
        
        expect(operationCount > 100, "Should perform many operations in " + 
               String(testDurationSeconds, 1) + " seconds");
        expect(successRate >= 0.95, "Should maintain high success rate over time");
        
        logMessage("Long-running test: " + String(operationCount) + " operations in " + 
                  String(actualDuration, 2) + "s (" + String(operationsPerSecond, 1) + " ops/sec), " + 
                  String(successRate * 100.0, 1) + "% success rate");
        
        logMessage("PASS: Long-running parsing stress test completed");
    }
    
    void testResourceExhaustionRecovery()
    {
        beginTest("Resource exhaustion recovery");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test parser behavior when system resources are stressed
        Array<std::shared_ptr<Chart>> chartCache;
        
        // Gradually increase pattern complexity until we hit limits
        for (int complexity = 1; complexity <= 20; ++complexity)
        {
            String complexPattern = R"({
                "title": "Complexity )" + String(complexity) + R"(",
                "tempo": 120,
                "sections": {
                    "complex": {
                        "bars": )" + String(complexity * 10) + R"(,
                        "layers": {)";
            
            // Add layers proportional to complexity
            for (int layer = 0; layer < complexity; ++layer)
            {
                complexPattern += "\"layer" + String(layer) + "\": [";
                
                // Add events proportional to complexity
                for (int event = 0; event < complexity * 20; ++event)
                {
                    double time = 1.0 + event * 0.1;
                    complexPattern += "[" + String(time, 1) + ", \"C4\", 0.7, 0.1]";
                    if (event < complexity * 20 - 1) complexPattern += ",";
                }
                
                complexPattern += "]";
                if (layer < complexity - 1) complexPattern += ",";
            }
            
            complexPattern += R"(
                        }
                    }
                }
            })";
            
            double startTime = Time::getMillisecondCounterHiRes();
            auto chart = parser.parseChart(complexPattern);
            double parseTime = Time::getMillisecondCounterHiRes() - startTime;
            
            if (chart)
            {
                chartCache.add(chart);
                logMessage("Complexity " + String(complexity) + ": Success (" + 
                          String(parseTime, 0) + "ms)");
                
                if (parseTime > 5000.0) // If taking too long, stop increasing complexity
                {
                    logMessage("Stopping at complexity " + String(complexity) + 
                              " due to parse time (" + String(parseTime, 0) + "ms)");
                    break;
                }
            }
            else
            {
                logMessage("Complexity " + String(complexity) + ": Failed - resource exhaustion");
                
                // Test recovery by trying a simple pattern
                String simplePattern = R"({
                    "title": "Recovery Test",
                    "tempo": 120,
                    "sections": {
                        "simple": [[1.0, "C4"]]
                    }
                })";
                
                auto recoveryChart = parser.parseChart(simplePattern);
                expect(recoveryChart != nullptr, "Should recover after resource exhaustion");
                
                if (recoveryChart)
                {
                    logMessage("Successfully recovered after resource exhaustion");
                }
                break;
            }
        }
        
        expect(chartCache.size() >= 5, "Should handle reasonable complexity levels");
        
        logMessage("Resource exhaustion test: Created " + String(chartCache.size()) + 
                  " increasingly complex charts");
        
        logMessage("PASS: Resource exhaustion recovery test completed");
    }
};

// Register test with JUCE's unit test system
static PatternParsingStressTests patternParsingStressTests;

} // namespace covertracks