#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Live test of Claude-generated My Girl pattern
 */
class MyGirlLiveTest : public juce::UnitTest
{
public:
    MyGirlLiveTest() : juce::UnitTest("My Girl Live Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testMyGirlPattern();
    }
    
private:
    void testMyGirlPattern()
    {
        beginTest("Claude-generated My Girl jazz arrangement");
        
        juce::String myGirlPattern = R"({
            "version": "2.0",
            "meta": {
                "title": "My Girl - Jazz Piano Arrangement",
                "composer": "William 'Smokey' Robinson/Ronald White",
                "tempo": 88,
                "timeSignature": "4/4",
                "key": "C major",
                "style": "Jazz with R&B feel"
            },
            "context": {
                "velocity": 0.75,
                "dynamics": "mf",
                "articulation": "legato"
            },
            "patterns": {
                "jazzComping": {
                    "context": {"octave": 3, "velocity": 0.65},
                    "events": [
                        {"t": 1.5, "d": 3, "dur": 0.5},
                        {"t": 1.5, "d": 7, "dur": 0.5},
                        {"t": 2.25, "d": 1, "dur": 0.25, "v": 0.5},
                        {"t": 3.5, "d": 5, "dur": 0.5},
                        {"t": 3.5, "d": 9, "dur": 0.5},
                        {"t": 4.25, "d": 3, "dur": 0.25, "v": 0.55}
                    ]
                },
                "bassLine": {
                    "context": {"octave": 2, "velocity": 0.8},
                    "events": [
                        {"t": 1.0, "d": 1, "dur": 0.75},
                        {"t": 2.0, "d": 5, "dur": 0.5},
                        {"t": 2.75, "d": 3, "dur": 0.25},
                        {"t": 3.0, "d": 1, "dur": 0.5},
                        {"t": 4.0, "d": 7, "dur": 0.75}
                    ]
                }
            },
            "sections": {
                "intro": {
                    "context": {"velocity": 0.7, "dynamics": "mp"},
                    "layers": {
                        "melody": {
                            "context": {"octave": 4, "velocity": 0.8},
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 0.33},
                                {"t": 1.33, "n": "C4", "dur": 0.33},
                                {"t": 1.67, "n": "D4", "dur": 0.33},
                                {"t": 2.0, "n": "E4", "dur": 0.33}
                            ]
                        },
                        "harmony": {
                            "context": {"chord": "Cmaj9", "octave": 3, "velocity": 0.6},
                            "events": [
                                {"t": 1.0, "n": "E3", "dur": 2.0},
                                {"t": 1.0, "n": "G3", "dur": 2.0},
                                {"t": 1.0, "n": "B3", "dur": 2.0}
                            ]
                        }
                    }
                },
                "verse": {
                    "context": {"velocity": 0.75, "dynamics": "mf"},
                    "layers": {
                        "melody": {
                            "context": {"octave": 4, "velocity": 0.85},
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 1.0},
                                {"t": 2.0, "n": "C4", "dur": 1.0},
                                {"t": 3.0, "n": "C4", "dur": 2.0}
                            ]
                        },
                        "leftHand": {
                            "pattern": "bassLine",
                            "context": {"chord": "Cmaj9"}
                        },
                        "comping": {
                            "pattern": "jazzComping",
                            "context": {"chord": "Cmaj9"}
                        }
                    },
                    "variations": {
                        "bar2": {
                            "layers": {
                                "melody": {
                                    "events": [
                                        {"t": 1.0, "n": "A4", "dur": 0.5, "v": 0.9},
                                        {"t": 1.5, "n": "G4", "dur": 0.5},
                                        {"t": 2.0, "n": "F4", "dur": 0.5},
                                        {"t": 2.5, "n": "E4", "dur": 0.5}
                                    ]
                                },
                                "leftHand": {
                                    "pattern": "bassLine",
                                    "context": {"chord": "Fmaj7#11"}
                                },
                                "comping": {
                                    "pattern": "jazzComping", 
                                    "context": {"chord": "Fmaj7#11"}
                                }
                            }
                        }
                    }
                }
            },
            "arrangement": ["intro", "verse"]
        })";
        
        auto musician = std::make_unique<Musician>("my-girl-test", "jazz");
        auto result = musician->convertNotationToMidiEvents(myGirlPattern, 88.0);
        
        expect(result.getProperty("success", false), "My Girl pattern should parse successfully");
        expect((double)result.getProperty("tempo", 0.0) == 88.0, "Should preserve tempo");
        
        auto events = result["events"];
        if (events.isArray()) {
            expect(events.getArray()->size() > 0, "Should generate MIDI events");
            
            int eventCount = events.getArray()->size();
            logMessage("Generated " + juce::String(eventCount) + " MIDI events from My Girl arrangement");
            
            // Check that we have a reasonable number of events for a jazz arrangement
            expect(eventCount >= 10, "Should have substantial number of events for complex arrangement");
        }
        
        logMessage("Claude-generated My Girl pattern successfully parsed and converted to MIDI!");
    }
};

// Register the test
static MyGirlLiveTest myGirlLiveTest;

} // namespace covertracks