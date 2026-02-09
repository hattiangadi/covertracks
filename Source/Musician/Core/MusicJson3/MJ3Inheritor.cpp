#include "MJ3Inheritor.h"

namespace covertracks {
namespace mj3 {

Document MJ3Inheritor::inherit(const Document& doc)
{
    Document result = doc;

    // Start with the document-level context
    Context3 rootContext = doc.context;

    // Apply defaults for unset fields
    if (!rootContext.hasModeSystem())
        rootContext.modeSystem = "chord";
    if (!rootContext.hasTuningSystem())
        rootContext.tuningSystem = "12tet";
    if (!rootContext.hasReferenceFrequency())
        rootContext.referenceFrequency = 440.0;
    if (!rootContext.hasVelocity())
        rootContext.velocity = 0.8f;
    if (!rootContext.hasChannel())
        rootContext.channel = 1;

    result.root = inheritNode(result.root, rootContext);

    return result;
}

Node MJ3Inheritor::inheritNode(const Node& node, const Context3& parentContext)
{
    Node result = node;

    // Merge: parent context + this node's local overrides
    result.resolvedContext = parentContext.merge(node.context);

    // Recurse into children
    for (auto& child : result.children)
        child = inheritNode(child, result.resolvedContext);

    return result;
}

} // namespace mj3
} // namespace covertracks
