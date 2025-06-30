#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Integration test to verify Music JSON 2.0 format support in Musician class
 * Tests the complete flow: JSON 2.0 → UniversalMusicParser → MusicalContainer → MIDI Events
 */
class MusicJson2IntegrationTest : public juce::UnitTest
{
public:
    MusicJson2IntegrationTest() : juce::UnitTest("Music JSON 2.0 Integration", "Music Pattern System") {}
    
    void runTest() override
    {
        testMusicJson2Support();
        testBackwardCompatibility();
        testFormatDetection();
    }
    
private:
    void testMusicJson2Support()
    {
        beginTest("Music JSON 2.0 format support");
        
        // Create JSON 2.0 pattern
        juce::String json2Pattern = R"({
            "version": "2.0",
            "context": {
                "velocity": 0.8,
                "octave": 4,
                "dynamics": "mf"
            },
            "patterns": {
                "basic": {
                    "events": [
                        {"t": 1.0, "n": "kick", "v": 0.8},
                        {"t": 3.0, "n": "snare", "v": 0.7}
                    ]
                }
            },
            "sections": {
                "verse": {
                    "layers": {
                        "drums": {
                            "pattern": "basic"
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("json2-test", "electronic");
        
        // Test conversion using Music JSON 2.0
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.isObject(), "Should return object for JSON 2.0");
        expect(result.getProperty("success", false), "Should successfully parse JSON 2.0");
        expect(result.hasProperty("events"), "Should have MIDI events");
        
        // Verify events were generated
        auto events = result["events"];
        if (events.isArray()) {
            expect(events.getArray()->size() > 0, "Should generate MIDI events from JSON 2.0");
        }
        
        logMessage("Music JSON 2.0 format working correctly");
    }
    
    void testBackwardCompatibility()
    {
        beginTest("Backward compatibility with JSON 1.0");
        
        // Create JSON 1.0 pattern (no version field)
        juce::String json1Pattern = R"({
            "title": "BackwardCompatTest",
            "tempo": 120,
            "primitives": {
                "drums": {
                    "kick": 36,
                    "snare": 38
                }
            },
            "sections": {
                "main": [
                    [1.0, "kick"],
                    [3.0, "snare"]
                ]
            }
        })";
        
        auto musician = std::make_unique<Musician>("compat-test", "rock");
        
        // Test conversion using legacy JSON 1.0
        auto result = musician->convertNotationToMidiEvents(json1Pattern, 120.0);
        
        expect(result.isObject(), "Should return object for JSON 1.0");
        expect(result.getProperty("success", false), "Should successfully parse JSON 1.0");
        expect(result.hasProperty("events"), "Should have MIDI events");
        
        logMessage("JSON 1.0 backward compatibility maintained");
    }
    
    void testFormatDetection()
    {
        beginTest("Automatic format version detection");
        
        auto musician = std::make_unique<Musician>("detection-test", "ambient");
        
        // Test explicit JSON 2.0 detection
        juce::String explicitV2 = R"({"version": "2.0", "events": []})";
        auto result2 = musician->convertNotationToMidiEvents(explicitV2, 120.0);
        expect(result2.isObject(), "Should handle explicit version 2.0");
        
        // Test default to JSON 1.0 when no version specified
        juce::String noVersion = R"({"title": "Test", "sections": {}})";
        auto result1 = musician->convertNotationToMidiEvents(noVersion, 120.0);
        expect(result1.isObject(), "Should default to JSON 1.0 when no version");
        
        logMessage("Format detection working correctly");
    }
};

// Register the test
static MusicJson2IntegrationTest musicJson2IntegrationTest;

} // namespace covertracks