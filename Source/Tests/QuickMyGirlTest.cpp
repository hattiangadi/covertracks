#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Quick test of My Girl pattern parsing
 */
class QuickMyGirlTest : public juce::UnitTest
{
public:
    QuickMyGirlTest() : juce::UnitTest("Quick My Girl Test", "Music Pattern System") {}
    
    void runTest() override
    {
        beginTest("Quick My Girl JSON 2.0 test");
        
        // Simplified version of the Claude-generated pattern
        juce::String myGirl = R"({
            "version": "2.0",
            "meta": {"title": "My Girl Test", "tempo": 88},
            "context": {"velocity": 0.75, "dynamics": "mf"},
            "patterns": {
                "jazzComping": {
                    "context": {"octave": 3, "velocity": 0.65},
                    "events": [
                        {"t": 1.5, "d": 3, "dur": 0.5},
                        {"t": 1.5, "d": 7, "dur": 0.5}
                    ]
                }
            },
            "sections": {
                "verse": {
                    "context": {"chord": "Cmaj9"},
                    "layers": {
                        "melody": {
                            "context": {"octave": 4, "velocity": 0.85},
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 1.0}
                            ]
                        },
                        "comping": {
                            "pattern": "jazzComping"
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("quick-test", "jazz");
        auto result = musician->convertNotationToMidiEvents(myGirl, 88.0);
        
        // Check basic parsing
        expect(result.isObject(), "Should return object");
        expect(result.getProperty("success", false), "Should parse successfully");
        expect((double)result.getProperty("tempo", 0.0) == 88.0, "Should preserve tempo");
        
        auto events = result["events"];
        if (events.isArray()) {
            int count = events.getArray()->size();
            expect(count > 0, "Should generate events");
            logMessage("Generated " + juce::String(count) + " events from My Girl test");
        }
        
        logMessage("✅ My Girl JSON 2.0 pattern working!");
    }
};

static QuickMyGirlTest quickMyGirlTest;

} // namespace covertracks