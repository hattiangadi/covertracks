// PracticeLessonTests.cpp — TDD tests for the Practice mode lesson loader
//
// Covers PracticeLesson (Source/Practice/Core/PracticeLesson.h) and
// LessonLoader (Source/Practice/Core/LessonLoader.h): loading a Standard MIDI
// File of a Vaccai vocalise into voice notes + syllables + piano
// accompaniment, and the pure transpose / sliceBars transformations.

#include <JuceHeader.h>
#include "../Practice/Core/LessonLoader.h"
#include "../Practice/Core/PracticeLesson.h"

using namespace covertracks::practice;

// =============================================================================
// Shared helpers
// =============================================================================

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

    std::optional<PracticeLesson> loadFixture()
    {
        auto file = getFixtureFile();
        return LessonLoader::loadFromFile (file);
    }

    // Serialises a set of tracks into a Standard MIDI File in memory, then
    // returns a fresh MemoryBlock holding the bytes (callers can make as many
    // independent MemoryInputStreams from it as they like).
    juce::MemoryBlock buildMidiBytes (const std::vector<juce::MidiMessageSequence>& tracks, int ppq = 480)
    {
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote (ppq);
        for (auto& t : tracks)
            midiFile.addTrack (t);

        juce::MemoryOutputStream out;
        midiFile.writeTo (out, 1);
        return out.getMemoryBlock();
    }

    juce::MidiFile readMidiFile (const juce::MemoryBlock& bytes)
    {
        juce::MemoryInputStream input (bytes, false);
        juce::MidiFile midiFile;
        midiFile.readFrom (input, true, nullptr);
        return midiFile;
    }

    juce::MidiMessageSequence makeNoteTrack (const juce::String& trackName, int program, int numNotes,
                                              bool withLyrics, int ppq = 480)
    {
        juce::MidiMessageSequence seq;
        if (trackName.isNotEmpty())
            seq.addEvent (juce::MidiMessage::textMetaEvent (0x03, trackName), 0.0);

        if (program >= 0)
            seq.addEvent (juce::MidiMessage::programChange (1, program), 0.0);

        for (int i = 0; i < numNotes; ++i)
        {
            double start = i * (double) ppq;
            seq.addEvent (juce::MidiMessage::noteOn (1, 60 + i, (juce::uint8) 100), start);
            seq.addEvent (juce::MidiMessage::noteOff (1, 60 + i), start + ppq * 0.5);

            if (withLyrics)
                seq.addEvent (juce::MidiMessage::textMetaEvent (0x05, "la"), start);
        }

        seq.updateMatchedPairs();
        return seq;
    }
}

// =============================================================================
// Test 1: Lesson I metadata
// =============================================================================
class PracticeLesson_Metadata : public juce::UnitTest
{
public:
    PracticeLesson_Metadata() : UnitTest ("PracticeLesson: loads Lesson I metadata", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("Fixture exists");
        auto file = getFixtureFile();
        expect (file.existsAsFile(), "Fixture MIDI file should exist at " + file.getFullPathName());
        if (! file.existsAsFile())
            return;

        beginTest ("Loads tempo / time signature / length");
        auto lesson = loadFixture();
        expect (lesson.has_value(), "loadFromFile should succeed");
        if (! lesson.has_value())
            return;

        expectWithinAbsoluteError (lesson->tempoBpm, 75.0, 0.01);
        expectEquals (lesson->timeSigNumerator, 4);
        expectEquals (lesson->timeSigDenominator, 4);
        expectWithinAbsoluteError (lesson->beatsPerBar(), 4.0, 1.0e-9);
        expectWithinAbsoluteError (lesson->lengthBeats, 64.0, 1.0e-6);
        expectEquals (lesson->numBars(), 16);
    }
};
static PracticeLesson_Metadata test_practiceLessonMetadata;

