// AccompanimentPlayerTests.cpp — TDD tests for Practice mode accompaniment
// playback (Source/Practice/Core/AccompanimentPlayer.h).
//
// Covers playing a PracticeLesson's piano accompaniment through a real
// te::MidiClip on a Piano Player's Tracktion track: tempo, transpose,
// count-in, bar-range looping and transport play/stop, all driven by the
// shared Edit transport for sample-accurate timing.

#include <JuceHeader.h>
#include <memory>
#include "../Practice/Core/AccompanimentPlayer.h"
#include "../Practice/Core/LessonLoader.h"
#include "../Practice/Core/PracticeLesson.h"
#include "../Player/Core/Player.h"
#include "../Audio/AudioEngineInterface.h"

using namespace covertracks::practice;
using covertracks::Player;
using covertracks::TracktionAudioEngine;

// =============================================================================
// Shared helpers
// =============================================================================
//
// NOTE ON DEVIATION FROM SPEC: the spec for this task describes checking
// TracktionAudioEngine::getInstance().getPlayerTrack(player) to confirm a
// Player has a Tracktion track. Investigation showed that method reads from
// an internal playerTracks map that is only populated by
// TracktionAudioEngine::registerPlayer(Player*) - and nothing in the
// codebase (Player::initialize(), PlayerManager, MainHostWindow) ever calls
// registerPlayer(). The actual track a Player's audio goes through is
// created directly by Instrument::registerWithAudioEngine() and exposed via
// Player::getInstrument()->getTrack(). getPlayerTrack() is therefore dead
// code for any Player built the normal way; using it as a gate here would
// make every test below silently no-op (expect() doesn't throw, so a
// "return early" on a nullptr would just skip all assertions). We check
// getInstrument()->getTrack() instead, which is the track the Instrument -
// and therefore AccompanimentPlayer - actually uses. See final report.

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

    struct TestPlayerHandle
    {
        std::unique_ptr<Player> player;
        te::AudioTrack* track = nullptr;

        bool isUsable() const { return player != nullptr && track != nullptr; }
    };

    TestPlayerHandle makeInitializedPlayer (juce::UnitTest& test, const juce::String& id)
    {
        TestPlayerHandle handle;

        auto& engine = TracktionAudioEngine::getInstance();
        handle.player = std::make_unique<Player> (id, "Accompaniment Test Piano " + id,
                                                    engine.getFormatManager(), engine.getKnownPluginList());

        test.expect (handle.player->initialize(), "Player should initialize successfully");
        if (! handle.player->isInitialized())
            return handle;

        auto* instrument = handle.player->getInstrument();
        test.expect (instrument != nullptr, "Player should have an Instrument");
        if (instrument == nullptr)
            return handle;

        handle.track = dynamic_cast<te::AudioTrack*> (instrument->getTrack());
        test.expect (handle.track != nullptr, "Player's Instrument should have a Tracktion track after initialize()");

        return handle;
    }

    int countAccompanimentClips (te::AudioTrack* track)
    {
        int count = 0;
        for (auto* c : track->getClips())
            if (c != nullptr && c->getName() == "Practice Accompaniment")
                ++count;
        return count;
    }

    int minNoteNumber (te::MidiClip* clip)
    {
        int minNote = 1000;
        for (auto* note : clip->getSequence().getNotes())
            minNote = juce::jmin (minNote, note->getNoteNumber());
        return minNote;
    }

    PracticeLesson makeSyntheticLessonWithDanglingNoteOn()
    {
        PracticeLesson lesson;
        lesson.title = "Synthetic";
        lesson.tempoBpm = 120.0;
        lesson.timeSigNumerator = 4;
        lesson.timeSigDenominator = 4;
        lesson.lengthBeats = 8.0;

        juce::MidiMessageSequence seq;
        seq.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0.0);
        seq.addEvent (juce::MidiMessage::noteOff (1, 60), 1.0);
        seq.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100), 1.0);
        seq.addEvent (juce::MidiMessage::noteOff (1, 62), 2.0);
        seq.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 100), 2.0);
        seq.addEvent (juce::MidiMessage::noteOff (1, 64), 3.0);
        // Dangling note-on with no matching note-off: should be dropped.
        seq.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 100), 4.0);
        seq.updateMatchedPairs();

        lesson.accompaniment = seq;
        return lesson;
    }
}

