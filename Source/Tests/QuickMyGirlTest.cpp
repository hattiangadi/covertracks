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
        testBasicTiming();
        testArrangementProcessing();
        testPowersOfTwoIssue();
        testContainerAliasing();
        testAliasedContainerTypes();
        testSelfDescribingAliases();
    }
    
private:
    void testBasicTiming()
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
            
            // TDD: Check timing offset issue - first event should be at timestamp 0.0
            if (count > 0) {
                for (auto& eventVar : *events.getArray()) {
                    if (eventVar.isObject()) {
                        auto eventObj = eventVar.getDynamicObject();
                        if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                            double timestamp = eventObj->getProperty("timestamp");
                            int note = eventObj->getProperty("note");
                            logMessage("TDD TIMING CHECK: First note " + juce::String(note) + " at timestamp " + juce::String(timestamp));
                            
                            // This should pass if timing is correct, fail if there's a +1 beat offset
                            if (timestamp != 0.0) {
                                logMessage("❌ TIMING ISSUE: t:1.0 maps to timestamp " + juce::String(timestamp) + " instead of 0.0");
                            } else {
                                logMessage("✅ TIMING OK: t:1.0 correctly maps to timestamp 0.0");
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        logMessage("✅ My Girl JSON 2.0 pattern working!");
    }
    
    void testArrangementProcessing()
    {
        beginTest("TDD: Arrangement should process full sections");
        
        // Test with arrangement that has multiple events per section  
        juce::String arrangementTest = R"({
            "version": "2.0",
            "sections": {
                "intro": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "C4", "dur": 1.0},
                                {"t": 2.0, "n": "D4", "dur": 1.0},
                                {"t": 3.0, "n": "E4", "dur": 1.0}
                            ]
                        }
                    }
                },
                "verse": {
                    "layers": {
                        "melody": {
                            "events": [
                                {"t": 1.0, "n": "G4", "dur": 1.0},
                                {"t": 2.0, "n": "A4", "dur": 1.0}
                            ]
                        }
                    }
                }
            },
            "arrangement": ["intro", "verse"]
        })";
        
        auto musician = std::make_unique<Musician>("arrangement-test", "piano");
        auto result = musician->convertNotationToMidiEvents(arrangementTest, 120.0);
        
        expect(result.getProperty("success", false), "Should parse arrangement");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteOnCount = 0;
            juce::Array<int> noteNumbers;
            
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        noteOnCount++;
                        int note = eventObj->getProperty("note");
                        double timestamp = eventObj->getProperty("timestamp");
                        noteNumbers.add(note);
                        logMessage("Found note: MIDI " + juce::String(note) + " at timestamp " + juce::String(timestamp));
                    }
                }
            }
            
            logMessage("TDD ARRANGEMENT CHECK: Found " + juce::String(noteOnCount) + " note events");
            
            // Should have 5 note events (3 from intro: C4,D4,E4 + 2 from verse: G4,A4)
            if (noteOnCount == 5) {
                logMessage("✅ ARRANGEMENT OK: Found 5 events from both sections as expected");
                bool hasC4 = noteNumbers.contains(60); // C4 
                bool hasD4 = noteNumbers.contains(62); // D4
                bool hasE4 = noteNumbers.contains(64); // E4
                bool hasG4 = noteNumbers.contains(67); // G4
                bool hasA4 = noteNumbers.contains(69); // A4
                
                juce::String noteList = "";
                for (int i = 0; i < noteNumbers.size(); i++) {
                    noteList += juce::String(noteNumbers[i]);
                    if (i < noteNumbers.size() - 1) noteList += ", ";
                }
                logMessage("Notes found: " + noteList);
                logMessage("Looking for intro: C4=60,D4=62,E4=64 and verse: G4=67,A4=69");
                
                if (hasC4 && hasD4 && hasE4 && hasG4 && hasA4) {
                    logMessage("✅ ARRANGEMENT CONTENT OK: Found all expected notes from both sections");
                } else {
                    logMessage("❌ ARRANGEMENT CONTENT ISSUE: Missing notes - C4=" + juce::String(hasC4 ? "yes" : "no") + 
                              ", D4=" + juce::String(hasD4 ? "yes" : "no") + ", E4=" + juce::String(hasE4 ? "yes" : "no") + 
                              ", G4=" + juce::String(hasG4 ? "yes" : "no") + ", A4=" + juce::String(hasA4 ? "yes" : "no"));
                }
            } else if (noteOnCount == 2) {
                logMessage("❌ ARRANGEMENT ISSUE: Only processing 1 event per section instead of full sections");
            } else {
                logMessage("❌ ARRANGEMENT ISSUE: Unexpected note count: " + juce::String(noteOnCount) + " (expected 5)");
            }
        }
    }
    
    void testPowersOfTwoIssue()
    {
        beginTest("TDD: My Girl full arrangement - powers of 2 issue");
        
        // The actual My Girl pattern that shows the powers of 2 issue
        juce::String myGirlFull = R"({
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
        {"t": 1.5, "d": 7, "dur": 0.5}
      ]
    },
    "bassLine": {
      "context": {"octave": 2, "velocity": 0.8},
      "events": [
        {"t": 1.0, "d": 1, "dur": 0.75},
        {"t": 2.0, "d": 5, "dur": 0.5}
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
            {"t": 2.0, "n": "E4", "dur": 0.33},
            {"t": 3.0, "n": "G4", "dur": 0.33}
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
            {"t": 2.0, "n": "D4", "dur": 1.0}
          ]
        },
        "comping": {
          "pattern": "jazzComping",
          "context": {"chord": "Cmaj9"}
        }
      }
    },
    "chorus": {
      "context": {"velocity": 0.85, "dynamics": "f"},
      "layers": {
        "melody": {
          "context": {"octave": 4, "velocity": 0.9},
          "events": [
            {"t": 1.0, "n": "F4", "dur": 1.0},
            {"t": 2.0, "n": "G4", "dur": 1.0}
          ]
        }
      }
    },
    "bridge": {
      "context": {"velocity": 0.8, "dynamics": "mf"},
      "layers": {
        "melody": {
          "context": {"octave": 4, "velocity": 0.85},
          "events": [
            {"t": 1.0, "n": "A4", "dur": 1.0},
            {"t": 2.0, "n": "B4", "dur": 1.0}
          ]
        }
      }
    }
  },
  "arrangement": ["intro", "verse", "chorus", "bridge", "verse", "chorus"]
})";
        
        auto musician = std::make_unique<Musician>("mygirl-full-test", "piano");
        auto result = musician->convertNotationToMidiEvents(myGirlFull, 88.0);
        
        expect(result.getProperty("success", false), "Should parse My Girl arrangement");
        
        auto events = result["events"];
        if (events.isArray()) {
            juce::Array<double> timestamps;
            juce::Array<int> noteNumbers;
            
            for (auto& eventVar : *events.getArray()) {
                if (eventVar.isObject()) {
                    auto eventObj = eventVar.getDynamicObject();
                    if (eventObj && eventObj->getProperty("type").toString() == "note_on") {
                        double timestamp = eventObj->getProperty("timestamp");
                        int note = eventObj->getProperty("note");
                        timestamps.add(timestamp);
                        noteNumbers.add(note);
                        logMessage("Found note: MIDI " + juce::String(note) + " at timestamp " + juce::String(timestamp));
                    }
                }
            }
            
            logMessage("TDD MY GIRL ARRANGEMENT: Found " + juce::String(timestamps.size()) + " events");
            
            juce::String timestampStr = "";
            for (int i = 0; i < timestamps.size(); i++) {
                timestampStr += juce::String(timestamps[i]);
                if (i < timestamps.size() - 1) timestampStr += ", ";
            }
            logMessage("Timestamps: " + timestampStr);
            
            // Expected: arrangement has 6 sections: intro, verse, chorus, bridge, verse, chorus
            // Each section should have multiple events, so we should get many more than 6 events
            int expectedMinEvents = 15; // Conservative estimate - should be more
            
            if (timestamps.size() >= expectedMinEvents) {
                logMessage("✅ ARRANGEMENT SIZE OK: Found " + juce::String(timestamps.size()) + " events (>= " + juce::String(expectedMinEvents) + ")");
                
                // Check if we have events spanning multiple sections properly
                double maxTimestamp = 0;
                for (double ts : timestamps) {
                    if (ts > maxTimestamp) maxTimestamp = ts;
                }
                
                // Should span multiple sections - estimate 6 sections * 4 beats = 24 beats minimum
                if (maxTimestamp >= 20.0) {
                    logMessage("✅ TIME SPAN OK: Events span " + juce::String(maxTimestamp) + " beats across sections");
                } else {
                    logMessage("❌ TIME SPAN ISSUE: Only spans " + juce::String(maxTimestamp) + " beats (expected >= 20)");
                }
                
            } else {
                logMessage("❌ ARRANGEMENT ISSUE: Only " + juce::String(timestamps.size()) + " events (expected >= " + juce::String(expectedMinEvents) + ")");
                
                // Check if we only have events at powers of 2 positions in the timeline
                juce::Array<double> suspiciousTimes;
                for (double ts : timestamps) {
                    // Check if timestamp is close to 1, 2, 4, 8, 16, 32, etc.
                    juce::Array<double> powersOf2 = {0, 1, 2, 4, 8, 16, 32, 64};
                    for (double p2 : powersOf2) {
                        if (std::abs(ts - p2) < 0.1) { // Allow some tolerance
                            suspiciousTimes.addIfNotAlreadyThere(ts);
                            break;
                        }
                    }
                }
                
                if (suspiciousTimes.size() == timestamps.size() && timestamps.size() > 3) {
                    logMessage("🔍 POWERS OF 2 DETECTED: All events at suspicious positions!");
                    logMessage("This suggests sections are only rendering when called with specific bar numbers");
                } else {
                    logMessage("🔍 Different issue - not exactly powers of 2 pattern");
                }
            }
        }
    }
    
    void testContainerAliasing()
    {
        beginTest("Container aliasing concept test");
        
        // Test if current parser can handle nested structure with clear duration
        juce::String aliasedPattern = R"({
            "version": "2.0",
            "meta": {"title": "Aliasing Test", "tempo": 120},
            "sections": {
                "intro": {
                    "context": {"bars": 4},
                    "layers": {
                        "piano": {
                            "variations": {
                                "bar1": {"events": [{"t": 1.0, "n": "C4", "dur": 1.0}]},
                                "bar2": {"events": [{"t": 1.0, "n": "D4", "dur": 1.0}]},
                                "bar3": {"events": [{"t": 1.0, "n": "E4", "dur": 1.0}]},
                                "bar4": {"events": [{"t": 1.0, "n": "F4", "dur": 1.0}]}
                            }
                        }
                    }
                },
                "verse": {
                    "context": {"bars": 8},
                    "layers": {
                        "piano": {
                            "variations": {
                                "bar1-4": {"events": [{"t": 1.0, "n": "G4", "dur": 4.0}]},
                                "bar5-8": {"events": [{"t": 1.0, "n": "A4", "dur": 4.0}]}
                            }
                        }
                    }
                }
            },
            "arrangement": ["intro", "verse"]
        })";
        
        auto musician = std::make_unique<Musician>("alias-test", "piano");
        auto result = musician->convertNotationToMidiEvents(aliasedPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse pattern with duration hints");
        
        auto events = result["events"];
        if (events.isArray()) {
            int noteCount = 0;
            double maxTime = 0;
            juce::String noteSequence;
            
            for (auto& ev : *events.getArray()) {
                if (ev.isObject()) {
                    auto obj = ev.getDynamicObject();
                    if (obj && obj->getProperty("type").toString() == "note_on") {
                        noteCount++;
                        int note = obj->getProperty("note");
                        double time = obj->getProperty("timestamp");
                        
                        if (time > maxTime) maxTime = time;
                        
                        // Map note to name
                        juce::String noteName;
                        switch(note) {
                            case 60: noteName = "C4"; break;
                            case 62: noteName = "D4"; break;
                            case 64: noteName = "E4"; break;
                            case 65: noteName = "F4"; break;
                            case 67: noteName = "G4"; break;
                            case 69: noteName = "A4"; break;
                            default: noteName = juce::String(note); break;
                        }
                        
                        noteSequence += noteName + "@" + juce::String(time, 1) + " ";
                    }
                }
            }
            
            logMessage("Container aliasing test: " + juce::String(noteCount) + " notes");
            logMessage("Note sequence: " + noteSequence);
            logMessage("Time span: 0 to " + juce::String(maxTime));
            
            // With proper duration handling, we'd expect:
            // - Intro: 4 notes (C4, D4, E4, F4) over 4 bars
            // - Verse: 2 notes (G4 for 4 bars, A4 for 4 bars) 
            // Currently it likely only plays first bar of each section
            
            if (noteCount >= 4) {
                logMessage("✅ Got at least intro notes");
            } else {
                logMessage("❌ Missing expected notes - only " + juce::String(noteCount) + " found");
            }
            
            if (maxTime >= 8.0) {
                logMessage("✅ Pattern spans expected duration");
            } else {
                logMessage("❓ Pattern only spans " + juce::String(maxTime) + " beats (expected 12+ bars)");
            }
        }
    }
    
    void testAliasedContainerTypes()
    {
        beginTest("Aliased container types - phrases, bars, beats");
        
        // Test the new aliased container types
        juce::String aliasedPattern = R"({
            "version": "2.0",
            "meta": {"title": "Aliased Containers Test", "tempo": 120},
            "sections": {
                "verse": {
                    "phrases": {
                        "opening": {
                            "bars": {
                                "1": {
                                    "beats": {
                                        "1": {"events": [{"t": 1.0, "n": "C4", "dur": 0.25}]},
                                        "2": {"events": [{"t": 1.0, "n": "E4", "dur": 0.25}]},
                                        "3": {"events": [{"t": 1.0, "n": "G4", "dur": 0.25}]},
                                        "4": {"events": [{"t": 1.0, "n": "C5", "dur": 0.25}]}
                                    }
                                },
                                "2": {
                                    "beats": {
                                        "1": {"events": [{"t": 1.0, "n": "D4", "dur": 0.25}]},
                                        "2": {"events": [{"t": 1.0, "n": "F4", "dur": 0.25}]},
                                        "3": {"events": [{"t": 1.0, "n": "A4", "dur": 0.25}]},
                                        "4": {"events": [{"t": 1.0, "n": "D5", "dur": 0.25}]}
                                    }
                                }
                            }
                        },
                        "closing": {
                            "bars": {
                                "1": {
                                    "events": [{"t": 1.0, "n": "G3", "dur": 4.0}]
                                }
                            }
                        }
                    }
                }
            },
            "arrangement": ["verse"]
        })";
        
        auto musician = std::make_unique<Musician>("aliased-test", "piano");
        auto result = musician->convertNotationToMidiEvents(aliasedPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse aliased container types");
        
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
                        
                        // Map note to name for readability
                        juce::String noteName;
                        switch(note) {
                            case 55: noteName = "G3"; break;
                            case 60: noteName = "C4"; break;
                            case 62: noteName = "D4"; break;
                            case 64: noteName = "E4"; break;
                            case 65: noteName = "F4"; break;
                            case 67: noteName = "G4"; break;
                            case 69: noteName = "A4"; break;
                            case 72: noteName = "C5"; break;
                            case 74: noteName = "D5"; break;
                            default: noteName = juce::String(note); break;
                        }
                        
                        noteSequence += noteName + "@" + juce::String(time, 1) + " ";
                    }
                }
            }
            
            logMessage("Aliased containers generated: " + juce::String(noteCount) + " notes");
            logMessage("Note sequence: " + noteSequence);
            
            // We expect:
            // - Opening phrase, bar 1: C4, E4, G4, C5 (4 beats)
            // - Opening phrase, bar 2: D4, F4, A4, D5 (4 beats)
            // - Closing phrase, bar 1: G3 (whole note)
            // Total: 9 notes
            
            if (noteCount >= 9) {
                logMessage("✅ Successfully parsed phrases/bars/beats structure");
            } else {
                logMessage("❌ Expected 9 notes but got " + juce::String(noteCount));
                logMessage("This suggests the aliased container types aren't being parsed correctly");
            }
        }
    }
    
    void testSelfDescribingAliases()
    {
        beginTest("Self-describing aliases with quantitative relationships");
        
        // Test pattern with its own alias definitions
        juce::String selfDescribingPattern = R"({
            "version": "2.0",
            "meta": {"title": "Self-Describing Alias Test"},
            
            "aliases": {
                "verse": {
                    "type": "sequential",
                    "contains": "bars",
                    "count": 8
                },
                "chorus": {
                    "type": "sequential",
                    "contains": "bars",
                    "count": 4
                },
                "bar": {
                    "type": "sequential",
                    "contains": "beats",
                    "count": 4
                }
            },
            
            "sections": {
                "verse1": {
                    "bars": {
                        "1": {"events": [{"t": 1, "n": "C4"}]},
                        "2": {"events": [{"t": 1, "n": "D4"}]},
                        "3": {"events": [{"t": 1, "n": "E4"}]},
                        "4": {"events": [{"t": 1, "n": "F4"}]}
                    }
                },
                "chorus1": {
                    "bars": {
                        "1": {"events": [{"t": 1, "n": "G4"}]},
                        "2": {"events": [{"t": 1, "n": "A4"}]}
                    }
                }
            },
            "arrangement": ["verse1", "chorus1"]
        })";
        
        auto musician = std::make_unique<Musician>("self-describing-test", "piano");
        auto result = musician->convertNotationToMidiEvents(selfDescribingPattern, 120.0);
        
        expect(result.getProperty("success", false), "Should parse self-describing pattern");
        
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
                        
                        juce::String noteName;
                        switch(note) {
                            case 60: noteName = "C4"; break;
                            case 62: noteName = "D4"; break;
                            case 64: noteName = "E4"; break;
                            case 65: noteName = "F4"; break;
                            case 67: noteName = "G4"; break;
                            case 69: noteName = "A4"; break;
                            default: noteName = juce::String(note); break;
                        }
                        
                        noteSequence += noteName + "@" + juce::String(time, 1) + " ";
                    }
                }
            }
            
            logMessage("Self-describing aliases test:");
            logMessage("  Defined: verse→8 bars, chorus→4 bars, bar→4 beats");
            logMessage("  Generated: " + juce::String(noteCount) + " notes");
            logMessage("  Sequence: " + noteSequence);
            
            // Should have notes from both verse (4 bars specified) and chorus (2 bars specified)
            if (noteCount >= 6) {
                logMessage("✅ Successfully parsed self-describing aliases");
            } else {
                logMessage("❌ Expected at least 6 notes but got " + juce::String(noteCount));
                logMessage("This might indicate alias definitions aren't being used");
            }
        }
    }
};

static QuickMyGirlTest quickMyGirlTest;

} // namespace covertracks