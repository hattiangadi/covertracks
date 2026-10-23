#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

namespace covertracks {
namespace practice {

PitchDetector::PitchDetector (double sampleRateToUse, int frameSizeToUse, double yinThresholdToUse)
    : sampleRate (sampleRateToUse),
      frameSize (frameSizeToUse),
      yinThreshold (yinThresholdToUse)
{
    // Upper bound: tauMax is always clamped to frameSize - 2, so sizing both
    // scratch buffers to frameSize is always sufficient and lets
    // setFrequencyRange() change the search range later without allocating.
    differenceFunction.assign ((size_t) frameSize, 0.0);
    cmndf.assign ((size_t) frameSize, 0.0);

    recomputeTauRange();
}

void PitchDetector::recomputeTauRange()
{
    if (maxFrequencyHz <= 0.0 || minFrequencyHz <= 0.0 || sampleRate <= 0.0)
    {
        tauMin = 1;
        tauMax = juce::jmax (2, frameSize - 2);
        return;
    }

    int newTauMin = (int) std::floor (sampleRate / maxFrequencyHz);
    int newTauMax = (int) std::ceil (sampleRate / minFrequencyHz);

    newTauMin = juce::jmax (1, newTauMin);
    newTauMax = juce::jmin (newTauMax, frameSize - 2);
    newTauMax = juce::jmax (newTauMax, newTauMin + 1);

    tauMin = newTauMin;
    tauMax = newTauMax;
}

void PitchDetector::setFrequencyRange (double minHz, double maxHz)
{
    minFrequencyHz = minHz;
    maxFrequencyHz = maxHz;
    recomputeTauRange();
}

void PitchDetector::setSilenceThresholdDb (double db)
{
    silenceThresholdDb = db;
}

PitchDetector::Result PitchDetector::process (const float* frame, int numSamples)
{
    Result result;

    if (frame == nullptr || numSamples != frameSize)
        return result;

    // RMS / silence gate -------------------------------------------------
    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        auto sample = (double) frame[i];
        sumSquares += sample * sample;
    }

    double rms = std::sqrt (sumSquares / (double) numSamples);

    if (rms <= 1.0e-9)
    {
        // True silence: avoid dividing by zero in the CMNDF below.
        result.voiced = false;
        return result;
    }

    double rmsDb = 20.0 * std::log10 (rms);
    bool aboveSilenceThreshold = rmsDb > silenceThresholdDb;

    // YIN difference function ---------------------------------------------
    // Integration window length: the remaining samples after reserving
    // tauMax lag samples at the end of the frame.
    int w = frameSize - tauMax;
    if (w <= 0)
    {
        result.voiced = false;
        return result;
    }

    differenceFunction[0] = 0.0;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        double sum = 0.0;
        for (int j = 0; j < w; ++j)
        {
            double diff = (double) frame[j] - (double) frame[j + tau];
            sum += diff * diff;
        }
        differenceFunction[(size_t) tau] = sum;
    }

    // Cumulative mean normalised difference function ----------------------
    cmndf[0] = 1.0;
    double runningSum = 0.0;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        runningSum += differenceFunction[(size_t) tau];
        if (runningSum <= 0.0)
            cmndf[(size_t) tau] = 1.0;
        else
            cmndf[(size_t) tau] = differenceFunction[(size_t) tau] * (double) tau / runningSum;
    }

    // Absolute threshold search, with fallback to global minimum ----------
    int chosenTau = -1;
    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (cmndf[(size_t) tau] < yinThreshold)
        {
            int localMin = tau;
            while (localMin + 1 <= tauMax && cmndf[(size_t) (localMin + 1)] < cmndf[(size_t) localMin])
                ++localMin;

            chosenTau = localMin;
            break;
        }
    }

    if (chosenTau < 0)
    {
        // No tau dipped below threshold: fall back to the global minimum
        // over the searched range.
        int bestTau = tauMin;
        double bestValue = cmndf[(size_t) tauMin];
        for (int tau = tauMin + 1; tau <= tauMax; ++tau)
        {
            if (cmndf[(size_t) tau] < bestValue)
            {
                bestValue = cmndf[(size_t) tau];
                bestTau = tau;
            }
        }
        chosenTau = bestTau;
    }

    // Parabolic interpolation around chosenTau -----------------------------
    double betterTau = (double) chosenTau;
    {
        int t0 = juce::jmax (1, chosenTau - 1);
        int t2 = juce::jmin (tauMax, chosenTau + 1);
        double y0 = cmndf[(size_t) t0];
        double y1 = cmndf[(size_t) chosenTau];
        double y2 = cmndf[(size_t) t2];

        double denom = y0 - 2.0 * y1 + y2;
        if (std::abs (denom) > 1.0e-12)
        {
            double shift = 0.5 * (y0 - y2) / denom;
            shift = juce::jlimit (-1.0, 1.0, shift);
            betterTau = (double) chosenTau + shift;
        }
    }

    double chosenCmndf = cmndf[(size_t) chosenTau];
    double confidence = juce::jlimit (0.0, 1.0, 1.0 - chosenCmndf);

    double frequencyHz = betterTau > 0.0 ? sampleRate / betterTau : 0.0;

    result.frequencyHz = frequencyHz;
    result.midiPitch = frequencyHz > 0.0 ? midiFromHz (frequencyHz) : 0.0;
    result.confidence = (float) confidence;
    result.voiced = aboveSilenceThreshold && confidence > 0.5;

    return result;
}

double PitchDetector::midiFromHz (double hz)
{
    if (hz <= 0.0)
        return 0.0;

    return 69.0 + 12.0 * std::log2 (hz / 440.0);
}

double PitchDetector::hzFromMidi (double midi)
{
    return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);
}

juce::String PitchDetector::noteNameWithCents (double midi)
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    int nearestNote = (int) std::lround (midi);
    double cents = (midi - (double) nearestNote) * 100.0;
    int roundedCents = (int) std::lround (cents);

    // Guard the (extremely rare) rounding edge case where rounding cents
    // pushes it to +/-100.
    if (roundedCents >= 100)
    {
        ++nearestNote;
        roundedCents -= 100;
    }
    else if (roundedCents <= -100)
    {
        --nearestNote;
        roundedCents += 100;
    }

    int pitchClass = ((nearestNote % 12) + 12) % 12;
    int octave = nearestNote / 12 - 1;

    juce::String sign = roundedCents < 0 ? "-" : "+";
    int absCents = std::abs (roundedCents);

    return juce::String (names[pitchClass]) + juce::String (octave) + " " + sign + juce::String (absCents) + "c";
}

} // namespace practice
} // namespace covertracks
