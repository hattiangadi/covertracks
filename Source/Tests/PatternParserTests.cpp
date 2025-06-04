// Source/Tests/PatternParserTests.cpp
#include <JuceHeader.h>
#include "../Musician/Core/PatternParser.h"
#include "../Musician/Core/MusicPatternSystem.h"

namespace covertracks {

class PatternParserTests : public juce::UnitTest
{
public:
    PatternParserTests() : juce::UnitTest("PatternParser", "Music Pattern System") {}

    void runTest() override
    {
        testBasicDrumPattern();
        testBasicPianoPattern();
        testPositionBasedPattern();
        testChordPattern();
        testInvalidPatterns();
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
        expect(chart->getTempo() == 120, "Tempo should be 120 BPM");
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
    
    void testPositionBasedPattern()
    {
        beginTest("Position-based pattern parsing (JSON T2_MultiBars)");
        
        MusicPatternSystem patternSystem;
        PatternParser parser(patternSystem);
        
        String jsonPattern = R"({
            "title": "T2_MultiBars_Drums",
            "tempo": 120,
            "primitives": {
                "drums": {
                    "kick": 36,
                    "snare": 38
                }
            },
            "sections": {
                "main": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 1.5, "primitive": "kick"},
                    {"beat": 2.0, "primitive": "snare"},
                    {"beat": 3.0, "primitive": "kick"},
                    {"beat": 4.0, "primitive": "snare"},
                    {"beat": 4.5, "primitive": "snare"}
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
                        logMessage("Position-based MIDI events: " + String(midiSequence->getNumEvents()));
                        
                        // Should have 6 events = 12 MIDI events (on+off)
                        expect(midiSequence->getNumEvents() >= 12, "Should have at least 12 MIDI events (6 notes)");
                        
                        // Check for kick and snare events
                        bool foundKick = false, foundSnare = false;
                        for (int i = 0; i < midiSequence->getNumEvents(); ++i)
                        {
                            auto event = midiSequence->getEventPointer(i);
                            if (event->message.isNoteOn())
                            {
                                int note = event->message.getNoteNumber();
                                if (note == 36) foundKick = true;  // Kick
                                if (note == 38) foundSnare = true; // Snare
                                logMessage("Position-based note: " + String(note) + " at time: " + String(event->message.getTimeStamp()));
                            }
                        }
                        
                        expect(foundKick, "Should find kick drum (MIDI 36)");
                        expect(foundSnare, "Should find snare drum (MIDI 38)");
                    }
                }
            }
        }
    }
    
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
};

// Register the test with JUCE's unit test system
static PatternParserTests patternParserTests;

} // namespace covertracks