// PitchTrackingTests.cpp — TDD tests for Practice mode step 4: microphone
// pitch tracking (Source/Practice/Core/PitchDetector.h, PitchTracker.h) and
// the AudioEngine input tap + PracticeModeComponent wiring built on top of
// them.
//
// Category string is exactly "Practice Mode" to match the rest of the
// Practice mode test suite (run with ./run_tests_clean.sh "Practice Mode").

#include <JuceHeader.h>
#include <cmath>
#include <vector>
#include <memory>
#include "../Practice/Core/PitchDetector.h"
#include "../Practice/Core/PitchTracker.h"
#include "../Practice/UI/PracticeModeComponent.h"
#include "../Player/Core/Player.h"
#include "../Player/Core/PlayerManager.h"
#include "../Audio/TracktionAudioEngine.h"

using namespace covertracks::practice;
using covertracks::Player;
using covertracks::PlayerManager;
using covertracks::TracktionAudioEngine;

// =============================================================================
// Shared signal-generation helpers
// =============================================================================

namespace
{
    std::vector<float> makeSine (double sampleRate, double freqHz, double amplitude, int numSamples, double startPhase = 0.0)
    {
        std::vector<float> buf ((size_t) numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            auto phase = 2.0 * juce::MathConstants<double>::pi * freqHz * (double) i / sampleRate + startPhase;
            buf[(size_t) i] = (float) (amplitude * std::sin (phase));
        }
        return buf;
    }

    // Vibrato sine: instantaneous frequency = freqHz * (1 + depth*sin(2*pi*vibratoRateHz*t)).
    // startTimeSeconds lets a caller take a window at an arbitrary offset into the vibrato cycle.
    std::vector<float> makeVibratoSine (double sampleRate, double freqHz, double amplitude, double depth,
                                         double vibratoRateHz, int numSamples, double startTimeSeconds = 0.0)
    {
        std::vector<float> buf ((size_t) numSamples);
        double phase = 0.0;
        double dt = 1.0 / sampleRate;
        double t = startTimeSeconds;

        for (int i = 0; i < numSamples; ++i)
        {
            double instFreq = freqHz * (1.0 + depth * std::sin (2.0 * juce::MathConstants<double>::pi * vibratoRateHz * t));
            phase += 2.0 * juce::MathConstants<double>::pi * instFreq * dt;
            buf[(size_t) i] = (float) (amplitude * std::sin (phase));
            t += dt;
        }
        return buf;
    }

    std::vector<float> makeWhiteNoise (double amplitude, int numSamples, juce::Random& random)
    {
        std::vector<float> buf ((size_t) numSamples);
        for (int i = 0; i < numSamples; ++i)
            buf[(size_t) i] = (float) (amplitude * (random.nextDouble() * 2.0 - 1.0));
        return buf;
    }

    // Odd-harmonic "clarinet-like" wave: harmonics 1,3,5,7 with amplitudes 1, 1/3, 1/5, 1/7,
    // normalised so the fundamental-only peak amplitude would be `amplitude`.
    std::vector<float> makeOddHarmonic (double sampleRate, double freqHz, double amplitude, int numSamples)
    {
        static const int harmonics[4] = { 1, 3, 5, 7 };
        static const double harmonicAmps[4] = { 1.0, 1.0 / 3.0, 1.0 / 5.0, 1.0 / 7.0 };
        double norm = harmonicAmps[0] + harmonicAmps[1] + harmonicAmps[2] + harmonicAmps[3];

        std::vector<float> buf ((size_t) numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            double sample = 0.0;
            for (int h = 0; h < 4; ++h)
            {
                auto phase = 2.0 * juce::MathConstants<double>::pi * freqHz * (double) harmonics[h] * (double) i / sampleRate;
                sample += harmonicAmps[h] * std::sin (phase);
            }
            buf[(size_t) i] = (float) (amplitude * sample / norm);
        }
        return buf;
    }

