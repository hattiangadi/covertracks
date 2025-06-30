#include "MusicalContainer.h"

namespace covertracks {

// ============================================================================
// Context Implementation
// ============================================================================

Context Context::merge(const Context& child) const
{
    Context result = *this;
    
    // Child values override parent values
    if (child.octave != 4) result.octave = child.octave;
    if (child.velocity != 0.8f) result.velocity = child.velocity;
    if (child.chord != "C") result.chord = child.chord;
    if (child.key != "C major") result.key = child.key;
    if (child.dynamics != "mf") result.dynamics = child.dynamics;
    if (child.articulation != "normal") result.articulation = child.articulation;
    if (!child.chordContext.isEmpty()) result.chordContext = child.chordContext;
    if (!child.processorChain.isEmpty()) result.processorChain = child.processorChain;
    if (child.complexity != 0.5f) result.complexity = child.complexity;
    if (child.energy != 0.5f) result.energy = child.energy;
    if (child.style != "default") result.style = child.style;
    
    return result;
}

bool Context::has(const juce::String& attribute) const
{
    if (attribute == "octave") return octave != 4;
    if (attribute == "velocity") return velocity != 0.8f;
    if (attribute == "chord") return chord != "C";
    if (attribute == "key") return key != "C major";
    if (attribute == "dynamics") return dynamics != "mf";
    if (attribute == "articulation") return articulation != "normal";
    if (attribute == "chordContext") return !chordContext.isEmpty();
    if (attribute == "processorChain") return !processorChain.isEmpty();
    if (attribute == "complexity") return complexity != 0.5f;
    if (attribute == "energy") return energy != 0.5f;
    if (attribute == "style") return style != "default";
    return false;
}

juce::var Context::get(const juce::String& attribute) const
{
    if (attribute == "octave") return octave;
    if (attribute == "velocity") return velocity;
    if (attribute == "chord") return chord;
    if (attribute == "key") return key;
    if (attribute == "dynamics") return dynamics;
    if (attribute == "articulation") return articulation;
    if (attribute == "chordContext") return chordContext;
    if (attribute == "processorChain") return processorChain;
    if (attribute == "complexity") return complexity;
    if (attribute == "energy") return energy;
    if (attribute == "style") return style;
    return juce::var();
}

// ============================================================================
// Event Implementation
// ============================================================================

bool Event::hasExpression(const juce::String& type) const
{
    return expression.count(type) > 0;
}

bool Event::hasProcessor(const juce::String& type) const
{
    return processors.count(type) > 0;
}

// ============================================================================
// EventStream Implementation
// ============================================================================

void EventStream::addEvent(const Event& event)
{
    events.push_back(event);
    // Keep events sorted by time
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        return a.time < b.time;
    });
}

void EventStream::merge(const EventStream& other)
{
    for (const auto& event : other.events) {
        addEvent(event);
    }
}

size_t EventStream::size() const
{
    return events.size();
}

bool EventStream::isEmpty() const
{
    return events.empty();
}

bool EventStream::hasEventAt(double time) const
{
    return std::any_of(events.begin(), events.end(), [time](const Event& e) {
        return std::abs(e.time - time) < 0.001; // Small tolerance for floating point
    });
}

Event EventStream::getEventAt(double time) const
{
    auto it = std::find_if(events.begin(), events.end(), [time](const Event& e) {
        return std::abs(e.time - time) < 0.001;
    });
    
    if (it != events.end()) {
        return *it;
    }
    
    return Event(); // Return empty event if not found
}

Event EventStream::getEvent(size_t index) const
{
    if (index < events.size()) {
        return events[index];
    }
    return Event();
}

std::vector<Event>::iterator EventStream::begin()
{
    return events.begin();
}

std::vector<Event>::iterator EventStream::end()
{
    return events.end();
}

std::vector<Event>::const_iterator EventStream::begin() const
{
    return events.begin();
}

std::vector<Event>::const_iterator EventStream::end() const
{
    return events.end();
}

// ============================================================================
// MusicalContainer Implementation
// ============================================================================

MusicalContainer::MusicalContainer()
{
    // Default constructor
}

MusicalContainer::~MusicalContainer()
{
    // Default destructor
}

EventStream MusicalContainer::render(const Context& parentContext, int currentBar, int totalBars) const
{
    // Resolve context by merging parent with local
    Context resolvedContext = resolveContext(parentContext);
    
    EventStream result;
    
    // 1. Render direct events
    result.merge(renderEvents(resolvedContext));
    
    // 2. Render simultaneous layers
    result.merge(renderLayers(resolvedContext, currentBar, totalBars));
    
    // 3. Render sequential variations (only one applies)
    result.merge(renderVariations(resolvedContext, currentBar));
    
    // 4. Render pattern references
    result.merge(renderPatterns(resolvedContext, currentBar, totalBars));
    
    return result;
}

void MusicalContainer::addEvent(const Event& event)
{
    events.push_back(event);
}

MusicalContainer& MusicalContainer::addLayer(const juce::String& name)
{
    layers[name] = MusicalContainer();
    return layers[name];
}

