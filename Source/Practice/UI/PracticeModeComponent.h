#pragma once
#include <JuceHeader.h>
#include <optional>
#include <memory>
#include <vector>
#include "PracticePianoRoll.h"
#include "../Core/PracticeLesson.h"
#include "../Core/AccompanimentPlayer.h"
#include "../../Player/Core/PlayerManager.h"

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

/**
    Top-level UI for Practice mode: pick a lesson and a Player, control
    tempo/transpose/count-in/loop, play/stop, and watch progress on a
    PracticePianoRoll.
*/
class PracticeModeComponent : public juce::Component,
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

    std::optional<PracticeLesson> currentLesson;
    std::unique_ptr<AccompanimentPlayer> accompanimentPlayer;

    std::vector<juce::File> lessonFiles;
    juce::File lastOpenDirectory;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PracticeModeComponent)
};

} // namespace practice
} // namespace covertracks
