#include "AccompanimentPlayer.h"
#include "LessonLoader.h"
#include "../../Audio/AudioEngineInterface.h"

namespace covertracks {
namespace practice {

AccompanimentPlayer::AccompanimentPlayer (Player& player)
    : pianoPlayer (player)
{
    if (! pianoPlayer.isInitialized())
        pianoPlayer.initialize();

    edit = TracktionAudioEngine::getInstance().getCurrentEdit();

    if (auto* instrument = pianoPlayer.getInstrument())
        track = dynamic_cast<te::AudioTrack*> (instrument->getTrack());
}

AccompanimentPlayer::~AccompanimentPlayer()
{
    stopTimer();

    if (loadedClip != nullptr)
    {
        loadedClip->removeFromParent();
        loadedClip = nullptr;
    }
}

bool AccompanimentPlayer::load (const PracticeLesson& newLesson)
{
    if (edit == nullptr || track == nullptr)
        return false;

    lesson = newLesson;
    tempoBpm = lesson.tempoBpm;
    loaded = true;

    rebuild();
    return true;
}

void AccompanimentPlayer::unload()
{
    if (loadedClip != nullptr)
    {
        loadedClip->removeFromParent();
        loadedClip = nullptr;
    }

    loaded = false;
}

void AccompanimentPlayer::setTempoBpm (double bpm)
{
    tempoBpm = bpm;

    if (loaded)
        rebuild();
}

void AccompanimentPlayer::setTranspose (int semitones)
{
    transposeSemitones = semitones;

    if (loaded)
        rebuild();
}

void AccompanimentPlayer::setCountInBars (int bars)
{
    countInBars = bars;

    if (loaded)
        rebuild();
    else
        updateLoopRange();
}

void AccompanimentPlayer::setLoopBars (int firstBar, int lastBarInclusive)
{
    loopFirstBar = firstBar;
    loopLastBarInclusive = lastBarInclusive;
    loopSet = true;

    updateLoopRange();
}

void AccompanimentPlayer::clearLoop()
{
    loopSet = false;

    if (edit != nullptr)
        edit->getTransport().looping = false;
}

bool AccompanimentPlayer::isLooping() const
{
    if (edit == nullptr)
        return false;

    return edit->getTransport().looping.get();
}

void AccompanimentPlayer::play()
{
    if (edit == nullptr)
        return;

    edit->getTransport().play (false);
    startTimer (30);
}

void AccompanimentPlayer::stop()
{
    if (edit != nullptr)
        edit->getTransport().stop (false, false);

    stopTimer();
}

bool AccompanimentPlayer::isPlaying() const
{
    return edit != nullptr && edit->getTransport().isPlaying();
}

void AccompanimentPlayer::setPositionBeats (double lessonBeats)
{
    if (edit == nullptr)
        return;

    auto absoluteBeats = lessonBeats + countInBeats();
    edit->getTransport().setPosition (edit->tempoSequence.toTime (te::BeatPosition::fromBeats (absoluteBeats)));
}

double AccompanimentPlayer::getPositionBeats() const
{
    if (edit == nullptr)
        return 0.0;

    auto absoluteBeats = edit->tempoSequence.toBeats (edit->getTransport().getPosition()).inBeats();
    return absoluteBeats - countInBeats();
}

void AccompanimentPlayer::timerCallback()
{
    if (edit == nullptr)
        return;

    if (isLooping())
        return;

    if (getPositionBeats() >= lesson.lengthBeats)
    {
        stop();

        if (onPlaybackFinished)
            onPlaybackFinished();
    }
}

double AccompanimentPlayer::countInBeats() const
{
    return countInBars * lesson.beatsPerBar();
}

void AccompanimentPlayer::rebuild()
{
    if (edit == nullptr || track == nullptr)
        return;

    if (loadedClip != nullptr)
    {
        loadedClip->removeFromParent();
        loadedClip = nullptr;
    }

    // Tempo / time signature.
    if (auto* tempo = edit->tempoSequence.getTempo (0))
        tempo->setBpm (tempoBpm);

    if (auto* timeSig = edit->tempoSequence.getTimeSig (0))
    {
        timeSig->numerator = lesson.timeSigNumerator;
        timeSig->denominator = lesson.timeSigDenominator;
    }

    // Clip time range, in beats including the count-in.
    const double countIn = countInBeats();
    const double clipStartBeat = countIn;
    const double clipEndBeat = countIn + lesson.lengthBeats;

    te::TimeRange clipRange (edit->tempoSequence.toTime (te::BeatPosition::fromBeats (clipStartBeat)),
                              edit->tempoSequence.toTime (te::BeatPosition::fromBeats (clipEndBeat)));

    auto newClip = track->insertMIDIClip ("Practice Accompaniment", clipRange, nullptr);
    if (newClip == nullptr)
        return;

    loadedClip = newClip;

    // Populate notes from the (possibly transposed) accompaniment.
    PracticeLesson transposedLesson;
    const PracticeLesson* sourceLesson = &lesson;
    if (transposeSemitones != 0)
    {
        transposedLesson = LessonLoader::transposed (lesson, transposeSemitones);
        sourceLesson = &transposedLesson;
    }

    auto& midiList = loadedClip->getSequence();
    auto& accompaniment = sourceLesson->accompaniment;

    for (int i = 0; i < accompaniment.getNumEvents(); ++i)
    {
        auto* holder = accompaniment.getEventPointer (i);
        if (holder == nullptr || ! holder->message.isNoteOn())
            continue;

        if (holder->noteOffObject == nullptr)
            continue; // drop unmatched note-ons

        const double startBeat = holder->message.getTimeStamp();
        const double endBeat = holder->noteOffObject->message.getTimeStamp();
        const double durationBeats = juce::jmax (0.0, endBeat - startBeat);
        const int velocity = (int) holder->message.getVelocity();

        midiList.addNote (holder->message.getNoteNumber(),
                           te::BeatPosition::fromBeats (startBeat),
                           te::BeatDuration::fromBeats (durationBeats),
                           velocity,
                           0,
                           nullptr);
    }

    updateLoopRange();
}

void AccompanimentPlayer::updateLoopRange()
{
    if (edit == nullptr)
        return;

    auto& transport = edit->getTransport();

    if (! loopSet)
    {
        transport.looping = false;
        return;
    }

    const double bpb = lesson.beatsPerBar();
    if (bpb <= 0.0)
        return;

    const double countIn = countInBeats();
    const double startBeat = countIn + loopFirstBar * bpb;
    const double endBeat = countIn + (loopLastBarInclusive + 1) * bpb;

    auto startTime = edit->tempoSequence.toTime (te::BeatPosition::fromBeats (startBeat));
    auto endTime = edit->tempoSequence.toTime (te::BeatPosition::fromBeats (endBeat));

    transport.setLoopRange (te::TimeRange (startTime, endTime));
    transport.looping = true;
}

} // namespace practice
} // namespace covertracks
