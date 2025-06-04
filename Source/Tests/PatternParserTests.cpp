// Source/Tests/PatternParserTests.cpp
#include <JuceHeader.h>
#include <cmath>
#include "../Musician/Core/PatternParser.h"
#include "../Musician/Core/MusicPatternSystem.h"

namespace covertracks {

// T1: Basic pattern tests
class BasicPatternTests : public juce::UnitTest
{
public:
    BasicPatternTests() : juce::UnitTest("T1 Basic Patterns", "Music Pattern System") {}

    void runTest() override
    {
        testBasicDrumPattern();
        testBasicPianoPattern();
        testMultiBarPattern();
    }

private:
    void testBasicDrumPattern()
    {
        beginTest("Basic drum pattern parsing (JSON)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // JSON format test
        String jsonPattern = R"({
            "title": "TestDrums",
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
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 2.0, "primitive": "snare"},
                    {"beat": 3.0, "primitive": "kick"},
                    {"beat": 4.0, "primitive": "snare"}
                ]
            }
        })";
        
        logMessage("Testing JSON drum pattern");
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getTitle() == "TestDrums", "Chart title should be TestDrums");
        expect(std::abs(chart->getTempo() - 120.0) < 0.01, "Tempo should be 120 BPM");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            expect(section != nullptr, "Section should not be null");
            expect(section->getName() == "main", "Section should be named 'main'");
            
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    expect(bar != nullptr, "First bar should not be null");
                    
                    // Convert to MIDI to check the notes
                    auto midiSequence = bar->toMidiSequence();
                    expect(midiSequence->getNumEvents() >= 8, "Should have note on/off events for 4 notes");
                    
                    logMessage("Drum MIDI events: " + String(midiSequence->getNumEvents()));
                    
                    // Check for correct MIDI notes
                    bool foundKick = false, foundSnare = false;
                    for (int i = 0; i < midiSequence->getNumEvents(); ++i)
                    {
                        auto event = midiSequence->getEventPointer(i);
                        if (event->message.isNoteOn())
                        {
                            int note = event->message.getNoteNumber();
                            if (note == 36) foundKick = true;  // Kick
                            if (note == 38) foundSnare = true; // Snare
                            logMessage("Found drum note: " + String(note));
                        }
                    }
                    
                    expect(foundKick, "Should find kick drum (MIDI 36)");
                    expect(foundSnare, "Should find snare drum (MIDI 38)");
                }
            }
        }
    }
    
    void testBasicPianoPattern()
    {
        beginTest("Basic piano pattern parsing (JSON)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "TestPiano",
            "tempo": 120,
            "primitives": {
                "piano": {
                    "C3": 48,
                    "E3": 52,
                    "G3": 55,
                    "C4": 60
                }
            },
            "sections": {
                "main": [
                    {"beat": 1.0, "primitive": "C3"},
                    {"beat": 2.0, "primitive": "E3"},
                    {"beat": 3.0, "primitive": "G3"},
                    {"beat": 4.0, "primitive": "C3"}
                ]
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getTitle() == "TestPiano", "Chart title should be TestPiano");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
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
                        auto midiSequence = bar->toMidiSequence();
                        logMessage("Piano MIDI events: " + String(midiSequence->getNumEvents()));
                        
                        // Check for correct piano notes (C3=48, E3=52, G3=55)
                        bool foundC3 = false, foundE3 = false, foundG3 = false;
                        for (int i = 0; i < midiSequence->getNumEvents(); ++i)
                        {
                            auto event = midiSequence->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note == 48) foundC3 = true;  // C3
                                else if (note == 52) foundE3 = true;  // E3
                                else if (note == 55) foundG3 = true;  // G3
                                logMessage("Found piano note: " + String(note));
                            }
                        }
                        
                        expect(foundC3, "Should find C3 (MIDI 48)");
                        expect(foundE3, "Should find E3 (MIDI 52)");
                        expect(foundG3, "Should find G3 (MIDI 55)");
                    }
                }
            }
        }
    }
    
    void testMultiBarPattern()
    {
        beginTest("Multi-bar pattern parsing (JSON T2_MultiBars)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T2_MultiBars_Drums",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42, "tom": 47}
            },
            "sections": {
                "main": {
                    "bars": 4,
                    "barPatterns": [
                        [
                            {"beat": 1.0, "primitive": "kick"},
                            {"beat": 2.0, "primitive": "snare"},
                            {"beat": 3.0, "primitive": "kick"},
                            {"beat": 4.0, "primitive": "snare"}
                        ],
                        [
                            {"beat": 1.0, "primitive": "kick"},
                            {"beat": 1.5, "primitive": "kick"},
                            {"beat": 2.0, "primitive": "snare"},
                            {"beat": 4.0, "primitive": "snare"}
                        ],
                        [
                            {"beat": 1.0, "primitive": "kick"},
                            {"beat": 2.0, "primitive": "snare"},
                            {"beat": 3.0, "primitive": "kick"},
                            {"beat": 4.0, "primitive": "snare"}
                        ],
                        [
                            {"beat": 1.0, "primitive": "kick"},
                            {"beat": 2.0, "primitive": "tom"},
                            {"beat": 2.5, "primitive": "tom"},
                            {"beat": 3.0, "primitive": "tom"},
                            {"beat": 4.0, "primitive": "snare"}
                        ]
                    ]
                }
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            expect(section != nullptr, "Section should not be null");
            expect(section->getNumPhrases() > 0, "Section should have at least one phrase");
            
            if (section && section->getNumPhrases() > 0)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Multi-bar phrase should not be null");
                expect(phrase->getNumBars() == 4, "Multi-bar phrase should have 4 bars");
                
                if (phrase && phrase->getNumBars() == 4)
                {
                    // Check each bar for different patterns
                    bool foundKick = false, foundSnare = false, foundTom = false;
                    
                    for (int barIdx = 0; barIdx < 4; ++barIdx)
                    {
                        auto bar = phrase->getBar(barIdx);
                        expect(bar != nullptr, "Bar " + String(barIdx + 1) + " should not be null");
                        
                        if (bar)
                        {
                            auto midiSequence = bar->toMidiSequence();
                            logMessage("Bar " + String(barIdx + 1) + " has " + String(midiSequence->getNumEvents()) + " MIDI events");
                            
                            for (int i = 0; i < midiSequence->getNumEvents(); ++i)
                            {
                                auto event = midiSequence->getEventPointer(i);
                                if (event->message.isNoteOn())
                                {
                                    int note = event->message.getNoteNumber();
                                    if (note == 36) foundKick = true;   // Kick
                                    if (note == 38) foundSnare = true; // Snare
                                    if (note == 47) foundTom = true;   // Tom (only in bar 4)
                                    
                                    logMessage("Bar " + String(barIdx + 1) + " - Note: " + String(note) + " at beat " + String(event->message.getTimeStamp()));
                                }
                            }
                            
                            // Bar-specific validations
                            if (barIdx == 3) // Bar 4 - fill pattern
                            {
                                expect(midiSequence->getNumEvents() >= 5, "Bar 4 (fill) should have at least 5 events");
                            }
                            else
                            {
                                expect(midiSequence->getNumEvents() >= 4, "Regular bars should have at least 4 events");
                            }
                        }
                    }
                    
                    expect(foundKick, "Should find kick drum notes across bars");
                    expect(foundSnare, "Should find snare drum notes across bars");
                    expect(foundTom, "Should find tom fills in bar 4");
                }
            }
        }
    }
};

