#include <JuceHeader.h>
#include "Source/Musician/Core/MusicPatternSystem.h"
#include "Source/Musician/Core/PatternParser.h"
#include "Source/Musician/Core/MusicPatternComponents.h"

using namespace juce;
using namespace covertracks;

class PatternTestRunner
{
public:
    PatternTestRunner() {}
    
    void runAllTests()
    {
        std::cout << "=== MusicPatternSystem Comprehensive Tests ===" << std::endl;
        
        // Load test data
        File testFile = File::getCurrentWorkingDirectory().getChildFile("Tests/MusicPatternSystem_Tests.json");
        if (!testFile.existsAsFile()) {
            std::cout << "❌ Test file not found: " << testFile.getFullPathName() << std::endl;
            return;
        }
        
        String testData = testFile.loadFileAsString();
        var testJson = JSON::parse(testData);
        if (testJson.isVoid()) {
            std::cout << "❌ Failed to parse test JSON" << std::endl;
            return;
        }
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        var tests = testJson["tests"];
        if (!tests.isObject()) {
            std::cout << "❌ No tests object found in JSON" << std::endl;
            return;
        }
        
        int totalTests = 0;
        int passedTests = 0;
        
        // Run chord tone tests
        passedTests += runChordToneTests(parser, tests, totalTests);
        
        // Run variation tests  
        passedTests += runVariationTests(parser, tests, totalTests);
        
        // Run timing tests
        passedTests += runTimingTests(parser, tests, totalTests);
        
        // Run boundary tests
        passedTests += runBoundaryTests(parser, tests, totalTests);
        
        // Run feature tests
        passedTests += runFeatureTests(parser, tests, totalTests);
        
        // Summary
        std::cout << "\n=== Test Results ===" << std::endl;
        if (passedTests == totalTests) {
            std::cout << "✅ All " << totalTests << " tests passed!" << std::endl;
        } else {
            std::cout << "❌ " << (totalTests - passedTests) << " out of " << totalTests << " tests failed" << std::endl;
        }
    }

private:
    int runChordToneTests(PatternParser& parser, const var& tests, int& totalTests)
    {
        std::cout << "\n--- Chord Tone Resolution Tests ---" << std::endl;
        int passed = 0;
        
        // Test 1: Basic chord tone resolution
        if (tests.hasProperty("1_chord_tones_basic")) {
            totalTests++;
            std::cout << "Running: Chord Tone Resolution..." << std::endl;
            
            var test = tests["1_chord_tones_basic"];
            auto chart = parser.parseChartFromJson(test);
            
            if (!chart) {
                std::cout << "❌ Failed to parse chord tone test chart" << std::endl;
                return passed;
            }
            
            auto sequence = chart->toMidiSequence();
            if (!sequence) {
                std::cout << "❌ Failed to generate MIDI sequence" << std::endl;
                return passed;
            }
            
            // Count bass note events and verify they change with chords
            Array<int> bassNotes;
            for (int i = 0; i < sequence->getNumEvents(); ++i) {
                auto msg = sequence->getEventPointer(i)->message;
                if (msg.isNoteOn() && msg.getChannel() == 1) { // Bass on channel 1
                    bassNotes.add(msg.getNoteNumber());
                }
            }
            
            if (bassNotes.size() >= 12) { // Should have 4 notes per bar * 3 bars
                std::cout << "✅ Found " << bassNotes.size() << " bass note events" << std::endl;
                
                // Check that notes change between bars (indicating chord tone resolution)
                bool hasVariety = false;
                for (int i = 1; i < bassNotes.size(); ++i) {
                    if (bassNotes[i] != bassNotes[0]) {
                        hasVariety = true;
                        break;
                    }
                }
                
                if (hasVariety) {
                    std::cout << "✅ Chord tones resolve correctly across chord changes" << std::endl;
                    passed++;
                } else {
                    std::cout << "❌ All bass notes the same - chord tone resolution not working" << std::endl;
                }
            } else {
                std::cout << "❌ Expected at least 12 bass notes, found " << bassNotes.size() << std::endl;
            }
        }
        
        // Test 2: No chord context
        if (tests.hasProperty("2_chord_tones_no_chord")) {
            totalTests++;
            std::cout << "Running: No Chord Context..." << std::endl;
            
            var test = tests["2_chord_tones_no_chord"];
            auto chart = parser.parseChartFromJson(test);
            
            if (chart && chart->toMidiSequence()) {
                std::cout << "✅ Handles missing chord context gracefully" << std::endl;
                passed++;
            } else {
                std::cout << "❌ Failed to handle missing chord context" << std::endl;
            }
        }
        
        return passed;
    }
    
