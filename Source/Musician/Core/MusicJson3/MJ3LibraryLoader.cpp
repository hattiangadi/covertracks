#include "MJ3LibraryLoader.h"

namespace covertracks {
namespace mj3 {

void MJ3LibraryLoader::setResolver(LibraryResolver res)
{
    resolver = std::move(res);
}

void MJ3LibraryLoader::registerLibrary(const juce::String& path, const Document& doc)
{
    registry[path] = doc;
}

bool MJ3LibraryLoader::resolveImports(Document& doc)
{
    lastError = {};

    for (const auto& imp : doc.imports)
    {
        std::set<juce::String> visited;
        auto library = loadLibrary(imp.library, visited);

        if (lastError.isNotEmpty())
            return false;

        mergeDefinitions(doc, library, imp.alias);
    }

    return true;
}

Document MJ3LibraryLoader::loadLibrary(const juce::String& path,
                                        std::set<juce::String>& visited)
{
    if (visited.count(path))
    {
        lastError = "Circular extends detected: " + path;
        return {};
    }
    visited.insert(path);

    // Try registry first, then resolver
    Document library;
    auto it = registry.find(path);
    if (it != registry.end())
    {
        library = it->second;
    }
    else if (resolver)
    {
        library = resolver(path);
    }
    else
    {
        lastError = "No resolver set and library not in registry: " + path;
        return {};
    }

    // Resolve extends chain
    if (library.extends.isNotEmpty())
        library = resolveExtends(library, visited);

    return library;
}

Document MJ3LibraryLoader::resolveExtends(const Document& library,
                                            std::set<juce::String>& visited)
{
    auto base = loadLibrary(library.extends, visited);

    if (lastError.isNotEmpty())
        return library;

    // Child definitions override base definitions
    for (const auto& [name, node] : library.definitions)
        base.definitions[name] = node;

    // Child modifications override base modifications
    for (const auto& [name, chain] : library.modifications)
        base.modifications[name] = chain;

    // Keep child's context if set
    base.context = base.context.merge(library.context);

    return base;
}

void MJ3LibraryLoader::mergeDefinitions(Document& target, const Document& library,
                                          const juce::String& prefix)
{
    for (const auto& [name, node] : library.definitions)
    {
        juce::String key = prefix.isEmpty() ? name : (prefix + "." + name);
        target.definitions[key] = node;
    }

    for (const auto& [name, chain] : library.modifications)
    {
        juce::String key = prefix.isEmpty() ? name : (prefix + "." + name);
        target.modifications[key] = chain;
    }
}

} // namespace mj3
} // namespace covertracks
