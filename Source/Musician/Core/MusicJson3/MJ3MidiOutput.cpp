#include "MJ3MidiOutput.h"

namespace covertracks {
namespace mj3 {

juce::var MJ3MidiOutput::toVar(const std::vector<ResolvedEvent>& events, double tempo)
{
    juce::DynamicObject::Ptr result = new juce::DynamicObject();

    juce::Array<juce::var> eventArray;

    for (const auto& event : events)
    {
        juce::DynamicObject::Ptr eventObj = new juce::DynamicObject();
        eventObj->setProperty("type", "note_on");
        eventObj->setProperty("timestamp", event.absoluteTimeBeat);
        eventObj->setProperty("note", event.midiNote);
        eventObj->setProperty("velocity", (double)event.velocity);
        eventObj->setProperty("channel", event.channel);

        if (event.durationBeats > 0)
            eventObj->setProperty("duration", event.durationBeats);

        // Optional microtonal metadata
        if (event.frequencyHz > 0 && event.frequencyHz != 0.0)
            eventObj->setProperty("frequency", event.frequencyHz);
        if (std::abs(event.pitchBendSemitones) > 0.001)
            eventObj->setProperty("pitch_bend", event.pitchBendSemitones);

        eventArray.add(juce::var(eventObj.get()));
    }

    result->setProperty("success", true);
    result->setProperty("tempo", tempo);
    result->setProperty("events", eventArray);

    return juce::var(result.get());
}

} // namespace mj3
} // namespace covertracks
