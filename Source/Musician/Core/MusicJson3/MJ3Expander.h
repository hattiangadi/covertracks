#pragma once
#include "MJ3Types.h"
#include <set>

namespace covertracks {
namespace mj3 {

/**
 * Stage 2: EXPAND
 * Resolve references, unroll repeats, detect circular references.
 * Input: Document with refs and repeats.
 * Output: Document with all refs resolved and repeats unrolled.
 */
class MJ3Expander
{
public:
    /** Expand all refs and repeats in the document. Returns expanded copy. */
    static Document expand(const Document& doc);

    /** Check for circular references. Returns empty string if none found,
        or the name of the first circular ref detected. */
    static juce::String detectCircularRefs(const Document& doc);

private:
    static Node expandNode(const Node& node, const Document& doc,
                          std::set<juce::String>& visited, int depth);
    static Node unrollRepeats(const Node& node);

    /** Compute duration of one iteration of a node (events + children). */
    static double computeIterDuration(const Node& node);

    static constexpr int MAX_DEPTH = 100;
};

} // namespace mj3
} // namespace covertracks