    juce::File getFixtureFile()
    {
        return juce::File (__FILE__)
            .getParentDirectory()   // Source/Tests
            .getParentDirectory()   // Source
            .getParentDirectory()   // repo root
            .getChildFile ("Lessons/Vaccai/Baritone/01-LessonI.mid");
    }
}

// =============================================================================
// Test 1: pure sine at 48k and 44.1k
// =============================================================================
class PitchDetector_PureSine : public juce::UnitTest
{
public:
    PitchDetector_PureSine() : UnitTest ("PitchDetector: pure sine at 48k and 44.1k", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("220 Hz @ 48000, amplitude 0.3");
        {
            PitchDetector detector (48000.0);
            auto frame = makeSine (48000.0, 220.0, 0.3, detector.getFrameSize());
            auto result = detector.process (frame.data(), (int) frame.size());

            logMessage ("48k: " + juce::String (result.frequencyHz, 3) + " Hz, confidence " + juce::String (result.confidence, 3));

            expect (result.voiced, "Should be voiced");
            expectWithinAbsoluteError (result.frequencyHz, 220.0, 0.5);
            expect (result.confidence > 0.9f, "Confidence should be > 0.9");
            expectWithinAbsoluteError (result.midiPitch, 57.0, 0.05);
        }

        beginTest ("220 Hz @ 44100, amplitude 0.3");
        {
            PitchDetector detector (44100.0);
            auto frame = makeSine (44100.0, 220.0, 0.3, detector.getFrameSize());
            auto result = detector.process (frame.data(), (int) frame.size());

            logMessage ("44.1k: " + juce::String (result.frequencyHz, 3) + " Hz, confidence " + juce::String (result.confidence, 3));

            expect (result.voiced, "Should be voiced");
            expectWithinAbsoluteError (result.frequencyHz, 220.0, 0.5);
            expect (result.confidence > 0.9f, "Confidence should be > 0.9");
        }
    }
};
static PitchDetector_PureSine test_pitchDetectorPureSine;

// =============================================================================
// Test 2: full baritone range
// =============================================================================
class PitchDetector_BaritoneRange : public juce::UnitTest
{
public:
    PitchDetector_BaritoneRange() : UnitTest ("PitchDetector: full baritone range", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchDetector detector (sampleRate);

        double freqs[] = { 98.0, 110.0, 146.8, 196.0, 261.6, 329.6 };

        for (auto freq : freqs)
        {
            beginTest (juce::String (freq, 1) + " Hz");
            auto frame = makeSine (sampleRate, freq, 0.3, detector.getFrameSize());
            auto result = detector.process (frame.data(), (int) frame.size());

            logMessage (juce::String (freq, 1) + " Hz target -> " + juce::String (result.frequencyHz, 3)
                        + " Hz detected, confidence " + juce::String (result.confidence, 3));

            expect (result.voiced, "Should be voiced at " + juce::String (freq));
            expectWithinAbsoluteError (result.frequencyHz, freq, freq * 0.01);
        }
    }
};
static PitchDetector_BaritoneRange test_pitchDetectorBaritoneRange;

// =============================================================================
// Test 3: no octave errors on odd-harmonic tone
// =============================================================================
class PitchDetector_OddHarmonicNoOctaveError : public juce::UnitTest
{
public:
    PitchDetector_OddHarmonicNoOctaveError() : UnitTest ("PitchDetector: no octave errors on odd-harmonic tone", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchDetector detector (sampleRate);

        beginTest ("196 Hz odd-harmonic wave");
        auto frame = makeOddHarmonic (sampleRate, 196.0, 0.3, detector.getFrameSize());
        auto result = detector.process (frame.data(), (int) frame.size());

        logMessage ("Odd-harmonic 196 Hz -> " + juce::String (result.frequencyHz, 3) + " Hz");

        expect (result.voiced, "Should be voiced");
        expectWithinAbsoluteError (result.frequencyHz, 196.0, 2.0);
    }
};
static PitchDetector_OddHarmonicNoOctaveError test_pitchDetectorOddHarmonicNoOctaveError;

