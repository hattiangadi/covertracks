#include "PracticePianoRoll.h"

namespace covertracks {
namespace practice {

namespace
{
    bool isWhiteKey (int midiNote)
    {
        static const bool whiteKeys[12] = { true, false, true, false, true, true, false, true, false, true, false, true };
        return whiteKeys[((midiNote % 12) + 12) % 12];
    }

    bool isC (int midiNote)
    {
        return ((midiNote % 12) + 12) % 12 == 0;
    }

    juce::String noteNameForC (int midiNote)
    {
        // midi 60 == C4 (middle C)
        int octave = (midiNote / 12) - 1;
        return "C" + juce::String (octave);
    }
}

PracticePianoRoll::PracticePianoRoll()
{
    setOpaque (true);
}

PracticePianoRoll::~PracticePianoRoll() = default;

void PracticePianoRoll::syncLayoutSizeToBounds()
{
    if (getWidth() > 0)
        layout.widthPx = (float) getWidth();

    if (getHeight() > 0)
        layout.heightPx = (float) getHeight();
}

void PracticePianoRoll::setLesson (const PracticeLesson& newLesson, int newCountInBars)
{
    lesson = newLesson;
    countInBars = newCountInBars;

    syncLayoutSizeToBounds();

    auto fitted = PianoRollLayout::fitToNotes (newLesson.voiceNotes, newLesson.lengthBeats);
    layout.lowMidi = fitted.lowMidi;
    layout.highMidi = fitted.highMidi;

    auto beatsPerBar = newLesson.beatsPerBar();
    layout.firstVisibleBeat = (countInBars > 0 && beatsPerBar > 0.0) ? -(countInBars * beatsPerBar) : 0.0;

    repaint();
}

void PracticePianoRoll::clearLesson()
{
    lesson.reset();
    countInBars = 0;
    repaint();
}

void PracticePianoRoll::setPlayheadBeat (double lessonBeats)
{
    playheadBeat = lessonBeats;
    scrollToKeepPlayheadVisible();
    repaint();
}

void PracticePianoRoll::setPitchTrace (std::vector<PitchPoint> trace)
{
    pitchTrace = std::move (trace);
    repaint();
}

void PracticePianoRoll::clearPitchTrace()
{
    pitchTrace.clear();
    repaint();
}

void PracticePianoRoll::setLoopBars (int firstBar, int lastBarInclusive)
{
    loopFirstBar = firstBar;
    loopLastBarInclusive = lastBarInclusive;
    loopIsSet = true;
    repaint();
}

void PracticePianoRoll::clearLoop()
{
    loopIsSet = false;
    repaint();
}

void PracticePianoRoll::followPlayhead (bool shouldFollow)
{
    followEnabled = shouldFollow;
}

void PracticePianoRoll::scrollToKeepPlayheadVisible()
{
    if (! followEnabled || playheadBeat < -1.0e8)
        return;

    auto margin = layout.beatsVisible * 0.2; // middle 60%
    auto lowBound = layout.firstVisibleBeat + margin;
    auto highBound = layout.firstVisibleBeat + layout.beatsVisible - margin;

    if (playheadBeat < lowBound)
        layout.firstVisibleBeat = playheadBeat - margin;
    else if (playheadBeat > highBound)
        layout.firstVisibleBeat = playheadBeat - layout.beatsVisible + margin;
}

void PracticePianoRoll::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.brighter (0.05f));

    auto bounds = getLocalBounds().toFloat();
    auto keyboardArea = bounds.withWidth (layout.keyboardWidthPx);
    auto headerArea = bounds.withHeight (layout.headerHeightPx);

    // --- Alternating row shading + keyboard strip -----------------------
    for (int midi = layout.lowMidi; midi <= layout.highMidi; ++midi)
    {
        auto top = layout.midiToY ((double) midi + 0.5);
        auto rowRect = juce::Rectangle<float> (layout.keyboardWidthPx, top, bounds.getWidth() - layout.keyboardWidthPx, layout.rowHeight());

        g.setColour (isWhiteKey (midi) ? juce::Colours::black.brighter (0.08f) : juce::Colours::black.brighter (0.14f));
        g.fillRect (rowRect);

        auto keyRect = juce::Rectangle<float> (0.0f, top, layout.keyboardWidthPx, layout.rowHeight());
        g.setColour (isWhiteKey (midi) ? juce::Colours::whitesmoke : juce::Colours::darkgrey);
        g.fillRect (keyRect);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRect (keyRect, 0.5f);

        if (isC (midi))
        {
            g.setColour (juce::Colours::black);
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
            g.drawText (noteNameForC (midi), keyRect.reduced (2.0f, 0.0f), juce::Justification::centredLeft);
        }
    }

    juce::ignoreUnused (keyboardArea);