// =============================================================================
// Test 1
// =============================================================================
class AccompanimentPlayer_LoadCreatesClip : public juce::UnitTest
{
public:
    AccompanimentPlayer_LoadCreatesClip() : UnitTest ("AccompanimentPlayer: load creates one clip with all notes", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "load_creates_clip");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value(), "Fixture lesson should load");
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);

        beginTest ("load() succeeds and creates exactly one named clip");
        expect (accompaniment.load (*lessonOpt), "load() should succeed");
        auto* clip = accompaniment.getClip();
        expect (clip != nullptr, "getClip() should be non-null after load()");
        expectEquals (countAccompanimentClips (handle.track), 1);

        if (clip != nullptr)
        {
            beginTest ("Clip has all 120 notes");
            expectEquals (clip->getSequence().getNumNotes(), 120);

            beginTest ("Edit tempo matches lesson tempo");
            auto* edit = TracktionAudioEngine::getInstance().getCurrentEdit();
            expect (edit != nullptr);
            if (edit != nullptr)
            {
                expectWithinAbsoluteError (edit->tempoSequence.getTempo (0)->getBpm(), 75.0, 0.01);

                beginTest ("Clip end is 64 beats");
                auto endBeats = edit->tempoSequence.toBeats (clip->getPosition().getEnd()).inBeats();
                expectWithinAbsoluteError (endBeats, 64.0, 0.01);
            }
        }

        beginTest ("getLessonLengthBeats()");
        expectWithinAbsoluteError (accompaniment.getLessonLengthBeats(), 64.0, 1.0e-6);

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_LoadCreatesClip test_accompanimentPlayerLoadCreatesClip;

// =============================================================================
// Test 2
// =============================================================================
class AccompanimentPlayer_ReloadReplacesClip : public juce::UnitTest
{
public:
    AccompanimentPlayer_ReloadReplacesClip() : UnitTest ("AccompanimentPlayer: reload replaces clip", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "reload_replaces_clip");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);

        beginTest ("Load twice still yields one clip with 120 notes");
        expect (accompaniment.load (*lessonOpt));
        expect (accompaniment.load (*lessonOpt));

        expectEquals (countAccompanimentClips (handle.track), 1);
        auto* clip = accompaniment.getClip();
        expect (clip != nullptr);
        if (clip != nullptr)
            expectEquals (clip->getSequence().getNumNotes(), 120);

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_ReloadReplacesClip test_accompanimentPlayerReloadReplacesClip;

// =============================================================================
// Test 3
// =============================================================================
class AccompanimentPlayer_TempoChangeKeepsBeatLength : public juce::UnitTest
{
public:
    AccompanimentPlayer_TempoChangeKeepsBeatLength() : UnitTest ("AccompanimentPlayer: tempo change keeps beat length", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "tempo_change");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);
        expect (accompaniment.load (*lessonOpt));

        beginTest ("setTempoBpm(60)");
        accompaniment.setTempoBpm (60.0);
        expectWithinAbsoluteError (accompaniment.getTempoBpm(), 60.0, 1.0e-6);

        auto* edit = TracktionAudioEngine::getInstance().getCurrentEdit();
        expect (edit != nullptr);
        if (edit == nullptr)
        {
            handle.player->shutdown();
            return;
        }

        expectWithinAbsoluteError (edit->tempoSequence.getTempo (0)->getBpm(), 60.0, 0.01);

        auto* clip = accompaniment.getClip();
        expect (clip != nullptr);
        if (clip != nullptr)
        {
            beginTest ("Clip end still 64 beats, 120 notes");
            auto endBeats = edit->tempoSequence.toBeats (clip->getPosition().getEnd()).inBeats();
            expectWithinAbsoluteError (endBeats, 64.0, 0.01);
            expectEquals (clip->getSequence().getNumNotes(), 120);

            beginTest ("Clip end is 64 seconds at 60bpm");
            auto endSeconds = clip->getPosition().getEnd().inSeconds();
            expectWithinAbsoluteError (endSeconds, 64.0, 0.05);
        }

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_TempoChangeKeepsBeatLength test_accompanimentPlayerTempoChangeKeepsBeatLength;

