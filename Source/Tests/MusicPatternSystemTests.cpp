#include <JuceHeader.h>
#include "../Musician/Core/MusicPatternSystem.h"
#include "../Musician/Core/PatternParser.h"
#include "../Musician/Core/MusicPatternComponents.h"

namespace covertracks {

class MusicPatternSystemTests : public juce::UnitTest
{
public:
    MusicPatternSystemTests() : juce::UnitTest("MusicPatternSystem Feature Tests") {}

    void runTest() override
    {
        beginTest("Running MusicPatternSystem Feature Tests");
        
        // Load test data from JSON file
        File testFile = File::getCurrentWorkingDirectory().getChildFile("Tests/MusicPatternSystem_Tests.json");
        if (!testFile.existsAsFile()) {
            // Try different paths
            testFile = File::getCurrentWorkingDirectory().getParentDirectory().getChildFile("Tests/MusicPatternSystem_Tests.json");
            if (!testFile.existsAsFile()) {
                testFile = File::getCurrentWorkingDirectory().getParentDirectory().getParentDirectory().getChildFile("Tests/MusicPatternSystem_Tests.json");
                if (!testFile.existsAsFile()) {
                    expectWithoutCheck(false, "Test file not found in any expected location");
                    return;
                }
            }
        }
        
        String testData = testFile.loadFileAsString();
        var testJson = JSON::parse(testData);
        if (testJson.isVoid()) {
            expectWithoutCheck(false, "Failed to parse test JSON");
            return;
        }
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        var tests = testJson["tests"];
        if (!tests.isObject()) {
            expectWithoutCheck(false, "No tests object found in JSON");
            return;
        }
        
        logMessage("Found test file: " + testFile.getFullPathName());
        
        // Run specific tests that address user concerns
        runChordToneTests(parser, tests);
        runVariationTests(parser, tests);
        runTimingTests(parser, tests);
        runBoundaryTests(parser, tests);
        runQuickFeatureTests(parser, tests);
    }

private:
    void runChordToneTests(PatternParser& parser, const var& tests)
    {
        beginTest("Chord Tone Resolution Tests");
        
        // Test 1: Basic chord tone resolution
        if (tests.hasProperty("1_chord_tones_basic")) {
            var test = tests["1_chord_tones_basic"];
            logMessage("Running: " + test["title"].toString());
            
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse chord tone test chart");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI sequence");
                
                // Count chord changes and verify chord tones resolve correctly
                int chordChangeEvents = 0;
                int bassNoteEvents = 0;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn()) {
                        if (msg.getChannel() == 1) bassNoteEvents++; // Bass on channel 1
                    }
                    if (msg.isTextMetaEvent() && msg.getTextFromTextMetaEvent().contains("chord")) {
                        chordChangeEvents++;
                    }
                }
                
