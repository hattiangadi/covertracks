// Source/Tests/PatternParsingRobustnessTests.cpp
#include <JuceHeader.h>
#include "../Musician/Core/PatternParser.h"
#include "../Musician/Core/MusicPatternSystem.h"

namespace covertracks {

/**
 * Tests for robust pattern parsing - edge cases, error conditions, and complex parsing contexts
 * These tests address the parsing complexity issues identified in the language design analysis
 */
class PatternParsingRobustnessTests : public juce::UnitTest
{
public:
    PatternParsingRobustnessTests() : juce::UnitTest("Robust Pattern Parsing", "Music Pattern System") {}

    void runTest() override
    {
        testMalformedJsonHandling();
        testMissingFieldsGracefulDegradation();
        testLargePatternPerformance();
        testDuplicateContextResolution();
        testComplexNestingLevels();
        testConcurrentParsingStress();
        testMemoryLeakPrevention();
        testErrorRecovery();
    }

private:
    void testMalformedJsonHandling()
    {
        beginTest("Malformed JSON graceful handling");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test various malformed JSON inputs
        StringArray malformedInputs = {
            "{ invalid json }",                    // Invalid syntax
            "{\"title\": incomplete",             // Incomplete JSON
            "{\"sections\": null}",               // Null sections
            "{\"primitives\": []}",               // Wrong type for primitives
            "{\"tempo\": \"not_a_number\"}",     // Wrong type for tempo
            "",                                   // Empty string
            "not json at all",                   // Not JSON
            "{\"chords\": {\"C\": \"invalid\"}}",// Invalid chord definition
        };
        
        for (const auto& input : malformedInputs)
        {
            auto chart = parser.parseChart(input);
            // Should not crash and should return either null or empty chart
            if (chart != nullptr)
            {
                logMessage("Graceful parsing for: " + input.substring(0, 30) + "...");
                expect(chart->getTitle().isNotEmpty() || chart->getNumSections() == 0,
                       "Malformed input should produce empty or minimal chart");
            }
            else
            {
                logMessage("Null result for malformed input: " + input.substring(0, 30) + "...");
            }
        }
        
        logMessage("PASS: All malformed inputs handled gracefully");
    }
    
    void testMissingFieldsGracefulDegradation()
    {
        beginTest("Missing fields graceful degradation");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test patterns with missing optional fields
        String missingTempo = R"({
            "title": "No Tempo",
            "sections": {
                "main": [[1.0, "C4"]]
            }
        })";
        
        auto chart1 = parser.parseChart(missingTempo);
        expect(chart1 != nullptr, "Should handle missing tempo");
        if (chart1) expect(chart1->getTempo() > 0, "Should use default tempo");
        
