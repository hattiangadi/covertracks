#pragma once
#include "MJ3Types.h"

namespace covertracks {
namespace mj3 {

/**
 * Stage 3: MODIFY
 * Apply named modification chains to the expanded tree.
 * Operations: add, remove, replace, set, scale, transpose
 */
class MJ3Modifier
{
public:
    /** Apply all modifications referenced in the tree */
    static Document modify(const Document& doc);

    /** Apply a single modification to a node */
    static Node applyModification(const Node& node, const Modification& mod);

private:
    static Node applyModificationsToNode(const Node& node, const Document& doc);

    // Individual operations
    static Node applyAdd(const Node& node, const Modification& mod);
    static Node applyRemove(const Node& node, const Modification& mod);
    static Node applyReplace(const Node& node, const Modification& mod);
    static Node applySet(const Node& node, const Modification& mod);
    static Node applyScale(const Node& node, const Modification& mod);
    static Node applyTranspose(const Node& node, const Modification& mod);

    // Parse a target path into segments
    struct TargetPath {
        juce::String nodeName;     // target child name (empty = this node)
        juce::String field;        // "events", "context.velocity", etc.
        int index = -1;            // array index (-1 = all/append)
    };
    static TargetPath parseTarget(const juce::String& target);

    // Helper: parse an event from a var
    static Event3 eventFromVar(const juce::var& value);
};

} // namespace mj3
} // namespace covertracks
