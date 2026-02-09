#pragma once
#include "MJ3Types.h"
#include "MJ3Loader.h"
#include <functional>
#include <set>

namespace covertracks {
namespace mj3 {

/**
 * Library loading and import resolution.
 *
 * Libraries are 3.0 documents with `define` but no root.
 * They export definitions that can be imported into other documents.
 *
 * Features:
 *   - Simple import: "jazz_bass" → merges all definitions directly
 *   - Aliased import: {"library": "...", "as": "raga"} → prefixes definitions
 *   - extends: library can inherit definitions from another library
 *   - Cycle detection for extends chains
 *
 * Usage:
 *   MJ3LibraryLoader loader;
 *   loader.setResolver([](const String& path) -> Document { ... });
 *   loader.resolveImports(doc);
 */
class MJ3LibraryLoader
{
public:
    /** Callback that resolves a library path to a loaded Document.
     *  Returns a Document with definitions (and optionally extends). */
    using LibraryResolver = std::function<Document(const juce::String& libraryPath)>;

    /** Set the resolver that loads library documents by path */
    void setResolver(LibraryResolver resolver);

    /** Register a library document directly (for testing, built-in libraries) */
    void registerLibrary(const juce::String& path, const Document& doc);

    /** Resolve all imports in a document, merging library definitions.
     *  Modifies the document in place. Returns true on success. */
    bool resolveImports(Document& doc);

    /** Resolve a library's extends chain and return merged definitions.
     *  Detects circular extends. */
    Document resolveExtends(const Document& library,
                            std::set<juce::String>& visited);

    /** Get the last error message (if resolveImports returned false) */
    const juce::String& getError() const { return lastError; }

private:
    /** Load a library (from registry or resolver), resolve its extends chain */
    Document loadLibrary(const juce::String& path,
                         std::set<juce::String>& visited);

    /** Merge library definitions into a target document with optional prefix */
    void mergeDefinitions(Document& target, const Document& library,
                          const juce::String& prefix);

    LibraryResolver resolver;
    std::map<juce::String, Document> registry;
    juce::String lastError;
};

} // namespace mj3
} // namespace covertracks
