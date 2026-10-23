// PracticeUITests.cpp — TDD tests for Practice mode UI
// (Source/Practice/UI/PracticePianoRoll.h, Source/Practice/UI/PracticeModeComponent.h)
//
// Covers the pure PianoRollLayout mapping math, the pure free helper
// functions (formatPosition, findLessonsDirectory, listLessonFiles), a light
// smoke test of PracticePianoRoll's state + paint, and construction of
// PracticeModeComponent with zero players and with a real "Piano" player.

#include <JuceHeader.h>
#include <memory>
#include "../Practice/UI/PracticePianoRoll.h"
#include "../Practice/UI/PracticeModeComponent.h"
#include "../Practice/Core/LessonLoader.h"
#include "../Practice/Core/PracticeLesson.h"
#include "../Player/Core/Player.h"
#include "../Player/Core/PlayerManager.h"
#include "../Audio/AudioEngineInterface.h"

using namespace covertracks::practice;
using covertracks::Player;
using covertracks::PlayerManager;
using covertracks::TracktionAudioEngine;

namespace
{
    juce::File getFixtureFile()
    {
        return juce::File (__FILE__)
            .getParentDirectory()   // Source/Tests
            .getParentDirectory()   // Source
            .getParentDirectory()   // repo root
            .getChildFile ("Lessons/Vaccai/Baritone/01-LessonI.mid");
    }

    std::optional<PracticeLesson> loadFixtureLesson()
    {
        return LessonLoader::loadFromFile (getFixtureFile());
    }
}

// =============================================================================
// Test 1: PianoRollLayout beat/x and midi/y mapping
// =============================================================================
class PianoRollLayout_Mapping : public juce::UnitTest
{
public:
    PianoRollLayout_Mapping() : UnitTest ("PianoRollLayout: beat/x and midi/y mapping are inverse and monotonic", "Practice Mode") {}
    void runTest() override
    {
        PianoRollLayout layout;
        layout.widthPx = 800.f;
        layout.keyboardWidthPx = 44.f;
        layout.firstVisibleBeat = -4.0;
        layout.beatsVisible = 16.0;

        beginTest ("beatToX endpoints");
        expectWithinAbsoluteError (layout.beatToX (-4.0), 44.0f, 0.01f);
        expectWithinAbsoluteError (layout.beatToX (12.0), 800.0f, 0.01f);

        beginTest ("xToBeat inverts beatToX");
        auto x = layout.beatToX (3.5);
        expectWithinAbsoluteError (layout.xToBeat (x), 3.5, 0.01);

        beginTest ("midiToY is monotonic (low pitch -> bottom, i.e. larger y)");
        expect (layout.midiToY ((double) layout.lowMidi) > layout.midiToY ((double) layout.highMidi));

        beginTest ("midiToY interpolates fractional pitches");
        auto yLow = layout.midiToY (60.0);
        auto yHigh = layout.midiToY (61.0);
        auto yMid = layout.midiToY (60.5);
        expectWithinAbsoluteError (yMid, (yLow + yHigh) * 0.5f, 0.01f);

        beginTest ("rowHeight formula");
        auto expectedRowHeight = (layout.heightPx - layout.headerHeightPx) / (float) (layout.highMidi - layout.lowMidi + 1);
        expectWithinAbsoluteError (layout.rowHeight(), expectedRowHeight, 0.001f);
    }
};
static PianoRollLayout_Mapping test_pianoRollLayoutMapping;

// =============================================================================
// Test 2: PianoRollLayout::fitToNotes
// =============================================================================
class PianoRollLayout_FitToNotes : public juce::UnitTest
{
public:
    PianoRollLayout_FitToNotes() : UnitTest ("PianoRollLayout: fitToNotes brackets the voice range", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Lesson I voice notes (57..73), padding 3");
        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value(), "Fixture lesson should load");
        if (! lessonOpt.has_value())
            return;

