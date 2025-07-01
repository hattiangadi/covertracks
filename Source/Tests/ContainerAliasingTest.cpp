#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * Test the container aliasing approach for Music JSON 2.0
 */
class ContainerAliasingTest : public juce::UnitTest
{
public:
    ContainerAliasingTest() : juce::UnitTest("Container Aliasing Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testSemanticAliasing();
        testDurationSpecification();
        testNestedStructure();
    }
    
private:
    void testSemanticAliasing()
    {
        beginTest("Semantic aliasing - arrangement/section/phrase/bar/beat");
        
        // Using semantic aliases instead of layers/variations/patterns
        juce::String aliasedPattern = R"({
            "version": "2.0",
            "meta": {"title": "Aliasing Test", "tempo": 120},
            
            "arrangement": {
                "intro": {
                    "bars": 4,
                    "phrases": {
                        "opening": {
                            "bar1": {
                                "beats": {
                                    "1": {"events": [{"t": 1.0, "n": "C4", "dur": 1.0}]},
                                    "2": {"events": [{"t": 1.0, "n": "E4", "dur": 1.0}]},
                                    "3": {"events": [{"t": 1.0, "n": "G4", "dur": 1.0}]},
                                    "4": {"events": [{"t": 1.0, "n": "C5", "dur": 1.0}]}
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("alias-test", "piano");
        auto result = musician->convertNotationToMidiEvents(aliasedPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse aliased structure");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::Array<int> notes;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        notes.add(obj->getProperty("note"));
                    }
                }
            }
            
            logMessage("Found " + juce::String(noteCount) + " notes using aliased structure");
            
            // Should have C4, E4, G4, C5
            expect(noteCount == 4, "Should generate 4 notes");
            expect(notes.contains(60), "Should have C4");
            expect(notes.contains(64), "Should have E4");
            expect(notes.contains(67), "Should have G4");
            expect(notes.contains(72), "Should have C5");
        }
    }
    
    void testDurationSpecification()
    {
        beginTest("Duration specification at each level");
        
        juce::String durationPattern = R"({
            "version": "2.0",
            "arrangement": [
                {"section": "intro", "bars": 4},
                {"section": "verse", "bars": 16},
                {"section": "chorus", "bars": 8}
            ],
            "sections": {
                "intro": {
                    "layers": {
                        "piano": {
                            "events": [{"t": 1.0, "n": "C4", "dur": 4.0}]
                        }
                    }
                },
                "verse": {
                    "layers": {
                        "piano": {
                            "variations": {
                                "bar1": {"events": [{"t": 1.0, "n": "F4", "dur": 1.0}]},
                                "bar2": {"events": [{"t": 1.0, "n": "G4", "dur": 1.0}]},
                                "bar3": {"events": [{"t": 1.0, "n": "A4", "dur": 1.0}]},
                                "bar4": {"events": [{"t": 1.0, "n": "F4", "dur": 1.0}]}
                            }
                        }
                    }
                },
                "chorus": {
                    "layers": {
                        "piano": {
                            "events": [{"t": 1.0, "n": "C5", "dur": 8.0}]
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("duration-test", "piano");
        auto result = musician->convertNotationToMidiEvents(durationPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse duration specifications");
        
        auto events = result["events"];
        if (events.isArray()) {
            double maxTime = 0;
            int sectionCount = 0;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        double time = obj->getProperty("timestamp");
                        if (time > maxTime) maxTime = time;
                        
                        // Count distinct sections based on timing
                        if (time < 4) sectionCount |= 1;      // Intro
                        else if (time < 20) sectionCount |= 2; // Verse (4 + 16)
                        else sectionCount |= 4;                 // Chorus
                    }
                }
            }
            
            logMessage("Max timestamp: " + juce::String(maxTime));
            logMessage("Section coverage: " + juce::String(sectionCount));
            
            // Total duration should be 4 + 16 + 8 = 28 bars
            expect(maxTime >= 20.0, "Should span at least to chorus section");
            
            // Should have events in all 3 sections
            expect(sectionCount == 7, "Should have events in all sections (1|2|4 = 7)");
        }
    }
    
    void testNestedStructure()
    {
        beginTest("Nested container structure with context inheritance");
        
        juce::String nestedPattern = R"({
            "version": "2.0",
            "context": {"velocity": 0.5, "octave": 4},
            
            "song": {
                "movements": {
                    "first": {
                        "context": {"velocity": 0.7},
                        "sections": {
                            "theme": {
                                "context": {"octave": 5},
                                "phrases": {
                                    "main": {
                                        "bars": {
                                            "1": {
                                                "events": [
                                                    {"t": 1.0, "n": "C4", "dur": 1.0},
                                                    {"t": 2.0, "n": "E4", "dur": 1.0, "velocity": 0.9}
                                                ]
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        auto musician = std::make_unique<Musician>("nested-test", "piano");
        auto result = musician->convertNotationToMidiEvents(nestedPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse deeply nested structure");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            juce::Array<float> velocities;
            juce::Array<int> notes;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        velocities.add(obj->getProperty("velocity"));
                        notes.add(obj->getProperty("note"));
                        
                        int note = obj->getProperty("note");
                        float vel = obj->getProperty("velocity");
                        logMessage("Note: " + juce::String(note) + " velocity: " + juce::String(vel));
                    }
                }
            }
            
            expect(noteCount == 2, "Should generate 2 notes");
            
            if (noteCount >= 2) {
                // First note should inherit velocity 0.7 from movement
                expect(velocities[0] == 0.7f, "First note inherits movement velocity");
                
                // Second note has explicit velocity 0.9
                expect(velocities[1] == 0.9f, "Second note uses explicit velocity");
                
                // Notes should be C5 and E5 (octave 5 from section context)
                expect(notes[0] == 72, "First note should be C5 (72)");
                expect(notes[1] == 76, "Second note should be E5 (76)");
            }
        }
    }
};

static ContainerAliasingTest containerAliasingTest;

} // namespace covertracks