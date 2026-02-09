#pragma once
#include "MJ3Types.h"

namespace covertracks {
namespace mj3 {

/**
 * Stage 4: INHERIT
 * Propagate context down the tree. Each node's resolvedContext
 * is the merge of its parent's resolvedContext + its own context overrides.
 */
class MJ3Inheritor
{
public:
    /** Propagate context through the entire document */
    static Document inherit(const Document& doc);

private:
    static Node inheritNode(const Node& node, const Context3& parentContext);
};

} // namespace mj3
} // namespace covertracks
