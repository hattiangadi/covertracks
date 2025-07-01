#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Debug tests for the comprehensive Autumn Jazz pattern
 */
class AutumnJazzDebugTest : public juce::UnitTest
{
public:
    AutumnJazzDebugTest() : juce::UnitTest("Autumn Jazz Debug Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testSimpleAliasedBars();
        testBasicPatternReference();
        testLayeredSection();
        testFullAutumnJazzIntro();
    }
    
private:
    void testSimpleAliasedBars()
    {
        beginTest("Simple aliased bars");
        
        // Start with the simplest possible case
        juce::String simplePattern = R"({
            "version": "2.0",
            "aliases": {
                "bar": {"type": "sequential", "contains": "beats", "count": 4}
            },
            "sections": {
                "test": {
                    "bars": {
                        "1": {"events": [{"t": 1, "n": "C4", "dur": 1.0}]},
                        "2": {"events": [{"t": 1, "n": "D4", "dur": 1.0}]}
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("simple-test", "piano");
        auto result = musician->convertNotationToMidiEvents(simplePattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse simple pattern");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::String noteSequence;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        int note = obj->getProperty("note");
                        double time = obj->getProperty("timestamp");
                        
                        juce::String noteName = (note == 60) ? "C4" : 
                                               (note == 62) ? "D4" : 
                                               juce::String(note);
                        noteSequence += noteName + "@" + juce::String(time, 1) + " ";
                    }
                }
            }
            
            logMessage("Simple bars test: " + juce::String(noteCount) + " notes");
            logMessage("Sequence: " + noteSequence);
            
            if (noteCount >= 2) {
                logMessage("✅ Basic aliased bars working");
            } else {
                logMessage("❌ Expected 2 notes but got " + juce::String(noteCount));
            }
        }
    }
    
    void testBasicPatternReference()
    {
        beginTest("Basic pattern reference");
        
        juce::String patternTest = R"({
            "version": "2.0",
            "patterns": {
                "testRiff": {
                    "events": [
                        {"t": 1, "n": "C4", "dur": 0.5},
                        {"t": 1.5, "n": "E4", "dur": 0.5}
                    ]
                }
            },
            "sections": {
                "test": {
                    "bars": {
                        "1": {"pattern": "testRiff"}
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("pattern-test", "piano");
        auto result = musician->convertNotationToMidiEvents(patternTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse pattern reference");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                    }
                }
            }
            
            logMessage("Pattern reference test: " + juce::String(noteCount) + " notes");
            
            if (noteCount >= 2) {
                logMessage("✅ Pattern references working");
            } else {
                logMessage("❌ Expected 2 notes from pattern but got " + juce::String(noteCount));
            }
        }
    }
    
    void testLayeredSection()
    {
        beginTest("Layered section with multiple instruments");
        
        juce::String layeredTest = R"({
            "version": "2.0",
            "patterns": {
                "bassLine": {
                    "context": {"octave": 2},
                    "events": [{"t": 1, "n": "C2", "dur": 1.0}]
                },
                "melody": {
                    "context": {"octave": 4},
                    "events": [{"t": 1, "n": "E4", "dur": 1.0}]
                }
            },
            "sections": {
                "test": {
                    "layers": {
                        "bass": {
                            "bars": {
                                "1": {"pattern": "bassLine"}
                            }
                        },
                        "lead": {
                            "bars": {
                                "1": {"pattern": "melody"}
                            }
                        }
                    }
                }
            },
            "arrangement": ["test"]
        })";
        
        auto musician = std::make_unique<Musician>("layer-test", "piano");
        auto result = musician->convertNotationToMidiEvents(layeredTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse layered section");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::String noteDetails;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        int note = obj->getProperty("note");
                        double time = obj->getProperty("timestamp");
                        noteDetails += "MIDI" + juce::String(note) + "@" + juce::String(time, 1) + " ";
                    }
                }
            }
            
            logMessage("Layered section test: " + juce::String(noteCount) + " notes");
            logMessage("Details: " + noteDetails);
            
            if (noteCount >= 2) {
                logMessage("✅ Layered sections working");
            } else {
                logMessage("❌ Expected 2 notes from layers but got " + juce::String(noteCount));
            }
        }
    }
    
    void testFullAutumnJazzIntro()
    {
        beginTest("Full Autumn Jazz intro section");
        
        // Test just the intro section from the full pattern
        juce::String introTest = R"({
            "version": "2.0",
            "aliases": {
                "bar": {"type": "sequential", "contains": "beats", "count": 4}
            },
            "patterns": {
                "classicalArpeggio": {
                    "context": {"octave": 4, "velocity": 0.7, "articulation": "legato"},
                    "events": [
                        {"t": 1.0, "d": 1, "dur": 0.25},
                        {"t": 1.25, "d": 3, "dur": 0.25},
                        {"t": 1.5, "d": 5, "dur": 0.25},
                        {"t": 1.75, "d": 8, "dur": 0.25}
                    ]
                },
                "swingBass": {
                    "context": {"octave": 2, "velocity": 0.8},
                    "events": [
                        {"t": 1.0, "d": 1, "dur": 0.75},
                        {"t": 2.5, "d": 5, "dur": 0.5}
                    ]
                }
            },
            "sections": {
                "intro": {
                    "context": {"chord": "BbMaj7"},
                    "layers": {
                        "piano": {
                            "bars": {
                                "1": {"pattern": "classicalArpeggio"},
                                "2": {"pattern": "classicalArpeggio"},
                                "3": {"pattern": "classicalArpeggio"},
                                "4": {"pattern": "classicalArpeggio"}
                            }
                        },
                        "bass": {
                            "bars": {
                                "3": {"pattern": "swingBass"},
                                "4": {"pattern": "swingBass"}
                            }
                        }
                    }
                }
            },
            "arrangement": ["intro"]
        })";
        
        auto musician = std::make_unique<Musician>("intro-test", "piano");
        auto result = musician->convertNotationToMidiEvents(introTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse intro section");
        
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
            
            logMessage("Intro section test: " + juce::String(noteCount) + " notes");
            
            // Sort timestamps for analysis
            timestamps.sort();
            juce::String timeStr;
            for (int i = 0; i < std::min(10, timestamps.size()); i++) {
                timeStr += juce::String(timestamps[i], 1) + " ";
            }
            logMessage("First 10 timestamps: " + timeStr);
            
            // Expected: 4 bars of piano arpeggio (4 notes each) + 2 bars of bass (2 notes each)
            // = 16 piano notes + 4 bass notes = 20 total
            if (noteCount >= 20) {
                logMessage("✅ Full intro working with " + juce::String(noteCount) + " notes");
            } else if (noteCount >= 16) {
                logMessage("⚠️ Piano working (" + juce::String(noteCount) + " notes) but bass might be missing");
            } else if (noteCount >= 4) {
                logMessage("⚠️ Only one bar working (" + juce::String(noteCount) + " notes)");
            } else {
                logMessage("❌ Intro failing - only " + juce::String(noteCount) + " notes generated");
            }
        }
    }
};

static AutumnJazzDebugTest autumnJazzDebugTest;

} // namespace covertracks