// Source/Tests/MusicJson2ArrangementTests.cpp
#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * TDD Test: JSON 2.0 Arrangement Processing Issue
 * 
 * EXPECTED TO FAIL: This test verifies that arrangement arrays should process all sections,
 * not just the first section or first bar. Currently fails because only 1 bar is generated
 * instead of the full arrangement.
 */
class MusicJson2ArrangementTests : public juce::UnitTest
{
public:
    MusicJson2ArrangementTests() : juce::UnitTest("JSON 2.0 Arrangement", "Music Pattern System") {}
    
    void runTest() override
    {
        testBasicArrangementCount();
        testMyGirlArrangement();
        testMultiSectionArrangement();
        testArrangementSectionOrder();
    }
    
private:
    void testBasicArrangementCount()
    {
        beginTest("Basic arrangement should process all sections (EXPECTED TO FAIL)");
        
        // Simple JSON 2.0 pattern with 2 sections in arrangement
        juce::String json2Pattern = R"({
            "version": "2.0",
            "sections": {
                "intro": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 1.0}
                            ]
                        }
                    }
                },
                "verse": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "G4", "dur": 1.0}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["intro", "verse"]
        })";
        
        auto musician = std::make_unique<Musician>("arrangement-test", "piano");
        auto result = musician->convertNotationToMidiEvents(json2Pattern, 120.0);
        
        expect(result.isObject(), "Should return object for JSON 2.0");
        expect(result.getProperty("success", false), "Should successfully parse JSON 2.0");
        expect(result.hasProperty("events"), "Should have MIDI events");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteOnCount = 0;
            juce::Array<int> noteNumbers;
            juce::Array<double> timestamps;
            
            // Count note-on events and collect details
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        noteOnCount++;
                        noteNumbers.add(eventObj->getProperty("note"));
                        timestamps.add(eventObj->getProperty("timestamp"));
                    }
                }
            }
            
            // Log details for debugging
            logMessage("Total note-on events: " + juce::String(noteOnCount));
            for (int i = 0; i < noteNumbers.size(); ++i) {
                logMessage("Note " + juce::String(i+1) + ": MIDI " + juce::String(noteNumbers[i]) + 
                          " at timestamp " + juce::String(timestamps[i]));
            }
            
            // CRITICAL TEST: Should have 2 note events (one from each section)
            // This will FAIL if only processing the first section
            expect(noteOnCount == 2, "Should have 2 note events from 2 sections, got " + juce::String(noteOnCount));
            
            if (noteOnCount >= 2) {
                // Should have C4 (60) from intro and G4 (67) from verse
                bool foundC4 = noteNumbers.contains(60);
                bool foundG4 = noteNumbers.contains(67);
                
                expect(foundC4, "Should find C4 (60) from intro section");
                expect(foundG4, "Should find G4 (67) from verse section");
                
                // Timestamps should show sequential sections
                if (timestamps.size() >= 2) {
                    double timeDifference = timestamps[1] - timestamps[0];
                    logMessage("Time difference between sections: " + juce::String(timeDifference));
                    expect(timeDifference > 0.0, "Second section should start after first section");
                }
            }
        } else {
            expect(false, "Should have events array");
        }
    }
    
    void testMyGirlArrangement()
    {
        beginTest("My Girl arrangement should process intro+verse (EXPECTED TO FAIL)");
        
        // Simplified version of the My Girl structure that was failing
        juce::String myGirlPattern = R"({
            "version": "2.0",
            "meta": {
                "title": "My Girl - Simplified Test",
                "tempo": 88,
                "timeSignature": "4/4"
            },
            "sections": {
                "intro": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 0.33},
                                {"t": 1.33, "n": "C4", "dur": 0.33},
                                {"t": 1.67, "n": "D4", "dur": 0.33},
                                {"t": 2.0, "n": "E4", "dur": 0.33}
                            ]
                        }
                    }
                },
                "verse": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "G4", "dur": 1.0},
                                {"t": 2.0, "n": "A4", "dur": 1.0},
                                {"t": 3.0, "n": "B4", "dur": 1.0},
                                {"t": 4.0, "n": "C5", "dur": 1.0}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["intro", "verse"]
        })";
        
        auto musician = std::make_unique<Musician>("mygirl-test", "piano");
        auto result = musician->convertNotationToMidiEvents(myGirlPattern, 88.0);
        
        expect(result.getProperty("success", false), "Should parse My Girl structure");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteOnCount = 0;
            juce::Array<int> noteNumbers;
            
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        noteOnCount++;
                        noteNumbers.add(eventObj->getProperty("note"));
                    }
                }
            }
            
            logMessage("My Girl total note events: " + juce::String(noteOnCount));
            
            // CRITICAL TEST: Should have 8 note events total (4 from intro + 4 from verse)
            // This will FAIL if only processing 4 notes from a single bar/section
            expect(noteOnCount == 8, "Should have 8 note events from both sections, got " + juce::String(noteOnCount));
            
            // Should have notes from both intro (C4, D4, E4) and verse (G4, A4, B4, C5)
            bool hasIntroNotes = noteNumbers.contains(60) || noteNumbers.contains(62) || noteNumbers.contains(64); // C4, D4, E4
            bool hasVerseNotes = noteNumbers.contains(67) || noteNumbers.contains(69) || noteNumbers.contains(71) || noteNumbers.contains(72); // G4, A4, B4, C5
            
            expect(hasIntroNotes, "Should have notes from intro section");
            expect(hasVerseNotes, "Should have notes from verse section");
        }
    }
    
    void testMultiSectionArrangement()
    {
        beginTest("Multi-section arrangement processing (EXPECTED TO FAIL)");
        
        // Test with more sections to ensure full arrangement processing
        juce::String multiSectionPattern = R"({
            "version": "2.0",
            "sections": {
                "a": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1.0, "n": "C4", "dur": 1.0}]
                        }
                    }
                },
                "b": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1.0, "n": "D4", "dur": 1.0}]
                        }
                    }
                },
                "c": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1.0, "n": "E4", "dur": 1.0}]
                        }
                    }
                },
                "d": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1.0, "n": "F4", "dur": 1.0}]
                        }
                    }
                }
            },
            "arrangement": ["a", "b", "c", "d"]
        })";
        
        auto musician = std::make_unique<Musician>("multi-section-test", "piano");
        auto result = musician->convertNotationToMidiEvents(multiSectionPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse multi-section arrangement");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteOnCount = 0;
            juce::Array<int> uniqueNotes;
            
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        noteOnCount++;
                        int note = eventObj->getProperty("note");
                        if (!uniqueNotes.contains(note)) {
                            uniqueNotes.add(note);
                        }
                    }
                }
            }
            
            logMessage("Multi-section note count: " + juce::String(noteOnCount));
            logMessage("Unique notes found: " + juce::String(uniqueNotes.size()));
            
            // CRITICAL TEST: Should have 4 note events (one from each section)
            expect(noteOnCount == 4, "Should have 4 note events from 4 sections, got " + juce::String(noteOnCount));
            expect(uniqueNotes.size() == 4, "Should have 4 unique notes, got " + juce::String(uniqueNotes.size()));
            
            // Should have all 4 expected notes: C4=60, D4=62, E4=64, F4=65
            expect(uniqueNotes.contains(60), "Should contain C4 from section a");
            expect(uniqueNotes.contains(62), "Should contain D4 from section b");
            expect(uniqueNotes.contains(64), "Should contain E4 from section c");
            expect(uniqueNotes.contains(65), "Should contain F4 from section d");
        }
    }
    
    void testArrangementSectionOrder()
    {
        beginTest("Arrangement section order preservation (EXPECTED TO FAIL)");
        
        juce::String orderTestPattern = R"({
            "version": "2.0",
            "sections": {
                "first": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1.0, "n": "C4", "dur": 0.5}]
                        }
                    }
                },
                "second": {
                    "layers": {
                        "melody": {
                            "events": [{"t": 1.0, "n": "G4", "dur": 0.5}]
                        }
                    }
                }
            },
            "arrangement": ["first", "second", "first"]
        })";
        
        auto musician = std::make_unique<Musician>("order-test", "piano");
        auto result = musician->convertNotationToMidiEvents(orderTestPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse arrangement order test");
        
        auto events = result["events"];
        if (events.isArray()) {
            juce::Array<int> noteSequence;
            juce::Array<double> timeSequence;
            
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        noteSequence.add(eventObj->getProperty("note"));
                        timeSequence.add(eventObj->getProperty("timestamp"));
                    }
                }
            }
            
            logMessage("Note sequence count: " + juce::String(noteSequence.size()));
            for (int i = 0; i < noteSequence.size(); ++i) {
                logMessage("Note " + juce::String(i+1) + ": " + juce::String(noteSequence[i]) + 
                          " at " + juce::String(timeSequence[i]));
            }
            
            // CRITICAL TEST: Should have 3 notes following arrangement order
            expect(noteSequence.size() == 3, "Should have 3 notes from arrangement, got " + juce::String(noteSequence.size()));
            
            if (noteSequence.size() >= 3) {
                // Should follow pattern: C4 (first), G4 (second), C4 (first again)
                expect(noteSequence[0] == 60, "First note should be C4 (60), got " + juce::String(noteSequence[0]));
                expect(noteSequence[1] == 67, "Second note should be G4 (67), got " + juce::String(noteSequence[1]));
                expect(noteSequence[2] == 60, "Third note should be C4 (60) again, got " + juce::String(noteSequence[2]));
                
                // Times should be sequential
                if (timeSequence.size() >= 3) {
                    expect(timeSequence[1] > timeSequence[0], "Second section should start after first");
                    expect(timeSequence[2] > timeSequence[1], "Third section should start after second");
                }
            }
        }
    }
};

// Register the test
static MusicJson2ArrangementTests musicJson2ArrangementTests;

} // namespace covertracks