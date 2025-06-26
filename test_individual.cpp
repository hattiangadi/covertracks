#include <JuceHeader.h>
#include "Source/Musician/Core/MusicPatternSystem.h"
#include "Source/Musician/Core/PatternParser.h"

using namespace juce;
using namespace covertracks;

int main()
{
    // Simple test of chord tone resolution
    String testJson = R"({
        "title": "Chord Tone Test",
        "tempo": 120,
        "chords": {
            "C": ["C3", "E3", "G3"],
            "F": ["F3", "A3", "C4"]
        },
        "sections": {
            "test": {
                "bars": 2,
                "layers": {
                    "harmony": [
                        [1.0, {"chord": "C"}, 0.7, 4.0],
                        [5.0, {"chord": "F"}, 0.7, 4.0]
                    ],
                    "bass": [
                        [1.0, {"tone": 1, "octave": 2}, 0.8, 1.0],
                        [2.0, {"tone": 3, "octave": 2}, 0.8, 1.0],
                        [5.0, {"tone": 1, "octave": 2}, 0.8, 1.0],
                        [6.0, {"tone": 3, "octave": 2}, 0.8, 1.0]
                    ]
                }
            }
        }
    })";
    
    MusicPatternSystem patternSystem;
    PatternParser parser(patternSystem);
    
    std::cout << "Testing chord tone resolution..." << std::endl;
    
    auto chart = parser.parseChart(testJson);
    if (!chart) {
        std::cout << "❌ Failed to parse chart" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Chart parsed successfully" << std::endl;
    std::cout << "   Title: " << chart->getTitle() << std::endl;
    std::cout << "   Tempo: " << chart->getTempo() << std::endl;
    std::cout << "   Sections: " << chart->getNumSections() << std::endl;
    
    if (chart->getNumSections() > 0) {
        auto section = chart->getSection(0);
        if (section && section->getNumPhrases() > 0) {
            auto phrase = section->getPhrase(0);
            if (phrase && phrase->getNumBars() > 0) {
                std::cout << "   Bars: " << phrase->getNumBars() << std::endl;
                
                // Check first bar
                auto bar = phrase->getBar(0);
                if (bar) {
                    auto sequence = bar->toMidiSequence();
                    std::cout << "   Bar 1 MIDI events: " << sequence->getNumEvents() << std::endl;
                    
                    // Look for chord tone notes
                    bool foundChordTones = false;
                    for (int i = 0; i < sequence->getNumEvents(); ++i) {
                        auto msg = sequence->getEventPointer(i)->message;
                        if (msg.isNoteOn()) {
                            int note = msg.getNoteNumber();
                            std::cout << "     Note-on: MIDI " << note << " (channel " << msg.getChannel() << ")" << std::endl;
                            
                            // Check if it's a valid chord tone (C=48, E=52, G=55, F=53, A=57, C=60)
                            if (note == 48 || note == 52 || note == 55 || note == 53 || note == 57 || note == 60) {
                                foundChordTones = true;
                            }
                        }
                    }
                    
                    if (foundChordTones) {
                        std::cout << "✅ Found chord tone resolution" << std::endl;
                    } else {
                        std::cout << "❌ No chord tones found" << std::endl;
                    }
                }
            }
        }
    }
    
    return 0;
}