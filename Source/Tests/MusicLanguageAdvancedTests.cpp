// Source/Tests/MusicLanguageAdvancedTests.cpp
#include <JuceHeader.h>
#include "../Musician/Core/PatternParser.h"
#include "../Musician/Core/MusicPatternSystem.h"

namespace covertracks {

/**
 * Tests for advanced music language features and future architecture preparation
 * These tests validate current advanced features and prepare for the unified v2.0 language
 */
class MusicLanguageAdvancedTests : public juce::UnitTest
{
public:
    MusicLanguageAdvancedTests() : juce::UnitTest("Advanced Music Language", "Music Pattern System") {}

    void runTest() override
    {
        testComplexChordProgressions();
        testAdvancedRhythmicPatterns();
        testPolyphonicVoicing();
        testDynamicExpressionSupport();
        testCrossSectionReferences();
        testRealWorldSongStructures();
        testLanguageExtensibilityPatterns();
        testPerformanceOptimizations();
    }

private:
    void testComplexChordProgressions()
    {
        beginTest("Complex chord progressions and voice leading");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jazzChordProgression = R"({
            "title": "Jazz Chord Progression",
            "tempo": 120,
            "primitives": {
                "piano": {
                    "C3": 48, "D3": 50, "E3": 52, "F3": 53, "G3": 55, "A3": 57, "B3": 59,
                    "C4": 60, "D4": 62, "E4": 64, "F4": 65, "G4": 67, "A4": 69, "B4": 71,
                    "C5": 72, "D5": 74, "E5": 76, "F5": 77, "G5": 79
                }
            },
            "chords": {
                "Cmaj7": ["C4", "E4", "G4", "B4"],
                "Am7": ["A3", "C4", "E4", "G4"],
                "Dm7": ["D4", "F4", "A4", "C5"],
                "G7": ["G3", "B3", "D4", "F4"],
                "Em7b5": ["E4", "G4", "B3", "D4"],
                "A7alt": ["A3", "C#4", "E4", "G4"]
            },
            "sections": {
                "jazzProgression": {
                    "bars": 8,
                    "layers": {
                        "chordProgression": [
                            [1.0, {"chord": "Cmaj7"}, 0.7, 4.0],
                            [5.0, {"chord": "Am7"}, 0.7, 4.0],
                            [9.0, {"chord": "Dm7"}, 0.7, 4.0],
                            [13.0, {"chord": "G7"}, 0.7, 4.0],
                            [17.0, {"chord": "Em7b5"}, 0.7, 4.0],
                            [21.0, {"chord": "A7alt"}, 0.7, 4.0],
                            [25.0, {"chord": "Dm7"}, 0.7, 4.0],
                            [29.0, {"chord": "G7"}, 0.7, 4.0]
                        ],
                        "walkingBass": [
                            [1.0, "C3", 0.8, 1.0], [2.0, "E3", 0.7, 1.0], [3.0, "G3", 0.7, 1.0], [4.0, "A3", 0.7, 1.0],
                            [5.0, "A3", 0.8, 1.0], [6.0, "C4", 0.7, 1.0], [7.0, "E3", 0.7, 1.0], [8.0, "D3", 0.7, 1.0],
                            [9.0, "D3", 0.8, 1.0], [10.0, "F3", 0.7, 1.0], [11.0, "A3", 0.7, 1.0], [12.0, "G3", 0.7, 1.0],
                            [13.0, "G3", 0.8, 1.0], [14.0, "B3", 0.7, 1.0], [15.0, "D4", 0.7, 1.0], [16.0, "E4", 0.7, 1.0]
                        ]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(jazzChordProgression);
        expect(chart != nullptr, "Jazz chord progression should parse");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Jazz phrase should exist");
                expect(phrase->getNumBars() == 8, "Should have 8 bars");
                
                // Check that chord voicings are present
                bool foundChordNotes = false, foundBassNotes = false;
                for (int barIdx = 0; barIdx < phrase->getNumBars(); ++barIdx)
                {
                    auto bar = phrase->getBar(barIdx);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        for (int i = 0; i < midi->getNumEvents(); ++i)
                        {
                            auto event = midi->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note >= 60) foundChordNotes = true; // Higher register chords
                                if (note < 60) foundBassNotes = true;   // Lower register bass
                            }
                        }
                    }
                }
                
                expect(foundChordNotes, "Should find chord voicing notes");
                expect(foundBassNotes, "Should find bass line notes");
                logMessage("Jazz progression: Chords=" + String(foundChordNotes ? "Yes" : "No") + 
                          ", Bass=" + String(foundBassNotes ? "Yes" : "No"));
            }
        }
        
