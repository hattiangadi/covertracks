#pragma once
#include <JuceHeader.h>
#include <optional>
#include "PracticeLesson.h"

namespace covertracks {
namespace practice {

/**
    Pure, UI-free loader that turns a Standard MIDI File (voice + piano) into a
    PracticeLesson: the sung target notes (with syllables), the piano
    accompaniment as a juce::MidiMessageSequence, tempo/meter metadata, plus
    pure transformation helpers (transpose, slice a bar range).
*/
class LessonLoader
{
public:
    struct Options
    {
        Options() noexcept : voiceTrackOverride (-1) {}
        int voiceTrackOverride;   // -1 = auto-detect
    };

    static std::optional<PracticeLesson> loadFromFile (const juce::File& file, Options options = {});
    static std::optional<PracticeLesson> loadFromStream (juce::InputStream& stream, Options options = {});

    // Pure transformations (return new objects, do not mutate input):

    /** Shifts voiceNotes and accompaniment note on/off/aftertouch by semitones.
        Skips channel 10 (percussion) and clamps resulting note numbers to 0..127. */
    static PracticeLesson transposed (const PracticeLesson& lesson, int semitones);

    /** 0-based bars, [firstBar, lastBarInclusive]. Re-times so firstBar starts at
        beat 0. Notes starting outside the range are dropped; notes that start
        inside but end after the range are truncated to the slice end.
        lengthBeats of the result is (number of bars) * beatsPerBar. */
    static PracticeLesson sliceBars (const PracticeLesson& lesson, int firstBar, int lastBarInclusive);

    /** Exposed for testing; returns -1 if no voice track can be detected. */
    static int detectVoiceTrack (const juce::MidiFile& midiFile);

private:
    static std::optional<PracticeLesson> loadFromMidiFile (juce::MidiFile& midiFile, Options options, const juce::String& titleFromFileName);
};

} // namespace practice
} // namespace covertracks
