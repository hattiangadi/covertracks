#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Debug range notation parsing like "1-4"
 */
class RangeNotationDebugTest : public juce::UnitTest
{
public:
    RangeNotationDebugTest() : juce::UnitTest("Range Notation Debug Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testIndividualBars();
        testRangeNotation();
        testLayeredRangeNotation();
    }
    
private:
    void testIndividualBars()
    {
        beginTest("Individual numbered bars: 1, 2, 3, 4");
        
        juce::String individualTest = R"({
            "version": "2.0",
            "patterns": {
                "riff": {
                    "events": [{"t": 1.5, "n": "E3", "dur": 0.5}]
                }
            },
            "sections": {
                "test": {
                    "bars": {
                        "1": {"pattern": "riff"},
                        "2": {"pattern": "riff"},
                        "3": {"pattern": "riff"},
                        "4": {"pattern": "riff"}
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("individual-test", "piano");
        auto result = musician->convertNotationToMidiEvents(individualTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse individual bars");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::Array<double> timestamps;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        double time = obj->getProperty("timestamp");
                        timestamps.add(time);
                    }
                }
            }
            
            timestamps.sort();
            
            logMessage("Individual bars test:");
            logMessage("  Total notes: " + juce::String(noteCount) + " (expected 4)");
            
            juce::String timeStr = "  Times: ";
            for (double t : timestamps) {
                timeStr += juce::String(t, 1) + " ";
            }
            logMessage(timeStr);
            logMessage("  Expected: 0.5, 4.5, 8.5, 12.5 (bars 1-4 at t=1.5 each)");
            
            if (noteCount == 4) {
                logMessage("✅ Individual bars working correctly");
            } else {
                logMessage("❌ Individual bars failed - got " + juce::String(noteCount) + " notes");
            }
        }
    }
    
    void testRangeNotation()
    {
        beginTest("Range notation: 1-4 should cover 4 bars");
        
        juce::String rangeTest = R"({
            "version": "2.0",
            "patterns": {
                "riff": {
                    "events": [{"t": 1.5, "n": "E3", "dur": 0.5}]
                }
            },
            "sections": {
                "test": {
                    "bars": {
                        "1-4": {"pattern": "riff"}
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("range-test", "piano");
        auto result = musician->convertNotationToMidiEvents(rangeTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse range notation");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::Array<double> timestamps;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        double time = obj->getProperty("timestamp");
                        timestamps.add(time);
                    }
                }
            }
            
            timestamps.sort();
            
            logMessage("Range notation test:");
            logMessage("  Total notes: " + juce::String(noteCount) + " (expected 4)");
            
            juce::String timeStr = "  Times: ";
            for (double t : timestamps) {
                timeStr += juce::String(t, 1) + " ";
            }
            logMessage(timeStr);
            
            if (noteCount == 4) {
                logMessage("✅ Range notation working - generates 4 notes");
            } else if (noteCount == 1) {
                logMessage("❌ Range notation broken - treating '1-4' as single bar");
                logMessage("  This means ranges aren't being expanded into individual bars");
            } else {
                logMessage("❌ Range notation unexpected result - got " + juce::String(noteCount) + " notes");
            }
        }
    }
    
