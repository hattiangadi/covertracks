#pragma once
#include <JuceHeader.h>
#include "PracticeLesson.h"
#include "../../Player/Core/Player.h"

// Include Tracktion Engine headers
#include "../../../modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"
namespace te = tracktion;

namespace covertracks {
namespace practice {

/**
    Plays a PracticeLesson's piano accompaniment through a real te::MidiClip
    on the given Piano Player's Tracktion track, driven by the shared Edit
    transport for sample-accurate timing (as opposed to the message-thread
    Timer used by Instrument::playMidiSequence, which drifts).

    AccompanimentPlayer owns exactly one te::MidiClip on the Player's track
    and never touches any other clip already on that track.
*/
class AccompanimentPlayer : private juce::Timer
{
public:
    /** Ensures pianoPlayer.isInitialized(), calling initialize() if needed,
        then caches the Edit and the Player's Tracktion track. */
    explicit AccompanimentPlayer (Player& pianoPlayer);

    /** Removes its clip (if any) and stops the timer. Does NOT stop the
        transport - something else may be using it. */
    ~AccompanimentPlayer() override;

    /** Rebuilds the clip from this lesson (tempo, time sig, notes, count-in,
        loop range). Returns false if the Player has no track/Edit. */
    bool load (const PracticeLesson& lesson);

    /** Removes the clip, if any. */
    void unload();

    bool isLoaded() const { return loaded; }

    /** Changes the Edit tempo and rebuilds the clip. The clip's length in
        BEATS is unchanged; only its length in time changes. */
    void setTempoBpm (double bpm);
    double getTempoBpm() const { return tempoBpm; }

    /** Rebuilds with LessonLoader::transposed. */
    void setTranspose (int semitones);
    int getTranspose() const { return transposeSemitones; }

    /** Shifts the clip (and loop range) right by bars * beatsPerBar.
        Position 0 corresponds to the start of the count-in. */
    void setCountInBars (int bars);
    int getCountInBars() const { return countInBars; }

    /** firstBar/lastBarInclusive are 0-based LESSON bars (ignoring any
        count-in). Sets transport.setLoopRange and transport.looping = true. */
    void setLoopBars (int firstBar, int lastBarInclusive);
    void clearLoop();
    bool isLooping() const;

    /** Plays from the current transport position. Call setPositionBeats
        first to restart from a particular point. */
    void play();
    void stop();
    bool isPlaying() const;

    /** lessonBeats is relative to the start of the lesson (beat 0); negative
        values are inside the count-in. */
    void setPositionBeats (double lessonBeats);

    /** Current transport position in lesson beats; negative during the
        count-in. */
    double getPositionBeats() const;

    double getLessonLengthBeats() const { return lesson.lengthBeats; }

    /** Fired on the message thread when playback position passes the end of
        the lesson while not looping. */
    std::function<void()> onPlaybackFinished;

    /** For tests. */
    te::MidiClip* getClip() const { return loadedClip.get(); }

private:
    void timerCallback() override;
    void rebuild();
    void updateLoopRange();
    double countInBeats() const;

    Player& pianoPlayer;
    te::Edit* edit = nullptr;
    te::AudioTrack* track = nullptr;

    te::MidiClip::Ptr loadedClip;
    bool loaded = false;

    PracticeLesson lesson;   // as loaded, untransposed
    double tempoBpm = 120.0;
    int transposeSemitones = 0;
    int countInBars = 0;

    bool loopSet = false;
    int loopFirstBar = 0;
    int loopLastBarInclusive = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AccompanimentPlayer)
};

} // namespace practice
} // namespace covertracks