// =============================================================================
// Test 2: voice notes
// =============================================================================
class PracticeLesson_VoiceNotes : public juce::UnitTest
{
public:
    PracticeLesson_VoiceNotes() : UnitTest ("PracticeLesson: extracts voice notes", "Practice Mode") {}
    void runTest() override
    {
        auto lesson = loadFixture();
        beginTest ("Loaded successfully");
        expect (lesson.has_value());
        if (! lesson.has_value())
            return;

        auto& notes = lesson->voiceNotes;

        beginTest ("Note count");
        expectEquals ((int) notes.size(), 46);
        if (notes.size() != 46)
            return;

        beginTest ("First six notes");
        const double expectedStarts[] = { 0, 2, 3, 4, 5, 6 };
        const int expectedPitches[]   = { 57, 59, 61, 62, 64, 66 };
        for (int i = 0; i < 6; ++i)
        {
            expectWithinAbsoluteError (notes[(size_t) i].startBeat, expectedStarts[i], 1.0e-6);
            expectEquals (notes[(size_t) i].midiNote, expectedPitches[i]);
        }

        beginTest ("Sorted by start beat");
        for (size_t i = 1; i < notes.size(); ++i)
            expect (notes[i].startBeat >= notes[i - 1].startBeat, "Notes should be non-decreasing in startBeat");

        beginTest ("Note at beat 8");
        bool found8 = false;
        for (auto& n : notes)
        {
            if (std::abs (n.startBeat - 8.0) < 1.0e-6)
            {
                expectEquals (n.midiNote, 59);
                found8 = true;
            }
        }
        expect (found8, "Expected a note starting at beat 8");

        beginTest ("Final note");
        auto& last = notes.back();
        expectWithinAbsoluteError (last.startBeat, 60.0, 1.0e-6);
        expectEquals (last.midiNote, 57);

        beginTest ("Max / min pitch");
        int maxPitch = -1, minPitch = 200;
        for (auto& n : notes)
        {
            maxPitch = juce::jmax (maxPitch, n.midiNote);
            minPitch = juce::jmin (minPitch, n.midiNote);
        }
        expectEquals (maxPitch, 73);
        expectEquals (minPitch, 57);

        // NOTE: the spec's "known facts" stated the note 73 at beat 44 lasts 4
        // beats. The actual MIDI data has two separate 2-beat note-on/off
        // events for pitch 73 (beat 44-46, then a 2-beat rest, then another
        // note-on at beat 48-50) rather than one tied 4-beat note. Standard
        // note-on/note-off pairing (what juce::MidiFile::readFrom with
        // createMatchingNoteOffs does) therefore yields durationBeats == 2
        // for the note at beat 44. See final report for details.
        beginTest ("Note 73 at beat 44 duration (deviation from stated facts, see report)");
        bool found44 = false;
        for (auto& n : notes)
        {
            if (std::abs (n.startBeat - 44.0) < 1.0e-6)
            {
                expectEquals (n.midiNote, 73);
                expectWithinAbsoluteError (n.durationBeats, 2.0, 1.0e-6);
                found44 = true;
            }
        }
        expect (found44, "Expected a note starting at beat 44");
    }
};
static PracticeLesson_VoiceNotes test_practiceLessonVoiceNotes;

