#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include "PitchDetector.h"

namespace covertracks {
namespace practice {

/** One pitch-detector result, time-stamped relative to the last reset(). */
struct PitchSample
{
    double timeSeconds = 0.0;
    double frequencyHz = 0.0;
    double midiPitch = 0.0;
    float confidence = 0.0f;
    bool voiced = false;
};

/**
    Bridges a live audio stream to a time-stamped series of PitchDetector
    results.

    Single producer / single consumer:
      - pushSamples() / pushInterleavedOrMultichannel() are called from the
        audio thread. They are lock-free (a juce::AbstractFifo over a
        preallocated ring buffer) and never allocate; if the ring buffer is
        full, the newest samples are dropped and counted.
      - analyse() and takeNewSamples() are called from the message thread
        (e.g. a UI timer). analyse() pops whatever is currently available
        from the ring buffer and runs PitchDetector on a sliding window
        (frameSize samples, advancing hopSize samples per step), appending
        results to an internal pending list. takeNewSamples() moves that
        pending list out.

    reset() must only be called from the message thread while the audio
    thread is not pushing (i.e. the producer is stopped) - it is not itself
    synchronised against concurrent pushSamples() calls.
*/
class PitchTracker
{
public:
    PitchTracker (double sampleRate, int frameSize = 2048, int hopSize = 512);
    ~PitchTracker();

    /** Clears the fifo, the sample counters used for window placement and
        timestamping, and any pending (not yet taken) results. Message
        thread only; the audio-thread producer must not be pushing samples
        while this runs. */
    void reset();

    /** Audio thread. Pushes mono samples into the lock-free ring buffer.
        Drops (and counts) samples that don't fit. */
    void pushSamples (const float* mono, int numSamples);

    /** Audio thread. Averages numChannels channels down to mono (via a
        preallocated scratch buffer, processed in bounded chunks so this
        never allocates) and forwards to pushSamples(). */
    void pushInterleavedOrMultichannel (const float* const* channels, int numChannels, int numSamples);

    /** Message thread (consumer). Pops whatever is available in the ring
        buffer and runs the pitch detector on every complete hop-advance of
        the sliding window. Returns the number of new PitchSamples produced
        by this call (also obtainable via takeNewSamples()). */
    int analyse();

    /** Message thread. Moves out every PitchSample produced since the last
        call to takeNewSamples() (across any number of analyse() calls),
        clearing the pending list. */
    std::vector<PitchSample> takeNewSamples();

    double getSampleRate() const { return sampleRate; }
    int getFrameSize() const { return frameSize; }
    int getHopSize() const { return hopSize; }
    int getDroppedSamples() const { return droppedSamples.load(); }

private:
    double sampleRate;
    int frameSize;
    int hopSize;

    PitchDetector detector;

    // Lock-free ring buffer: audio thread writes, message thread reads.
    juce::AbstractFifo fifo { 1 };
    std::vector<float> ringBuffer;
    std::atomic<int> droppedSamples { 0 };

    // Bounded scratch buffer for channel averaging (pushInterleavedOrMultichannel
    // never allocates: it processes the input in chunks of at most this size).
    static constexpr int scratchCapacitySamples = 8192;
    std::vector<float> channelAverageScratch;

    // Consumer-side state (message thread only).
    std::vector<float> accumulated;       // raw samples not yet fully consumed into a window
    long long accumulatedBaseIndex = 0;   // global sample index (since reset) of accumulated[0]
    long long nextWindowStart = 0;        // global sample index where the next window begins
    std::vector<PitchSample> pendingResults;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchTracker)
};

} // namespace practice
} // namespace covertracks