// T3: Chord pattern tests  
class ChordPatternTests : public juce::UnitTest
{
public:
    ChordPatternTests() : juce::UnitTest("T3 Chord Patterns", "Music Pattern System") {}

    void runTest() override
    {
        testChordPattern();
    }

private:
    void testChordPattern()
    {
        beginTest("Chord pattern parsing (JSON T3_Chords)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T3_Chords",
            "tempo": 120,
            "primitives": {
                "piano": {
                    "C3": 48,
                    "E3": 52,
                    "G3": 55,
                    "F3": 53,
                    "A3": 57,
                    "C4": 60
                }
            },
            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"]
            },
            "sections": {
                "main": [
                    {"beat": 1.0, "chord": "Cmaj"},
                    {"beat": 3.0, "chord": "Fmaj"}
                ]
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
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
                        auto midiSequence = bar->toMidiSequence();
                        logMessage("Chord MIDI events: " + String(midiSequence->getNumEvents()));
                        
                        // Should have 6 notes (3 per chord × 2 chords) = 12 MIDI events (on+off)
                        expect(midiSequence->getNumEvents() >= 12, "Should have at least 12 MIDI events (6 chord notes)");
                        
                        // Check for chord notes
                        bool foundCmaj = false, foundFmaj = false;
                        for (int i = 0; i < midiSequence->getNumEvents(); ++i)
                        {
                            auto event = midiSequence->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note == 48 || note == 52 || note == 55) foundCmaj = true;  // C3, E3, G3
                                if (note == 53 || note == 57 || note == 60) foundFmaj = true;  // F3, A3, C4
                                logMessage("Chord note: " + String(note));
                            }
                        }
                        