    void testLayeredRangeNotation()
    {
        beginTest("Layered range notation (like Autumn Jazz head)");
        
        // Simplified version of the problematic Autumn Jazz head section
        juce::String layeredRangeTest = R"({
            "version": "2.0",
            "patterns": {
                "jazzComping": {
                    "events": [
                        {"t": 1.5, "n": "E3", "dur": 0.5},
                        {"t": 2.5, "n": "G3", "dur": 0.5}
                    ]
                }
            },
            "sections": {
                "head": {
                    "layers": {
                        "melody": {
                            "bars": {
                                "1": {"events": [{"t": 1.0, "n": "G4", "dur": 1.0}]},
                                "2": {"events": [{"t": 1.0, "n": "A4", "dur": 1.0}]},
                                "3": {"events": [{"t": 1.0, "n": "B4", "dur": 1.0}]},
                                "4": {"events": [{"t": 1.0, "n": "C5", "dur": 1.0}]}
                            }
                        },
                        "comping": {
                            "bars": {
                                "1-4": {"pattern": "jazzComping", "context": {"chord": "BbMaj7"}}
                            }
                        }
                    }
                }
            },
            "arrangement": ["head"]
        })";
        
        auto musician = std::make_unique<Musician>("layered-range-test", "piano");
        auto result = musician->convertNotationToMidiEvents(layeredRangeTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse layered range notation");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::Array<double> timestamps;
            juce::Array<int> noteNumbers;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        double time = obj->getProperty("timestamp");
                        int note = obj->getProperty("note");
                        timestamps.add(time);
                        noteNumbers.add(note);
                    }
                }
            }
            
            // Sort both arrays by timestamp
            for (int i = 0; i < timestamps.size() - 1; i++) {
                for (int j = i + 1; j < timestamps.size(); j++) {
                    if (timestamps[i] > timestamps[j]) {
                        timestamps.swap(i, j);
                        noteNumbers.swap(i, j);
                    }
                }
            }
            
            logMessage("Layered range notation test:");
            logMessage("  Total notes: " + juce::String(noteCount) + " (expected 12)");
            logMessage("  Expected: 4 melody notes + 8 comping notes = 12 total");
            
            juce::String noteStr = "  Notes: ";
            for (int i = 0; i < std::min(12, noteCount); i++) {
                noteStr += "MIDI" + juce::String(noteNumbers[i]) + "@" + juce::String(timestamps[i], 1) + " ";
            }
            logMessage(noteStr);
            
            // Analysis
            int notesAtBar1 = 0, notesAtBar2 = 0, notesAtBar3 = 0, notesAtBar4 = 0;
            for (double t : timestamps) {
                if (t >= 0.0 && t < 4.0) notesAtBar1++;
                else if (t >= 4.0 && t < 8.0) notesAtBar2++;
                else if (t >= 8.0 && t < 12.0) notesAtBar3++;
                else if (t >= 12.0 && t < 16.0) notesAtBar4++;
            }
            
            logMessage("  Distribution:");
            logMessage("    Bar 1 (0-4): " + juce::String(notesAtBar1) + " notes (expected 3: 1 melody + 2 comping)");
            logMessage("    Bar 2 (4-8): " + juce::String(notesAtBar2) + " notes (expected 3)");
            logMessage("    Bar 3 (8-12): " + juce::String(notesAtBar3) + " notes (expected 3)");
            logMessage("    Bar 4 (12-16): " + juce::String(notesAtBar4) + " notes (expected 3)");
            
            if (noteCount == 12 && notesAtBar1 == 3 && notesAtBar2 == 3 && notesAtBar3 == 3 && notesAtBar4 == 3) {
                logMessage("✅ Layered range notation working perfectly");
            } else if (noteCount == 6 && notesAtBar1 == 3) {
                logMessage("❌ Range notation broken - only processing bar 1 of '1-4' range");
            } else if (noteCount == 4) {
                logMessage("❌ Range notation broken - only melody layer working");
            } else {
                logMessage("❌ Unexpected result - analyzing...");
                
                if (notesAtBar1 > 0 && notesAtBar2 == 0 && notesAtBar3 == 0 && notesAtBar4 == 0) {
                    logMessage("    Issue: All notes at bar 1 - range expansion not working");
                } else if (notesAtBar1 == 1 && notesAtBar2 == 1 && notesAtBar3 == 1 && notesAtBar4 == 1) {
                    logMessage("    Issue: Only melody working - comping layer with range not working");
                }
            }
        }
    }
};

static RangeNotationDebugTest rangeNotationDebugTest;

} // namespace covertracks