#pragma once
#include <JuceHeader.h>
#include <vector>
#include <optional>
#include "../Core/PracticeLesson.h"

namespace covertracks {
namespace practice {

/** A single point of the sung-pitch trace overlay (step 4 of Practice mode
    feeds this from pitch detection). midiPitch is fractional - 69.0 == A4. */
struct PitchPoint
{
    double beat = 0.0;
    double midiPitch = 60.0;
    float confidence = 0.0f;
};

/** Pure, JUCE-Component-free layout model for the practice piano roll.
    Converts between (beat, MIDI pitch) musical coordinates and pixel
    coordinates for a given viewport. */
struct PianoRollLayout
{
    double firstVisibleBeat = -4.0;  // allow negative for count-in
    double beatsVisible = 16.0;
    int lowMidi = 48, highMidi = 84; // inclusive vertical range
    float keyboardWidthPx = 44.f, headerHeightPx = 18.f;
    float widthPx = 800.f, heightPx = 400.f;

    /** X pixel position (left edge of the beat-grid area, i.e. including the
        keyboard strip offset) for a given beat. */
    float beatToX (double beat) const
    {
        if (beatsVisible <= 0.0)
            return keyboardWidthPx;

        auto frac = (beat - firstVisibleBeat) / beatsVisible;
        return keyboardWidthPx + (float) (frac * (double) (widthPx - keyboardWidthPx));
    }

    /** Inverse of beatToX. */
    double xToBeat (float x) const
    {
        auto gridWidth = (double) (widthPx - keyboardWidthPx);
        if (gridWidth <= 0.0)
            return firstVisibleBeat;

        auto frac = ((double) x - (double) keyboardWidthPx) / gridWidth;
        return firstVisibleBeat + frac * beatsVisible;
    }

    /** Height in pixels of a single semitone row. */
    float rowHeight() const
    {
        auto numRows = highMidi - lowMidi + 1;
        if (numRows <= 0)
            return 0.f;

        return (heightPx - headerHeightPx) / (float) numRows;
    }

    /** Y pixel position of the centre line of the given (possibly
        fractional) MIDI pitch. Higher pitches are nearer the top. */
    float midiToY (double midi) const
    {
        auto rh = rowHeight();
        auto rowsFromTop = (double) highMidi - midi;
        return headerHeightPx + (float) (rowsFromTop * (double) rh) + rh * 0.5f;
    }

    /** Builds a layout whose lowMidi/highMidi bracket the given notes' pitch
        range (plus paddingSemitones on each side), with a floor of a
        two-octave (24 semitone) span. An empty note list keeps the defaults. */
    static PianoRollLayout fitToNotes (const std::vector<TargetNote>& notes, double lengthBeats, int paddingSemitones = 3)
    {
        PianoRollLayout layout;
        juce::ignoreUnused (lengthBeats);

        if (notes.empty())
            return layout;

        int minNote = notes.front().midiNote;
        int maxNote = notes.front().midiNote;
        for (auto& n : notes)
        {
            minNote = juce::jmin (minNote, n.midiNote);
            maxNote = juce::jmax (maxNote, n.midiNote);
        }

        int lo = minNote - paddingSemitones;
        int hi = maxNote + paddingSemitones;

        const int minSpan = 24; // two octaves
        int span = hi - lo;
        if (span < minSpan)
        {
            int extra = minSpan - span;
            int extraLow = extra / 2;
            int extraHigh = extra - extraLow;
            lo -= extraLow;
            hi += extraHigh;
        }

        layout.lowMidi = lo;
        layout.highMidi = hi;
        return layout;
    }
};

/** Pure, testable Component that draws a practice-mode piano roll: the
    lesson's target voice notes with syllables, bar/beat grid, a playhead,
    an optional loop-region overlay, and an optional sung-pitch trace. */
class PracticePianoRoll : public juce::Component
{
public:
    PracticePianoRoll();
    ~PracticePianoRoll() override;

    /** Loads a lesson's voice notes into the roll and sizes the vertical
        range and count-in offset accordingly. */
    void setLesson (const PracticeLesson& lesson, int countInBars);
    void clearLesson();

    /** Lesson-relative beat position of the playhead; pass -1.0e9 (or any
        very negative value) to hide it. */
    void setPlayheadBeat (double lessonBeats);

    void setPitchTrace (std::vector<PitchPoint> trace);
    void clearPitchTrace();
    int getPitchTraceSize() const { return (int) pitchTrace.size(); }

    /** 0-based lesson bars, inclusive. */
    void setLoopBars (int firstBar, int lastBarInclusive);
    void clearLoop();

    const PianoRollLayout& getLayout() const { return layout; }

    /** When enabled, the view auto-scrolls horizontally so the playhead
        stays within the middle 60% of the visible beats. */
    void followPlayhead (bool shouldFollow);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    void syncLayoutSizeToBounds();
    void scrollToKeepPlayheadVisible();

    PianoRollLayout layout;
    std::optional<PracticeLesson> lesson;
    int countInBars = 0;

    double playheadBeat = -1.0e9;
    std::vector<PitchPoint> pitchTrace;

    bool loopIsSet = false;
    int loopFirstBar = 0;
    int loopLastBarInclusive = 0;

    bool followEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PracticePianoRoll)
};

} // namespace practice
} // namespace covertracks