// =============================================================================
// Test 4: vibrato tone stays near centre
// =============================================================================
class PitchDetector_VibratoNearCentre : public juce::UnitTest
{
public:
    PitchDetector_VibratoNearCentre() : UnitTest ("PitchDetector: vibrato tone stays near centre", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchDetector detector (sampleRate);

        beginTest ("440 Hz, depth 0.03, rate 6 Hz, arbitrary offset");
        auto frame = makeVibratoSine (sampleRate, 440.0, 0.3, 0.03, 6.0, detector.getFrameSize(), 0.37);
        auto result = detector.process (frame.data(), (int) frame.size());

        logMessage ("Vibrato 440 Hz -> " + juce::String (result.frequencyHz, 3) + " Hz");

        expect (result.voiced, "Should be voiced");
        expectWithinAbsoluteError (result.frequencyHz, 440.0, 440.0 * 0.035);
    }
};
static PitchDetector_VibratoNearCentre test_pitchDetectorVibratoNearCentre;

// =============================================================================
// Test 5: silence and noise are unvoiced
// =============================================================================
class PitchDetector_SilenceAndNoise : public juce::UnitTest
{
public:
    PitchDetector_SilenceAndNoise() : UnitTest ("PitchDetector: silence and noise are unvoiced", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchDetector detector (sampleRate);
        int frameSize = detector.getFrameSize();

        beginTest ("All-zero frame");
        {
            std::vector<float> frame ((size_t) frameSize, 0.0f);
            auto result = detector.process (frame.data(), frameSize);
            expect (! result.voiced, "Silence should be unvoiced");
            expectEquals (result.confidence, 0.0f);
        }

        beginTest ("White noise, amplitude 0.3");
        {
            juce::Random random (12345);
            auto frame = makeWhiteNoise (0.3, frameSize, random);
            auto result = detector.process (frame.data(), frameSize);
            expect (! result.voiced || result.confidence < 0.5f, "Noise should be unvoiced or low-confidence");
        }

        beginTest ("Very quiet sine, amplitude 0.001 (below -50 dBFS)");
        {
            auto frame = makeSine (sampleRate, 220.0, 0.001, frameSize);
            auto result = detector.process (frame.data(), frameSize);
            expect (! result.voiced, "Very quiet sine should be unvoiced");
        }
    }
};
static PitchDetector_SilenceAndNoise test_pitchDetectorSilenceAndNoise;

// =============================================================================
// Test 6: wrong frame size is unvoiced
// =============================================================================
class PitchDetector_WrongFrameSize : public juce::UnitTest
{
public:
    PitchDetector_WrongFrameSize() : UnitTest ("PitchDetector: wrong frame size is unvoiced", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchDetector detector (sampleRate);
        int halfSize = detector.getFrameSize() / 2;

        beginTest ("numSamples = frameSize/2 does not crash and is unvoiced");
        auto frame = makeSine (sampleRate, 220.0, 0.3, halfSize);
        auto result = detector.process (frame.data(), halfSize);

        expect (! result.voiced, "Wrong frame size should be unvoiced");
    }
};
static PitchDetector_WrongFrameSize test_pitchDetectorWrongFrameSize;