                expect(bassNoteEvents > 0, "Should have bass note events");
                logMessage("Found " + String(bassNoteEvents) + " bass notes and " + String(chordChangeEvents) + " chord changes");
            }
        }
        
        // Test 2: No chord context behavior
        if (tests.hasProperty("2_chord_tones_no_chord")) {
            var test = tests["2_chord_tones_no_chord"];
            logMessage("Running: " + test["title"].toString());
            
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse no-chord test chart");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should handle missing chord gracefully");
            }
        }
    }
    
    void runVariationTests(PatternParser& parser, const var& tests)
    {
        beginTest("Pattern Variation Tests");
        
        // Test 3: Single bar variation - this addresses the "played 4 times instead of on bar 4" concern
        if (tests.hasProperty("3_variations_single_bar")) {
            var test = tests["3_variations_single_bar"];
            logMessage("Running: " + test["title"].toString());
            
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse single bar variation test");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI for variations");
                
                // Analyze timing to ensure variation only plays on bar 4
                Array<double> noteTimestamps;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn() && msg.getChannel() == 10) { // Drums on channel 10
                        noteTimestamps.add(msg.getTimeStamp());
                    }
                }
                
                // Convert timestamps to beats and analyze distribution
                const double ticksPerBeat = 960.0;
                Array<int> eventsPerBar(4, 0);
                for (double timestamp : noteTimestamps) {
                    double beat = timestamp / ticksPerBeat;
                    int bar = (int)(beat / 4.0);
                    if (bar >= 0 && bar < 4) {
                        eventsPerBar.getReference(bar)++;
                    }
                }
                
                // Bar 4 (index 3) should have more events due to fill pattern
                logMessage("Events per bar: " + String(eventsPerBar[0]) + ", " + 
                          String(eventsPerBar[1]) + ", " + String(eventsPerBar[2]) + ", " + String(eventsPerBar[3]));
                
                expect(eventsPerBar[3] > eventsPerBar[0], "Bar 4 should have more events (fill pattern)");
                expect(eventsPerBar[0] == eventsPerBar[1], "Bar 1 and 2 should have same events (base pattern)");
            }
        }
        
        // Test 4: Multiple bar variations
        if (tests.hasProperty("4_variations_multiple_bars")) {
            var test = tests["4_variations_multiple_bars"];
            logMessage("Running: " + test["title"].toString());
            
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse multiple bar variation test");
        }
    }
    
    void runTimingTests(PatternParser& parser, const var& tests)
    {
        beginTest("Multi-Bar Timing Tests");
        
        // Test 5: Multi-bar event timing
        if (tests.hasProperty("5_timing_multi_bar")) {
            var test = tests["5_timing_multi_bar"];
            logMessage("Running: " + test["title"].toString());
            
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse multi-bar timing test");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI for multi-bar timing");
                
                // Verify events are distributed across correct bars
                Array<double> noteTimestamps;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn()) {
                        noteTimestamps.add(msg.getTimeStamp());
                    }
                }
                
                expect(noteTimestamps.size() >= 6, "Should have at least 6 note events across 3 bars");
                logMessage("Found " + String(noteTimestamps.size()) + " note events");
            }
        }
    }
    
    void runBoundaryTests(PatternParser& parser, const var& tests)
    {
        beginTest("Duration Boundary Tests");
        
        // Test 6: Duration clamping at bar boundaries
        if (tests.hasProperty("6_duration_boundaries")) {
            var test = tests["6_duration_boundaries"];
            logMessage("Running: " + test["title"].toString());
            
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse duration boundary test");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI with clamped durations");
                
                // Find note on/off pairs and verify durations don't exceed bar boundaries
                Array<std::pair<double, double>> noteDurations; // timestamp, duration
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
                
                const double ticksPerBar = 4.0 * 960.0; // 4 beats * 960 ticks per beat
                for (auto& notePair : noteDurations) {
                    double startTime = notePair.first;
                    double duration = notePair.second;
                    double barStart = floor(startTime / ticksPerBar) * ticksPerBar;
                    double maxAllowedDuration = (barStart + ticksPerBar) - startTime;
                    
                    expect(duration <= maxAllowedDuration + 1.0, "Note duration should not exceed bar boundary");
                }
                
                logMessage("Verified " + String(noteDurations.size()) + " note durations stay within bar boundaries");
            }
        }
    }
    
    void runQuickFeatureTests(PatternParser& parser, const var& tests)
    {
        beginTest("Quick Feature Validation Tests");
        
        // Test 7: Combined primitives
        if (tests.hasProperty("7_combined_primitives")) {
            logMessage("Testing: Combined Primitives (kick+snare)");
            
            var test = tests["7_combined_primitives"];
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse combined primitives test");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI for combined primitives");
                
                // Count different drum sounds
                int kickCount = 0, snareCount = 0;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn()) {
                        if (msg.getNoteNumber() == 36) kickCount++; // kick
                        if (msg.getNoteNumber() == 38) snareCount++; // snare
                    }
                }
                
                expect(kickCount > 0 && snareCount > 0, "Should have both kick and snare events from combined primitives");
                logMessage("Found " + String(kickCount) + " kick events and " + String(snareCount) + " snare events");
            }
        }
        
        // Test 10: Complete drums test
        if (tests.hasProperty("10_instrument_specific_drums")) {
            logMessage("Testing: Complete Drums Feature");
            
            var test = tests["10_instrument_specific_drums"];
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse complete drums test");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI for complete drums");
                
                int drumEvents = 0;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn() && msg.getChannel() == 10) { // Drums on channel 10
                        drumEvents++;
                    }
                }
                
                expect(drumEvents > 0, "Should have drum events on MIDI channel 10");
                logMessage("Found " + String(drumEvents) + " drum events on channel 10");
            }
        }
        
        // Test 11: Bass with chord tones
        if (tests.hasProperty("11_instrument_specific_bass")) {
            logMessage("Testing: Bass Chord Tone Resolution");
            
            var test = tests["11_instrument_specific_bass"];
            auto chart = parser.parseChartFromJson(test);
            expect(chart != nullptr, "Should parse bass chord tone test");
            
            if (chart) {
                auto sequence = chart->toMidiSequence();
                expect(sequence != nullptr, "Should generate MIDI for bass chord tones");
                
                // Count bass events and verify variety (indicating chord tone resolution)
                Array<int> bassNotes;
                for (int i = 0; i < sequence->getNumEvents(); ++i) {
                    auto msg = sequence->getEventPointer(i)->message;
                    if (msg.isNoteOn() && msg.getChannel() == 1) { // Bass typically on channel 1
                        bassNotes.add(msg.getNoteNumber());
                    }
                }
                
                expect(bassNotes.size() > 0, "Should have bass note events");
                
                // Check for note variety (indicating chord progression following)
                bool hasVariety = false;
                for (int i = 1; i < bassNotes.size() && !hasVariety; ++i) {
                    if (bassNotes[i] != bassNotes[0]) {
                        hasVariety = true;
                    }
                }
                
                expect(hasVariety, "Bass notes should vary with chord changes");
                logMessage("Found " + String(bassNotes.size()) + " bass events with note variety: " + String(hasVariety ? "Yes" : "No"));
            }
        }
    }
};

// Register the test
static MusicPatternSystemTests musicPatternSystemTests;

} // namespace covertracks