        logMessage("PASS: Complex chord progressions test completed");
    }
    
    void testAdvancedRhythmicPatterns()
    {
        beginTest("Advanced rhythmic patterns with subdivisions");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String complexRhythms = R"({
            "title": "Complex Rhythms",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42, "ride": 51, "crash": 49}
            },
            "sections": {
                "polyrhythm": {
                    "bars": 4,
                    "layers": {
                        "tripletHihat": [
                            [1.0, "hihat", 0.6, 0.33], [1.33, "hihat", 0.5, 0.33], [1.67, "hihat", 0.6, 0.33],
                            [2.0, "hihat", 0.6, 0.33], [2.33, "hihat", 0.5, 0.33], [2.67, "hihat", 0.6, 0.33],
                            [3.0, "hihat", 0.6, 0.33], [3.33, "hihat", 0.5, 0.33], [3.67, "hihat", 0.6, 0.33],
                            [4.0, "hihat", 0.6, 0.33], [4.33, "hihat", 0.5, 0.33], [4.67, "hihat", 0.6, 0.33]
                        ],
                        "sixteenthKick": [
                            [1.0, "kick", 0.9, 0.25], [1.25, "kick", 0.7, 0.25], [1.75, "kick", 0.8, 0.25],
                            [2.5, "kick", 0.85, 0.25], [3.0, "kick", 0.9, 0.25], [3.75, "kick", 0.7, 0.25],
                            [4.25, "kick", 0.8, 0.25]
                        ],
                        "syncopatedSnare": [
                            [1.75, "snare", 0.8, 0.25], [2.5, "snare", 0.9, 0.25],
                            [3.25, "snare", 0.7, 0.25], [4.0, "snare", 0.85, 0.25]
                        ],
                        "offbeatRide": [
                            [1.5, "ride", 0.6, 0.5], [2.25, "ride", 0.6, 0.25], [2.75, "ride", 0.6, 0.25],
                            [3.5, "ride", 0.6, 0.5], [4.25, "ride", 0.6, 0.25]
                        ]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(complexRhythms);
        expect(chart != nullptr, "Complex rhythms should parse");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Polyrhythm phrase should exist");
                
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        
                        // Count different drum sounds
                        int kickCount = 0, snareCount = 0, hihatCount = 0, rideCount = 0;
                        for (int i = 0; i < midi->getNumEvents(); ++i)
                        {
                            auto event = midi->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note == 36) kickCount++;       // Kick
                                else if (note == 38) snareCount++; // Snare
                                else if (note == 42) hihatCount++; // Hihat
                                else if (note == 51) rideCount++;  // Ride
                            }
                        }
                        
                        expect(kickCount >= 3, "Should have multiple kick events");
                        expect(snareCount >= 2, "Should have syncopated snare events");
                        expect(hihatCount >= 4, "Should have triplet hihat events");
                        expect(rideCount >= 2, "Should have offbeat ride events");
                        
                        logMessage("Polyrhythm bar 1: Kick=" + String(kickCount) + 
                                  ", Snare=" + String(snareCount) + 
                                  ", Hihat=" + String(hihatCount) + 
                                  ", Ride=" + String(rideCount));
                    }
                }
            }
        }
        
        logMessage("PASS: Advanced rhythmic patterns test completed");
    }
    
    void testPolyphonicVoicing()
    {
        beginTest("Polyphonic voicing and simultaneous events");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String polyphonicPattern = R"({
            "title": "Polyphonic Voicing",
            "tempo": 120,
            "sections": {
                "orchestral": {
                    "bars": 2,
                    "layers": {
                        "soprano": [
                            [1.0, "C5", 0.8, 2.0], [3.0, "D5", 0.7, 2.0]
                        ],
                        "alto": [
                            [1.0, "G4", 0.7, 2.0], [3.0, "A4", 0.7, 2.0]
                        ],
                        "tenor": [
                            [1.0, "E4", 0.7, 2.0], [3.0, "F4", 0.7, 2.0]
                        ],
                        "bass": [
                            [1.0, "C3", 0.8, 2.0], [3.0, "D3", 0.8, 2.0]
                        ],
                        "counterpoint": [
                            [1.5, "E5", 0.6, 0.5], [2.0, "D5", 0.6, 0.5], [2.5, "C5", 0.6, 0.5],
                            [3.5, "F5", 0.6, 0.5], [4.0, "E5", 0.6, 0.5], [4.5, "D5", 0.6, 0.5]
                        ]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(polyphonicPattern);
        expect(chart != nullptr, "Polyphonic pattern should parse");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        
                        // Check for simultaneous notes (polyphony)
                        Array<double> timestamps;
                        for (int i = 0; i < midi->getNumEvents(); ++i)
                        {
                            auto event = midi->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                timestamps.add(event->message.getTimeStamp());
                            }
                        }
                        
                        // Count simultaneous events (within 0.001 seconds)
                        int simultaneousCount = 0;
                        for (int i = 0; i < timestamps.size(); ++i)
                        {
                            for (int j = i + 1; j < timestamps.size(); ++j)
                            {
                                if (std::abs(timestamps[i] - timestamps[j]) < 0.001)
                                {
                                    simultaneousCount++;
                                }
                            }
                        }
                        
                        expect(simultaneousCount >= 3, "Should have simultaneous polyphonic notes");
                        expect(midi->getNumEvents() >= 10, "Should have multiple voice parts");
                        
                        logMessage("Polyphonic bar: " + String(midi->getNumEvents()) + " events, " + 
                                  String(simultaneousCount) + " simultaneous pairs");
                    }
                }
            }
        }
        
        logMessage("PASS: Polyphonic voicing test completed");
    }
    
    void testDynamicExpressionSupport()
    {
        beginTest("Dynamic expression and velocity variations");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String expressivePattern = R"({
            "title": "Dynamic Expression",
            "tempo": 120,
            "sections": {
                "expressive": {
                    "bars": 4,
                    "layers": {
                        "crescendo": [
                            [1.0, "C4", 0.3, 1.0], [2.0, "D4", 0.4, 1.0], 
                            [3.0, "E4", 0.5, 1.0], [4.0, "F4", 0.6, 1.0],
                            [5.0, "G4", 0.7, 1.0], [6.0, "A4", 0.8, 1.0], 
                            [7.0, "B4", 0.9, 1.0], [8.0, "C5", 1.0, 1.0]
                        ],
                        "accents": [
                            [1.0, "C3", 0.9, 0.5], [1.75, "C3", 0.4, 0.25],
                            [2.5, "C3", 0.95, 0.5], [3.25, "C3", 0.3, 0.25],
                            [4.0, "C3", 0.85, 0.5], [4.75, "C3", 0.35, 0.25]
                        ],
                        "sforzando": [
                            [2.0, "G4", 1.0, 0.25], [4.0, "G4", 1.0, 0.25], 
                            [6.0, "G4", 1.0, 0.25], [8.0, "G4", 1.0, 0.25]
                        ]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(expressivePattern);
        expect(chart != nullptr, "Dynamic expression pattern should parse");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Expressive phrase should exist");
                expect(phrase->getNumBars() == 4, "Should have 4 bars");
                
                // Check velocity ranges across all bars
                float minVelocity = 1.0f, maxVelocity = 0.0f;
                int totalNotes = 0;
                
                for (int barIdx = 0; barIdx < phrase->getNumBars(); ++barIdx)
                {
                    auto bar = phrase->getBar(barIdx);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        for (int i = 0; i < midi->getNumEvents(); ++i)
                        {
                            auto event = midi->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                float velocity = event->message.getFloatVelocity();
                                minVelocity = jmin(minVelocity, velocity);
                                maxVelocity = jmax(maxVelocity, velocity);
                                totalNotes++;
                            }
                        }
                    }
                }
                
                expect(totalNotes >= 10, "Should have multiple expressive notes");
                expect(maxVelocity - minVelocity >= 0.5f, "Should have wide dynamic range");
                expect(maxVelocity >= 0.9f, "Should have forte dynamics");
                expect(minVelocity <= 0.4f, "Should have piano dynamics");
                
                logMessage("Dynamic range: " + String(minVelocity, 2) + " to " + String(maxVelocity, 2) + 
                          " (" + String(totalNotes) + " notes)");
            }
        }
        
        logMessage("PASS: Dynamic expression test completed");
    }
    
    void testCrossSectionReferences()
    {
        beginTest("Cross-section pattern references and modularity");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String modularPattern = R"({
            "title": "Modular References",
            "tempo": 120,
            "patterns": {
                "mainMelody": [[1.0, "C4", 0.8, 1.0], [2.0, "E4", 0.7, 1.0], [3.0, "G4", 0.8, 1.0], [4.0, "C4", 0.7, 1.0]],
                "baseLine": [[1.0, "C3", 0.8, 1.0], [2.0, "G2", 0.7, 1.0], [3.0, "C3", 0.8, 1.0], [4.0, "E3", 0.7, 1.0]],
                "drumFill": [[1.0, "kick"], [1.5, "snare"], [2.0, "kick"], [2.5, "snare"], [3.0, "crash"]],
                "verseHarmony": [[1.0, "C3", 0.6, 4.0], [1.0, "E3", 0.6, 4.0], [1.0, "G3", 0.6, 4.0]]
            },
            "sections": {
                "intro": {
                    "bars": 4,
                    "layers": {
                        "melody": {"pattern": "mainMelody"}
                    }
                },
                "verse": {
                    "bars": 8,
                    "layers": {
                        "melody": {"pattern": "mainMelody"},
                        "bass": {"pattern": "baseLine"},
                        "harmony": {"pattern": "verseHarmony"}
                    }
                },
                "chorus": {
                    "bars": 8,
                    "layers": {
                        "melody": {
                            "basePattern": "mainMelody",
                            "variations": {
                                "bar4": "drumFill",
                                "bar8": [[1.0, "C5", 0.9, 4.0]]
                            }
                        },
                        "bass": {"pattern": "baseLine"}
                    }
                }
            },
            "arrangement": ["intro", "verse", "chorus", "verse"]
        })";
        
        auto chart = parser.parseChart(modularPattern);
        expect(chart != nullptr, "Modular pattern should parse");
        
        if (chart)
        {
            expect(chart->getNumSections() == 4, "Should have 4 sections per arrangement");
            
            // Verify pattern reuse across sections
            bool foundIntro = false, foundVerse = false, foundChorus = false;
            for (int i = 0; i < chart->getNumSections(); ++i)
            {
                auto section = chart->getSection(i);
                if (section)
                {
                    String sectionName = section->getName();
                    if (sectionName == "intro") foundIntro = true;
                    else if (sectionName == "verse") foundVerse = true;
                    else if (sectionName == "chorus") foundChorus = true;
                    
                    // Verify section has content
                    expect(section->getNumPhrases() > 0, "Section '" + sectionName + "' should have phrases");
                }
            }
            
            expect(foundIntro, "Should find intro section");
            expect(foundVerse, "Should find verse section");
            expect(foundChorus, "Should find chorus section");
            
            logMessage("Found sections: Intro=" + String(foundIntro ? "Yes" : "No") + 
                      ", Verse=" + String(foundVerse ? "Yes" : "No") + 
                      ", Chorus=" + String(foundChorus ? "Yes" : "No"));
        }
        
        logMessage("PASS: Cross-section references test completed");
    }
    
    void testRealWorldSongStructures()
    {
        beginTest("Real-world song structures and complexity");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test a realistic pop song structure
        String popSongStructure = R"({
            "title": "Pop Song Structure",
            "tempo": 128,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42, "crash": 49},
                "bass": {"E2": 40, "F2": 41, "G2": 43, "A2": 45, "B2": 47, "C3": 48, "D3": 50}
            },
            "chords": {
                "C": ["C3", "E3", "G3"],
                "G": ["G3", "B3", "D4"],
                "Am": ["A3", "C4", "E4"],
                "F": ["F3", "A3", "C4"]
            },
            "patterns": {
                "basicDrums": [[1.0, "kick"], [2.0, "snare"], [3.0, "kick"], [4.0, "snare"]],
                "rockDrums": [[1.0, "kick"], [1.5, "hihat"], [2.0, "snare"], [2.5, "hihat"], [3.0, "kick"], [3.5, "hihat"], [4.0, "snare"], [4.5, "hihat"]],
                "walkingBass": [[1.0, "C3"], [2.0, "D3"], [3.0, "E2"], [4.0, "F2"]],
                "powerChords": [[1.0, {"chord": "C"}], [3.0, {"chord": "G"}]]
            },
            "sections": {
                "intro": {
                    "bars": 8,
                    "layers": {
                        "drums": {"pattern": "basicDrums"}
                    }
                },
                "verse": {
                    "bars": 16,
                    "layers": {
                        "drums": {"pattern": "basicDrums"},
                        "bass": {"pattern": "walkingBass"},
                        "chords": {"pattern": "powerChords"}
                    }
                },
                "prechorus": {
                    "bars": 8,
                    "layers": {
                        "drums": {
                            "basePattern": "basicDrums",
                            "variations": {
                                "bar8": [[1.0, "crash"], [2.0, "snare"], [2.5, "snare"], [3.0, "snare"], [4.0, "crash"]]
                            }
                        },
                        "bass": {"pattern": "walkingBass"}
                    }
                },
                "chorus": {
                    "bars": 16,
                    "layers": {
                        "drums": {"pattern": "rockDrums"},
                        "bass": {"pattern": "walkingBass"},
                        "chords": {"pattern": "powerChords"}
                    }
                },
                "bridge": {
                    "bars": 8,
                    "layers": {
                        "drums": {
                            "basePattern": "basicDrums",
                            "variations": {
                                "bar4": [[1.0, "kick"], [2.0, "kick"], [3.0, "snare"], [4.0, "crash"]],
                                "bar8": [[1.0, "crash"], [2.0, "snare"], [3.0, "snare"], [4.0, "snare"]]
                            }
                        }
                    }
                }
            },
            "arrangement": [
                "intro", "verse", "prechorus", "chorus", 
                "verse", "prechorus", "chorus", 
                "bridge", "chorus", "chorus"
            ]
        })";
        
        auto chart = parser.parseChart(popSongStructure);
        expect(chart != nullptr, "Pop song structure should parse");
        
        if (chart)
        {
            expect(chart->getNumSections() == 10, "Should have 10 sections per arrangement");
            
            // Verify each section type appears correctly
            Array<String> expectedSections = {"intro", "verse", "prechorus", "chorus", "verse", "prechorus", "chorus", "bridge", "chorus", "chorus"};
            
            for (int i = 0; i < jmin(chart->getNumSections(), expectedSections.size()); ++i)
            {
                auto section = chart->getSection(i);
                if (section)
                {
                    expect(section->getName() == expectedSections[i], 
                           "Section " + String(i) + " should be '" + expectedSections[i] + "'");
                    expect(section->getNumPhrases() > 0, "Section should have content");
                }
            }
            
            // Check total song length
            int totalBars = 0;
            for (int i = 0; i < chart->getNumSections(); ++i)
            {
                auto section = chart->getSection(i);
                if (section && section->getNumPhrases() > 0)
                {
                    auto phrase = section->getPhrase(0);
                    if (phrase) totalBars += phrase->getNumBars();
                }
            }
            
            expect(totalBars > 100, "Pop song should be substantial length");
            logMessage("Total song length: " + String(totalBars) + " bars");
        }
        
        logMessage("PASS: Real-world song structures test completed");
    }
    
    void testLanguageExtensibilityPatterns()
    {
        beginTest("Language extensibility patterns for future features");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test forward-compatibility patterns that could support future v2.0 features
        String extensiblePattern = R"({
            "title": "Extensible Language Test",
            "tempo": 120,
            "metadata": {
                "composer": "Test Suite",
                "style": "experimental",
                "version": "1.0"
            },
            "primitives": {
                "synth": {"C4": 60, "E4": 64, "G4": 67}
            },
            "sections": {
                "experimental": {
                    "bars": 4,
                    "metadata": {
                        "description": "Testing extensibility",
                        "complexity": "medium"
                    },
                    "layers": {
                        "melody": [
                            [1.0, "C4", 0.8, 1.0, {"custom": "test", "future": "ready"}],
                            [2.0, "E4", 0.7, 1.0, {"custom": "test2"}],
                            [3.0, "G4", 0.8, 1.0],
                            [4.0, "C4", 0.7, 1.0]
                        ]
                    },
                    "extensions": {
                        "future_feature": "placeholder",
                        "experimental": true
                    }
                }
            },
            "extensions": {
                "test_mode": true,
                "future_compatibility": "v2.0"
            }
        })";
        
        auto chart = parser.parseChart(extensiblePattern);
        expect(chart != nullptr, "Extensible pattern should parse without errors");
        
        if (chart && chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midi = bar->toMidiSequence();
                        expect(midi->getNumEvents() >= 8, "Should parse core events despite extensions");
                        
                        // Verify basic functionality still works
                        bool foundNotes = false;
                        for (int i = 0; i < midi->getNumEvents(); ++i)
                        {
                            auto event = midi->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                foundNotes = true;
                                break;
                            }
                        }
                        expect(foundNotes, "Should find musical notes despite extensions");
                    }
                }
            }
        }
        
        logMessage("PASS: Language extensibility patterns test completed");
    }
    
    void testPerformanceOptimizations()
    {
        beginTest("Performance optimizations and caching");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test pattern that exercises caching and reuse
        String cachingPattern = R"({
            "title": "Caching Test",
            "tempo": 120,
            "patterns": {
                "reusedPattern": [[1.0, "C4"], [2.0, "E4"], [3.0, "G4"], [4.0, "C4"]]
            },
            "sections": {
                "section1": {
                    "bars": 4,
                    "layers": {
                        "melody1": {"pattern": "reusedPattern"},
                        "melody2": {"pattern": "reusedPattern"},
                        "melody3": {"pattern": "reusedPattern"}
                    }
                },
                "section2": {
                    "bars": 4,
                    "layers": {
                        "melody1": {"pattern": "reusedPattern"},
                        "melody2": {"pattern": "reusedPattern"}
                    }
                }
            },
            "arrangement": ["section1", "section2", "section1", "section2"]
        })";
        
        // Time multiple parses to test caching
        double totalTime = 0.0;
        const int iterations = 10;
        
        for (int i = 0; i < iterations; ++i)
        {
            double startTime = Time::getMillisecondCounterHiRes();
            auto chart = parser.parseChart(cachingPattern);
            double parseTime = Time::getMillisecondCounterHiRes() - startTime;
            totalTime += parseTime;
            
            expect(chart != nullptr, "Iteration " + String(i) + " should parse successfully");
            if (chart) expect(chart->getNumSections() == 4, "Should have correct number of sections");
        }
        
        double averageTime = totalTime / iterations;
        expect(averageTime < 100.0, "Average parse time should be under 100ms");
        
        logMessage("Performance test: " + String(averageTime, 2) + "ms average over " + 
                  String(iterations) + " iterations");
        
        logMessage("PASS: Performance optimizations test completed");
    }
};

// Register test with JUCE's unit test system
static MusicLanguageAdvancedTests musicLanguageAdvancedTests;

} // namespace covertracks