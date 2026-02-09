#pragma once
#include "MJ3Types.h"
#include "MJ3Loader.h"
#include "MJ3Expander.h"
#include "MJ3Modifier.h"
#include "MJ3Inheritor.h"
#include "MJ3Resolver.h"
#include "MJ3Flattener.h"
#include "MJ3MidiOutput.h"
#include "MJ3LibraryLoader.h"

namespace covertracks {
namespace mj3 {

/**
 * Music JSON 3.0 Parser — entry point.
 *
 * Static API for one-shot parsing:
 *   auto result = MJ3Parser::parse(jsonVar, 120.0);
 *
 * Incremental API for conversational (LLM) workflow:
 *   MJ3Parser parser;
 *   parser.setContext(contextJson);
 *   parser.addDefinition("bass", bassJson);
 *   parser.setRoot(rootJson);
 *   auto result = parser.resolve(120.0);
 */
class MJ3Parser
{
public:
    MJ3Parser();

    // ── Static API ──────────────────────────────────────────────
    /** Parse a complete JSON 3.0 document from already-parsed var */
    static juce::var parseVar(const juce::var& json, double tempo);

    /** Parse from JSON string */
    static juce::var parse(const juce::String& jsonString, double tempo);

    // ── Incremental API ─────────────────────────────────────────
    /** Set the document-level context */
    void setContext(const juce::var& contextJson);

    /** Add a named definition */
    void addDefinition(const juce::String& name, const juce::var& nodeJson);

    /** Add a named modification chain */
    void addModification(const juce::String& name, const juce::var& modJson);

    /** Set the root node */
    void setRoot(const juce::var& rootJson);

    /** Run the full pipeline on current state and return Player-compatible var */
    juce::var resolve(double tempo);

    /** Get the current document state (for inspection/testing) */
    const Document& getDocument() const { return document; }

    /** Run the pipeline on an already-loaded document (for library integration) */
    static juce::var runPipelineFromDoc(const Document& doc, double tempo);

private:
    Document document;

    /** Run the six-stage pipeline on a document */
    static juce::var runPipeline(const Document& doc, double tempo);
};

} // namespace mj3
} // namespace covertracks
