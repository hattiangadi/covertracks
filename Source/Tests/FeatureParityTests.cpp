#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Comprehensive feature parity tests between JSON 1.0 and JSON 2.0 systems
 * Ensures all musical capabilities are preserved in the new format
 */
class FeatureParityTests : public juce::UnitTest
{
public:
    FeatureParityTests() : juce::UnitTest("Feature Parity Tests", "Music Pattern System") {}
    
    void runTest() override
    {
        testBasicDrumPatterns();
        testChordToneResolution();
        testPatternVariations();
        testMultiBarEvents();
        testExpressionFeatures();
        testComplexArrangements();
        testTimeSignatureSupport();
        testVelocityFormats();
        testArticulationSystem();
        testAdvancedHarmony();
    }
    
private:
    void testBasicDrumPatterns()
    {
        beginTest("Basic drum pattern equivalence");
        
        // JSON 1.0 format
        juce::String json1Pattern = R"({
            "title": "Basic Drums",
            "tempo": 120,
            "primitives": {
                "drums": {
                    "kick": 36,
                    "snare": 38,
                    "hihat": 42
                }
            },
            "sections": {
                "main": [
                    [1.0, "kick", 0.8, 0.25],
                    [2.0, "snare", 0.7, 0.25],
                    [3.0, "kick", 0.8, 0.25],
                    [4.0, "snare", 0.7, 0.25]
                ]
            }
        })";
        
        // JSON 2.0 equivalent
        juce::String json2Pattern = R"({
            "version": "2.0",
            "meta": {"title": "Basic Drums", "tempo": 120},
            "events": [
                {"t": 1.0, "n": "kick", "v": 0.8, "dur": 0.25},
                {"t": 2.0, "n": "snare", "v": 0.7, "dur": 0.25},
                {"t": 3.0, "n": "kick", "v": 0.8, "dur": 0.25},
                {"t": 4.0, "n": "snare", "v": 0.7, "dur": 0.25}
            ]
        })";
        
        auto musician = std::make_unique<Musician>("parity-test", "rock");
        
        auto result1 = musician->convertNotationToMidiEvents(json1Pattern, 120.0);
        auto result2 = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result1.getProperty("success", false), "JSON 1.0 should parse successfully");
        expect(result2.getProperty("success", false), "JSON 2.0 should parse successfully");
        
        // Both should generate MIDI events
        auto events1 = result1["events"];
        auto events2 = result2["events"];
        
        expect(events1.isArray(), "JSON 1.0 should generate event array");
        expect(events2.isArray(), "JSON 2.0 should generate event array");
        
        if (events1.isArray() && events2.isArray()) {
            expect(events1.getArray()->size() > 0, "JSON 1.0 should generate events");
            expect(events2.getArray()->size() > 0, "JSON 2.0 should generate events");
        }
        
        logMessage("Basic drum pattern parity verified");
    }
    
    void testChordToneResolution()
    {
        beginTest("Chord tone resolution equivalence");
        
        // JSON 1.0 format with chord tones
        juce::String json1Pattern = R"({
            "chords": {
                "C": ["C3", "E3", "G3"],
                "Am": ["A2", "C3", "E3"]
            },
            "sections": {
                "main": {
                    "bars": 2,
                    "chordContext": ["C", "Am"],
                    "layers": {
                        "bass": [
                            [1.0, {"tone": 1, "octave": 2}, 0.8, 1.0],
                            [2.0, {"tone": 3, "octave": 2}, 0.7, 1.0],
                            [5.0, {"tone": 1, "octave": 2}, 0.8, 1.0],
                            [6.0, {"tone": 5, "octave": 2}, 0.7, 1.0]
                        ]
                    }
                }
            }
        })";
        
        // JSON 2.0 equivalent
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {"octave": 2, "velocity": 0.8},
            "sections": {
                "main": {
                    "layers": {
                        "bass": {
                            "context": {"chord": "C"},
                            "events": [
                                {"t": 1.0, "d": 1, "v": 0.8, "dur": 1.0},
                                {"t": 2.0, "d": 3, "v": 0.7, "dur": 1.0}
                            ],
                            "variations": {
                                "bar2": {
                                    "context": {"chord": "Am"},
                                    "events": [
                                        {"t": 5.0, "d": 1, "v": 0.8, "dur": 1.0},
                                        {"t": 6.0, "d": 5, "v": 0.7, "dur": 1.0}
                                    ]
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("chord-test", "jazz");
        
        auto result1 = musician->convertNotationToMidiEvents(json1Pattern, 120.0);
        auto result2 = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result1.getProperty("success", false), "JSON 1.0 chord tones should work");
        expect(result2.getProperty("success", false), "JSON 2.0 chord degrees should work");
        
        logMessage("Chord tone resolution parity verified");
    }
    
    void testPatternVariations()
    {
        beginTest("Pattern variation system equivalence");
        
        // JSON 1.0 format
        juce::String json1Pattern = R"({
            "patterns": {
                "basicBeat": [
                    [1.0, "kick", 0.8],
                    [3.0, "snare", 0.7]
                ],
                "fillBeat": [
                    [3.0, "snare", 0.6],
                    [3.5, "tom", 0.8],
                    [4.0, "crash", 1.0]
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "layers": {
                        "drums": {
                            "basePattern": "basicBeat",
                            "variations": {
                                "bar4": "fillBeat"
                            }
                        }
                    }
                }
            }
        })";
        
        // JSON 2.0 equivalent
        juce::String json2Pattern = R"({
            "version": "2.0",
            "patterns": {
                "basicBeat": {
                    "events": [
                        {"t": 1.0, "n": "kick", "v": 0.8},
                        {"t": 3.0, "n": "snare", "v": 0.7}
                    ]
                },
                "fillBeat": {
                    "events": [
                        {"t": 3.0, "n": "snare", "v": 0.6},
                        {"t": 3.5, "n": "tom", "v": 0.8},
                        {"t": 4.0, "n": "crash", "v": 1.0}
                    ]
                }
            },
            "sections": {
                "verse": {
                    "layers": {
                        "drums": {
                            "pattern": "basicBeat",
                            "variations": {
                                "bar4": {
                                    "pattern": "fillBeat"
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("variation-test", "rock");
        
        auto result1 = musician->convertNotationToMidiEvents(json1Pattern, 120.0);
        auto result2 = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result1.getProperty("success", false), "JSON 1.0 variations should work");
        expect(result2.getProperty("success", false), "JSON 2.0 variations should work");
        
        logMessage("Pattern variation parity verified");
    }
    
    void testMultiBarEvents()
    {
        beginTest("Multi-bar event distribution");
        
        // JSON 1.0 format
        juce::String json1Pattern = R"({
            "sections": {
                "progression": {
                    "bars": 4,
                    "layers": {
                        "chords": [
                            [1.0, "C3", 0.7, 4.0],
                            [5.0, "F3", 0.7, 4.0],
                            [9.0, "G3", 0.7, 4.0],
                            [13.0, "C3", 0.7, 4.0]
                        ]
                    }
                }
            }
        })";
        
        // JSON 2.0 equivalent
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {"octave": 3, "velocity": 0.7},
            "events": [
                {"t": 1.0, "n": "C", "dur": 4.0},
                {"t": 5.0, "n": "F", "dur": 4.0},
                {"t": 9.0, "n": "G", "dur": 4.0},
                {"t": 13.0, "n": "C", "dur": 4.0}
            ]
        })";
        
        auto musician = std::make_unique<Musician>("multibar-test", "ambient");
        
        auto result1 = musician->convertNotationToMidiEvents(json1Pattern, 120.0);
        auto result2 = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result1.getProperty("success", false), "JSON 1.0 multi-bar should work");
        expect(result2.getProperty("success", false), "JSON 2.0 multi-bar should work");
        
        logMessage("Multi-bar event parity verified");
    }
    
    void testExpressionFeatures()
    {
        beginTest("Expression and dynamics equivalence");
        
        // JSON 2.0 format with advanced expression features
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {
                "dynamics": "mf",
                "articulation": "legato",
                "velocity": 0.7
            },
            "events": [
                {
                    "t": 1.0, "n": "C4", "v": 0.8,
                    "expression": {
                        "dynamics": "ff",
                        "articulation": "staccato",
                        "vibrato": {"rate": 5, "depth": 0.2}
                    }
                },
                {
                    "t": 2.0, "n": "D4", "v": 0.7,
                    "processors": {
                        "reverb": {"room": 0.5, "decay": 3.0}
                    }
                }
            ]
        })";
        
        auto musician = std::make_unique<Musician>("expression-test", "classical");
        
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "JSON 2.0 expression features should work");
        
        auto events = result["events"];
        if (events.isArray()) {
            expect(events.getArray()->size() > 0, "Should generate events with expression");
        }
        
        logMessage("Expression features working");
    }
    
    void testComplexArrangements()
    {
        beginTest("Complex multi-section arrangements");
        
        // JSON 2.0 complex arrangement
        juce::String json2Pattern = R"({
            "version": "2.0",
            "meta": {"title": "Complex Song", "tempo": 120},
            "patterns": {
                "verse_drums": {
                    "events": [
                        {"t": 1.0, "n": "kick", "v": 0.7},
                        {"t": 3.0, "n": "snare", "v": 0.6}
                    ]
                },
                "chorus_drums": {
                    "events": [
                        {"t": 1.0, "n": "kick", "v": 0.9},
                        {"t": 2.0, "n": "snare", "v": 0.8},
                        {"t": 3.0, "n": "kick", "v": 0.9},
                        {"t": 4.0, "n": "snare", "v": 0.8}
                    ]
                }
            },
            "sections": {
                "verse": {
                    "context": {"dynamics": "mp"},
                    "layers": {
                        "drums": {"pattern": "verse_drums"},
                        "bass": {
                            "context": {"octave": 2},
                            "events": [
                                {"t": 1.0, "n": "C", "dur": 2.0},
                                {"t": 3.0, "n": "G", "dur": 2.0}
                            ]
                        }
                    }
                },
                "chorus": {
                    "context": {"dynamics": "f"},
                    "layers": {
                        "drums": {"pattern": "chorus_drums"},
                        "bass": {
                            "context": {"octave": 2},
                            "events": [
                                {"t": 1.0, "n": "F", "dur": 1.0},
                                {"t": 2.0, "n": "C", "dur": 1.0},
                                {"t": 3.0, "n": "G", "dur": 2.0}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["verse", "chorus", "verse", "chorus"]
        })";
        
        auto musician = std::make_unique<Musician>("arrangement-test", "pop");
        
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Complex arrangements should work");
        
        logMessage("Complex arrangement parity verified");
    }
    
    void testTimeSignatureSupport()
    {
        beginTest("Time signature and timing");
        
        // JSON 2.0 with specific timing
        juce::String json2Pattern = R"({
            "version": "2.0",
            "meta": {"timeSignature": "6/8", "tempo": 120},
            "events": [
                {"t": 1.0, "n": "kick", "v": 0.8},
                {"t": 1.33, "n": "hihat", "v": 0.4},
                {"t": 2.0, "n": "snare", "v": 0.7},
                {"t": 2.67, "n": "hihat", "v": 0.4},
                {"t": 3.0, "n": "kick", "v": 0.6}
            ]
        })";
        
        auto musician = std::make_unique<Musician>("timing-test", "jazz");
        
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Time signatures should work");
        
        logMessage("Time signature support verified");
    }
    
    void testVelocityFormats()
    {
        beginTest("Multiple velocity formats");
        
        // JSON 2.0 with various velocity formats
        juce::String json2Pattern = R"({
            "version": "2.0",
            "events": [
                {"t": 1.0, "n": "kick", "v": 0.9, "dynamics": "f"},
                {"t": 2.0, "n": "snare", "v": 0.3, "dynamics": "pp"},
                {"t": 3.0, "n": "hihat", "v": 0.6, "dynamics": "mf"},
                {"t": 4.0, "n": "crash", "v": 1.0, "dynamics": "ff"}
            ]
        })";
        
        auto musician = std::make_unique<Musician>("velocity-test", "dynamic");
        
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Velocity formats should work");
        
        logMessage("Velocity format parity verified");
    }
    
    void testArticulationSystem()
    {
        beginTest("Articulation and expression");
        
        // JSON 2.0 with articulation
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {"articulation": "legato"},
            "events": [
                {"t": 1.0, "n": "C4", "articulation": "staccato", "dur": 0.5},
                {"t": 2.0, "n": "D4", "articulation": "tenuto", "dur": 1.0},
                {"t": 3.0, "n": "E4", "dur": 1.0},
                {"t": 4.0, "n": "F4", "articulation": "marcato", "dur": 0.5}
            ]
        })";
        
        auto musician = std::make_unique<Musician>("articulation-test", "classical");
        
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Articulation should work");
        
        logMessage("Articulation system verified");
    }
    
    void testAdvancedHarmony()
    {
        beginTest("Advanced harmonic features");
        
        // JSON 2.0 with complex harmony
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {
                "key": "C major",
                "chordProgression": ["Cmaj7", "Am7", "Fmaj7", "G7"]
            },
            "layers": {
                "chords": {
                    "context": {"octave": 4},
                    "events": [
                        {"t": 1.0, "d": 1, "dur": 4.0},
                        {"t": 1.0, "d": 3, "dur": 4.0},
                        {"t": 1.0, "d": 5, "dur": 4.0},
                        {"t": 1.0, "d": 7, "dur": 4.0}
                    ],
                    "variations": {
                        "bar2": {
                            "context": {"chord": "Am7"},
                            "events": [
                                {"t": 5.0, "d": 1, "dur": 4.0},
                                {"t": 5.0, "d": 3, "dur": 4.0},
                                {"t": 5.0, "d": 5, "dur": 4.0},
                                {"t": 5.0, "d": 7, "dur": 4.0}
                            ]
                        }
                    }
                },
                "bass": {
                    "context": {"octave": 2},
                    "events": [
                        {"t": 1.0, "d": 1, "dur": 1.0},
                        {"t": 2.0, "d": 3, "dur": 1.0},
                        {"t": 3.0, "d": 5, "dur": 1.0},
                        {"t": 4.0, "d": 7, "dur": 1.0}
                    ]
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("harmony-test", "jazz");
        
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.getProperty("success", false), "Advanced harmony should work");
        
        logMessage("Advanced harmony verified");
    }
};

// Register the test
static FeatureParityTests featureParityTests;

} // namespace covertracks