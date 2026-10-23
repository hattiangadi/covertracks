#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

namespace covertracks {
namespace practice {

struct TargetNote {
    double startBeat = 0.0;      // quarter-note beats from lesson start
    double durationBeats = 0.0;
    int midiNote = 60;
    juce::String syllable;       // may be empty
};

struct PracticeLesson {
    juce::String title;
    double tempoBpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    double lengthBeats = 0.0;                 // end of last note-off across all tracks
    int keySignatureSharpsFlats = 0;          // from 0x59 meta if present, else 0
    std::vector<TargetNote> voiceNotes;       // sorted by startBeat
    juce::MidiMessageSequence accompaniment;  // piano only, timestamps in BEATS (quarter notes), channel preserved, note on/off matched

    double beatsPerBar() const
    {
        return timeSigNumerator * 4.0 / (double) timeSigDenominator;
    }

    int numBars() const
    {
        auto bpb = beatsPerBar();
        if (bpb <= 0.0)
            return 0;

        return (int) std::ceil (lengthBeats / bpb - 1.0e-9);
    }
};

} // namespace practice
} // namespace covertracks