// =============================================================================
// Test 7: midi/hz helpers and note names
// =============================================================================
class PitchDetector_MidiHzHelpers : public juce::UnitTest
{
public:
    PitchDetector_MidiHzHelpers() : UnitTest ("PitchDetector: midi/hz helpers and note names", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("midiFromHz(440) == 69");
        expectWithinAbsoluteError (PitchDetector::midiFromHz (440.0), 69.0, 1.0e-9);

        // Note: standard convention (A4 = midi 69 = 440 Hz, as fixed by
        // midiFromHz's formula above and confirmed by Test 1, where a
        // 220 Hz tone measures midiPitch ~= 57) puts midi 57 at A3 = 220 Hz,
        // not 110 Hz (that's A2 = midi 45). hzFromMidi(57) ~= 220.
        beginTest ("hzFromMidi(57) ~= 220");
        expectWithinAbsoluteError (PitchDetector::hzFromMidi (57.0), 220.0, 0.01);

        beginTest ("Round trips");
        for (double midi : { 40.0, 57.0, 60.0, 69.0, 84.0 })
            expectWithinAbsoluteError (PitchDetector::midiFromHz (PitchDetector::hzFromMidi (midi)), midi, 1.0e-6);

        beginTest ("noteNameWithCents");
        expectEquals (PitchDetector::noteNameWithCents (69.0), juce::String ("A4 +0c"));
        expectEquals (PitchDetector::noteNameWithCents (69.12), juce::String ("A4 +12c"));
        expectEquals (PitchDetector::noteNameWithCents (68.9), juce::String ("A4 -10c"));
        expectEquals (PitchDetector::noteNameWithCents (60.0), juce::String ("C4 +0c"));

        auto edgeCase = PitchDetector::noteNameWithCents (58.5);
        expect (edgeCase == juce::String ("A#3 +50c") || edgeCase == juce::String ("B3 -50c"),
                "58.5 should be A#3 +50c or B3 -50c, got: " + edgeCase);
    }
};
static PitchDetector_MidiHzHelpers test_pitchDetectorMidiHzHelpers;

// =============================================================================
// Test 8: PitchTracker streams a sine into time-stamped samples
// =============================================================================
class PitchTracker_StreamsSine : public juce::UnitTest
{
public:
    PitchTracker_StreamsSine() : UnitTest ("PitchTracker: streams a sine into time-stamped samples", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchTracker tracker (sampleRate); // defaults: frameSize 2048, hopSize 512

        int totalSamples = (int) sampleRate; // 1 second
        auto sine = makeSine (sampleRate, 261.63, 0.3, totalSamples);

        beginTest ("Push in 256-sample chunks");
        int chunkSize = 256;
        int offset = 0;
        while (offset < totalSamples)
        {
            int n = juce::jmin (chunkSize, totalSamples - offset);
            tracker.pushSamples (sine.data() + offset, n);
            offset += n;
        }

        beginTest ("analyse() produces expected count");
        int expectedN = (totalSamples - tracker.getFrameSize()) / tracker.getHopSize() + 1;
        int produced = tracker.analyse();
        expectEquals (produced, expectedN);

        beginTest ("takeNewSamples() returns N samples, all voiced near midi 60");
        auto samples = tracker.takeNewSamples();
        expectEquals ((int) samples.size(), expectedN);

        double lastTime = -1.0;
        for (auto& s : samples)
        {
            expect (s.voiced, "Each sample should be voiced");
            expectWithinAbsoluteError (s.midiPitch, 60.0, 0.1);
            expect (s.timeSeconds > lastTime, "timeSeconds should strictly increase");
            lastTime = s.timeSeconds;
        }

        if (! samples.empty())
        {
            double expectedFirstTime = (double) tracker.getFrameSize() / 2.0 / sampleRate;
            expectWithinAbsoluteError (samples.front().timeSeconds, expectedFirstTime, 1.0e-6);
        }

        beginTest ("Second takeNewSamples() call is empty");
        auto empty = tracker.takeNewSamples();
        expect (empty.empty());

        beginTest ("No samples were dropped");
        expectEquals (tracker.getDroppedSamples(), 0);
    }
};
static PitchTracker_StreamsSine test_pitchTrackerStreamsSine;