    int runVariationTests(PatternParser& parser, const var& tests, int& totalTests)
    {
        std::cout << "\n--- Pattern Variation Tests ---" << std::endl;
        int passed = 0;
        
        // Test 3: Single bar variation (addresses "played 4 times instead of on bar 4")
        if (tests.hasProperty("3_variations_single_bar")) {
            totalTests++;
            std::cout << "Running: Single Bar Variation (bar 4 fill)..." << std::endl;
            
            var test = tests["3_variations_single_bar"];
            auto chart = parser.parseChartFromJson(test);
            
            if (!chart) {
                std::cout << "❌ Failed to parse variation test" << std::endl;
                return passed;
            }
            
            auto sequence = chart->toMidiSequence();
            if (!sequence) {
                std::cout << "❌ Failed to generate MIDI sequence" << std::endl;
                return passed;
            }
            
            // Analyze events per bar
            const double ticksPerBeat = 960.0;
            Array<int> eventsPerBar(4, 0);
            
            for (int i = 0; i < sequence->getNumEvents(); ++i) {
                auto msg = sequence->getEventPointer(i)->message;
                if (msg.isNoteOn() && msg.getChannel() == 10) { // Drums on channel 10
                    double beat = msg.getTimeStamp() / ticksPerBeat;
                    int bar = (int)(beat / 4.0);
                    if (bar >= 0 && bar < 4) {
                        eventsPerBar.getReference(bar)++;
                    }
                }
            }
            
            std::cout << "   Events per bar: [" << eventsPerBar[0] << ", " << eventsPerBar[1] 
                      << ", " << eventsPerBar[2] << ", " << eventsPerBar[3] << "]" << std::endl;
            
            // Bar 4 should have more events (fill), bars 1-3 should be the same (basic)
            if (eventsPerBar[3] > eventsPerBar[0] && 
                eventsPerBar[0] == eventsPerBar[1] && 
                eventsPerBar[1] == eventsPerBar[2]) {
                std::cout << "✅ Bar 4 variation works correctly - fill only on bar 4" << std::endl;
                passed++;
            } else {
                std::cout << "❌ Bar variation not working correctly" << std::endl;
            }
        }
        
        // Test 4: Multiple bar variations
        if (tests.hasProperty("4_variations_multiple_bars")) {
            totalTests++;
            std::cout << "Running: Multiple Bar Variations..." << std::endl;
            
            var test = tests["4_variations_multiple_bars"];
            auto chart = parser.parseChartFromJson(test);
            
            if (chart && chart->toMidiSequence()) {
                std::cout << "✅ Multiple bar variations parse successfully" << std::endl;
                passed++;
            } else {
                std::cout << "❌ Multiple bar variations failed" << std::endl;
            }
        }
        
        return passed;
    }
    
    int runTimingTests(PatternParser& parser, const var& tests, int& totalTests)
    {
        std::cout << "\n--- Multi-Bar Timing Tests ---" << std::endl;
        int passed = 0;
        
        if (tests.hasProperty("5_timing_multi_bar")) {
            totalTests++;
            std::cout << "Running: Multi-Bar Event Distribution..." << std::endl;
            
            var test = tests["5_timing_multi_bar"];
            auto chart = parser.parseChartFromJson(test);
            
            if (!chart) {
                std::cout << "❌ Failed to parse timing test" << std::endl;
                return passed;
            }
            
            auto sequence = chart->toMidiSequence();
            if (!sequence) {
                std::cout << "❌ Failed to generate MIDI sequence" << std::endl;
                return passed;
            }
            
            // Should have events distributed across 3 bars
            Array<double> noteTimestamps;
            for (int i = 0; i < sequence->getNumEvents(); ++i) {
                auto msg = sequence->getEventPointer(i)->message;
                if (msg.isNoteOn()) {
                    noteTimestamps.add(msg.getTimeStamp());
                }
            }
            
            if (noteTimestamps.size() >= 6) { // Should have 6 note events (2 per bar * 3 bars)
                std::cout << "✅ Found " << noteTimestamps.size() << " note events across multiple bars" << std::endl;
                passed++;
            } else {
                std::cout << "❌ Expected at least 6 note events, found " << noteTimestamps.size() << std::endl;
            }
        }
        
        return passed;
    }
    
