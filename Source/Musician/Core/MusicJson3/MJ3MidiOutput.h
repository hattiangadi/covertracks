#pragma once
#include "MJ3Types.h"

namespace covertracks {
namespace mj3 {

/**
 * Convert ResolvedEvents to juce::var format consumed by Player/Instrument.
 * Output format: {success: true, tempo: N, events: [{type, timestamp, note, velocity, duration, channel}]}
 */
class MJ3MidiOutput
{
public:
    /** Convert resolved events to the juce::var MIDI event chart format */
    static juce::var toVar(const std::vector<ResolvedEvent>& events, double tempo);
};

} // namespace mj3
} // namespace covertracks