        auto layout = PianoRollLayout::fitToNotes (lessonOpt->voiceNotes, lessonOpt->lengthBeats, 3);
        expect (layout.lowMidi <= 54, "lowMidi should be <= 54");
        expect (layout.highMidi >= 76, "highMidi should be >= 76");
        expect (layout.highMidi - layout.lowMidi >= 24, "span should be at least two octaves");

        beginTest ("Empty note list yields defaults");
        std::vector<TargetNote> empty;
        auto defaultLayout = PianoRollLayout::fitToNotes (empty, 0.0, 3);
        expectEquals (defaultLayout.lowMidi, 48);
        expectEquals (defaultLayout.highMidi, 84);
    }
};
static PianoRollLayout_FitToNotes test_pianoRollLayoutFitToNotes;

// =============================================================================
// Test 3: formatPosition
// =============================================================================
class PracticeMode_FormatPosition : public juce::UnitTest
{
public:
    PracticeMode_FormatPosition() : UnitTest ("PracticeMode: formatPosition", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Negative beats -> Count-in");
        expectEquals (formatPosition (-2.0, 4.0), juce::String ("Count-in"));

        beginTest ("Beat 0 -> Bar 1, beat 1");
        expectEquals (formatPosition (0.0, 4.0), juce::String::fromUTF8 ("Bar 1 \xc2\xb7 beat 1"));

        beginTest ("Beat 5.9 (4/4) -> Bar 2, beat 2");
        expectEquals (formatPosition (5.9, 4.0), juce::String::fromUTF8 ("Bar 2 \xc2\xb7 beat 2"));

        beginTest ("Beat 16 (4/4) -> Bar 5, beat 1");
        expectEquals (formatPosition (16.0, 4.0), juce::String::fromUTF8 ("Bar 5 \xc2\xb7 beat 1"));

        beginTest ("Beat 7.0 (3/4) -> Bar 3, beat 2");
        expectEquals (formatPosition (7.0, 3.0), juce::String::fromUTF8 ("Bar 3 \xc2\xb7 beat 2"));
    }
};
static PracticeMode_FormatPosition test_practiceModeFormatPosition;

// =============================================================================
// Test 4: findLessonsDirectory and listLessonFiles
// =============================================================================
class PracticeMode_LessonsDirectory : public juce::UnitTest
{
public:
    PracticeMode_LessonsDirectory() : UnitTest ("PracticeMode: findLessonsDirectory and listLessonFiles", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("findLessonsDirectory from Source/Tests finds the repo's Lessons dir");
        auto dir = findLessonsDirectory (juce::File (__FILE__));
        expect (dir.isDirectory(), "Lessons directory should be found");
        expect (dir.getChildFile ("Vaccai/Baritone/01-LessonI.mid").existsAsFile(),
                "Lessons dir should contain the Vaccai fixture");

        beginTest ("listLessonFiles finds the fixture");
        auto files = listLessonFiles (dir);
        expect (files.size() >= 1, "Should find at least one .mid file");
        bool foundFixture = false;
        for (auto& f : files)
            if (f.getFullPathName().endsWith ("01-LessonI.mid"))
                foundFixture = true;
        expect (foundFixture, "Should find 01-LessonI.mid among the listed files");

        beginTest ("findLessonsDirectory from a directory with no Lessons child returns non-existent");
        auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory);
        auto notFound = findLessonsDirectory (tempDir);
        expect (! notFound.exists(), "Should return a non-existent File when no Lessons dir is found");
    }
};
static PracticeMode_LessonsDirectory test_practiceModeLessonsDirectory;

