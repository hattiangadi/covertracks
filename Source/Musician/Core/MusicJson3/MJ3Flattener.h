#pragma once
#include "MJ3Types.h"

namespace covertracks {
namespace mj3 {

/**
 * Stage 6: FLATTEN
 * Take a list of ResolvedEvents (which may contain nested ornament events),
 * expand ornaments into the flat list, sort by absolute time, remove duplicates.
 */
class MJ3Flattener
{
public:
    /** Flatten: expand ornaments, sort by time, dedup */
    static std::vector<ResolvedEvent> flatten(const std::vector<ResolvedEvent>& events);

private:
    static void collectEvents(const ResolvedEvent& event,
                             std::vector<ResolvedEvent>& output);
};

} // namespace mj3
} // namespace covertracks