    // --- Loop region overlay ---------------------------------------------
    if (loopIsSet && lesson.has_value())
    {
        auto bpb = lesson->beatsPerBar();
        if (bpb > 0.0)
        {
            auto startBeat = loopFirstBar * bpb;
            auto endBeat = (loopLastBarInclusive + 1) * bpb;
            auto x1 = layout.beatToX (startBeat);
            auto x2 = layout.beatToX (endBeat);
            g.setColour (juce::Colours::yellow.withAlpha (0.12f));
            g.fillRect (juce::Rectangle<float> (x1, layout.headerHeightPx, x2 - x1, bounds.getHeight() - layout.headerHeightPx));
        }
    }

    // --- Bar / beat grid lines + header bar numbers -----------------------
    if (lesson.has_value())
    {
        auto bpb = lesson->beatsPerBar();
        if (bpb > 0.0)
        {
            auto firstBeat = layout.firstVisibleBeat;
            auto lastBeat = layout.firstVisibleBeat + layout.beatsVisible;

            // Snap down to the nearest beat line at/before firstBeat.
            double beat = std::floor (firstBeat);
            for (; beat <= lastBeat + 1.0; beat += 1.0)
            {
                auto x = layout.beatToX (beat);
                if (x < layout.keyboardWidthPx - 1.0f || x > bounds.getWidth() + 1.0f)
                    continue;

                bool isBarLine = std::abs (std::fmod (beat, bpb)) < 1.0e-6 || std::abs (std::fmod (beat, bpb) - bpb) < 1.0e-6;
                g.setColour (isBarLine ? juce::Colours::grey : juce::Colours::darkgrey.withAlpha (0.6f));
                g.drawLine (x, layout.headerHeightPx, x, bounds.getHeight(), isBarLine ? 1.2f : 0.5f);

                if (isBarLine)
                {
                    int barIndex = (int) std::round (beat / bpb); // 0-based, can be negative (count-in)
                    juce::String label = (barIndex < 0) ? juce::String ("count-in") : juce::String (barIndex + 1);

                    auto nextX = layout.beatToX (beat + bpb);
                    auto labelRect = juce::Rectangle<float> (x + 2.0f, 0.0f, juce::jmax (10.0f, nextX - x - 4.0f), layout.headerHeightPx);
                    g.setColour (juce::Colours::lightgrey);
                    g.setFont (juce::Font (juce::FontOptions (11.0f)));
                    g.drawText (label, labelRect, juce::Justification::centredLeft);
                }
            }
        }
    }

    juce::ignoreUnused (headerArea);

    // --- Target notes -------------------------------------------------------
    if (lesson.has_value())
    {
        for (auto& note : lesson->voiceNotes)
        {
            auto x1 = layout.beatToX (note.startBeat);
            auto x2 = layout.beatToX (note.startBeat + note.durationBeats);
            if (x2 < layout.keyboardWidthPx || x1 > bounds.getWidth())
                continue;

            auto y = layout.midiToY ((double) note.midiNote + 0.5f);
            auto h = juce::jmax (4.0f, layout.rowHeight() - 2.0f);
            juce::Rectangle<float> noteRect (x1, y - h * 0.5f, juce::jmax (2.0f, x2 - x1), h);

            g.setColour (juce::Colours::orange.withAlpha (0.85f));
            g.fillRoundedRectangle (noteRect, 3.0f);
            g.setColour (juce::Colours::orange.darker (0.6f));
            g.drawRoundedRectangle (noteRect, 3.0f, 1.0f);

            if (note.syllable.isNotEmpty() && noteRect.getWidth() > 14.0f)
            {
                g.setColour (juce::Colours::black);
                g.setFont (juce::Font (juce::FontOptions (11.0f)));
                g.drawText (note.syllable, noteRect.reduced (2.0f, 0.0f), juce::Justification::centred, true);
            }
        }
    }

    // --- Pitch trace --------------------------------------------------------
    if (pitchTrace.size() > 1)
    {
        juce::Path path;
        bool started = false;

        for (auto& point : pitchTrace)
        {
            if (point.confidence < 0.5f)
            {
                started = false;
                continue;
            }

            auto x = layout.beatToX (point.beat);
            auto y = layout.midiToY (point.midiPitch);

            if (! started)
            {
                path.startNewSubPath (x, y);
                started = true;
            }
            else
            {
                path.lineTo (x, y);
            }
        }

        g.setColour (juce::Colours::cyan);
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

    // --- Playhead -------------------------------------------------------
    if (playheadBeat > -1.0e8)
    {
        auto x = layout.beatToX (playheadBeat);
        if (x >= layout.keyboardWidthPx && x <= bounds.getWidth())
        {
            g.setColour (juce::Colours::red);
            g.drawLine (x, 0.0f, x, bounds.getHeight(), 1.5f);
        }
    }
}

void PracticePianoRoll::resized()
{
    syncLayoutSizeToBounds();
}

void PracticePianoRoll::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    auto delta = (wheel.deltaX != 0.0f) ? wheel.deltaX : wheel.deltaY;
    layout.firstVisibleBeat -= delta * (layout.beatsVisible * 0.25);
    repaint();
}

} // namespace practice
} // namespace covertracks
