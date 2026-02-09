#include "MusicalContainer.h"

namespace covertracks {

// ============================================================================
// Context Implementation
// ============================================================================

Context Context::merge(const Context& child) const
{
    Context result = *this;
    
    // Special handling for octave: only override if child has non-default octave
    // This prevents variations with default octave=4 from overriding parent octave=2
    if (child.octave != 4) result.octave = child.octave;
    
    // For other attributes: child overrides parent when they differ
    // This ensures explicit overrides work (e.g., root velocity=0.8 overrides global velocity=0.6)
    if (child.velocity != velocity) result.velocity = child.velocity;
    if (child.chord != chord) result.chord = child.chord;
    if (child.key != key) result.key = child.key;
    if (child.dynamics != dynamics) result.dynamics = child.dynamics;
    if (child.articulation != articulation) result.articulation = child.articulation;
    if (child.chordContext != chordContext) result.chordContext = child.chordContext;
    if (child.processorChain != processorChain) result.processorChain = child.processorChain;
    if (child.complexity != complexity) result.complexity = child.complexity;
    if (child.energy != energy) result.energy = child.energy;
    if (child.style != style) result.style = child.style;
    
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

MusicalContainer::MusicalContainer(const MusicalContainer& other)
    : events(other.events)
    , layers(other.layers)
    , variations(other.variations)
    , patterns(other.patterns)
    , sections(other.sections)
    , context(other.context)
    , reference(other.reference)
    , arrangement(other.arrangement)
    , numBars(other.numBars)
    , shortcuts(other.shortcuts)
    , transforms(other.transforms)
    , chordProgression(other.chordProgression)
    , processorChains(other.processorChains)
    , constraints(other.constraints)
    , performance(other.performance)
{
    // Copy constructor
}

MusicalContainer& MusicalContainer::operator=(const MusicalContainer& other)
{
    if (this != &other) {
        events = other.events;
        layers = other.layers;
        variations = other.variations;
        patterns = other.patterns;
        sections = other.sections;
        context = other.context;
        reference = other.reference;
        arrangement = other.arrangement;
        numBars = other.numBars;
        shortcuts = other.shortcuts;
        transforms = other.transforms;
        chordProgression = other.chordProgression;
        processorChains = other.processorChains;
        constraints = other.constraints;
        performance = other.performance;
    }
    return *this;
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
    
    // 5. Render sections
    result.merge(renderSections(resolvedContext, currentBar, totalBars));
    
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
        if (bar == 1) // Only log on first bar to avoid spam
            DBG("  Layer '" << layerName << "' bar " << bar << ": " << layerStream.size() << " events, ref='" << layer.reference << "'");
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
        auto result = pattern->render(resolvedContext, bar, totalBars);
        if (bar == 1)
            DBG("    Pattern '" << reference << "': " << result.size() << " events rendered");
        return result;
    }

    DBG("*** Pattern NOT found: '" << reference << "' (available: " << patterns.size() << " patterns)");
    for (const auto& [name, _] : patterns)
        DBG("      available: '" << name << "'");
    return EventStream();
}

EventStream MusicalContainer::renderSections(const Context& resolvedContext, int bar, int totalBars) const
{
    EventStream stream;
    
    // Check if we have an arrangement - if so, render sections sequentially in arrangement order
    if (!arrangement.empty()) {
        DBG("Rendering sections in arrangement order: " << arrangement.size() << " sections");
        
        double currentTime = 0.0;
        for (const auto& sectionName : arrangement) {
            DBG("Processing arrangement section: " << sectionName);
            auto sectionIt = sections.find(sectionName);
            if (sectionIt != sections.end()) {
                DBG("Rendering arranged section: " << sectionName << " at time offset " << currentTime);
                
                // Render the section fully - calculate how many bars it contains
                int sectionBars = calculateSectionBars(sectionIt->second);
                DBG("Section " << sectionName << " contains " << sectionBars << " bars");
                
                // Render all bars in the section sequentially
                EventStream sectionStream;
                for (int bar = 1; bar <= sectionBars; ++bar) {
                    auto barStream = sectionIt->second.render(resolvedContext, bar, sectionBars);
                    
                    // Offset bar events by (bar-1) * beatsPerBar
                    double barOffset = (bar - 1) * 4.0; // Assume 4 beats per bar
                    for (auto& event : barStream) {
                        event.time += barOffset;
                    }
                    
                    sectionStream.merge(barStream);
                }
                
                DBG("Section " << sectionName << " generated " << sectionStream.size() << " events");
                
                // Offset all events in the section by currentTime
                for (auto& event : sectionStream) {
                    DBG("Original event time: " << event.time << ", adding offset: " << currentTime);
                    event.time += currentTime;
                    DBG("Final event time: " << event.time << ", note: " << event.resolvedNote);
                }
                
                stream.merge(sectionStream);
                
                // Advance time for next section based on actual section duration
                double sectionDuration = 4.0; // Default to 4 beats/1 bar
                if (!sectionStream.isEmpty()) {
                    // Find the last event time relative to section start (subtract currentTime offset)
                    double maxRelativeTime = 0.0;
                    for (const auto& event : sectionStream) {
                        double relativeTime = (event.time - currentTime) + event.duration;
                        maxRelativeTime = std::max(maxRelativeTime, relativeTime);
                    }
                    if (maxRelativeTime > 0.0) {
                        sectionDuration = maxRelativeTime;
                    }
                }
                currentTime += sectionDuration;
                DBG("Advanced currentTime by " << sectionDuration << " to: " << currentTime);
            } else {
                DBG("Warning: Section '" << sectionName << "' in arrangement not found in sections map");
                DBG("Available sections in map:");
                for (const auto& availableSection : sections) {
                    DBG("  - " << availableSection.first);
                }
            }
        }
    } else {
        // Fallback: render all sections simultaneously (like layers) - original behavior
        DBG("No arrangement found, rendering all sections simultaneously");
        for (const auto& [sectionName, section] : sections) {
            auto sectionStream = section.render(resolvedContext, bar, totalBars);
            stream.merge(sectionStream);
        }
    }
    
    return stream;
}

Context MusicalContainer::resolveContext(const Context& parentContext) const
{
    return parentContext.merge(context);
}

const MusicalContainer* MusicalContainer::resolvePattern(const juce::String& patternName) const
{
    DBG("Resolving pattern: " << patternName << ", local patterns count: " << patterns.size());
    
    // First check local patterns
    auto it = patterns.find(patternName);
    if (it != patterns.end()) {
        DBG("Found pattern locally: " << patternName);
        return &it->second;
    }
    
    // TODO: Check parent patterns (for now return nullptr)
    DBG("Pattern not found locally: " << patternName);
    return nullptr;
}

void MusicalContainer::applyExpression(Event& event, const Context& context) const
{
    // Apply context to event - event attributes override context
    // If event doesn't specify attribute (default value), use context

    // For octave: use event value if specified, otherwise context
    event.resolvedOctave = (event.resolvedOctave != 4) ? event.resolvedOctave : context.octave;
    
    // For velocity: use event value if it's specified (non-zero), otherwise context  
    // Note: A velocity of 0.0 is treated as "unspecified" and should inherit from context
    event.resolvedVelocity = (event.velocity > 0.0f) ? event.velocity : context.velocity;
    
    // For chord: use event chord if specified, otherwise context
    event.resolvedChord = (!event.chord.isEmpty()) ? event.chord : context.chord;
    
    // For dynamics: check expression map first, then direct field, then context
    juce::String resolvedDynamics = context.dynamics;
    if (event.expression.count("dynamics") > 0) {
        resolvedDynamics = event.expression["dynamics"].toString();
    } else if (!event.dynamics.isEmpty()) {
        resolvedDynamics = event.dynamics;
    }
    event.dynamics = resolvedDynamics; // Store resolved value back in event
    
    // For articulation: check expression map first, then direct field, then context  
    juce::String resolvedArticulation = context.articulation;
    if (event.expression.count("articulation") > 0) {
        resolvedArticulation = event.expression["articulation"].toString();
    } else if (!event.articulation.isEmpty()) {
        resolvedArticulation = event.articulation;
    }
    event.articulation = resolvedArticulation; // Store resolved value back in event
    
    // Resolve note name to MIDI number if needed
    if (!event.note.isEmpty() && event.resolvedNote == 60) { // Only resolve if not already resolved
        event.resolvedNote = parseMusicalNoteName(event.note);
    }

    // Resolve chord degrees if needed
    if (event.degree > 0) {
        event.resolvedNote = resolveChordDegree(event.degree, event.resolvedChord, event.resolvedOctave);
    }
}

int MusicalContainer::resolveChordDegree(int degree, const juce::String& chord, int octave) const
{
    // Parse chord root note as semitone offset from C (0-11)
    int rootSemitone = 0; // Default to C
    int pos = 0;

    if (chord.isNotEmpty()) {
        char first = chord[0];
        switch (first) {
            case 'C': rootSemitone = 0; break;
            case 'D': rootSemitone = 2; break;
            case 'E': rootSemitone = 4; break;
            case 'F': rootSemitone = 5; break;
            case 'G': rootSemitone = 7; break;
            case 'A': rootSemitone = 9; break;
            case 'B': rootSemitone = 11; break;
            default: break;
        }
        pos = 1;

        // Handle sharps and flats in chord root (e.g., Bb, F#, Eb)
        if (pos < chord.length()) {
            if (chord[pos] == 'b' || chord[pos] == 0x266D) { // flat
                rootSemitone = (rootSemitone + 11) % 12; // -1 mod 12
                pos++;
            } else if (chord[pos] == '#' || chord[pos] == 0x266F) { // sharp
                rootSemitone = (rootSemitone + 1) % 12;
                pos++;
            }
        }
    }

    // Calculate MIDI note: C(N) = (N+1)*12, matching parseMusicalNoteName
    int chordRoot = (octave + 1) * 12 + rootSemitone;

    // Scale degrees (major scale intervals)
    switch (degree) {
        case 1: return chordRoot;          // Root
        case 2: return chordRoot + 2;      // Second
        case 3: return chordRoot + 4;      // Major third
        case 4: return chordRoot + 5;      // Fourth
        case 5: return chordRoot + 7;      // Fifth
        case 6: return chordRoot + 9;      // Sixth
        case 7: return chordRoot + 11;     // Major seventh
        case 8: return chordRoot + 12;     // Octave
        default: return chordRoot;
    }
}

bool MusicalContainer::matchesBar(const juce::String& varName, int bar) const
{
    // Handle both "bar1" and "1" formats
    if (varName.startsWith("bar")) {
        juce::String barNumberStr = varName.substring(3);
        int varBar = barNumberStr.getIntValue();
        return varBar == bar;
    }
    
    // Handle simple numeric names like "1", "2", "3"
    if (varName.containsOnly("0123456789")) {
        int varBar = varName.getIntValue();
        return varBar == bar;
    }
    
    return false;
}

int MusicalContainer::parseMusicalNoteName(const juce::String& noteName) const
{
    // Parse standard musical note names like "C4", "F#5", "Bb3"
    if (noteName.length() < 2)
        return -1; // Invalid note name
    
    juce::String noteStr = noteName.trim().toUpperCase();
    
    // Extract note name (C, D, E, F, G, A, B)
    char noteLetter = noteStr[0];
    int noteIndex = -1;
    
    switch (noteLetter)
    {
        case 'C': noteIndex = 0; break;
        case 'D': noteIndex = 2; break;
        case 'E': noteIndex = 4; break;
        case 'F': noteIndex = 5; break;
        case 'G': noteIndex = 7; break;
        case 'A': noteIndex = 9; break;
        case 'B': noteIndex = 11; break;
        default: return -1; // Invalid note
    }
    
    // Check for sharp or flat
    int accidentalOffset = 0;
    int octaveStartIndex = 1;
    
    if (noteStr.length() > 1)
    {
        if (noteStr[1] == '#' || noteStr[1] == 'S') // Sharp
        {
            accidentalOffset = 1;
            octaveStartIndex = 2;
        }
        else if (noteStr[1] == 'B' && noteStr.length() > 2 && noteStr[2] != '0') // Flat (but not B0)
        {
            accidentalOffset = -1;
            octaveStartIndex = 2;
        }
    }
    
    // Extract octave number
    if (noteStr.length() <= octaveStartIndex)
        return -1; // No octave specified
    
    juce::String octaveStr = noteStr.substring(octaveStartIndex);
    int octave = octaveStr.getIntValue();
    
    // Validate octave range (MIDI supports 0-10, but typically we use 0-8)
    if (octave < 0 || octave > 10)
        return -1;
    
    // Calculate MIDI note number
    // MIDI note 60 = C4 (Middle C), which means octave 4 starts at MIDI note 60
    // In scientific pitch notation: C0=12, C1=24, C2=36, C3=48, C4=60, etc.
    // Each octave spans 12 semitones
    int midiNote = ((octave + 1) * 12) + noteIndex + accidentalOffset;
    
    // Ensure MIDI note is in valid range (0-127)
    if (midiNote < 0 || midiNote > 127)
        return -1;
    
    return midiNote;
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

int MusicalContainer::calculateSectionBars(const MusicalContainer& section) const
{
    // Use explicit bar count if set (e.g., "bars": 8 in LLM-generated JSON)
    if (section.numBars > 0) {
        DBG("calculateSectionBars: using explicit numBars = " << section.numBars);
        return section.numBars;
    }

    // Find the highest numbered bar in the section
    int maxBar = 0;
    
    // Function to extract bar number from a name
    auto extractBarNumber = [](const juce::String& name) -> int {
        // Check if this is a numeric bar name like "1", "2", "3"
        if (name.containsOnly("0123456789")) {
            return name.getIntValue();
        }
        // Check if this is a "bar1" format name
        else if (name.startsWith("bar")) {
            juce::String barNumberStr = name.substring(3);
            if (barNumberStr.containsOnly("0123456789")) {
                return barNumberStr.getIntValue();
            }
        }
        // Check if this is a range like "1-4" or "5-8"
        else if (name.contains("-")) {
            int dashIndex = name.indexOfChar('-');
            if (dashIndex > 0 && dashIndex < name.length() - 1) {
                juce::String endStr = name.substring(dashIndex + 1);
                if (endStr.containsOnly("0123456789")) {
                    return endStr.getIntValue();
                }
            }
        }
        return 0;
    };
    
    // Check variations (where bars are usually stored)
    for (const auto& [varName, variation] : section.variations) {
        int barNum = extractBarNumber(varName);
        maxBar = std::max(maxBar, barNum);
    }
    
    // Also check patterns in case bars are stored there
    for (const auto& [patternName, pattern] : section.patterns) {
        int barNum = extractBarNumber(patternName);
        maxBar = std::max(maxBar, barNum);
    }
    
    // Check layers (in case there are nested bars in layers)
    for (const auto& [layerName, layer] : section.layers) {
        for (const auto& [varName, variation] : layer.variations) {
            int barNum = extractBarNumber(varName);
            maxBar = std::max(maxBar, barNum);
        }
    }
    
    DBG("calculateSectionBars: found max bar " << maxBar << " in section with " 
        << section.variations.size() << " variations, " 
        << section.patterns.size() << " patterns, " 
        << section.layers.size() << " layers");
    
    // Default to 1 bar if no bars found
    return std::max(1, maxBar);
}

} // namespace covertracks