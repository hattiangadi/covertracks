#include "LessonLoader.h"

#include <algorithm>
#include <cmath>

namespace covertracks {
namespace practice {

namespace
{
    constexpr double kLyricAttachToleranceBeats = 1.0 / 16.0;

    bool trackHasNotes (const juce::MidiMessageSequence& track)
    {
        for (int i = 0; i < track.getNumEvents(); ++i)
            if (track.getEventPointer (i)->message.isNoteOn())
                return true;
        return false;
    }

    bool trackHasLyrics (const juce::MidiMessageSequence& track)
    {
        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            auto& m = track.getEventPointer (i)->message;
            if (m.isMetaEvent() && m.getMetaEventType() == 0x05)
                return true;
        }
        return false;
    }

    juce::String getTrackName (const juce::MidiMessageSequence& track)
    {
        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            auto& m = track.getEventPointer (i)->message;
            if (m.isTrackNameEvent())
                return m.getTextFromTextMetaEvent();
        }
        return {};
    }

    int getFirstProgramChange (const juce::MidiMessageSequence& track)
    {
        for (int i = 0; i < track.getNumEvents(); ++i)
        {
            auto& m = track.getEventPointer (i)->message;
            if (m.isProgramChange())
                return m.getProgramChangeNumber();
        }
        return -1;
    }

    bool nameLooksLikeVoice (const juce::String& name)
    {
        if (name.isEmpty())
            return false;

        auto lower = name.toLowerCase().trim();

        if (lower == "bass")
            return true;

        static const char* kKeywords[] = {
            "voice", "vocal", "voce", "canto", "soprano", "alto", "tenor",
            "bariton", "bass ", "mezzo", "singer", "melody"
        };

        for (auto* keyword : kKeywords)
            if (lower.contains (keyword))
                return true;

        return false;
    }

    /** Strips LilyPond/MuseScore hyphenation artifacts: trailing "-" or " -"
        and leading "-". */
    juce::String stripHyphenation (const juce::String& text)
    {
        auto s = text.trim();

        while (s.endsWithChar ('-'))
            s = s.dropLastCharacters (1).trimEnd();

        while (s.startsWithChar ('-'))
            s = s.substring (1).trimStart();

        return s;
    }

    struct LyricEvent
    {
        double beat;
        juce::String text;
    };
}

int LessonLoader::detectVoiceTrack (const juce::MidiFile& midiFile)
{
    const int numTracks = midiFile.getNumTracks();

    // Priority 1: track name contains a voice-like keyword.
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = midiFile.getTrack (i);
        if (track != nullptr && nameLooksLikeVoice (getTrackName (*track)))
            return i;
    }

    // Priority 2: program change in 52..54 (choir aahs / voice oohs / synth
    // voice) on a track that has notes.
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = midiFile.getTrack (i);
        if (track == nullptr || ! trackHasNotes (*track))
            continue;

        auto program = getFirstProgramChange (*track);
        if (program >= 52 && program <= 54)
            return i;
    }

    // Priority 3: a track with notes that also carries lyric meta events.
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = midiFile.getTrack (i);
        if (track != nullptr && trackHasNotes (*track) && trackHasLyrics (*track))
            return i;
    }

    return -1;
}

