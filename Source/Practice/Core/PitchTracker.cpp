#include "PitchTracker.h"
#include <algorithm>

namespace covertracks {
namespace practice {

namespace
{
    constexpr double ringBufferSeconds = 8.0;
}

PitchTracker::PitchTracker (double sampleRateToUse, int frameSizeToUse, int hopSizeToUse)
    : sampleRate (sampleRateToUse),
      frameSize (frameSizeToUse),
      hopSize (hopSizeToUse),
      detector (sampleRateToUse, frameSizeToUse)
{
    int capacitySamples = juce::jmax (frameSize * 4, (int) std::ceil (sampleRate * ringBufferSeconds));
    fifo.setTotalSize (capacitySamples);
    ringBuffer.assign ((size_t) capacitySamples, 0.0f);

    channelAverageScratch.assign ((size_t) scratchCapacitySamples, 0.0f);

    accumulated.reserve ((size_t) (frameSize * 4));
}

PitchTracker::~PitchTracker() = default;

void PitchTracker::reset()
{
    fifo.reset();
    droppedSamples.store (0);

    accumulated.clear();
    accumulatedBaseIndex = 0;
    nextWindowStart = 0;
    pendingResults.clear();
}

void PitchTracker::pushSamples (const float* mono, int numSamples)
{
    if (mono == nullptr || numSamples <= 0)
        return;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    int written = 0;

    if (size1 > 0)
    {
        std::copy (mono, mono + size1, ringBuffer.begin() + start1);
        written += size1;
    }

    if (size2 > 0)
    {
        std::copy (mono + written, mono + written + size2, ringBuffer.begin() + start2);
        written += size2;
    }

    fifo.finishedWrite (written);

    if (written < numSamples)
        droppedSamples.fetch_add (numSamples - written);
}

void PitchTracker::pushInterleavedOrMultichannel (const float* const* channels, int numChannels, int numSamples)
{
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    int offset = 0;
    while (offset < numSamples)
    {
        int chunk = juce::jmin (scratchCapacitySamples, numSamples - offset);

        if (numChannels == 1)
        {
            std::copy (channels[0] + offset, channels[0] + offset + chunk, channelAverageScratch.begin());
        }
        else
        {
            float invN = 1.0f / (float) numChannels;
            for (int i = 0; i < chunk; ++i)
            {
                float sum = 0.0f;
                for (int c = 0; c < numChannels; ++c)
                    sum += channels[c][offset + i];
                channelAverageScratch[(size_t) i] = sum * invN;
            }
        }

        pushSamples (channelAverageScratch.data(), chunk);
        offset += chunk;
    }
}

int PitchTracker::analyse()
{
    // Pop everything currently available in the fifo into `accumulated`.
    int ready = fifo.getNumReady();
    if (ready > 0)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead (ready, start1, size1, start2, size2);

        auto oldSize = accumulated.size();
        accumulated.resize (oldSize + (size_t) size1 + (size_t) size2);

        if (size1 > 0)
            std::copy (ringBuffer.begin() + start1, ringBuffer.begin() + start1 + size1, accumulated.begin() + (long) oldSize);

        if (size2 > 0)
            std::copy (ringBuffer.begin() + start2, ringBuffer.begin() + start2 + size2,
                       accumulated.begin() + (long) oldSize + size1);

        fifo.finishedRead (size1 + size2);
    }

    long long accumulatedEndIndex = accumulatedBaseIndex + (long long) accumulated.size();

    int produced = 0;

    while (nextWindowStart + frameSize <= accumulatedEndIndex)
    {
        auto localStart = (size_t) (nextWindowStart - accumulatedBaseIndex);

        auto result = detector.process (accumulated.data() + localStart, frameSize);

        PitchSample sample;
        double centreIndex = (double) nextWindowStart + (double) frameSize / 2.0;
        sample.timeSeconds = centreIndex / sampleRate;
        sample.frequencyHz = result.frequencyHz;
        sample.midiPitch = result.midiPitch;
        sample.confidence = result.confidence;
        sample.voiced = result.voiced;

        pendingResults.push_back (sample);
        ++produced;

        nextWindowStart += hopSize;
    }

    // Trim everything before nextWindowStart: no future window needs it.
    if (nextWindowStart > accumulatedBaseIndex)
    {
        auto trimCount = (size_t) juce::jmin ((long long) accumulated.size(), nextWindowStart - accumulatedBaseIndex);
        accumulated.erase (accumulated.begin(), accumulated.begin() + (long) trimCount);
        accumulatedBaseIndex += (long long) trimCount;
    }

    return produced;
}

std::vector<PitchSample> PitchTracker::takeNewSamples()
{
    std::vector<PitchSample> result;
    result.swap (pendingResults);
    return result;
}

} // namespace practice
} // namespace covertracks
