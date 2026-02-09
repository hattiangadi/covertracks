#include "MJ3Expander.h"

namespace covertracks {
namespace mj3 {

Document MJ3Expander::expand(const Document& doc)
{
    Document result = doc;

    // Check for circular refs first
    auto circular = detectCircularRefs(doc);
    if (circular.isNotEmpty())
    {
        DBG("MJ3Expander: circular reference detected: " << circular);
        return result;
    }

    // Expand all definitions (they may reference each other)
    for (auto& [name, node] : result.definitions)
    {
        std::set<juce::String> visited;
        visited.insert(name);
        node = expandNode(node, doc, visited, 0);
    }

    // Expand the root
    std::set<juce::String> visited;
    result.root = expandNode(result.root, doc, visited, 0);

    return result;
}

juce::String MJ3Expander::detectCircularRefs(const Document& doc)
{
    // DFS with three-color marking to detect cycles in the definition graph.
    // UNVISITED → IN_PROGRESS (on current path) → DONE (fully explored).
    // A cycle exists when we encounter an IN_PROGRESS node.
    enum Color { UNVISITED, IN_PROGRESS, DONE };
    std::map<juce::String, Color> colors;

    for (const auto& [name, _] : doc.definitions)
        colors[name] = UNVISITED;

    // Collect unique ref names from a node and its children
    auto collectRefs = [](const Node& node) {
        std::set<juce::String> refs;
        std::function<void(const Node&)> gather = [&](const Node& n) {
            if (n.ref.isNotEmpty())
                refs.insert(n.ref);
            for (const auto& child : n.children)
                gather(child);
        };
        if (node.ref.isNotEmpty())
            refs.insert(node.ref);
        for (const auto& child : node.children)
            gather(child);
        return refs;
    };

    std::function<juce::String(const juce::String&)> dfs =
        [&](const juce::String& name) -> juce::String
    {
        colors[name] = IN_PROGRESS;

        auto it = doc.definitions.find(name);
        if (it != doc.definitions.end())
        {
            auto refs = collectRefs(it->second);
            for (const auto& ref : refs)
            {
                if (colors.count(ref) == 0)
                    continue;  // ref to non-existent definition

                if (colors[ref] == IN_PROGRESS)
                    return ref;  // cycle!

                if (colors[ref] == UNVISITED)
                {
                    auto result = dfs(ref);
                    if (result.isNotEmpty())
                        return result;
                }
                // DONE nodes are safe to skip
            }
        }

        colors[name] = DONE;
        return {};
    };

    for (const auto& [name, _] : doc.definitions)
    {
        if (colors[name] == UNVISITED)
        {
            auto result = dfs(name);
            if (result.isNotEmpty())
                return result;
        }
    }

    return {};
}

Node MJ3Expander::expandNode(const Node& node, const Document& doc,
                              std::set<juce::String>& visited, int depth)
{
    if (depth > MAX_DEPTH)
        return node;

    Node result = node;

    // Resolve ref: replace this node with the referenced definition's content
    if (result.ref.isNotEmpty())
    {
        auto it = doc.definitions.find(result.ref);
        if (it != doc.definitions.end() && visited.count(result.ref) == 0)
        {
            visited.insert(result.ref);

            // Start with the definition content
            Node resolved = it->second;

            // Preserve the referencing node's overrides
            if (result.name.isNotEmpty())
                resolved.name = result.name;
            if (result.repeat != 1)
                resolved.repeat = result.repeat;
            if (result.beats > 0.0)
                resolved.beats = result.beats;
            if (!result.modify.empty())
            {
                for (const auto& m : result.modify)
                    resolved.modify.push_back(m);
            }

            // Merge context: referencing node's context overrides definition's
            resolved.context = resolved.context.merge(result.context);

            result = resolved;
            result.ref = {};  // ref has been resolved

            visited.erase(it->first);
        }
    }

    // Recursively expand children
    for (auto& child : result.children)
        child = expandNode(child, doc, visited, depth + 1);

    // Unroll repeats
    result = unrollRepeats(result);

    return result;
}

double MJ3Expander::computeIterDuration(const Node& node)
{
    double maxEnd = 0.0;

    // Duration from direct events
    for (const auto& evt : node.events)
        maxEnd = std::max(maxEnd, evt.beat + evt.duration);

    // Duration from children
    if (node.mode == ContainerMode::Sequential)
    {
        double seqTotal = 0.0;
        for (const auto& child : node.children)
            seqTotal += computeIterDuration(child);
        maxEnd = std::max(maxEnd, seqTotal);
    }
    else // Simultaneous
    {
        for (const auto& child : node.children)
            maxEnd = std::max(maxEnd, computeIterDuration(child));
    }

    // Apply minimum duration floor if set
    if (node.beats > 0.0)
        maxEnd = std::max(maxEnd, node.beats);

    return maxEnd;
}

Node MJ3Expander::unrollRepeats(const Node& node)
{
    if (node.repeat <= 1)
        return node;

    // Create a template without the repeat
    Node templateNode = node;
    templateNode.repeat = 1;

    double iterDuration = computeIterDuration(templateNode);

    if (node.mode == ContainerMode::Sequential)
    {
        // Sequential repeat: duplicate events with beat offsets,
        // and duplicate children (Resolver handles their offsets).
        Node result;
        result.name = node.name;
        result.mode = node.mode;
        result.context = node.context;
        result.resolvedContext = node.resolvedContext;
        result.modify = node.modify;
        result.repeat = 1;

        for (int i = 0; i < node.repeat; ++i)
        {
            double beatOffset = iterDuration * i;
            for (const auto& child : templateNode.children)
                result.children.push_back(child);
            for (const auto& event : templateNode.events)
            {
                Event3 e = event;
                e.beat += beatOffset;
                result.events.push_back(e);
            }
        }
        return result;
    }
    else
    {
        // Simultaneous repeat: wrap N copies in a Sequential container.
        // "repeat 3 on a simultaneous block" means "play the block 3 times in sequence".
        Node wrapper;
        wrapper.name = node.name;
        wrapper.mode = ContainerMode::Sequential;
        wrapper.context = node.context;
        wrapper.resolvedContext = node.resolvedContext;
        wrapper.modify = node.modify;
        wrapper.repeat = 1;

        for (int i = 0; i < node.repeat; ++i)
            wrapper.children.push_back(templateNode);

        return wrapper;
    }
}

} // namespace mj3
} // namespace covertracks