                        expect(foundCmaj, "Should find Cmaj chord notes");
                        expect(foundFmaj, "Should find Fmaj chord notes");
                    }
                }
            }
        }
    }
};

// T4: Layer pattern tests
class LayerPatternTests : public juce::UnitTest
{
public:
    LayerPatternTests() : juce::UnitTest("T4 Layer Patterns", "Music Pattern System") {}

    void runTest() override
    {
        testLayeredPattern();
    }

private:
    void testLayeredPattern()
    {
        beginTest("Layered pattern parsing (JSON T4_Layers)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T4_Layers_Drums",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42}
            },
            "sections": {
                "main": {
                    "layers": {
                        "kickPattern": [
                            {"beat": 1.0, "primitive": "kick"},
                            {"beat": 3.0, "primitive": "kick"}
                        ],
                        "snarePattern": [
                            {"beat": 2.0, "primitive": "snare"},
                            {"beat": 4.0, "primitive": "snare"}
                        ],
                        "hihatPattern": [
                            {"beat": 1.0, "primitive": "hihat"},
                            {"beat": 1.5, "primitive": "hihat"},
                            {"beat": 2.0, "primitive": "hihat"},
                            {"beat": 2.5, "primitive": "hihat"},
                            {"beat": 3.0, "primitive": "hihat"},
                            {"beat": 3.5, "primitive": "hihat"},
                            {"beat": 4.0, "primitive": "hihat"},
                            {"beat": 4.5, "primitive": "hihat"}
                        ]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            expect(section != nullptr, "Section should not be null");
            expect(section->getNumPhrases() >= 1, "Should have 1 merged phrase containing all drum layers");
            
            if (section->getNumPhrases() >= 1)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Merged phrase should not be null");
                
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midiSequence = bar->toMidiSequence();
                        logMessage("Merged phrase MIDI events: " + String(midiSequence->getNumEvents()));
                        
                        bool foundKick = false, foundSnare = false, foundHihat = false;
                        for (int j = 0; j < midiSequence->getNumEvents(); ++j)
                        {
                            auto event = midiSequence->getEventPointer(j);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note == 36) foundKick = true;
                                else if (note == 38) foundSnare = true;
                                else if (note == 42) foundHihat = true;
                            }
                        }
                        
                        expect(foundKick, "Should find kick drum events in merged phrase");
                        expect(foundSnare, "Should find snare drum events in merged phrase");
                        expect(foundHihat, "Should find hi-hat events in merged phrase");
                        expect(midiSequence->getNumEvents() >= 12, "Should have at least 12 events from merged layers");
                    }
                }
            }
        }
    }
};

// T5: Layer + Chord pattern tests
class LayerChordPatternTests : public juce::UnitTest
{
public:
    LayerChordPatternTests() : juce::UnitTest("T5 Layers + Chords", "Music Pattern System") {}