std::optional<PracticeLesson> LessonLoader::loadFromMidiFile (juce::MidiFile& midiFile, Options options,
                                                                const juce::String& titleFromFileName)
{
    const short timeFormat = midiFile.getTimeFormat();
    if (timeFormat <= 0) // negative/zero => SMPTE or invalid; we only support ticks-per-quarter-note files
        return std::nullopt;

    const double ppq = (double) timeFormat;
    const int numTracks = midiFile.getNumTracks();
    if (numTracks <= 0)
        return std::nullopt;

    const int voiceTrackIndex = options.voiceTrackOverride >= 0
        ? options.voiceTrackOverride
        : detectVoiceTrack (midiFile);

    if (voiceTrackIndex < 0 || voiceTrackIndex >= numTracks)
        return std::nullopt;

    auto* voiceTrack = midiFile.getTrack (voiceTrackIndex);
    if (voiceTrack == nullptr)
        return std::nullopt;

    PracticeLesson lesson;

    // ---- tempo / time signature / key signature (first found, track 0 preferred) ----
    bool foundTempo = false, foundTimeSig = false, foundKeySig = false;
    for (int t = 0; t < numTracks; ++t)
    {
        auto* track = midiFile.getTrack (t);
        if (track == nullptr)
            continue;

        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto& m = track->getEventPointer (i)->message;

            if (! foundTempo && m.isTempoMetaEvent())
            {
                auto secondsPerQuarterNote = m.getTempoSecondsPerQuarterNote();
                if (secondsPerQuarterNote > 0.0)
                    lesson.tempoBpm = 60.0 / secondsPerQuarterNote;
                foundTempo = true;
            }
            else if (! foundTimeSig && m.isTimeSignatureMetaEvent())
            {
                m.getTimeSignatureInfo (lesson.timeSigNumerator, lesson.timeSigDenominator);
                foundTimeSig = true;
            }
            else if (! foundKeySig && m.isKeySignatureMetaEvent())
            {
                lesson.keySignatureSharpsFlats = m.getKeySignatureNumberOfSharpsOrFlats();
                foundKeySig = true;
            }
        }
    }

    // ---- title ----
    juce::String track0Name;
    if (auto* track0 = midiFile.getTrack (0))
        track0Name = getTrackName (*track0);

    if (track0Name.isNotEmpty() && ! track0Name.trim().equalsIgnoreCase ("control track"))
        lesson.title = track0Name;
    else
        lesson.title = titleFromFileName;

    // ---- voice notes ----
    std::vector<TargetNote> voiceNotes;
    for (int i = 0; i < voiceTrack->getNumEvents(); ++i)
    {
        auto* holder = voiceTrack->getEventPointer (i);
        auto& m = holder->message;
        if (! m.isNoteOn())
            continue;

        TargetNote note;
        note.startBeat = m.getTimeStamp() / ppq;
        note.midiNote = m.getNoteNumber();
        note.durationBeats = holder->noteOffObject != nullptr
            ? (holder->noteOffObject->message.getTimeStamp() - m.getTimeStamp()) / ppq
            : 0.0;
        voiceNotes.push_back (note);
    }

    std::stable_sort (voiceNotes.begin(), voiceNotes.end(),
                       [] (const TargetNote& a, const TargetNote& b) { return a.startBeat < b.startBeat; });

    // ---- lyrics, gathered from ALL tracks ----
    std::vector<LyricEvent> lyrics;
    for (int t = 0; t < numTracks; ++t)
    {
        auto* track = midiFile.getTrack (t);
        if (track == nullptr)
            continue;

        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto& m = track->getEventPointer (i)->message;
            if (m.isMetaEvent() && m.getMetaEventType() == 0x05)
                lyrics.push_back ({ m.getTimeStamp() / ppq, m.getTextFromTextMetaEvent() });
        }
    }

    for (auto& lyric : lyrics)
    {
        for (auto& note : voiceNotes)
        {
            if (std::abs (note.startBeat - lyric.beat) <= kLyricAttachToleranceBeats + 1.0e-9)
            {
                auto cleaned = stripHyphenation (lyric.text);
                note.syllable = note.syllable.isEmpty() ? cleaned : note.syllable + cleaned;
                break;
            }
        }
    }

    lesson.voiceNotes = voiceNotes;

    // ---- accompaniment: every other track that has notes ----
    juce::MidiMessageSequence accompaniment;
    for (int t = 0; t < numTracks; ++t)
    {
        if (t == voiceTrackIndex)
            continue;

        auto* track = midiFile.getTrack (t);
        if (track == nullptr || ! trackHasNotes (*track))
            continue;

        juce::MidiMessageSequence filtered;
        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto message = track->getEventPointer (i)->message; // copy
            if (message.isMetaEvent() || message.isSysEx())
                continue;

            message.setTimeStamp (message.getTimeStamp() / ppq);
            filtered.addEvent (message, 0.0);
        }

        accompaniment.addSequence (filtered, 0.0);
    }
    accompaniment.updateMatchedPairs();
    lesson.accompaniment = accompaniment;

    // ---- lengthBeats: end of last note-off across all tracks, rounded up to
    // the end of the bar it falls in (a lesson is always a whole number of bars) ----
    double rawEndBeats = 0.0;
    for (int t = 0; t < numTracks; ++t)
    {
        auto* track = midiFile.getTrack (t);
        if (track == nullptr)
            continue;

        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            auto* holder = track->getEventPointer (i);
            if (! holder->message.isNoteOn())
                continue;

            double endBeat = holder->noteOffObject != nullptr
                ? holder->noteOffObject->message.getTimeStamp() / ppq
                : holder->message.getTimeStamp() / ppq;
            rawEndBeats = juce::jmax (rawEndBeats, endBeat);
        }
    }

    const double bpb = lesson.beatsPerBar();
    lesson.lengthBeats = bpb > 0.0
        ? std::ceil (rawEndBeats / bpb - 1.0e-9) * bpb
        : rawEndBeats;

    return lesson;
}

