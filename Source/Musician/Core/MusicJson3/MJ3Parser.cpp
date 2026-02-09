#include "MJ3Parser.h"

namespace covertracks {
namespace mj3 {

MJ3Parser::MJ3Parser()
{
    document.version = "3.0";
}

// ── Static API ──────────────────────────────────────────────────

juce::var MJ3Parser::parseVar(const juce::var& json, double tempo)
{
    auto doc = MJ3Loader::loadFromVar(json);
    return runPipeline(doc, tempo);
}

juce::var MJ3Parser::parse(const juce::String& jsonString, double tempo)
{
    auto doc = MJ3Loader::load(jsonString);
    return runPipeline(doc, tempo);
}

// ── Incremental API ─────────────────────────────────────────────

void MJ3Parser::setContext(const juce::var& contextJson)
{
    if (contextJson.isObject())
    {
        // Parse context using the loader's context parsing (via a wrapper document)
        juce::DynamicObject::Ptr wrapper = new juce::DynamicObject();
        wrapper->setProperty("version", "3.0");
        wrapper->setProperty("context", contextJson);
        wrapper->setProperty("root", juce::var(new juce::DynamicObject()));

        auto tempDoc = MJ3Loader::loadFromVar(juce::var(wrapper.get()));
        document.context = tempDoc.context;
    }
}

void MJ3Parser::addDefinition(const juce::String& name, const juce::var& nodeJson)
{
    // Parse the node JSON
    juce::DynamicObject::Ptr wrapper = new juce::DynamicObject();
    wrapper->setProperty("version", "3.0");

    juce::DynamicObject::Ptr define = new juce::DynamicObject();
    define->setProperty(name, nodeJson);
    wrapper->setProperty("define", juce::var(define.get()));
    wrapper->setProperty("root", juce::var(new juce::DynamicObject()));

    auto tempDoc = MJ3Loader::loadFromVar(juce::var(wrapper.get()));

    if (tempDoc.definitions.count(name))
        document.definitions[name] = tempDoc.definitions[name];
}

void MJ3Parser::addModification(const juce::String& name, const juce::var& modJson)
{
    juce::DynamicObject::Ptr wrapper = new juce::DynamicObject();
    wrapper->setProperty("version", "3.0");

    juce::DynamicObject::Ptr modify = new juce::DynamicObject();
    modify->setProperty(name, modJson);
    wrapper->setProperty("modify", juce::var(modify.get()));
    wrapper->setProperty("root", juce::var(new juce::DynamicObject()));

    auto tempDoc = MJ3Loader::loadFromVar(juce::var(wrapper.get()));

    if (tempDoc.modifications.count(name))
        document.modifications[name] = tempDoc.modifications[name];
}

void MJ3Parser::setRoot(const juce::var& rootJson)
{
    juce::DynamicObject::Ptr wrapper = new juce::DynamicObject();
    wrapper->setProperty("version", "3.0");
    wrapper->setProperty("root", rootJson);

    auto tempDoc = MJ3Loader::loadFromVar(juce::var(wrapper.get()));
    document.root = tempDoc.root;
}

juce::var MJ3Parser::resolve(double tempo)
{
    return runPipeline(document, tempo);
}

juce::var MJ3Parser::runPipelineFromDoc(const Document& doc, double tempo)
{
    return runPipeline(doc, tempo);
}

// ── Pipeline ────────────────────────────────────────────────────

juce::var MJ3Parser::runPipeline(const Document& doc, double tempo)
{
    // Stage 1: LOAD — already done (doc is the loaded document)

    // Stage 2: EXPAND — resolve refs, unroll repeats
    auto expanded = MJ3Expander::expand(doc);

    // Stage 3: MODIFY — apply modification chains
    auto modified = MJ3Modifier::modify(expanded);

    // Stage 4: INHERIT — propagate context
    auto inherited = MJ3Inheritor::inherit(modified);

    // Stage 5: RESOLVE — convert abstract → concrete
    MJ3Resolver resolver;
    auto resolved = resolver.resolve(inherited);

    // Stage 6: FLATTEN — expand ornaments, sort, dedup
    auto flat = MJ3Flattener::flatten(resolved);

    // Convert to Player-compatible var format
    return MJ3MidiOutput::toVar(flat, tempo);
}

} // namespace mj3
} // namespace covertracks