    void runTest() override
    {
        testLayersWithChords();
    }

private:
    void testLayersWithChords()
    {
        beginTest("Layers with chords parsing (JSON T5_LayersChords)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T5_LayersChords_Piano",
            "tempo": 120,
            "primitives": {
                "piano": {
                    "C3": 48, "E3": 52, "G3": 55, "C4": 60,
                    "F3": 53, "A3": 57, "D4": 62, "G4": 67,
                    "B3": 59
                }
            },
            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"],
                "Gmaj": ["G3", "B3", "D4"]
            },
            "sections": {
                "main": {
                    "layers": {
                        "leftHand": [
                            {"beat": 1.0, "chord": "Cmaj"},
                            {"beat": 2.0, "chord": "Cmaj"},
                            {"beat": 3.0, "chord": "Fmaj"},
                            {"beat": 4.0, "chord": "Gmaj"}
                        ],
                        "rightHandStructure": [
                            {"beat": 1.5, "primitive": "C4"},
                            {"beat": 2.5, "primitive": "D4"},
                            {"beat": 3.5, "primitive": "G4"}
                        ]
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            expect(section != nullptr, "Section should not be null");
            expect(section->getNumPhrases() >= 1, "Should have 1 merged phrase with layers and chords");
            
            if (section->getNumPhrases() >= 1)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Merged phrase should not be null");
                
                if (phrase && phrase->getNumBars() > 0)
                {
                    auto bar = phrase->getBar(0);
                    if (bar)
                    {
                        auto midiSequence = bar->toMidiSequence();
                        logMessage("T5 merged phrase MIDI events: " + String(midiSequence->getNumEvents()));
                        
                        // Should have: 4 chords × 3 notes each + 3 melody notes = 15 total events
                        expect(midiSequence->getNumEvents() >= 15, "Should have chord notes plus melody notes");
                        
                        // Check for chord notes and melody notes
                        bool foundChordNotes = false, foundMelodyNotes = false;
                        
                        for (int j = 0; j < midiSequence->getNumEvents(); ++j)
                        {
                            auto event = midiSequence->getEventPointer(j);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                
                                // Chord notes (lower register)
                                if (note == 48 || note == 52 || note == 55 || note == 53 || note == 57 || note == 59)
                                    foundChordNotes = true;
                                
                                // Melody notes (higher register)  
                                if (note == 60 || note == 62 || note == 67)
                                    foundMelodyNotes = true;
                                
                                logMessage("Layer+Chord note: " + String(note) + " at beat " + String(event->message.getTimeStamp()));
                            }
                        }
                        
                        expect(foundChordNotes, "Should find chord notes from left hand layer");
                        expect(foundMelodyNotes, "Should find melody notes from right hand layer");
                    }
                }
            }
        }
    }
};

// T6: Multi-section song tests
class MultiSectionSongTests : public juce::UnitTest
{
public:
    MultiSectionSongTests() : juce::UnitTest("T6 Multi-Section Songs", "Music Pattern System") {}

    void runTest() override
    {
        testSectionsWithArrangement();
    }

private:
    void testSectionsWithArrangement()
    {
        beginTest("Multi-section songs with arrangement (JSON T6_Sections)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T6_Sections_Song",
            "tempo": 120,
            "primitives": {
                "piano": {"C4": 60, "E4": 64, "G4": 67, "B4": 71, "D5": 74, "G5": 79}
            },
            "sections": {
                "verse": [
                    {"beat": 1.0, "primitive": "C4"},
                    {"beat": 2.0, "primitive": "E4"},
                    {"beat": 3.0, "primitive": "G4"},
                    {"beat": 4.0, "primitive": "C4"}
                ],
                "chorus": [
                    {"beat": 1.0, "primitive": "G4"},
                    {"beat": 2.0, "primitive": "B4"},
                    {"beat": 3.0, "primitive": "D5"},
                    {"beat": 4.0, "primitive": "G5"}
                ],
                "bridge": [
                    {"beat": 1.0, "primitive": "B4"},
                    {"beat": 2.0, "primitive": "D5"},
                    {"beat": 3.0, "primitive": "G5"},
                    {"beat": 4.0, "primitive": "B4"}
                ]
            },
            "arrangement": ["verse", "chorus", "verse", "chorus", "bridge", "chorus"]
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() == 6, "Should have 6 sections according to arrangement");
        
        if (chart->getNumSections() == 6)
        {
            // Verify arrangement order: verse, chorus, verse, chorus, bridge, chorus
            String expectedOrder[] = {"verse", "chorus", "verse", "chorus", "bridge", "chorus"};
            
            for (int i = 0; i < 6; ++i)
            {
                auto section = chart->getSection(i);
                expect(section != nullptr, "Section " + String(i) + " should not be null");
                
                if (section)
                {
                    String sectionName = section->getName();
                    expect(sectionName == expectedOrder[i], 
                           "Section " + String(i) + " should be '" + expectedOrder[i] + "' but got '" + sectionName + "'");
                    
                    // Verify section has content
                    expect(section->getNumPhrases() > 0, "Section '" + sectionName + "' should have phrases");
                    
                    if (section->getNumPhrases() > 0)
                    {
                        auto phrase = section->getPhrase(0);
                        if (phrase && phrase->getNumBars() > 0)
                        {
                            auto bar = phrase->getBar(0);
                            if (bar)
                            {
                                auto midiSequence = bar->toMidiSequence();
                                expect(midiSequence->getNumEvents() >= 4, 
                                       "Section '" + sectionName + "' should have 4 note events");
                                
                                // Verify section-specific notes
                                bool foundExpectedNotes = false;
                                for (int j = 0; j < midiSequence->getNumEvents(); ++j)
                                {
                                    auto event = midiSequence->getEventPointer(j);
                                    if (event->message.isNoteOn())
                                    {
                                        int note = event->message.getNoteNumber();
                                        if (sectionName == "verse" && note == 60) foundExpectedNotes = true;     // C4
                                        else if (sectionName == "chorus" && note == 67) foundExpectedNotes = true; // G4
                                        else if (sectionName == "bridge" && note == 71) foundExpectedNotes = true; // B4
                                    }
                                }
                                expect(foundExpectedNotes, "Should find expected notes for section '" + sectionName + "'");
                            }
                        }
                    }
                }
            }
        }
    }
};