// =============================================================================
// Test 5: PracticePianoRoll state + paint smoke test
// =============================================================================
class PracticePianoRoll_StateAndPaint : public juce::UnitTest
{
public:
    PracticePianoRoll_StateAndPaint() : UnitTest ("PracticePianoRoll: setLesson adjusts layout and setPitchTrace stores points", "Practice Mode") {}
    void runTest() override
    {
        auto lessonOpt = loadFixtureLesson();
        beginTest ("Fixture loads");
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
            return;

        PracticePianoRoll roll;
        roll.setBounds (0, 0, 800, 400);

        beginTest ("setLesson adjusts layout");
        roll.setLesson (*lessonOpt, 1);
        expectWithinAbsoluteError (roll.getLayout().firstVisibleBeat, -4.0, 1.0e-6);
        expect (roll.getLayout().lowMidi <= 54, "lowMidi should bracket the voice range");

        beginTest ("setPitchTrace stores points, clearPitchTrace empties it");
        std::vector<PitchPoint> trace { { 0.0, 57.2, 0.9f }, { 0.5, 57.1, 0.2f } };
        roll.setPitchTrace (trace);
        expectEquals (roll.getPitchTraceSize(), 2);
        roll.clearPitchTrace();
        expectEquals (roll.getPitchTraceSize(), 0);

        beginTest ("paint does not crash with lesson + playhead + loop + trace set");
        roll.setPitchTrace (trace);
        roll.setPlayheadBeat (2.0);
        roll.setLoopBars (0, 1);

        juce::Image image (juce::Image::ARGB, 800, 400, true);
        juce::Graphics g (image);
        roll.paint (g);
        expect (true, "paint() should complete without crashing");
    }
};
static PracticePianoRoll_StateAndPaint test_practicePianoRollStateAndPaint;

// =============================================================================
// Test 6: PracticeModeComponent construction
// =============================================================================
class PracticeModeComponent_Construction : public juce::UnitTest
{
public:
    PracticeModeComponent_Construction() : UnitTest ("PracticeModeComponent: constructs with no players and with a Piano player", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("(a) Zero players: constructs without crashing, play button disabled");
        {
            PlayerManager emptyManager;
            // Note: PlayerManager's default constructor loads any players already
            // saved to disk (see loadAllPlayers()), so "zero players" isn't
            // guaranteed on a developer machine that has saved real players.
            // We only assert the component survives construction either way,
            // and that the play button is disabled specifically when there
            // really are no players.
            PracticeModeComponent component (&emptyManager);
            if (emptyManager.getPlayers().isEmpty())
                expect (! component.getPlayButton().isEnabled(), "Play button should be disabled with no players");
        }

        beginTest ("(b) PlayerManager with a Piano player");
        {
            PlayerManager manager;

            auto& engine = TracktionAudioEngine::getInstance();
            auto player = std::make_unique<Player> ("practice_ui_test_piano", "Piano",
                                                      engine.getFormatManager(), engine.getKnownPluginList());
            expect (player->initialize(), "Test Piano player should initialize");
            player->setName ("Piano");
            manager.addPlayer (std::move (player));

            PracticeModeComponent component (&manager);

            beginTest ("playerSelector defaults to Piano");
            expectEquals (component.getPlayerSelector().getText(), juce::String ("Piano"));

            beginTest ("loadLessonFile loads Lesson I");
            component.loadLessonFile (getFixtureFile());
            expect (component.getCurrentLesson().has_value(), "Lesson should load");
            if (component.getCurrentLesson().has_value())
            {
                expectWithinAbsoluteError (component.getTempoSlider().getValue(), 75.0, 0.01);

                beginTest ("status label mentions the lesson title or file name");
                auto statusText = component.getStatusLabel().getText();
                expect (statusText.contains ("Lesson I") || statusText.contains ("01-LessonI"),
                        "Status label should mention the lesson: " + statusText);

                beginTest ("piano roll layout brackets the voice range");
                expect (component.getPianoRoll().getLayout().lowMidi <= 54,
                        "Piano roll lowMidi should be <= 54");
            }
        }
    }
};
static PracticeModeComponent_Construction test_practiceModeComponentConstruction;
