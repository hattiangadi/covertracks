#include "MJ3Modifier.h"
#include "MJ3Loader.h"

namespace covertracks {
namespace mj3 {

Document MJ3Modifier::modify(const Document& doc)
{
    Document result = doc;

    // Process root and all definitions
    result.root = applyModificationsToNode(result.root, doc);

    for (auto& [name, node] : result.definitions)
        node = applyModificationsToNode(node, doc);

    return result;
}

Node MJ3Modifier::applyModificationsToNode(const Node& node, const Document& doc)
{
    Node result = node;

    // Apply this node's modification chains
    for (const auto& modName : result.modify)
    {
        auto it = doc.modifications.find(modName);
        if (it != doc.modifications.end())
        {
            for (const auto& mod : it->second.operations)
                result = applyModification(result, mod);
        }
    }
    result.modify.clear();  // consumed

    // Recurse into children
    for (auto& child : result.children)
        child = applyModificationsToNode(child, doc);

    return result;
}

Node MJ3Modifier::applyModification(const Node& node, const Modification& mod)
{
    if (mod.operation == "add")
        return applyAdd(node, mod);
    if (mod.operation == "remove")
        return applyRemove(node, mod);
    if (mod.operation == "replace")
        return applyReplace(node, mod);
    if (mod.operation == "set")
        return applySet(node, mod);
    if (mod.operation == "scale")
        return applyScale(node, mod);
    if (mod.operation == "transpose")
        return applyTranspose(node, mod);

    return node;
}

MJ3Modifier::TargetPath MJ3Modifier::parseTarget(const juce::String& target)
{
    TargetPath path;

    auto parts = juce::StringArray::fromTokens(target, ".", "");
    if (parts.size() == 0)
        return path;

    // Check if first part is a known field name (strip brackets for comparison)
    auto firstBase = parts[0];
    auto bracketCheck = firstBase.indexOf("[");
    if (bracketCheck >= 0)
        firstBase = firstBase.substring(0, bracketCheck);

    bool isField = (firstBase == "events" || firstBase == "context" ||
                    firstBase == "velocity" || firstBase == "duration" ||
                    firstBase == "children");

    if (!isField)
    {
        path.nodeName = parts[0];
        parts.remove(0);
    }

    if (parts.size() > 0)
    {
        auto field = parts[0];
        // Check for array index: "events[2]"
        auto bracketPos = field.indexOf("[");
        if (bracketPos >= 0)
        {
            auto indexStr = field.substring(bracketPos + 1, field.indexOf("]"));
            path.index = indexStr.getIntValue();
            field = field.substring(0, bracketPos);
        }
        path.field = field;

        // If there are more parts, append them
        for (int i = 1; i < parts.size(); ++i)
            path.field += "." + parts[i];
    }

    return path;
}

Event3 MJ3Modifier::eventFromVar(const juce::var& value)
{
    // Re-use loader's event parsing by constructing a JSON-like var
    if (value.isObject())
    {
        // Parse pitch
        Event3 event;
        event.pitch = MJ3Loader::parsePitch(value);
        if (value.hasProperty("beat"))
            event.beat = (double)value["beat"];
        if (value.hasProperty("duration"))
            event.duration = (double)value["duration"];
        if (value.hasProperty("velocity"))
            event.velocity = (float)(double)value["velocity"];
        if (value.hasProperty("channel"))
            event.channel = (int)value["channel"];
        return event;
    }
    return Event3{};
}

Node MJ3Modifier::applyAdd(const Node& node, const Modification& mod)
{
    auto path = parseTarget(mod.target);
    Node result = node;

    // Find the target node
    Node* target = &result;
    if (path.nodeName.isNotEmpty())
    {
        for (auto& child : result.children)
        {
            if (child.name == path.nodeName)
            {
                target = &child;
                break;
            }
        }
    }

    if (path.field == "events")
    {
        // Add an event
        auto event = eventFromVar(mod.value);
        if (path.index >= 0 && path.index <= (int)target->events.size())
            target->events.insert(target->events.begin() + path.index, event);
        else
            target->events.push_back(event);
    }
    else if (path.field == "children")
    {
        // Add a child node
        Node child;
        if (mod.value.isObject())
        {
            if (mod.value.hasProperty("name"))
                child.name = mod.value["name"].toString();
            if (mod.value.hasProperty("events") && mod.value["events"].isArray())
            {
                auto& events = mod.value["events"];
                for (int i = 0; i < events.size(); ++i)
                    child.events.push_back(eventFromVar(events[i]));
            }
        }
        target->children.push_back(child);
    }

    return result;
}

Node MJ3Modifier::applyRemove(const Node& node, const Modification& mod)
{
    auto path = parseTarget(mod.target);
    Node result = node;

    Node* target = &result;
    if (path.nodeName.isNotEmpty())
    {
        for (auto& child : result.children)
        {
            if (child.name == path.nodeName)
            {
                target = &child;
                break;
            }
        }
    }

    if (path.field == "events" && path.index >= 0 &&
        path.index < (int)target->events.size())
    {
        target->events.erase(target->events.begin() + path.index);
    }
    else if (path.field == "children" && path.index >= 0 &&
             path.index < (int)target->children.size())
    {
        target->children.erase(target->children.begin() + path.index);
    }

    return result;
}

Node MJ3Modifier::applyReplace(const Node& node, const Modification& mod)
{
    auto path = parseTarget(mod.target);
    Node result = node;

    Node* target = &result;
    if (path.nodeName.isNotEmpty())
    {
        for (auto& child : result.children)
        {
            if (child.name == path.nodeName)
            {
                target = &child;
                break;
            }
        }
    }

    if (path.field == "events" && path.index >= 0 &&
        path.index < (int)target->events.size())
    {
        target->events[(size_t)path.index] = eventFromVar(mod.value);
    }

    return result;
}

Node MJ3Modifier::applySet(const Node& node, const Modification& mod)
{
    auto path = parseTarget(mod.target);
    Node result = node;

    Node* target = &result;
    if (path.nodeName.isNotEmpty())
    {
        for (auto& child : result.children)
        {
            if (child.name == path.nodeName)
            {
                target = &child;
                break;
            }
        }
    }

    if (path.field == "context.velocity")
        target->context.velocity = (float)(double)mod.value;
    else if (path.field == "context.channel")
        target->context.channel = (int)mod.value;
    else if (path.field == "context.mode")
        target->context.mode = mod.value.toString();
    else if (path.field == "context.mode_system")
        target->context.modeSystem = mod.value.toString();
    else if (path.field == "context.tuning_system")
        target->context.tuningSystem = mod.value.toString();

    return result;
}

Node MJ3Modifier::applyScale(const Node& node, const Modification& mod)
{
    auto path = parseTarget(mod.target);
    Node result = node;

    Node* target = &result;
    if (path.nodeName.isNotEmpty())
    {
        for (auto& child : result.children)
        {
            if (child.name == path.nodeName)
            {
                target = &child;
                break;
            }
        }
    }

    double factor = (double)mod.value;

    if (path.field == "velocity" || path.field == "events.velocity")
    {
        for (auto& event : target->events)
        {
            if (event.hasVelocity())
                event.velocity *= (float)factor;
        }
        if (target->context.hasVelocity())
            target->context.velocity *= (float)factor;
    }
    else if (path.field == "duration" || path.field == "events.duration")
    {
        for (auto& event : target->events)
            event.duration *= factor;
    }

    return result;
}

Node MJ3Modifier::applyTranspose(const Node& node, const Modification& mod)
{
    Node result = node;

    int semitones = (int)mod.value;

    // Transpose all events in this node and children
    std::function<void(Node&)> transposeNode = [&](Node& n) {
        for (auto& event : n.events)
        {
            // Only transpose MidiPitch and NotePitch
            if (auto* mp = std::get_if<MidiPitch>(&event.pitch))
                event.pitch = MidiPitch{mp->midi + semitones};
            else if (auto* dp = std::get_if<DegreePitch>(&event.pitch))
            {
                // For degrees, transpose shifts the degree
                event.pitch = DegreePitch{dp->degree + semitones, dp->octave};
            }
        }
        for (auto& child : n.children)
            transposeNode(child);
    };

    transposeNode(result);
    return result;
}

} // namespace mj3
} // namespace covertracks