// T7: Pattern reference tests
class PatternReferenceTests : public juce::UnitTest
{
public:
    PatternReferenceTests() : juce::UnitTest("T7 Pattern References", "Music Pattern System") {}

    void runTest() override
    {
        testPatternReferences();
    }

private:
    void testPatternReferences()
    {
        beginTest("Pattern references (JSON T7_References)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T7_References",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42}
            },
            "patterns": {
                "basicBeat": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 2.0, "primitive": "snare"},
                    {"beat": 3.0, "primitive": "kick"},
                    {"beat": 4.0, "primitive": "snare"}
                ],
                "hihatPattern": [
                    {"beat": 1.0, "primitive": "hihat"},
                    {"beat": 1.5, "primitive": "hihat"},
                    {"beat": 2.0, "primitive": "hihat"},
                    {"beat": 2.5, "primitive": "hihat"}
                ]
            },
            "sections": {
                "verse": {
                    "layers": {
                        "drums": {"pattern": "basicBeat"},
                        "hihat": {"pattern": "hihatPattern"}
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            if (section && section->getNumPhrases() >= 2)
            {
                // Should have 2 layers using pattern references
                bool foundBasicBeat = false, foundHihatPattern = false;
                
                for (int i = 0; i < section->getNumPhrases(); ++i)
                {
                    auto phrase = section->getPhrase(i);
                    if (phrase && phrase->getNumBars() > 0)
                    {
                        auto bar = phrase->getBar(0);
                        if (bar)
                        {
                            auto midiSequence = bar->toMidiSequence();
                            logMessage("Pattern reference layer MIDI events: " + String(midiSequence->getNumEvents()));
                            
                            for (int j = 0; j < midiSequence->getNumEvents(); ++j)
                            {
                                auto event = midiSequence->getEventPointer(j);
                                if (event->message.isNoteOn())
                                {
                                    int note = event->message.getNoteNumber();
                                    if (note == 36 || note == 38) foundBasicBeat = true;  // Kick/snare
                                    if (note == 42) foundHihatPattern = true;  // Hihat
                                }
                            }
                        }
                    }
                }
                
                expect(foundBasicBeat, "Should find basic beat pattern notes");
                expect(foundHihatPattern, "Should find hihat pattern notes");
            }
        }
    }
};

// T8: Pattern variations and validation tests  
class PatternVariationTests : public juce::UnitTest
{
public:
    PatternVariationTests() : juce::UnitTest("T8 Pattern Variations", "Music Pattern System") {}

    void runTest() override
    {
        testInvalidPatterns();
        testPatternVariations();
    }

private:
    void testInvalidPatterns()
    {
        beginTest("Invalid JSON pattern handling");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        // Test invalid JSON
        String invalidJson = "{ invalid json syntax }";
        auto chart1 = parser.parseChart(invalidJson);
        expect(chart1 == nullptr, "Should return null for invalid JSON");
        
        // Test empty JSON
        String emptyJson = "{}";
        auto chart2 = parser.parseChart(emptyJson);
        expect(chart2 != nullptr, "Should create empty chart for empty JSON");
        
        logMessage("Invalid pattern tests completed");
    }
    