MusicalContainer& MusicalContainer::addVariation(const juce::String& name)
{
    variations[name] = MusicalContainer();
    return variations[name];
}

MusicalContainer& MusicalContainer::addPattern(const juce::String& name)
{
    patterns[name] = MusicalContainer();
    return patterns[name];
}

bool MusicalContainer::hasLayer(const juce::String& name) const
{
    return layers.count(name) > 0;
}

bool MusicalContainer::hasVariation(const juce::String& name) const
{
    return variations.count(name) > 0;
}

bool MusicalContainer::hasPattern(const juce::String& name) const
{
    return patterns.count(name) > 0;
}

bool MusicalContainer::hasSection(const juce::String& name) const
{
    return sections.count(name) > 0;
}

MusicalContainer& MusicalContainer::getLayer(const juce::String& name)
{
    return layers[name];
}

MusicalContainer& MusicalContainer::getVariation(const juce::String& name)
{
    return variations[name];
}

MusicalContainer& MusicalContainer::getPattern(const juce::String& name)
{
    return patterns[name];
}

MusicalContainer& MusicalContainer::getSection(const juce::String& name)
{
    return sections[name];
}

void MusicalContainer::setReference(const juce::String& patternName)
{
    reference = patternName;
}

juce::String MusicalContainer::getReference() const
{
    return reference;
}

// ============================================================================
// Private Implementation Methods
// ============================================================================

EventStream MusicalContainer::renderEvents(const Context& resolvedContext) const
{
    EventStream stream;
    
    for (auto event : events) {
        // Apply context resolution to event
        applyExpression(event, resolvedContext);
        stream.addEvent(event);
    }
    
    return stream;
}

EventStream MusicalContainer::renderLayers(const Context& resolvedContext, int bar, int totalBars) const
{
    EventStream stream;
    
    // Render all layers simultaneously
    for (const auto& [layerName, layer] : layers) {
        auto layerStream = layer.render(resolvedContext, bar, totalBars);
        stream.merge(layerStream);
    }
    
    return stream;
}

EventStream MusicalContainer::renderVariations(const Context& resolvedContext, int bar) const
{
    // Find the variation that matches the current bar
    for (const auto& [varName, variation] : variations) {
        if (matchesBar(varName, bar)) {
            return variation.render(resolvedContext, bar, bar);
        }
    }
    
    return EventStream(); // No matching variation
}

EventStream MusicalContainer::renderPatterns(const Context& resolvedContext, int bar, int totalBars) const
{
    if (reference.isEmpty()) {
        return EventStream();
    }
    
    // Find pattern by reference
    const MusicalContainer* pattern = resolvePattern(reference);
    if (pattern != nullptr) {
        return pattern->render(resolvedContext, bar, totalBars);
    }
    
    return EventStream();
}

Context MusicalContainer::resolveContext(const Context& parentContext) const
{
    return parentContext.merge(context);
}

const MusicalContainer* MusicalContainer::resolvePattern(const juce::String& patternName) const
{
    // First check local patterns
    auto it = patterns.find(patternName);
    if (it != patterns.end()) {
        return &it->second;
    }
    
    // TODO: Check parent patterns (for now return nullptr)
    return nullptr;
}

void MusicalContainer::applyExpression(Event& event, const Context& context) const
{
    // Apply context to event if event doesn't override
    if (event.resolvedOctave == 4) event.resolvedOctave = context.octave;
    if (event.resolvedVelocity == 0.8f) event.resolvedVelocity = context.velocity;
    if (event.resolvedChord == "C") event.resolvedChord = context.chord;
    
    // Resolve chord degrees if needed
    if (event.degree > 0) {
        event.resolvedNote = resolveChordDegree(event.degree, event.resolvedChord, event.resolvedOctave);
    }
}

int MusicalContainer::resolveChordDegree(int degree, const juce::String& chord, int octave) const
{
    // Simple chord degree resolution (C major for now)
    int baseNote = 60 + (octave - 4) * 12; // C4 = 60
    
    // Basic major scale degrees
    switch (degree) {
        case 1: return baseNote;      // Root
        case 2: return baseNote + 2;  // Second
        case 3: return baseNote + 4;  // Third
        case 4: return baseNote + 5;  // Fourth  
        case 5: return baseNote + 7;  // Fifth
        case 6: return baseNote + 9;  // Sixth
        case 7: return baseNote + 11; // Seventh
        case 8: return baseNote + 12; // Octave
        default: return baseNote;
    }
}

bool MusicalContainer::matchesBar(const juce::String& varName, int bar) const
{
    // Simple bar matching for now
    if (varName.startsWith("bar")) {
        juce::String barNumberStr = varName.substring(3);
        int varBar = barNumberStr.getIntValue();
        return varBar == bar;
    }
    
    return false;
}

void MusicalContainer::applyShorthand(EventStream& stream) const
{
    // TODO: Implement shorthand notation processing
    (void)stream; // Suppress unused parameter warning
}

void MusicalContainer::applyTransforms(EventStream& stream) const
{
    // TODO: Implement transform processing  
    (void)stream; // Suppress unused parameter warning
}

} // namespace covertracks