std::optional<PracticeLesson> LessonLoader::loadFromFile (const juce::File& file, Options options)
{
    if (! file.existsAsFile())
        return std::nullopt;

    auto stream = file.createInputStream();
    if (stream == nullptr)
        return std::nullopt;

    juce::MidiFile midiFile;
    if (! midiFile.readFrom (*stream, true, nullptr))
        return std::nullopt;

    return loadFromMidiFile (midiFile, options, file.getFileNameWithoutExtension());
}

std::optional<PracticeLesson> LessonLoader::loadFromStream (juce::InputStream& stream, Options options)
{
    juce::MidiFile midiFile;
    if (! midiFile.readFrom (stream, true, nullptr))
        return std::nullopt;

    return loadFromMidiFile (midiFile, options, juce::String());
}

PracticeLesson LessonLoader::transposed (const PracticeLesson& lesson, int semitones)
{
    PracticeLesson result = lesson;

    auto shiftClamped = [semitones] (int note)
    {
        return juce::jlimit (0, 127, note + semitones);
    };

    for (auto& note : result.voiceNotes)
        note.midiNote = shiftClamped (note.midiNote);

    for (int i = 0; i < result.accompaniment.getNumEvents(); ++i)
    {
        auto& m = result.accompaniment.getEventPointer (i)->message;

        if (m.getChannel() == 10)
            continue;

        if (m.isNoteOnOrOff() || m.isAftertouch())
            m.setNoteNumber (shiftClamped (m.getNoteNumber()));
    }

    result.accompaniment.updateMatchedPairs();

    return result;
}

PracticeLesson LessonLoader::sliceBars (const PracticeLesson& lesson, int firstBar, int lastBarInclusive)
{
    const double bpb = lesson.beatsPerBar();
    const double startBeat = firstBar * bpb;
    const double endBeat = (lastBarInclusive + 1) * bpb;
    const int numBarsInSlice = juce::jmax (0, lastBarInclusive - firstBar + 1);

    PracticeLesson result;
    result.title = lesson.title;
    result.tempoBpm = lesson.tempoBpm;
    result.timeSigNumerator = lesson.timeSigNumerator;
    result.timeSigDenominator = lesson.timeSigDenominator;
    result.keySignatureSharpsFlats = lesson.keySignatureSharpsFlats;
    result.lengthBeats = numBarsInSlice * bpb;

    constexpr double kEps = 1.0e-9;

    for (auto& note : lesson.voiceNotes)
    {
        if (note.startBeat < startBeat - kEps || note.startBeat >= endBeat - kEps)
            continue;

        TargetNote newNote = note;
        newNote.startBeat = note.startBeat - startBeat;

        double originalEnd = note.startBeat + note.durationBeats;
        double clippedEnd = juce::jmin (originalEnd, endBeat);
        newNote.durationBeats = juce::jmax (0.0, clippedEnd - note.startBeat);

        result.voiceNotes.push_back (newNote);
    }

    juce::MidiMessageSequence slicedAccompaniment;
    for (int i = 0; i < lesson.accompaniment.getNumEvents(); ++i)
    {
        auto* holder = lesson.accompaniment.getEventPointer (i);
        auto& m = holder->message;
        double t = m.getTimeStamp();

        if (t < startBeat - kEps || t >= endBeat - kEps)
            continue;

        auto copy = m;
        copy.setTimeStamp (t - startBeat);

        if (m.isNoteOn() && holder->noteOffObject != nullptr
            && holder->noteOffObject->message.getTimeStamp() >= endBeat - kEps)
        {
            // Note straddles the slice end: truncate it to the slice boundary.
            slicedAccompaniment.addEvent (copy, 0.0);
            auto truncatedOff = juce::MidiMessage::noteOff (m.getChannel(), m.getNoteNumber());
            truncatedOff.setTimeStamp (endBeat - startBeat);
            slicedAccompaniment.addEvent (truncatedOff, 0.0);
            continue;
        }

        slicedAccompaniment.addEvent (copy, 0.0);
    }
    slicedAccompaniment.updateMatchedPairs();
    result.accompaniment = slicedAccompaniment;

    return result;
}

} // namespace practice
} // namespace covertracks