// =============================================================================
// Test 4
// =============================================================================
class AccompanimentPlayer_TransposeShiftsNotes : public juce::UnitTest
{
public:
    AccompanimentPlayer_TransposeShiftsNotes() : UnitTest ("AccompanimentPlayer: transpose shifts notes", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "transpose_shifts_notes");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);
        expect (accompaniment.load (*lessonOpt));

        auto* clipBefore = accompaniment.getClip();
        expect (clipBefore != nullptr);
        if (clipBefore == nullptr)
        {
            handle.player->shutdown();
            return;
        }

        int originalMin = minNoteNumber (clipBefore);

        beginTest ("setTranspose(-3) shifts minimum note down by 3");
        accompaniment.setTranspose (-3);
        expectEquals (accompaniment.getTranspose(), -3);
        auto* clipDown = accompaniment.getClip();
        expect (clipDown != nullptr);
        if (clipDown != nullptr)
            expectEquals (minNoteNumber (clipDown), originalMin - 3);

        beginTest ("setTranspose(0) restores original notes");
        accompaniment.setTranspose (0);
        expectEquals (accompaniment.getTranspose(), 0);
        auto* clipRestored = accompaniment.getClip();
        expect (clipRestored != nullptr);
        if (clipRestored != nullptr)
            expectEquals (minNoteNumber (clipRestored), originalMin);

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_TransposeShiftsNotes test_accompanimentPlayerTransposeShiftsNotes;

// =============================================================================
// Test 5
// =============================================================================
class AccompanimentPlayer_CountInShiftsClip : public juce::UnitTest
{
public:
    AccompanimentPlayer_CountInShiftsClip() : UnitTest ("AccompanimentPlayer: count-in shifts clip", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "count_in_shifts_clip");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);
        expect (accompaniment.load (*lessonOpt));

        auto* clipBefore = accompaniment.getClip();
        expect (clipBefore != nullptr);
        if (clipBefore == nullptr)
        {
            handle.player->shutdown();
            return;
        }

        double firstNoteBeatBefore = 0.0;
        expect (clipBefore->getSequence().getNumNotes() > 0);
        if (clipBefore->getSequence().getNumNotes() > 0)
            firstNoteBeatBefore = clipBefore->getSequence().getNote (0)->getStartBeat().inBeats();

        auto* edit = TracktionAudioEngine::getInstance().getCurrentEdit();
        expect (edit != nullptr);
        if (edit == nullptr)
        {
            handle.player->shutdown();
            return;
        }

        beginTest ("setCountInBars(1) shifts clip start to beat 4");
        accompaniment.setCountInBars (1);
        expectEquals (accompaniment.getCountInBars(), 1);

        auto* clipAfter = accompaniment.getClip();
        expect (clipAfter != nullptr);
        if (clipAfter != nullptr)
        {
            auto startBeats = edit->tempoSequence.toBeats (clipAfter->getPosition().getStart()).inBeats();
            expectWithinAbsoluteError (startBeats, 4.0, 0.01);

            beginTest ("First note shifted by count-in");
            if (clipAfter->getSequence().getNumNotes() > 0)
            {
                auto firstNoteAbsBeat = startBeats + clipAfter->getSequence().getNote (0)->getStartBeat().inBeats();
                expectWithinAbsoluteError (firstNoteAbsBeat, 4.0 + firstNoteBeatBefore, 0.01);
            }
        }

        beginTest ("setPositionBeats(0) lands transport at beat 4 (start of lesson, after count-in)");
        accompaniment.setPositionBeats (0.0);
        expectWithinAbsoluteError (accompaniment.getPositionBeats(), 0.0, 0.05);
        auto transportBeats = edit->tempoSequence.toBeats (edit->getTransport().getPosition()).inBeats();
        expectWithinAbsoluteError (transportBeats, 4.0, 0.05);

        beginTest ("setPositionBeats(-4) lands transport at beat 0 (start of count-in)");
        accompaniment.setPositionBeats (-4.0);
        auto transportBeats2 = edit->tempoSequence.toBeats (edit->getTransport().getPosition()).inBeats();
        expectWithinAbsoluteError (transportBeats2, 0.0, 0.05);

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_CountInShiftsClip test_accompanimentPlayerCountInShiftsClip;

