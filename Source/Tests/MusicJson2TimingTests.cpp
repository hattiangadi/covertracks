// Source/Tests/MusicJson2TimingTests.cpp
#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * TDD Test: JSON 2.0 Timing Offset Issue
 * 
 * EXPECTED TO FAIL: This test verifies that t:1.0 events should start at beat 1, not beat 2.
 * Currently fails because the timing system has an offset bug.
 */
class MusicJson2TimingTests : public juce::UnitTest
{
public:
    MusicJson2TimingTests() : juce::UnitTest("JSON 2.0 Timing", "Music Pattern System") {}
    
    void runTest() override
    {
        testBasicTimingOffset();
        testSequentialTiming();
        testFirstBeatMapping();
    }
    
private:
    void testBasicTimingOffset()
    {
        beginTest("t:1.0 should map to beat 1 (EXPECTED TO FAIL)");
        
        // Simple JSON 2.0 pattern with events at t:1.0
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {
                "velocity": 0.8,
                "octave": 4
            },
            "sections": {
                "test": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 0.5}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("timing-test", "piano");
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.isObject(), "Should return object for JSON 2.0");
        expect(result.getProperty("success", false), "Should successfully parse JSON 2.0");
        expect(result.hasProperty("events"), "Should have MIDI events");
        
        auto events = result["events"];
        if (events.isArray() && events.getArray()->size() > 0) {
            // Find the first note-on event
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        double timestamp = eventObj->getProperty("timestamp");
                        
                        // Log the actual timestamp for debugging
                        logMessage("First note-on timestamp: " + juce::String(timestamp));
                        
                        // CRITICAL TEST: t:1.0 should map to timestamp 0.0 (beat 1)
                        // This will FAIL if there's a timing offset bug
                        expect(timestamp == 0.0, "t:1.0 should map to timestamp 0.0 (beat 1), got " + juce::String(timestamp));
                        break;
                    }
                }
            }
        } else {
            expect(false, "Should generate at least one MIDI event");
        }
    }
    
    void testSequentialTiming()
    {
        beginTest("Sequential t values should map correctly (EXPECTED TO FAIL)");
        
        juce::String json2Pattern = R"({
            "version": "2.0",
            "sections": {
                "test": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 0.5},
                                {"t": 2.0, "n": "D4", "dur": 0.5},
                                {"t": 3.0, "n": "E4", "dur": 0.5},
                                {"t": 4.0, "n": "F4", "dur": 0.5}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("sequential-test", "piano");
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse sequential timing");
        
        auto events = result["events"];
        if (events.isArray()) {
            juce::Array<double> noteOnTimestamps;
            
            // Collect all note-on timestamps
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        double timestamp = eventObj->getProperty("timestamp");
                        noteOnTimestamps.add(timestamp);
                    }
                }
            }
            
            // Log all timestamps for debugging
            for (int i = 0; i < noteOnTimestamps.size(); ++i) {
                logMessage("Note " + juce::String(i+1) + " timestamp: " + juce::String(noteOnTimestamps[i]));
            }
            
            if (noteOnTimestamps.size() >= 4) {
                // CRITICAL TESTS: Sequential timing should be correct
                // These will FAIL if there's a consistent offset
                expect(noteOnTimestamps[0] == 0.0, "First note (t:1.0) should be at 0.0, got " + juce::String(noteOnTimestamps[0]));
                expect(noteOnTimestamps[1] == 1.0, "Second note (t:2.0) should be at 1.0, got " + juce::String(noteOnTimestamps[1]));
                expect(noteOnTimestamps[2] == 2.0, "Third note (t:3.0) should be at 2.0, got " + juce::String(noteOnTimestamps[2]));
                expect(noteOnTimestamps[3] == 3.0, "Fourth note (t:4.0) should be at 3.0, got " + juce::String(noteOnTimestamps[3]));
            } else {
                expect(false, "Should have 4 note-on events, got " + juce::String(noteOnTimestamps.size()));
            }
        }
    }
    
    void testFirstBeatMapping()
    {
        beginTest("First beat should always be at timestamp 0.0 (EXPECTED TO FAIL)");
        
        // Test pattern that explicitly starts at the beginning of a bar
        juce::String json2Pattern = R"({
            "version": "2.0",
            "meta": {
                "tempo": 120,
                "timeSignature": "4/4"
            },
            "sections": {
                "firstBeat": {
                    "layers": {
                        "drums": {
                            "events": [
                                {"t": 1.0, "n": "kick", "dur": 0.25, "v": 0.9}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["firstBeat"]
        })";
        
        auto musician = std::make_unique<Musician>("first-beat-test", "drums");
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse first beat pattern");
        
        auto events = result["events"];
        if (events.isArray() && events.getArray()->size() > 0) {
            auto firstEventVar = (*events.getArray())[0];
            if (firstEventVar.isObject()) {
                auto firstEvent = firstEventVar.getDynamicObject();
                if (firstEvent && firstEvent->getProperty("type").toString() == "note_on") {
                    double firstTimestamp = firstEvent->getProperty("timestamp");
                    
                    logMessage("First beat timestamp: " + juce::String(firstTimestamp));
                    
                    // CRITICAL TEST: The very first event should always be at timestamp 0.0
                    // This represents beat 1 of the pattern
                    expect(firstTimestamp == 0.0, "First beat should be at timestamp 0.0, got " + juce::String(firstTimestamp));
                } else {
                    expect(false, "First event should be note_on");
                }
            }
        } else {
            expect(false, "Should have at least one event");
        }
    }
};

// Register the test
static MusicJson2TimingTests musicJson2TimingTests;

} // namespace covertracks