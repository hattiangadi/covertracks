#include "MJ3Flattener.h"
#include <algorithm>
#include <cmath>

namespace covertracks {
namespace mj3 {

std::vector<ResolvedEvent> MJ3Flattener::flatten(const std::vector<ResolvedEvent>& events)
{
    std::vector<ResolvedEvent> flat;

    for (const auto& event : events)
        collectEvents(event, flat);

    // Sort by absolute time, then by MIDI note for stable ordering
    std::sort(flat.begin(), flat.end(), [](const ResolvedEvent& a, const ResolvedEvent& b) {
        if (std::abs(a.absoluteTimeBeat - b.absoluteTimeBeat) > 0.0001)
            return a.absoluteTimeBeat < b.absoluteTimeBeat;
        return a.midiNote < b.midiNote;
    });

    // Remove exact duplicates (same time, same note, same duration)
    flat.erase(std::unique(flat.begin(), flat.end(),
        [](const ResolvedEvent& a, const ResolvedEvent& b) {
            return std::abs(a.absoluteTimeBeat - b.absoluteTimeBeat) < 0.0001 &&
                   a.midiNote == b.midiNote &&
                   std::abs(a.durationBeats - b.durationBeats) < 0.0001 &&
                   a.channel == b.channel;
        }), flat.end());

    return flat;
}

void MJ3Flattener::collectEvents(const ResolvedEvent& event,
                                  std::vector<ResolvedEvent>& output)
{
    if (!event.ornamentEvents.empty())
    {
        // If ornaments exist, use them instead of the base event
        for (const auto& ornEvent : event.ornamentEvents)
            collectEvents(ornEvent, output);
    }
    else
    {
        // Leaf event: add a copy without ornaments
        ResolvedEvent flat = event;
        flat.ornamentEvents.clear();
        output.push_back(flat);
    }
}

} // namespace mj3
} // namespace covertracks
