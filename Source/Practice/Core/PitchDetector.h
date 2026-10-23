#pragma once
#include <JuceHeader.h>
#include <vector>

namespace covertracks {
namespace practice {

/**
    Single-pitch (monophonic) detector using the YIN algorithm
    (de Cheveigne & Kawahara, 2002).

    process() is allocation-free and lock-free: all scratch buffers are
    preallocated in the constructor, sized for the fixed frameSize given
    there. Intended to be called from the message thread (via PitchTracker)
    on short-ish frames (e.g. 2048 samples) at a steady hop rate.
*/
class PitchDetector
{
public:
    struct Result
    {
        double frequencyHz = 0.0;
        double midiPitch = 0.0;
        float confidence = 0.0f;
        bool voiced = false;
    };

    explicit PitchDetector (double sampleRate, int frameSize = 2048, double yinThreshold = 0.15);

    int getFrameSize() const { return frameSize; }

    /** numSamples must equal getFrameSize(); otherwise returns an unvoiced,
        zeroed Result and does not crash. */
    Result process (const float* frame, int numSamples);

    /** Sets the pitch search range; recomputes the internal tau (lag) search
        bounds. Default is 60..1200 Hz. */
    void setFrequencyRange (double minHz, double maxHz);

    /** RMS (in dBFS) below this is always treated as unvoiced. Default -50. */
    void setSilenceThresholdDb (double db);

    static double midiFromHz (double hz);
    static double hzFromMidi (double midi);

    /** Nearest equal-tempered note name + signed cents offset, e.g.
        "A4 +0c", "A4 +12c", "A4 -10c". Octave numbering: MIDI 60 == C4. */
    static juce::String noteNameWithCents (double midi);

private:
    void recomputeTauRange();

    double sampleRate;
    int frameSize;
    double yinThreshold;

    double minFrequencyHz = 60.0;
    double maxFrequencyHz = 1200.0;
    double silenceThresholdDb = -50.0;

    int tauMin = 1;
    int tauMax = 1;

    // Preallocated scratch buffers, sized to frameSize (an upper bound on
    // tauMax+1 given tauMax is always clamped below frameSize).
    std::vector<double> differenceFunction;
    std::vector<double> cmndf; // cumulative mean normalised difference function

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchDetector)
};

} // namespace practice
} // namespace covertracks