// =============================================================================
// Test 3: UTF-8 syllables
// =============================================================================
class PracticeLesson_Syllables : public juce::UnitTest
{
public:
    PracticeLesson_Syllables() : UnitTest ("PracticeLesson: attaches UTF-8 syllables", "Practice Mode") {}
    void runTest() override
    {
        auto lesson = loadFixture();
        beginTest ("Loaded successfully");
        expect (lesson.has_value());
        if (! lesson.has_value())
            return;

        auto& notes = lesson->voiceNotes;
        expect (notes.size() == 46, "Expected 46 voice notes");
        if (notes.size() != 46)
            return;

        auto findSyllableAt = [&] (double beat) -> const TargetNote*
        {
            for (auto& n : notes)
                if (std::abs (n.startBeat - beat) < 1.0e-6)
                    return &n;
            return nullptr;
        };

        beginTest ("beat 0 = Ma");
        if (auto* n = findSyllableAt (0.0)) expectEquals (n->syllable, juce::String ("Ma"));
        else expect (false, "missing note at beat 0");

        beginTest ("beat 2 = nca");
        if (auto* n = findSyllableAt (2.0)) expectEquals (n->syllable, juce::String ("nca"));
        else expect (false, "missing note at beat 2");

        beginTest ("beat 8 = UTF-8 piu with accent");
        if (auto* n = findSyllableAt (8.0))
        {
            auto expected = juce::String::fromUTF8 ("pi\xc3\xb9"); // "più"
            expectEquals (n->syllable, expected);
        }
        else expect (false, "missing note at beat 8");

        beginTest ("beat 10 = de");
        if (auto* n = findSyllableAt (10.0)) expectEquals (n->syllable, juce::String ("de"));
        else expect (false, "missing note at beat 10");

        beginTest ("42 notes carry a non-empty syllable");
        int withSyllable = 0;
        for (auto& n : notes)
            if (n.syllable.isNotEmpty())
                ++withSyllable;
        expectEquals (withSyllable, 42);
    }
};
static PracticeLesson_Syllables test_practiceLessonSyllables;

// =============================================================================
// Test 4: accompaniment excludes voice
// =============================================================================
class PracticeLesson_Accompaniment : public juce::UnitTest
{
public:
    PracticeLesson_Accompaniment() : UnitTest ("PracticeLesson: accompaniment excludes voice", "Practice Mode") {}
    void runTest() override
    {
        auto lesson = loadFixture();
        beginTest ("Loaded successfully");
        expect (lesson.has_value());
        if (! lesson.has_value())
            return;

        auto& acc = lesson->accompaniment;

        beginTest ("Note-on count is 78 + 42");
        int numNoteOns = 0;
        for (int i = 0; i < acc.getNumEvents(); ++i)
        {
            auto* holder = acc.getEventPointer (i);
            if (holder != nullptr && holder->message.isNoteOn())
                ++numNoteOns;
        }
        expectEquals (numNoteOns, 120);

        // NOTE: the spec's "known facts" suggested checking that no
        // accompaniment note-on at midi 73 / beat 44 exists, as a sanity
        // check that the voice track wasn't merged in. That check doesn't
        // hold here: the piano accompaniment (upper:1) legitimately doubles
        // the voice's note 73 at beat 44 in its own part. The note-on COUNT
        // assertion above (120 == 78 + 42, not 166 == 120 + 46) is the real
        // proof the voice track was excluded, so we rely on that instead.
        // See final report for details.

        beginTest ("All timestamps within [0, lengthBeats]");
        bool allInRange = true;
        for (int i = 0; i < acc.getNumEvents(); ++i)
        {
            auto* holder = acc.getEventPointer (i);
            auto t = holder->message.getTimeStamp();
            if (t < -1.0e-6 || t > lesson->lengthBeats + 1.0e-6)
                allInRange = false;
        }
        expect (allInRange, "All accompaniment timestamps should be within [0, lengthBeats]");

        beginTest ("No meta events in accompaniment");
        bool noMeta = true;
        for (int i = 0; i < acc.getNumEvents(); ++i)
            if (acc.getEventPointer (i)->message.isMetaEvent())
                noMeta = false;
        expect (noMeta, "Accompaniment should contain only channel messages");

        beginTest ("Every note-on has a matched note-off");
        expect (acc.getNumEvents() > 0);
        bool allMatched = true;
        for (int i = 0; i < acc.getNumEvents(); ++i)
        {
            auto* holder = acc.getEventPointer (i);
            if (holder->message.isNoteOn() && holder->noteOffObject == nullptr)
                allMatched = false;
        }
        expect (allMatched, "Every note-on in the accompaniment should have a matching note-off");
    }
};
static PracticeLesson_Accompaniment test_practiceLessonAccompaniment;