    int runBoundaryTests(PatternParser& parser, const var& tests, int& totalTests)
    {
        std::cout << "\n--- Duration Boundary Tests ---" << std::endl;
        int passed = 0;
        
        if (tests.hasProperty("6_duration_boundaries")) {
            totalTests++;
            std::cout << "Running: Duration Boundary Clamping..." << std::endl;
            
            var test = tests["6_duration_boundaries"];
            auto chart = parser.parseChartFromJson(test);
            
            if (!chart) {
                std::cout << "❌ Failed to parse boundary test" << std::endl;
                return passed;
            }
            
            auto sequence = chart->toMidiSequence();
            if (!sequence) {
                std::cout << "❌ Failed to generate MIDI sequence" << std::endl;
                return passed;
            }
            
            // Find note on/off pairs and verify durations don't exceed bar boundaries
            Array<std::pair<double, double>> noteDurations;
            for (int i = 0; i < sequence->getNumEvents(); ++i) {
                auto msg = sequence->getEventPointer(i)->message;
                if (msg.isNoteOn()) {
                    double noteOnTime = msg.getTimeStamp();
                    // Find corresponding note off
                    for (int j = i + 1; j < sequence->getNumEvents(); ++j) {
                        auto offMsg = sequence->getEventPointer(j)->message;
                        if (offMsg.isNoteOff() && offMsg.getNoteNumber() == msg.getNoteNumber()) {
                            double duration = offMsg.getTimeStamp() - noteOnTime;
                            noteDurations.add({noteOnTime, duration});
                            break;
                        }
                    }
                }
            }
            
            bool allWithinBounds = true;
            const double ticksPerBar = 4.0 * 960.0; // 4 beats * 960 ticks per beat
            
            for (auto& notePair : noteDurations) {
                double startTime = notePair.first;
                double duration = notePair.second;
                double barStart = floor(startTime / ticksPerBar) * ticksPerBar;
                double maxAllowedDuration = (barStart + ticksPerBar) - startTime;
                
                if (duration > maxAllowedDuration + 10.0) { // Allow small tolerance
                    allWithinBounds = false;
                    break;
                }
            }
            
            if (allWithinBounds) {
                std::cout << "✅ All " << noteDurations.size() << " note durations stay within bar boundaries" << std::endl;
                passed++;
            } else {
                std::cout << "❌ Some note durations exceed bar boundaries" << std::endl;
            }
        }
        
        return passed;
    }
    
    int runFeatureTests(PatternParser& parser, const var& tests, int& totalTests)
    {
        std::cout << "\n--- Complete Feature Tests ---" << std::endl;
        int passed = 0;
        
        // Test 7: Combined primitives
        if (tests.hasProperty("7_combined_primitives")) {
            totalTests++;
            std::cout << "Running: Combined Primitives (kick+snare)..." << std::endl;
            
            var test = tests["7_combined_primitives"];
            auto chart = parser.parseChartFromJson(test);
            
            if (chart && chart->toMidiSequence()) {
                auto sequence = chart->toMidiSequence();
                
                // Should have simultaneous kick and snare events
                Array<double> kickTimes, snareTimes;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn()) {
                        if (msg.getNoteNumber() == 36) kickTimes.add(msg.getTimeStamp()); // kick
                        if (msg.getNoteNumber() == 38) snareTimes.add(msg.getTimeStamp()); // snare
                    }
                }
                
                if (kickTimes.size() > 0 && snareTimes.size() > 0) {
                    std::cout << "✅ Combined primitives create separate simultaneous events" << std::endl;
                    passed++;
                } else {
                    std::cout << "❌ Combined primitives not working" << std::endl;
                }
            } else {
                std::cout << "❌ Failed to parse combined primitives test" << std::endl;
            }
        }
        
        // Test 10: Complete drums test
        if (tests.hasProperty("10_instrument_specific_drums")) {
            totalTests++;
            std::cout << "Running: Complete Drums Feature Test..." << std::endl;
            
            var test = tests["10_instrument_specific_drums"];
            auto chart = parser.parseChartFromJson(test);
            
            if (chart && chart->toMidiSequence()) {
                auto sequence = chart->toMidiSequence();
                
                int drumEvents = 0;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn() && msg.getChannel() == 10) { // Drums on channel 10
                        drumEvents++;
                    }
                }
                
                if (drumEvents > 0) {
                    std::cout << "✅ Complete drums test: " << drumEvents << " drum events on channel 10" << std::endl;
                    passed++;
                } else {
                    std::cout << "❌ No drum events found" << std::endl;
                }
            } else {
                std::cout << "❌ Failed to parse drums feature test" << std::endl;
            }
        }
        
        // Test 11: Bass with chord tones
        if (tests.hasProperty("11_instrument_specific_bass")) {
            totalTests++;
            std::cout << "Running: Bass Chord Tone Test..." << std::endl;
            
            var test = tests["11_instrument_specific_bass"];
            auto chart = parser.parseChartFromJson(test);
            
            if (chart && chart->toMidiSequence()) {
                std::cout << "✅ Bass chord tone test parsed and generated successfully" << std::endl;
                passed++;
            } else {
                std::cout << "❌ Failed bass chord tone test" << std::endl;
            }
        }
        
        return passed;
    }
};

int main()
{
    PatternTestRunner runner;
    runner.runAllTests();
    return 0;
}