    void testPatternVariations()
    {
        beginTest("Pattern variations (JSON T8_Exceptions)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T8_Exceptions_Drums",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42, "crash": 49, "tom": 47}
            },
            "patterns": {
                "basicBeat": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 2.0, "primitive": "snare"},
                    {"beat": 3.0, "primitive": "kick"},
                    {"beat": 4.0, "primitive": "snare"}
                ],
                "fillEnding": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 1.5, "primitive": "tom"},
                    {"beat": 2.0, "primitive": "tom"},
                    {"beat": 2.5, "primitive": "tom"},
                    {"beat": 3.0, "primitive": "snare"},
                    {"beat": 4.0, "primitive": "crash"}
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "basePattern": "basicBeat",
                    "variations": {
                        "bar4": "fillEnding"
                    }
                }
            }
        })";
        
        auto chart = parser.parseChart(jsonPattern);
        
        expect(chart != nullptr, "Chart should not be null");
        expect(chart->getNumSections() > 0, "Chart should have at least one section");
        
        if (chart->getNumSections() > 0)
        {
            auto section = chart->getSection(0);
            expect(section != nullptr, "Section should not be null");
            expect(section->getNumPhrases() >= 1, "Should have 1 variation phrase");
            
            if (section->getNumPhrases() >= 1)
            {
                auto phrase = section->getPhrase(0);
                expect(phrase != nullptr, "Variation phrase should not be null");
                expect(phrase->getNumBars() == 4, "Should have 4 bars as specified");
                
                if (phrase->getNumBars() == 4)
                {
                    // Check first 3 bars use basicBeat pattern (kick-snare-kick-snare)
                    for (int barIdx = 0; barIdx < 3; ++barIdx)
                    {
                        auto bar = phrase->getBar(barIdx);
                        if (bar)
                        {
                            auto midiSequence = bar->toMidiSequence();
                            expect(midiSequence->getNumEvents() >= 4, "Basic beat bars should have 4 events");
                            
                            // Check for basic beat pattern notes
                            bool foundKick = false, foundSnare = false;
                            for (int j = 0; j < midiSequence->getNumEvents(); ++j)
                            {
                                auto event = midiSequence->getEventPointer(j);
                                if (event->message.isNoteOn())
                                {
                                    int note = event->message.getNoteNumber();
                                    if (note == 36) foundKick = true;
                                    if (note == 38) foundSnare = true;
                                }
                            }
                            expect(foundKick && foundSnare, "Basic beat bar should have kick and snare");
                        }
                    }
                    
                    // Check bar 4 uses fillEnding pattern (kick-tom-tom-tom-snare-crash)
                    auto bar4 = phrase->getBar(3);  // 0-indexed, so bar 4 is index 3
                    if (bar4)
                    {
                        auto midiSequence = bar4->toMidiSequence();
                        expect(midiSequence->getNumEvents() >= 6, "Fill ending should have 6 events");
                        
                        // Check for fill pattern notes
                        bool foundTom = false, foundCrash = false;
                        for (int j = 0; j < midiSequence->getNumEvents(); ++j)
                        {
                            auto event = midiSequence->getEventPointer(j);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note == 47) foundTom = true;    // Tom
                                if (note == 49) foundCrash = true;  // Crash
                                logMessage("Bar 4 variation note: " + String(note));
                            }
                        }
                        expect(foundTom, "Bar 4 should have tom fills (note 47)");
                        expect(foundCrash, "Bar 4 should have crash (note 49)");
                    }
                }
            }
        }
    }
};

// Register all test classes with JUCE's unit test system
static BasicPatternTests basicPatternTests;
static ChordPatternTests chordPatternTests;
static LayerPatternTests layerPatternTests;
static LayerChordPatternTests layerChordPatternTests;
static MultiSectionSongTests multiSectionSongTests;
static PatternReferenceTests patternReferenceTests;
static PatternVariationTests patternVariationTests;

} // namespace covertracks