// =============================================================================
// Test 5: voice track detection
// =============================================================================
class LessonLoader_DetectVoiceTrack : public juce::UnitTest
{
public:
    LessonLoader_DetectVoiceTrack() : UnitTest ("LessonLoader: detects voice track by name/program/lyrics", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("(a) Track named Soprano is detected by name");
        {
            std::vector<juce::MidiMessageSequence> tracks;
            tracks.push_back (makeNoteTrack ("Piano", 0, 4, false));
            tracks.push_back (makeNoteTrack ("Soprano", 0, 4, false));
            auto bytes = buildMidiBytes (tracks);
            auto midiFile = readMidiFile (bytes);
            expectEquals (LessonLoader::detectVoiceTrack (midiFile), 1);
        }

        beginTest ("(b) Unnamed track with program 52 is detected");
        {
            std::vector<juce::MidiMessageSequence> tracks;
            tracks.push_back (makeNoteTrack ("Piano", 0, 4, false));
            tracks.push_back (makeNoteTrack ("", 52, 4, false));
            auto bytes = buildMidiBytes (tracks);
            auto midiFile = readMidiFile (bytes);
            expectEquals (LessonLoader::detectVoiceTrack (midiFile), 1);
        }

        beginTest ("(c) Unnamed track with notes + lyrics is detected");
        {
            std::vector<juce::MidiMessageSequence> tracks;
            tracks.push_back (makeNoteTrack ("Piano", 0, 4, false));
            tracks.push_back (makeNoteTrack ("", -1, 4, true));
            auto bytes = buildMidiBytes (tracks);
            auto midiFile = readMidiFile (bytes);
            expectEquals (LessonLoader::detectVoiceTrack (midiFile), 1);
        }

        beginTest ("(d) Only Piano tracks -> -1, loadFromStream -> nullopt");
        {
            std::vector<juce::MidiMessageSequence> tracks;
            tracks.push_back (makeNoteTrack ("Piano", 0, 4, false));
            tracks.push_back (makeNoteTrack ("Piano 2", 0, 4, false));
            auto bytes = buildMidiBytes (tracks);
            auto midiFile = readMidiFile (bytes);
            expectEquals (LessonLoader::detectVoiceTrack (midiFile), -1);

            juce::MemoryInputStream input (bytes, false);
            auto lesson = LessonLoader::loadFromStream (input);
            expect (! lesson.has_value(), "loadFromStream should return nullopt when no voice track can be found");
        }

        beginTest ("(e) voiceTrackOverride forces a given track");
        {
            std::vector<juce::MidiMessageSequence> tracks;
            tracks.push_back (makeNoteTrack ("Piano", 0, 3, false));
            tracks.push_back (makeNoteTrack ("", 0, 5, false)); // no distinguishing features
            auto bytes = buildMidiBytes (tracks);

            juce::MemoryInputStream input (bytes, false);
            LessonLoader::Options options;
            options.voiceTrackOverride = 1;
            auto lesson = LessonLoader::loadFromStream (input, options);
            expect (lesson.has_value(), "loadFromStream should succeed with an explicit override");
            if (lesson.has_value())
                expectEquals ((int) lesson->voiceNotes.size(), 5);
        }
    }
};
static LessonLoader_DetectVoiceTrack test_lessonLoaderDetectVoiceTrack;

