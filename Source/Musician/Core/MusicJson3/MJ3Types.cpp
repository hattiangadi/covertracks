#include "MJ3Types.h"

namespace covertracks {
namespace mj3 {

Context3 Context3::merge(const Context3& child) const
{
    Context3 result = *this;

    if (child.hasModeSystem())
        result.modeSystem = child.modeSystem;
    if (child.hasMode())
        result.mode = child.mode;
    if (child.cycle.beats != 4 || child.cycle.name.isNotEmpty())
        result.cycle = child.cycle;
    if (child.hasTuningSystem())
        result.tuningSystem = child.tuningSystem;
    if (child.hasReferenceFrequency())
        result.referenceFrequency = child.referenceFrequency;
    if (child.hasVelocity())
        result.velocity = child.velocity;
    if (child.hasChannel())
        result.channel = child.channel;

    // Merge extras: child properties override parent
    if (child.extras.isObject())
    {
        if (!result.extras.isObject())
            result.extras = child.extras;
        else
        {
            // Merge child properties into result
            if (auto* childObj = child.extras.getDynamicObject())
            {
                if (auto* resultObj = result.extras.getDynamicObject())
                {
                    for (const auto& prop : childObj->getProperties())
                        resultObj->setProperty(prop.name, prop.value);
                }
            }
        }
    }

    return result;
}

} // namespace mj3
} // namespace covertracks