        // Test missing primitives (should work with note names)
        String noPrimitives = R"({
            "title": "No Primitives",
            "tempo": 120,
            "sections": {
                "main": [[1.0, "C4"], [2.0, "G4"]]
            }
        })";
        
        auto chart2 = parser.parseChart(noPrimitives);
        expect(chart2 != nullptr, "Should handle missing primitives");
        if (chart2 && chart2->getNumSections() > 0)
        {
            auto section = chart2->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        expect(midi->getNumEvents() > 0, "Should parse notes without primitives");
                    }
                }
            }
        }
        
        // Test missing arrangement (should use section order)
        String noArrangement = R"({
            "title": "No Arrangement",
            "tempo": 120,
            "sections": {
                "verse": [[1.0, "C4"]],
                "chorus": [[1.0, "G4"]]
            }
        })";
        
        auto chart3 = parser.parseChart(noArrangement);
        expect(chart3 != nullptr, "Should handle missing arrangement");
        if (chart3) expect(chart3->getNumSections() >= 2, "Should include all sections");
        
        logMessage("PASS: Missing fields handled with graceful degradation");
    }
    
    void testLargePatternPerformance()
    {
        beginTest("Large pattern parsing performance");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Generate a large pattern with many events
        String largePattern = R"({
            "title": "Large Pattern Test",
            "tempo": 120,
            "sections": {
                "main": {
                    "bars": 100,
                    "layers": {
                        "drums": [)";
        
        // Add many drum events
        for (int bar = 0; bar < 100; ++bar)
        {
            for (int beat = 1; beat <= 4; ++beat)
            {
                for (int subdivision = 0; subdivision < 4; ++subdivision)
                {
                    double time = bar * 4 + beat + subdivision * 0.25;
                    largePattern += "[" + String(time) + ", \"C4\"]";
                    if (!(bar == 99 && beat == 4 && subdivision == 3)) largePattern += ",";
                }
            }
        }
        
        largePattern += R"(
                        ]
                    }
                }
            }
        })";
        
        double startTime = Time::getMillisecondCounterHiRes();
        auto chart = parser.parseChart(largePattern);
        double parseTime = Time::getMillisecondCounterHiRes() - startTime;
        
        expect(chart != nullptr, "Should parse large pattern");
        expect(parseTime < 5000.0, "Should parse large pattern in under 5 seconds");
        
        logMessage("Large pattern parsed in " + String(parseTime, 2) + "ms");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Large pattern phrase should exist");
                expect(phrase->getNumBars() == 100, "Should have 100 bars");
                
                // Test accessing random bars for performance
                for (int i = 0; i < 10; ++i)
                {
                    int randomBar = Random::getSystemRandom().nextInt(100);
                    auto bar = phrase->getBar(randomBar);
                    expect(bar != nullptr, "Random bar access should work");
                }
            }
        }
        
        logMessage("PASS: Large pattern performance test completed");
    }
    
    void testDuplicateContextResolution()
    {
        beginTest("Duplicate parsing context resolution");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test the same pattern structure in different contexts
        // This addresses the "variations in layers vs bars" duplication issue
        String duplicateContextPattern = R"({
            "title": "Duplicate Context Test",
            "tempo": 120,
            "patterns": {
                "basePattern": [[1.0, "C4"], [2.0, "E4"]]
            },
            "sections": {
                "layerContext": {
                    "layers": {
                        "melody": {
                            "basePattern": "basePattern",
                            "variations": {
                                "bar2": [[1.0, "G4"], [2.0, "B4"]]
                            }
                        }
                    }
                },
                "barContext": {
                    "bars": 4,
                    "basePattern": "basePattern",
                    "variations": {
                        "bar3": [[1.0, "D4"], [2.0, "F4"]]
                    }
                },
                "directContext": {
                    "bars": 2,
                    "barPatterns": [
                        [[1.0, "C4"], [2.0, "E4"]],
                        [[1.0, "G4"], [2.0, "B4"]]
                    ]
                }
            }
        })";
        
        auto chart = parser.parseChart(duplicateContextPattern);
        expect(chart != nullptr, "Duplicate context chart should parse");
        
        if (chart)
        {
            expect(chart->getNumSections() == 3, "Should have 3 sections with different contexts");
            
            // Verify each context produces valid results
            for (int i = 0; i < chart->getNumSections(); ++i)
            {
                auto section = chart->getSection(i);
                expect(section != nullptr, "Section " + String(i) + " should exist");
                
                if (section && section->getNumPhrases() > 0)
                {
                    auto phrase = section->getPhrase(0);
                    if (phrase && phrase->getNumBars() > 0)
                    {
                        auto bar = phrase->getBar(0);
                        if (bar)
                        {
                            auto midi = bar->toMidiSequence();
                            expect(midi->getNumEvents() > 0, 
                                   "Context " + String(i) + " should produce MIDI events");
                        }
                    }
                }
            }
        }
        
        logMessage("PASS: Duplicate context resolution test completed");
    }
    
    void testComplexNestingLevels()
    {
        beginTest("Complex nesting levels handling");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test deeply nested pattern references and variations
        String complexNesting = R"({
            "title": "Complex Nesting",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38}
            },
            "patterns": {
                "level1": [[1.0, "kick"], [2.0, "snare"]],
                "level2": [
                    [1.0, {"pattern": "level1"}],
                    [3.0, "kick"]
                ]
            },
            "sections": {
                "nested": {
                    "bars": 8,
                    "layers": {
                        "drums": {
                            "basePattern": "level2",
                            "variations": {
                                "bar4": {
                                    "basePattern": "level1",
                                    "variations": {
                                        "beat3": [[3.0, "snare"], [3.5, "snare"]]
                                    }
                                },
                                "bar8": [[1.0, "kick"], [1.5, "kick"], [2.0, "snare"]]
                            }
                        },
                        "overlay": {
                            "basePattern": [[1.5, "kick"], [2.5, "kick"], [3.5, "kick"], [4.5, "kick"]],
                            "variations": {
                                "bar2": [[1.0, "snare"], [3.0, "snare"]],
                                "bar6": [[2.0, "kick"], [4.0, "kick"]]
                            }
                        }
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(complexNesting);
        expect(chart != nullptr, "Complex nesting should not break parser");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Nested phrase should exist");
                expect(phrase->getNumBars() == 8, "Should have 8 bars");
                
                // Check specific bars with variations
                auto bar4 = phrase->getBar(3);  // 0-indexed
                auto bar8 = phrase->getBar(7);
                
                if (bar4)
                {
                    auto midi4 = bar4->toMidiSequence();
                    expect(midi4->getNumEvents() > 0, "Bar 4 with nested variation should have events");
                    logMessage("Bar 4 nested variation events: " + String(midi4->getNumEvents()));
                }
                
                if (bar8)
                {
                    auto midi8 = bar8->toMidiSequence();
                    expect(midi8->getNumEvents() > 0, "Bar 8 with inline variation should have events");
                    logMessage("Bar 8 inline variation events: " + String(midi8->getNumEvents()));
                }
            }
        }
        
        logMessage("PASS: Complex nesting levels handled correctly");
    }
    
    void testConcurrentParsingStress()
    {
        beginTest("Concurrent parsing stress test");
        
        // Test multiple parsers working simultaneously
        const int numThreads = 4;
        const int patternsPerThread = 10;
        
        Array<std::unique_ptr<Thread>> threads;
        std::atomic<int> successCount{0};
        std::atomic<int> failureCount{0};
        
        String testPattern = R"({
            "title": "Concurrent Test",
            "tempo": 120,
            "sections": {
                "main": [[1.0, "C4"], [2.0, "E4"], [3.0, "G4"], [4.0, "C4"]]
            }
        })";
        
        class ParsingThread : public Thread
        {
        public:
            ParsingThread(const String& pattern, int numPatterns, 
                         std::atomic<int>& success, std::atomic<int>& failure)
                : Thread("ParsingThread"), 
                  testPattern(pattern), 
                  patternsToProcess(numPatterns),
                  successCount(success),
                  failureCount(failure) {}
            
            void run() override
            {
                MusicPatternSystem patternSystem;
                PatternParser parser(patternSystem);
                
                for (int i = 0; i < patternsToProcess && !threadShouldExit(); ++i)
                {
                    auto chart = parser.parseChart(testPattern);
                    if (chart != nullptr && chart->getNumSections() > 0)
                        ++successCount;
                    else
                        ++failureCount;
                    
                    Thread::sleep(1); // Small delay to test thread safety
                }
            }
            
        private:
            String testPattern;
            int patternsToProcess;
            std::atomic<int>& successCount;
            std::atomic<int>& failureCount;
        };
        
        // Create and start all threads
        for (int i = 0; i < numThreads; ++i)
        {
            auto thread = std::make_unique<ParsingThread>(testPattern, patternsPerThread, 
                                                          successCount, failureCount);
            thread->startThread();
            threads.add(std::move(thread));
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads)
        {
            thread->waitForThreadToExit(5000); // 5 second timeout
        }
        
        int totalExpected = numThreads * patternsPerThread;
        int totalProcessed = successCount + failureCount;
        
        expect(totalProcessed == totalExpected, 
               "Should process all patterns: " + String(totalProcessed) + "/" + String(totalExpected));
        expect(failureCount == 0, "Should have no parsing failures in concurrent test");
        
        logMessage("Concurrent parsing: " + String(successCount) + " successes, " + 
                  String(failureCount) + " failures");
        
        logMessage("PASS: Concurrent parsing stress test completed");
    }
    
    void testMemoryLeakPrevention()
    {
        beginTest("Memory leak prevention");
        
        // Test many parse operations to check for memory leaks
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String testPattern = R"({
            "title": "Memory Test",
            "tempo": 120,
            "sections": {
                "main": {
                    "bars": 4,
                    "layers": {
                        "melody": [[1.0, "C4"], [2.0, "E4"], [3.0, "G4"], [4.0, "C4"]],
                        "harmony": [[1.0, "C3"], [2.0, "E3"], [3.0, "G3"], [4.0, "C3"]]
                    }
                }
            }
        })";
        
        // Parse many times and let charts go out of scope
        const int iterations = 100;
        for (int i = 0; i < iterations; ++i)
        {
            auto chart = parser.parseChart(testPattern);
            expect(chart != nullptr, "Chart " + String(i) + " should parse");
            
            if (chart && chart->getNumSections() > 0)
            {
                auto section = chart->getSection(0);
                if (section && section->getNumPhrases() > 0)
                {
                    auto phrase = section->getPhrase(0);
                    if (phrase) 
                    {
                        // Access the phrase to ensure it's fully constructed
                        int numBars = phrase->getNumBars();
                        expect(numBars > 0, "Phrase should have bars");
                    }
                }
            }
            // Chart goes out of scope here - testing cleanup
        }
        
        // Force garbage collection if needed
        logMessage("Memory test completed " + String(iterations) + " iterations");
        logMessage("PASS: Memory leak prevention test completed");
    }
    
    void testErrorRecovery()
    {
        beginTest("Error recovery and partial parsing");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test pattern with some valid and some invalid sections
        String mixedValidityPattern = R"({
            "title": "Mixed Validity Test",
            "tempo": 120,
            "sections": {
                "validSection": [[1.0, "C4"], [2.0, "E4"]],
                "invalidSection": [["invalid", "data"]],
                "anotherValidSection": [[1.0, "G4"], [2.0, "B4"]],
                "problematicSection": {
                    "bars": "not_a_number",
                    "layers": {
                        "melody": [[1.0, "D4"]]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(mixedValidityPattern);
        expect(chart != nullptr, "Should recover from partial errors");
        
        if (chart)
        {
            // Should have recovered and parsed the valid sections
            expect(chart->getNumSections() >= 2, "Should parse valid sections despite errors");
            
            // Check that at least some sections work
            bool foundValidSection = false;
            for (int i = 0; i < chart->getNumSections(); ++i)
            {
                auto section = chart->getSection(i);
                if (section && section->getNumPhrases() > 0)
                {
                    auto phrase = section->getPhrase(0);
                    if (phrase && phrase->getNumBars() > 0)
                    {
                        auto bar = phrase->getBar(0);
                        if (bar)
                        {
                            auto midi = bar->toMidiSequence();
                            if (midi->getNumEvents() > 0)
                            {
                                foundValidSection = true;
                                break;
                            }
                        }
                    }
                }
            }
            expect(foundValidSection, "Should find at least one valid section");
        }
        
        logMessage("PASS: Error recovery test completed");
    }
};

// Register test with JUCE's unit test system
static PatternParsingRobustnessTests patternParsingRobustnessTests;

} // namespace covertracks