// =============================================================================
// Test 6
// =============================================================================
class AccompanimentPlayer_LoopRange : public juce::UnitTest
{
public:
    AccompanimentPlayer_LoopRange() : UnitTest ("AccompanimentPlayer: loop range", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "loop_range");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);
        expect (accompaniment.load (*lessonOpt));

        auto* edit = TracktionAudioEngine::getInstance().getCurrentEdit();
        expect (edit != nullptr);
        if (edit == nullptr)
        {
            handle.player->shutdown();
            return;
        }

        beginTest ("setLoopBars(2, 3) sets looping with no count-in");
        accompaniment.setLoopBars (2, 3);
        expect (accompaniment.isLooping());

        auto expectedStart = edit->tempoSequence.toTime (te::BeatPosition::fromBeats (8.0));
        auto expectedEnd   = edit->tempoSequence.toTime (te::BeatPosition::fromBeats (16.0));
        auto loopRange = edit->getTransport().getLoopRange();
        expectWithinAbsoluteError (loopRange.getStart().inSeconds(), expectedStart.inSeconds(), 0.01);
        expectWithinAbsoluteError (loopRange.getEnd().inSeconds(), expectedEnd.inSeconds(), 0.01);

        beginTest ("setCountInBars(1) shifts loop range by 4 beats");
        accompaniment.setCountInBars (1);
        auto expectedStart2 = edit->tempoSequence.toTime (te::BeatPosition::fromBeats (12.0));
        auto expectedEnd2   = edit->tempoSequence.toTime (te::BeatPosition::fromBeats (20.0));
        auto loopRange2 = edit->getTransport().getLoopRange();
        expectWithinAbsoluteError (loopRange2.getStart().inSeconds(), expectedStart2.inSeconds(), 0.01);
        expectWithinAbsoluteError (loopRange2.getEnd().inSeconds(), expectedEnd2.inSeconds(), 0.01);

        beginTest ("clearLoop() stops looping");
        accompaniment.clearLoop();
        expect (! accompaniment.isLooping());

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_LoopRange test_accompanimentPlayerLoopRange;

// =============================================================================
// Test 7
// =============================================================================
class AccompanimentPlayer_PlayAndStopToggleTransport : public juce::UnitTest
{
public:
    AccompanimentPlayer_PlayAndStopToggleTransport() : UnitTest ("AccompanimentPlayer: play and stop toggle transport", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "play_stop_toggle");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        AccompanimentPlayer accompaniment (*handle.player);
        expect (accompaniment.load (*lessonOpt));

        beginTest ("play() starts the transport");
        accompaniment.setPositionBeats (0.0);
        accompaniment.play();

        if (! accompaniment.isPlaying())
            juce::Thread::sleep (50);

        expect (accompaniment.isPlaying(), "Transport should be playing after play()");

        beginTest ("stop() stops the transport");
        accompaniment.stop();
        expect (! accompaniment.isPlaying(), "Transport should not be playing after stop()");

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_PlayAndStopToggleTransport test_accompanimentPlayerPlayAndStopToggleTransport;

// =============================================================================
// Test 8
// =============================================================================
class AccompanimentPlayer_DropsUnmatchedNoteOns : public juce::UnitTest
{
public:
    AccompanimentPlayer_DropsUnmatchedNoteOns() : UnitTest ("AccompanimentPlayer: drops unmatched note-ons", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "drops_unmatched");
        if (! handle.isUsable())
            return;

        auto synthetic = makeSyntheticLessonWithDanglingNoteOn();

        AccompanimentPlayer accompaniment (*handle.player);

        beginTest ("load() drops the dangling note-on");
        expect (accompaniment.load (synthetic));
        auto* clip = accompaniment.getClip();
        expect (clip != nullptr);
        if (clip != nullptr)
            expectEquals (clip->getSequence().getNumNotes(), 3);

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_DropsUnmatchedNoteOns test_accompanimentPlayerDropsUnmatchedNoteOns;

// =============================================================================
// Test 9
// =============================================================================
class AccompanimentPlayer_UnloadRemovesClip : public juce::UnitTest
{
public:
    AccompanimentPlayer_UnloadRemovesClip() : UnitTest ("AccompanimentPlayer: unload removes clip", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Setup");
        auto handle = makeInitializedPlayer (*this, "unload_removes_clip");
        if (! handle.isUsable())
            return;

        auto lessonOpt = loadFixtureLesson();
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
        {
            handle.player->shutdown();
            return;
        }

        beginTest ("unload() removes the clip");
        {
            AccompanimentPlayer accompaniment (*handle.player);
            expect (accompaniment.load (*lessonOpt));
            expect (accompaniment.getClip() != nullptr);

            accompaniment.unload();
            expect (accompaniment.getClip() == nullptr, "getClip() should be null after unload()");
            expectEquals (countAccompanimentClips (handle.track), 0);
        }

        beginTest ("Destructor also removes the clip");
        {
            AccompanimentPlayer accompaniment2 (*handle.player);
            expect (accompaniment2.load (*lessonOpt));
            expect (accompaniment2.getClip() != nullptr);
        }
        expectEquals (countAccompanimentClips (handle.track), 0);

        handle.player->shutdown();
    }
};
static AccompanimentPlayer_UnloadRemovesClip test_accompanimentPlayerUnloadRemovesClip;
