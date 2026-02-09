#pragma once
#include "MJ3Types.h"

namespace covertracks {
namespace mj3 {

/**
 * Stage 1: LOAD
 * Parse JSON string into a Document with definition registry,
 * modification chains, and root tree.
 */
class MJ3Loader
{
public:
    /** Parse a JSON string into a Document */
    static Document load(const juce::String& jsonString);

    /** Parse a juce::var (already-parsed JSON) into a Document */
    static Document loadFromVar(const juce::var& json);

    /** Parse a pitch from a JSON event object (public for use by Modifier) */
    static Pitch parsePitch(const juce::var& json);

private:
    static Context3 parseContext(const juce::var& json);
    static Cycle parseCycle(const juce::var& json);
    static Node parseNode(const juce::var& json);
    static Event3 parseEvent(const juce::var& json);
    static Ornament parseOrnament(const juce::var& json);
    static ModificationChain parseModificationChain(const juce::String& name, const juce::var& json);
    static Modification parseModification(const juce::var& json);

    /** Detect if a JSON object looks like an event rather than a container node.
     *  LLMs often put events in "children" instead of "events". */
    static bool looksLikeEvent(const juce::var& json);

    /** Detect rest-like objects (beat/duration but no pitch, no container props).
     *  LLMs generate these as silence markers; skip them to avoid inflating duration. */
    static bool looksLikeRest(const juce::var& json);

    /** Parse an event-like child, handling degree arrays */
    static void parseEventLikeChild(const juce::var& json, std::vector<Event3>& events);
};

} // namespace mj3
} // namespace covertracks
