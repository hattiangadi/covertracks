#include "MJ3Loader.h"

namespace covertracks {
namespace mj3 {

Document MJ3Loader::load(const juce::String& jsonString)
{
    auto json = juce::JSON::parse(jsonString);
    return loadFromVar(json);
}

Document MJ3Loader::loadFromVar(const juce::var& json)
{
    Document doc;

    if (!json.isObject())
        return doc;

    doc.version = json.getProperty("version", "3.0").toString();

    // Parse top-level context
    if (json.hasProperty("context"))
        doc.context = parseContext(json["context"]);

    // Parse imports
    if (json.hasProperty("import"))
    {
        auto& imports = json["import"];
        if (imports.isArray())
        {
            for (int i = 0; i < imports.size(); ++i)
            {
                Import imp;
                if (imports[i].isString())
                {
                    imp.library = imports[i].toString();
                }
                else if (imports[i].isObject() && imports[i].hasProperty("library"))
                {
                    imp.library = imports[i]["library"].toString();
                    if (imports[i].hasProperty("as"))
                        imp.alias = imports[i]["as"].toString();
                }
                if (imp.library.isNotEmpty())
                    doc.imports.push_back(imp);
            }
        }
    }

    // Parse extends (for library files)
    if (json.hasProperty("extends"))
        doc.extends = json["extends"].toString();

    // Parse definitions
    if (json.hasProperty("define"))
    {
        auto& defs = json["define"];
        if (defs.isObject())
        {
            if (auto* obj = defs.getDynamicObject())
            {
                for (const auto& prop : obj->getProperties())
                    doc.definitions[prop.name.toString()] = parseNode(prop.value);
            }
        }
    }

    // Parse modifications
    if (json.hasProperty("modify"))
    {
        auto& mods = json["modify"];
        if (mods.isObject())
        {
            if (auto* obj = mods.getDynamicObject())
            {
                for (const auto& prop : obj->getProperties())
                    doc.modifications[prop.name.toString()] =
                        parseModificationChain(prop.name.toString(), prop.value);
            }
        }
    }

    // Parse root
    if (json.hasProperty("root"))
        doc.root = parseNode(json["root"]);

    return doc;
}

Context3 MJ3Loader::parseContext(const juce::var& json)
{
    Context3 ctx;
    if (!json.isObject())
        return ctx;

    if (json.hasProperty("mode_system"))
        ctx.modeSystem = json["mode_system"].toString();
    if (json.hasProperty("mode"))
        ctx.mode = json["mode"].toString();
    if (json.hasProperty("tuning_system"))
        ctx.tuningSystem = json["tuning_system"].toString();
    if (json.hasProperty("reference_frequency"))
        ctx.referenceFrequency = (double)json["reference_frequency"];
    if (json.hasProperty("velocity"))
        ctx.velocity = (float)(double)json["velocity"];
    if (json.hasProperty("channel"))
        ctx.channel = (int)json["channel"];

    // Parse cycle
    if (json.hasProperty("cycle"))
        ctx.cycle = parseCycle(json["cycle"]);

    // Parse extras
    if (json.hasProperty("extras"))
        ctx.extras = json["extras"];

    return ctx;
}

Cycle MJ3Loader::parseCycle(const juce::var& json)
{
    Cycle cycle;
    if (json.isObject())
    {
        if (json.hasProperty("beats"))
            cycle.beats = (int)json["beats"];
        if (json.hasProperty("name"))
            cycle.name = json["name"].toString();
    }
    else if (json.isInt() || json.isDouble())
    {
        cycle.beats = (int)json;
    }
    return cycle;
}

Node MJ3Loader::parseNode(const juce::var& json)
{
    Node node;
    if (!json.isObject())
        return node;

    if (json.hasProperty("name"))
        node.name = json["name"].toString();

    // Container mode
    if (json.hasProperty("mode"))
    {
        auto modeStr = json["mode"].toString();
        if (modeStr == "simultaneous")
            node.mode = ContainerMode::Simultaneous;
        else
            node.mode = ContainerMode::Sequential;
    }

    // Reference
    if (json.hasProperty("ref"))
        node.ref = json["ref"].toString();

    // Repeat
    if (json.hasProperty("repeat"))
        node.repeat = (int)json["repeat"];

    // Minimum duration in beats
    if (json.hasProperty("beats"))
        node.beats = (double)json["beats"];

    // Modify list
    if (json.hasProperty("modify"))
    {
        auto& modList = json["modify"];
        if (modList.isArray())
        {
            for (int i = 0; i < modList.size(); ++i)
                node.modify.push_back(modList[i].toString());
        }
        else if (modList.isString())
        {
            node.modify.push_back(modList.toString());
        }
    }

    // Local context
    if (json.hasProperty("context"))
        node.context = parseContext(json["context"]);

    // Children — detect event-like objects that LLMs put in children instead of events
    if (json.hasProperty("children"))
    {
        auto& children = json["children"];
        if (children.isArray())
        {
            for (int i = 0; i < children.size(); ++i)
            {
                if (looksLikeEvent(children[i]))
                    parseEventLikeChild(children[i], node.events);
                else if (looksLikeRest(children[i]))
                    ; // Skip rest-like objects — they'd become empty nodes with wrong duration
                else
                    node.children.push_back(parseNode(children[i]));
            }
        }
    }

    // Events
    if (json.hasProperty("events"))
    {
        auto& events = json["events"];
        if (events.isArray())
        {
            for (int i = 0; i < events.size(); ++i)
            {
                auto& evtJson = events[i];
                // Expand degree arrays into multiple simultaneous events
                if (evtJson.hasProperty("degree") && evtJson["degree"].isArray())
                {
                    auto* degrees = evtJson["degree"].getArray();
                    for (int d = 0; d < degrees->size(); ++d)
                    {
                        // Create a copy of the event JSON with a single degree
                        juce::DynamicObject::Ptr single = new juce::DynamicObject();
                        if (auto* obj = evtJson.getDynamicObject())
                        {
                            for (auto& prop : obj->getProperties())
                            {
                                if (prop.name.toString() != "degree")
                                    single->setProperty(prop.name, prop.value);
                            }
                        }
                        single->setProperty("degree", (*degrees)[d]);
                        node.events.push_back(parseEvent(juce::var(single.get())));
                    }
                }
                else
                {
                    node.events.push_back(parseEvent(evtJson));
                }
            }
        }
    }

    return node;
}

Event3 MJ3Loader::parseEvent(const juce::var& json)
{
    Event3 event;
    if (!json.isObject())
        return event;

    // Parse pitch (check each type)
    event.pitch = parsePitch(json);

    // Timing
    if (json.hasProperty("beat"))
        event.beat = (double)json["beat"];
    if (json.hasProperty("duration"))
        event.duration = (double)json["duration"];

    // Overrides — normalize velocity: LLMs often output 0-127 MIDI-style instead of 0.0-1.0
    if (json.hasProperty("velocity"))
    {
        float vel = (float)(double)json["velocity"];
        if (vel > 1.0f)
            vel = vel / 127.0f;
        event.velocity = vel;
    }
    if (json.hasProperty("channel"))
        event.channel = (int)json["channel"];

    // Ornaments
    if (json.hasProperty("ornaments"))
    {
        auto& orns = json["ornaments"];
        if (orns.isArray())
        {
            for (int i = 0; i < orns.size(); ++i)
                event.ornaments.push_back(parseOrnament(orns[i]));
        }
    }

    // Extras
    if (json.hasProperty("extras"))
        event.extras = json["extras"];

    return event;
}

Pitch MJ3Loader::parsePitch(const juce::var& json)
{
    if (json.hasProperty("note"))
        return NotePitch{json["note"].toString()};

    if (json.hasProperty("degree"))
    {
        int oct = json.hasProperty("octave") ? (int)json["octave"] : -1;
        return DegreePitch{(int)json["degree"], oct};
    }

    if (json.hasProperty("drum"))
        return DrumPitch{json["drum"].toString()};

    if (json.hasProperty("sound"))
        return DrumPitch{json["sound"].toString()};

    if (json.hasProperty("cents"))
        return CentsPitch{(double)json["cents"]};

    if (json.hasProperty("swara"))
        return SwaraPitch{json["swara"].toString()};

    if (json.hasProperty("maqam_degree"))
    {
        double micro = json.hasProperty("micro") ? (double)json["micro"] : 0.0;
        return MaqamDegreePitch{(int)json["maqam_degree"], micro};
    }

    if (json.hasProperty("ratio"))
    {
        auto& ratio = json["ratio"];
        if (ratio.isArray() && ratio.size() >= 2)
            return RatioPitch{(int)ratio[0], (int)ratio[1]};
    }

    if (json.hasProperty("midi"))
        return MidiPitch{(int)json["midi"]};

    if (json.hasProperty("tone"))
    {
        juce::String tuning = json.hasProperty("tuning") ? json["tuning"].toString() : "";
        return GamelanPitch{(int)json["tone"], tuning};
    }

    if (json.hasProperty("frequency"))
        return FrequencyPitch{(double)json["frequency"]};

    // Default: degree 1
    return DegreePitch{1};
}

Ornament MJ3Loader::parseOrnament(const juce::var& json)
{
    Ornament orn;
    if (json.isObject())
    {
        if (json.hasProperty("type"))
            orn.type = json["type"].toString();
        if (json.hasProperty("params"))
            orn.params = json["params"];
    }
    return orn;
}

ModificationChain MJ3Loader::parseModificationChain(const juce::String& name, const juce::var& json)
{
    ModificationChain chain;
    chain.name = name;

    if (json.isArray())
    {
        for (int i = 0; i < json.size(); ++i)
            chain.operations.push_back(parseModification(json[i]));
    }
    else if (json.isObject())
    {
        chain.operations.push_back(parseModification(json));
    }

    return chain;
}

Modification MJ3Loader::parseModification(const juce::var& json)
{
    Modification mod;
    if (!json.isObject())
        return mod;

    if (json.hasProperty("operation"))
        mod.operation = json["operation"].toString();
    if (json.hasProperty("target"))
        mod.target = json["target"].toString();
    if (json.hasProperty("value"))
        mod.value = json["value"];

    return mod;
}

bool MJ3Loader::looksLikeEvent(const juce::var& json)
{
    if (!json.isObject())
        return false;

    // Must have a pitch property ("sound" is an LLM alias for "drum")
    bool hasPitch = json.hasProperty("degree") || json.hasProperty("note")
                 || json.hasProperty("drum")   || json.hasProperty("sound")
                 || json.hasProperty("midi")   || json.hasProperty("cents")
                 || json.hasProperty("swara")  || json.hasProperty("frequency");
    if (!hasPitch)
        return false;

    // Must have timing
    bool hasTiming = json.hasProperty("beat") || json.hasProperty("duration");
    if (!hasTiming)
        return false;

    // Must NOT have container properties (children, events, ref)
    if (json.hasProperty("children") || json.hasProperty("events") || json.hasProperty("ref"))
        return false;

    return true;
}

bool MJ3Loader::looksLikeRest(const juce::var& json)
{
    if (!json.isObject())
        return false;

    // Has timing (beat/duration) but NO pitch — looks like a rest/silence
    bool hasTiming = json.hasProperty("beat") || json.hasProperty("duration");
    if (!hasTiming)
        return false;

    // Must NOT have any pitch property ("sound" is an LLM alias for "drum")
    bool hasPitch = json.hasProperty("degree") || json.hasProperty("note")
                 || json.hasProperty("drum")   || json.hasProperty("sound")
                 || json.hasProperty("midi")   || json.hasProperty("cents")
                 || json.hasProperty("swara")  || json.hasProperty("frequency");
    if (hasPitch)
        return false;

    // Must NOT have container properties (those are real containers)
    if (json.hasProperty("children") || json.hasProperty("events") || json.hasProperty("ref"))
        return false;

    return true;
}

void MJ3Loader::parseEventLikeChild(const juce::var& json, std::vector<Event3>& events)
{
    // Handle degree arrays (chord voicings like {"degree": [1, 3, 5, 7]})
    if (json.hasProperty("degree") && json["degree"].isArray())
    {
        auto* degrees = json["degree"].getArray();
        for (int d = 0; d < degrees->size(); ++d)
        {
            juce::DynamicObject::Ptr single = new juce::DynamicObject();
            if (auto* obj = json.getDynamicObject())
            {
                for (auto& prop : obj->getProperties())
                {
                    if (prop.name.toString() != "degree")
                        single->setProperty(prop.name, prop.value);
                }
            }
            single->setProperty("degree", (*degrees)[d]);
            events.push_back(parseEvent(juce::var(single.get())));
        }
    }
    else
    {
        events.push_back(parseEvent(json));
    }
}

} // namespace mj3
} // namespace covertracks