// =============================================================================
// Test 9: multichannel averaging and reset
// =============================================================================
class PitchTracker_MultichannelAndReset : public juce::UnitTest
{
public:
    PitchTracker_MultichannelAndReset() : UnitTest ("PitchTracker: multichannel averaging and reset", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchTracker tracker (sampleRate);

        int totalSamples = tracker.getFrameSize() * 3;
        auto sine = makeSine (sampleRate, 220.0, 0.3, totalSamples);
        std::vector<float> invertedSine (sine.size());
        for (size_t i = 0; i < sine.size(); ++i)
            invertedSine[i] = -sine[i];

        beginTest ("Two cancelling channels -> unvoiced");
        {
            const float* channels[2] = { sine.data(), invertedSine.data() };
            tracker.pushInterleavedOrMultichannel (channels, 2, totalSamples);

            tracker.analyse();
            auto samples = tracker.takeNewSamples();
            expect (! samples.empty(), "Should have produced some analysed frames");
            for (auto& s : samples)
                expect (! s.voiced, "Cancelling channels should be unvoiced");
        }

        beginTest ("reset() then mono sine is voiced again, timestamps restart");
        {
            tracker.reset();
            tracker.pushSamples (sine.data(), totalSamples);
            tracker.analyse();
            auto samples = tracker.takeNewSamples();

            expect (! samples.empty(), "Should have produced some analysed frames after reset");
            if (! samples.empty())
            {
                expect (samples.front().voiced, "Mono sine after reset should be voiced");

                double expectedFirstTime = (double) tracker.getFrameSize() / 2.0 / sampleRate;
                expectWithinAbsoluteError (samples.front().timeSeconds, expectedFirstTime, 1.0e-6);
            }
        }
    }
};
static PitchTracker_MultichannelAndReset test_pitchTrackerMultichannelAndReset;

// =============================================================================
// Test 10: fifo overflow is counted not crashed
// =============================================================================
class PitchTracker_FifoOverflow : public juce::UnitTest
{
public:
    PitchTracker_FifoOverflow() : UnitTest ("PitchTracker: fifo overflow is counted not crashed", "Practice Mode") {}
    void runTest() override
    {
        double sampleRate = 48000.0;
        PitchTracker tracker (sampleRate);

        beginTest ("Push 20 seconds of samples without analysing");
        int totalSamples = (int) (sampleRate * 20.0);
        auto sine = makeSine (sampleRate, 220.0, 0.3, totalSamples);

        int chunkSize = 1024;
        int offset = 0;
        while (offset < totalSamples)
        {
            int n = juce::jmin (chunkSize, totalSamples - offset);
            tracker.pushSamples (sine.data() + offset, n);
            offset += n;
        }

        expect (tracker.getDroppedSamples() > 0, "Should have dropped samples once the ring buffer filled");

        beginTest ("analyse() still works after overflow");
        auto produced = tracker.analyse();
        expect (produced > 0, "Should still produce analysed frames from whatever is buffered");
    }
};
static PitchTracker_FifoOverflow test_pitchTrackerFifoOverflow;

// =============================================================================
// Test 11: AudioEngine input listener receives blocks
// =============================================================================
class AudioEngine_InputListener : public juce::UnitTest
{
public:
    AudioEngine_InputListener() : UnitTest ("AudioEngine: input listener receives blocks", "Practice Mode") {}

    struct TestListener : public TracktionAudioEngine::AudioInputListener
    {
        int callCount = 0;
        int lastNumChannels = 0;
        int lastNumSamples = 0;
        double lastSampleRate = 0.0;

        void audioInputBlock (const float* const* inputChannels, int numInputChannels, int numSamples, double sampleRate) override
        {
            juce::ignoreUnused (inputChannels);
            ++callCount;
            lastNumChannels = numInputChannels;
            lastNumSamples = numSamples;
            lastSampleRate = sampleRate;
        }
    };

