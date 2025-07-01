#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Debug layer rendering and timing offset issues
 */
class LayerTimingDebugTest : public juce::UnitTest
{
public:
    LayerTimingDebugTest() : juce::UnitTest("Layer Timing Debug Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testSimultaneousLayers();
        testBarTimingOffset();
        testMultiBarLayers();
    }
    
private:
    void testSimultaneousLayers()
    {
        beginTest("Simultaneous layers should play at same time");
        
        juce::String layerTest = R"({
            "version": "2.0",
            "sections": {
                "test": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1, "n": "C4", "dur": 1.0}]
                        },
                        "bass": {
                            "events": [{"t": 1, "n": "C2", "dur": 1.0}]
                        },
                        "chord": {
                            "events": [{"t": 1, "n": "E4", "dur": 1.0}]
                        }
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("layer-test", "piano");
        auto result = musician->convertNotationToMidiEvents(layerTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse layers");
        
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
            
            logMessage("Simultaneous layers test:");
            logMessage("  Notes: " + juce::String(noteCount));
            
            for (int i = 0; i < timestamps.size(); i++) {
                logMessage("  Note " + juce::String(noteNumbers[i]) + " at time " + juce::String(timestamps[i], 2));
            }
            
            if (noteCount == 3) {
                // Check if all notes start at the same time (should all be at 0.0)
                bool allAtSameTime = true;
                double firstTime = timestamps[0];
                for (double t : timestamps) {
                    if (std::abs(t - firstTime) > 0.001) {
                        allAtSameTime = false;
                        break;
                    }
                }
                
                if (allAtSameTime) {
                    logMessage("✅ All 3 layers play simultaneously at time " + juce::String(firstTime, 2));
                } else {
                    logMessage("❌ Layers NOT simultaneous - times spread out");
                }
                
                // Check timing offset
                if (std::abs(firstTime - 0.0) < 0.001) {
                    logMessage("✅ Timing correct - starts at 0.0");
                } else {
                    logMessage("❌ Timing offset - starts at " + juce::String(firstTime, 2) + " instead of 0.0");
                }
            } else {
                logMessage("❌ Expected 3 notes but got " + juce::String(noteCount));
            }
        }
    }
    
    void testBarTimingOffset()
    {
        beginTest("Bar timing should start at 0.0, 4.0, 8.0, etc.");
        
        juce::String barTest = R"({
            "version": "2.0",
            "sections": {
                "test": {
                    "bars": {
                        "1": {"events": [{"t": 1, "n": "C4"}]},
                        "2": {"events": [{"t": 1, "n": "D4"}]},
                        "3": {"events": [{"t": 1, "n": "E4"}]}
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("bar-timing-test", "piano");
        auto result = musician->convertNotationToMidiEvents(barTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse bars");
        
        auto events = result["events"];
        if (events.isArray()) {
            juce::Array<double> timestamps;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        double time = obj->getProperty("timestamp");
                        timestamps.add(time);
                    }
                }
            }
            
            timestamps.sort();
            
            logMessage("Bar timing test:");
            logMessage("  Expected: 0.0, 4.0, 8.0 (bars 1, 2, 3)");
            
            juce::String actualStr = "  Actual: ";
            for (int i = 0; i < timestamps.size(); i++) {
                actualStr += juce::String(timestamps[i], 1);
                if (i < timestamps.size() - 1) actualStr += ", ";
            }
            logMessage(actualStr);
            
            if (timestamps.size() == 3) {
                bool timingCorrect = (std::abs(timestamps[0] - 0.0) < 0.1) &&
                                   (std::abs(timestamps[1] - 4.0) < 0.1) &&
                                   (std::abs(timestamps[2] - 8.0) < 0.1);
                
                if (timingCorrect) {
                    logMessage("✅ Bar timing is correct");
                } else {
                    logMessage("❌ Bar timing is wrong");
                    
                    // Check for +4 offset (if it's 4.0, 8.0, 12.0 instead)
                    bool offsetBy4 = (std::abs(timestamps[0] - 4.0) < 0.1) &&
                                    (std::abs(timestamps[1] - 8.0) < 0.1) &&
                                    (std::abs(timestamps[2] - 12.0) < 0.1);
                    
                    if (offsetBy4) {
                        logMessage("  🔍 Detected +4 offset (starting at bar 2 instead of bar 1)");
                    }
                }
            } else {
                logMessage("❌ Expected 3 bars but got " + juce::String(timestamps.size()));
            }
        }
    }
    
    void testMultiBarLayers()
    {
        beginTest("Multi-bar layers should work correctly");
        
        // Simplified version of the Autumn Jazz head section
        juce::String headTest = R"({
            "version": "2.0",
            "patterns": {
                "comping": {
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
                                "1": {"events": [{"t": 1, "n": "C4"}]},
                                "2": {"events": [{"t": 1, "n": "D4"}]}
                            }
                        },
                        "comping": {
                            "bars": {
                                "1": {"pattern": "comping"},
                                "2": {"pattern": "comping"}
                            }
                        }
                    }
                }
            },
            "arrangement": ["head"]
        })";
        
        auto musician = std::make_unique<Musician>("head-test", "piano");
        auto result = musician->convertNotationToMidiEvents(headTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse multi-bar layers");
        
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
            
            logMessage("Multi-bar layers test:");
            logMessage("  Total notes: " + juce::String(noteCount));
            logMessage("  Expected: 6 notes (2 melody + 4 comping)");
            
            juce::String timeStr = "  Times: ";
            for (int i = 0; i < std::min(8, timestamps.size()); i++) {
                timeStr += juce::String(timestamps[i], 1) + " ";
            }
            logMessage(timeStr);
            
            // Expected pattern:
            // Bar 1: melody at 0.0, comping at 0.5 and 1.5
            // Bar 2: melody at 4.0, comping at 4.5 and 5.5
            
            if (noteCount >= 6) {
                logMessage("✅ Got expected number of notes");
                
                // Check if we have simultaneous notes (melody + comping in each bar)
                int notesAtBar1 = 0, notesAtBar2 = 0;
                for (double t : timestamps) {
                    if (t >= 0.0 && t < 4.0) notesAtBar1++;
                    else if (t >= 4.0 && t < 8.0) notesAtBar2++;
                }
                
                logMessage("  Bar 1 notes: " + juce::String(notesAtBar1) + " (expected 3)");
                logMessage("  Bar 2 notes: " + juce::String(notesAtBar2) + " (expected 3)");
                
                if (notesAtBar1 >= 3 && notesAtBar2 >= 3) {
                    logMessage("✅ Multi-bar layers working correctly");
                } else {
                    logMessage("❌ Multi-bar layers not distributing correctly");
                }
            } else {
                logMessage("❌ Expected 6+ notes but got " + juce::String(noteCount));
            }
        }
    }
};

static LayerTimingDebugTest layerTimingDebugTest;

} // namespace covertracks