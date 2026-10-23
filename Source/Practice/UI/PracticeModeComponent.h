#pragma once
#include <JuceHeader.h>
#include <optional>
#include <memory>
#include <vector>
#include <atomic>
#include "PracticePianoRoll.h"
#include "../Core/PracticeLesson.h"
#include "../Core/AccompanimentPlayer.h"
#include "../Core/PitchTracker.h"
#include "../../Player/Core/PlayerManager.h"
#include "../../Audio/TracktionAudioEngine.h"

namespace covertracks {
namespace practice {

/** Formats a lesson-relative beat position for the status readout.
    Negative lessonBeats (inside the count-in) always formats as "Count-in". */
juce::String formatPosition (double lessonBeats, double beatsPerBar);

/** Walks up from startFrom (or its parent directory, if startFrom is a
    file) looking for a child directory named "Lessons", up to 8 levels.
    Returns a non-existent File if none is found. */
juce::File findLessonsDirectory (const juce::File& startFrom);

/** Recursively lists *.mid files under dir, sorted by full path. Returns an
    empty vector if dir does not exist. */
std::vector<juce::File> listLessonFiles (const juce::File& dir);

/** Converts one time-stamped pitch-tracker result into a piano-roll overlay
    point. The beat conversion uses the TAKE-START tempo (tempoBpm), not
    whatever the live tempo is at the time the sample arrives: if the user
    changes tempo mid-take, already-converted and new overlay points keep
    using the tempo that was in effect when the take started, so the trace
    doesn't warp retroactively. micLatencySeconds shifts the sample's
    timestamp earlier to compensate for input monitoring/round-trip latency
    before converting to a beat position. An unvoiced sample always reports
    confidence 0, regardless of the detector's raw confidence value. */
PitchPoint pitchSampleToPoint (const PitchSample& sample, double takeStartBeat, double tempoBpm, double micLatencySeconds);

/**
    Top-level UI for Practice mode: pick a lesson and a Player, control
    tempo/transpose/count-in/loop, play/stop, and watch progress on a
    PracticePianoRoll (including a live overlay of the sung pitch, captured
    from the microphone while a "take" - the span between pressing Play and
    the playback stopping - is active).
*/
class PracticeModeComponent : public juce::Component,
                               public TracktionAudioEngine::AudioInputListener,
                               private juce::Timer
{
public:
    explicit PracticeModeComponent (PlayerManager* playerManager);
    ~PracticeModeComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Loads the given Standard MIDI File as the current lesson, resetting
        tempo/status to match it. Public so it can be driven directly by
        tests and by the Open… file chooser callback. */
    void loadLessonFile (const juce::File& file);

    const std::optional<PracticeLesson>& getCurrentLesson() const { return currentLesson; }

    const juce::Slider& getTempoSlider() const { return tempoSlider; }
    const juce::ComboBox& getPlayerSelector() const { return playerSelector; }
    const juce::TextButton& getPlayButton() const { return playButton; }

    // Not in the original spec's accessor list, but needed for the status-label
    // and piano-roll-layout assertions the spec's own test descriptions call for;
    // added for testability (see final report).
    const juce::Label& getStatusLabel() const { return statusLabel; }
    const PracticePianoRoll& getPianoRoll() const { return pianoRoll; }

    const juce::ToggleButton& getMicToggle() const { return micToggle; }
    const std::vector<PitchPoint>& getCurrentTrace() const { return currentTrace; }

    /** TracktionAudioEngine::AudioInputListener. Called on the AUDIO THREAD:
        forwards raw input to the pitch tracker only while a take is active.
        Must stay lock-free/allocation-free. */
    void audioInputBlock (const float* const* inputChannels, int numInputChannels, int numSamples, double sampleRate) override;

    /** Test hook: applies the same conversion/append/push path the 30ms
        timer uses for live pitch-tracker output, without needing real
        audio. Message thread only. */
    void ingestPitchSamples (const std::vector<PitchSample>& samples);

private:
    void timerCallback() override;

    void refreshLessonSelector();
    void refreshPlayerSelector();
    void onLessonSelectorChanged();
    void onPlayerSelectorChanged();
    void onOpenButtonClicked();
    void onTempoChanged();
    void onTempoResetClicked();
    void onTransposeChanged();
    void onCountInChanged();
    void onLoopToggled();
    void onLoopRangeChanged();
    void onPlayButtonClicked();
    void onRewindButtonClicked();
    void onPlaybackFinished();

    void recreateAccompanimentPlayerForSelectedPlayer();
    void applyLessonToPlayerAndRoll();
    void updateStatusLabel();
    void updatePlayButtonText();
    int currentCountInBars() const;
    Player* findSelectedPlayer() const;
    juce::File getLessonsDirectory() const;
    void rememberLessonsDirectory (const juce::File& dir);

    void onMicToggleChanged();
    void updateMicListenerRegistration();
    void startTake();
    void endTake();
    double getMicLatencySeconds() const;

    PlayerManager* playerManager = nullptr;

    juce::ComboBox lessonSelector;
    juce::TextButton openButton { "Open..." };

    juce::ComboBox playerSelector;

    juce::Slider tempoSlider;
    juce::TextButton tempoResetButton { "Lesson tempo" };

    juce::Slider transposeSlider;
    juce::Label transposeLabel;

    juce::ComboBox countInSelector;

    juce::ToggleButton loopToggle { "Loop bars" };
    juce::Slider loopStartBar;
    juce::Slider loopEndBar;

    juce::TextButton playButton { "Play" };
    juce::TextButton rewindButton { juce::String::fromUTF8 ("\xe2\x8f\xae") };

    juce::Label statusLabel;

    PracticePianoRoll pianoRoll;

    juce::ToggleButton micToggle { "Listen to mic" };
    juce::Label pitchReadout;

    std::optional<PracticeLesson> currentLesson;
    std::unique_ptr<AccompanimentPlayer> accompanimentPlayer;

    // Pitch tracking (step 4): pitchTracker is created/recreated on the
    // message thread when a take starts, sized to the engine's current
    // sample rate. takeActive gates audioInputBlock() (audio thread) from
    // touching it; it is only flipped to true once the tracker is fully
    // (re)built and reset, and is flipped back to false before any rebuild,
    // so the audio thread never sees a half-constructed tracker.
    std::unique_ptr<PitchTracker> pitchTracker;
    std::atomic<bool> takeActive { false };
    double takeStartBeat = 0.0;
    double takeStartTempoBpm = 120.0; // tempo at take start; intentionally NOT re-read mid-take (see pitchSampleToPoint)
    std::vector<PitchPoint> currentTrace;

    std::vector<juce::File> lessonFiles;
    juce::File lastOpenDirectory;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PracticeModeComponent)
};

} // namespace practice
} // namespace covertracks