    void runTest() override
    {
        auto& engine = TracktionAudioEngine::getInstance();

        const int numSamples = 256;
        float inputChannel0[numSamples];
        float inputChannel1[numSamples];
        float outputChannel0[numSamples];
        float outputChannel1[numSamples];

        for (int i = 0; i < numSamples; ++i)
        {
            inputChannel0[i] = 0.1f;
            inputChannel1[i] = -0.1f;
        }

        const float* inputs[2] = { inputChannel0, inputChannel1 };
        float* outputs[2] = { outputChannel0, outputChannel1 };

        juce::AudioIODeviceCallbackContext context;

        TestListener listener;

        beginTest ("Listener receives one call with 2 channels / 256 samples");
        engine.setAudioInputListener (&listener);
        engine.audioDeviceIOCallbackWithContext (inputs, 2, outputs, 2, numSamples, context);

        expectEquals (listener.callCount, 1);
        expectEquals (listener.lastNumChannels, 2);
        expectEquals (listener.lastNumSamples, numSamples);
        expect (listener.lastSampleRate > 0.0, "Sample rate should be positive");

        beginTest ("Clearing the listener stops further calls");
        engine.setAudioInputListener (nullptr);
        engine.audioDeviceIOCallbackWithContext (inputs, 2, outputs, 2, numSamples, context);

        expectEquals (listener.callCount, 1);
    }
};
static AudioEngine_InputListener test_audioEngineInputListener;

// =============================================================================
// Test 12: PracticeMode pitchSampleToPoint and ingestPitchSamples
// =============================================================================
class PracticeMode_PitchIngest : public juce::UnitTest
{
public:
    PracticeMode_PitchIngest() : UnitTest ("PracticeMode: pitchSampleToPoint and ingestPitchSamples", "Practice Mode") {}
    void runTest() override
    {
        beginTest ("pitchSampleToPoint: basic beat conversion");
        {
            PitchSample sample;
            sample.timeSeconds = 2.0;
            sample.frequencyHz = 261.63;
            sample.midiPitch = 60.0;
            sample.confidence = 0.9f;
            sample.voiced = true;

            auto point = pitchSampleToPoint (sample, -4.0, 120.0, 0.0);
            expectWithinAbsoluteError (point.beat, 0.0, 1.0e-9);
            expectWithinAbsoluteError (point.midiPitch, 60.0, 1.0e-9);
            expectWithinAbsoluteError ((double) point.confidence, 0.9, 1.0e-6);

            beginTest ("pitchSampleToPoint: with mic latency");
            auto pointWithLatency = pitchSampleToPoint (sample, -4.0, 120.0, 0.5);
            expectWithinAbsoluteError (pointWithLatency.beat, -1.0, 1.0e-9);

            beginTest ("pitchSampleToPoint: unvoiced sample reports confidence 0");
            sample.voiced = false;
            auto unvoicedPoint = pitchSampleToPoint (sample, -4.0, 120.0, 0.0);
            expectEquals (unvoicedPoint.confidence, 0.0f);
        }

        beginTest ("Construct PracticeModeComponent with a Piano player");
        {
            PlayerManager manager;

            auto& engine = TracktionAudioEngine::getInstance();
            auto player = std::make_unique<Player> ("pitch_ingest_test_piano", "Piano",
                                                      engine.getFormatManager(), engine.getKnownPluginList());
            expect (player->initialize(), "Test Piano player should initialize");
            player->setName ("Piano");
            manager.addPlayer (std::move (player));

            PracticeModeComponent component (&manager);

            beginTest ("loadLessonFile loads Lesson I");
            component.loadLessonFile (getFixtureFile());
            expect (component.getCurrentLesson().has_value(), "Lesson should load");

            beginTest ("Mic toggle defaults ON");
            expect (component.getMicToggle().getToggleState(), "Mic toggle should default to true");

            beginTest ("ingestPitchSamples appends to trace and piano roll");
            std::vector<PitchSample> samples;
            for (int i = 0; i < 3; ++i)
            {
                PitchSample s;
                s.timeSeconds = (double) i * 0.1;
                s.frequencyHz = 220.0;
                s.midiPitch = 57.0;
                s.confidence = 0.8f;
                s.voiced = true;
                samples.push_back (s);
            }

            component.ingestPitchSamples (samples);

            expectEquals ((int) component.getCurrentTrace().size(), 3);
            expectEquals (component.getPianoRoll().getPitchTraceSize(), 3);
        }
    }
};
static PracticeMode_PitchIngest test_practiceModePitchIngest;