// =============================================================================
// Test 6: transposed()
// =============================================================================
class LessonLoader_Transposed : public juce::UnitTest
{
public:
    LessonLoader_Transposed() : UnitTest ("LessonLoader: transposed shifts voice and accompaniment", "Practice Mode") {}
    void runTest() override
    {
        auto lessonOpt = loadFixture();
        beginTest ("Loaded successfully");
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
            return;

        auto& original = *lessonOpt;

        beginTest ("First voice note / max shift by -3 semitones");
        auto shifted = LessonLoader::transposed (original, -3);
        expect (! shifted.voiceNotes.empty());
        if (! shifted.voiceNotes.empty())
            expectEquals (shifted.voiceNotes.front().midiNote, 54);

        int maxPitch = -1;
        for (auto& n : shifted.voiceNotes)
            maxPitch = juce::jmax (maxPitch, n.midiNote);
        expectEquals (maxPitch, 70);

        beginTest ("Accompaniment lowest note shifts by -3");
        auto lowestOf = [] (const juce::MidiMessageSequence& seq) -> int
        {
            int lowest = 200;
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                auto* holder = seq.getEventPointer (i);
                if (holder->message.isNoteOn())
                    lowest = juce::jmin (lowest, holder->message.getNoteNumber());
            }
            return lowest;
        };
        int originalLowest = lowestOf (original.accompaniment);
        int shiftedLowest = lowestOf (shifted.accompaniment);
        expectEquals (shiftedLowest, originalLowest - 3);

        beginTest ("Original object unchanged");
        expectEquals (original.voiceNotes.front().midiNote, 57);
        expectEquals (lowestOf (original.accompaniment), originalLowest);

        beginTest ("Channel 10 notes are not shifted");
        {
            PracticeLesson synthetic;
            synthetic.voiceNotes.push_back ({ 0.0, 1.0, 60, "" });
            synthetic.accompaniment.addEvent (juce::MidiMessage::noteOn (10, 40, (juce::uint8) 100), 0.0);
            synthetic.accompaniment.addEvent (juce::MidiMessage::noteOff (10, 40), 1.0);
            synthetic.accompaniment.updateMatchedPairs();

            auto synthShifted = LessonLoader::transposed (synthetic, 5);
            expectEquals (synthShifted.voiceNotes.front().midiNote, 65);
            expectEquals (lowestOf (synthShifted.accompaniment), 40);
        }

        beginTest ("Clamping at 0 / 127");
        {
            PracticeLesson synthetic;
            synthetic.voiceNotes.push_back ({ 0.0, 1.0, 1, "" });
            synthetic.voiceNotes.push_back ({ 1.0, 1.0, 126, "" });

            auto down = LessonLoader::transposed (synthetic, -10);
            expectEquals (down.voiceNotes[0].midiNote, 0);

            auto up = LessonLoader::transposed (synthetic, 10);
            expectEquals (up.voiceNotes[1].midiNote, 127);
        }
    }
};
static LessonLoader_Transposed test_lessonLoaderTransposed;

// =============================================================================
// Test 7: sliceBars()
// =============================================================================
class LessonLoader_SliceBars : public juce::UnitTest
{
public:
    LessonLoader_SliceBars() : UnitTest ("LessonLoader: sliceBars re-times a bar range", "Practice Mode") {}
    void runTest() override
    {
        auto lessonOpt = loadFixture();
        beginTest ("Loaded successfully");
        expect (lessonOpt.has_value());
        if (! lessonOpt.has_value())
            return;

        auto& lesson = *lessonOpt;

        beginTest ("Bars 2-3 (beats 8..16)");
        auto slice = LessonLoader::sliceBars (lesson, 2, 3);
        expectWithinAbsoluteError (slice.lengthBeats, 8.0, 1.0e-6);
        expectEquals (slice.numBars(), 2);

        expect (! slice.voiceNotes.empty());
        if (! slice.voiceNotes.empty())
        {
            expectWithinAbsoluteError (slice.voiceNotes.front().startBeat, 0.0, 1.0e-6);
            expectEquals (slice.voiceNotes.front().midiNote, 59);
            auto expectedSyllable = juce::String::fromUTF8 ("pi\xc3\xb9");
            expectEquals (slice.voiceNotes.front().syllable, expectedSyllable);
        }

        for (auto& n : slice.voiceNotes)
            expect (n.startBeat < 8.0, "No voice note should start at/after beat 8 in an 8-beat slice");

        for (int i = 0; i < slice.accompaniment.getNumEvents(); ++i)
        {
            auto t = slice.accompaniment.getEventPointer (i)->message.getTimeStamp();
            expect (t >= -1.0e-6 && t <= 8.0 + 1.0e-6, "Accompaniment timestamps should be within [0, 8]");
        }

        // NOTE: the spec's "known facts" stated the beat-44 note lasts 4 beats
        // and expected sliceBars(11, 11) to keep it "with duration 4". The
        // real MIDI data has that note lasting 2 beats (see test 2's note),
        // so after re-timing to the start of bar 11 it keeps its real
        // duration of 2 beats. See final report for details.
        beginTest ("Bar 11 keeps the beat-44 note (re-timed to beat 0) with its real duration");
        auto bar11 = LessonLoader::sliceBars (lesson, 11, 11);
        bool found = false;
        for (auto& n : bar11.voiceNotes)
        {
            if (std::abs (n.startBeat - 0.0) < 1.0e-6 && n.midiNote == 73)
            {
                expectWithinAbsoluteError (n.durationBeats, 2.0, 1.0e-6);
                found = true;
            }
        }
        expect (found, "Expected the beat-44 note re-timed to beat 0 in the bar-11 slice");

        beginTest ("A note straddling the slice end is truncated (synthetic)");
        {
            PracticeLesson synthetic;
            synthetic.tempoBpm = 120.0;
            synthetic.timeSigNumerator = 4;
            synthetic.timeSigDenominator = 4;
            synthetic.lengthBeats = 16.0;
            // One note starting in bar 1 (beats 4-8) and lasting 3 beats past the end of bar 1.
            synthetic.voiceNotes.push_back ({ 6.0, 5.0, 65, "straddle" }); // beats 6..11, bar1 ends at beat 8
            auto sliced = LessonLoader::sliceBars (synthetic, 1, 1); // beats 4..8 -> re-timed 0..4
            expect (! sliced.voiceNotes.empty());
            if (! sliced.voiceNotes.empty())
            {
                auto& n = sliced.voiceNotes.front();
                expectWithinAbsoluteError (n.startBeat, 2.0, 1.0e-6);     // 6 - 4
                expectWithinAbsoluteError (n.durationBeats, 2.0, 1.0e-6); // truncated to slice end (beat 4 in re-timed coords)
            }
        }
    }
};
static LessonLoader_SliceBars test_lessonLoaderSliceBars;

// =============================================================================
// Test 8: rejects SMPTE and empty files
// =============================================================================
class LessonLoader_RejectsInvalid : public juce::UnitTest
{
public:
    LessonLoader_RejectsInvalid() : UnitTest ("LessonLoader: rejects SMPTE and empty files", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("SMPTE time format is rejected");
        {
            juce::MidiFile midiFile;
            midiFile.setSmpteTimeFormat (25, 40);
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0.0);
            seq.addEvent (juce::MidiMessage::noteOff (1, 60), 100.0);
            seq.updateMatchedPairs();
            midiFile.addTrack (seq);

            juce::MemoryOutputStream out;
            midiFile.writeTo (out, 1);
            juce::MemoryInputStream input (out.getData(), out.getDataSize(), false);

            auto lesson = LessonLoader::loadFromStream (input);
            expect (! lesson.has_value(), "SMPTE-timed files should be rejected");
        }

        beginTest ("Empty stream is rejected");
        {
            juce::MemoryInputStream input (nullptr, 0, false);
            auto lesson = LessonLoader::loadFromStream (input);
            expect (! lesson.has_value(), "An empty/unreadable stream should return nullopt");
        }
    }
};
static LessonLoader_RejectsInvalid test_lessonLoaderRejectsInvalid;
