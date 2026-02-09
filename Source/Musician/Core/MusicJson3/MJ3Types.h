#pragma once
#include <JuceHeader.h>
#include <variant>
#include <vector>
#include <map>
#include <optional>

namespace covertracks {
namespace mj3 {

// ============================================================================
// Pitch Types — 10 representations as a std::variant
// ============================================================================

struct DegreePitch    { int degree = 1; int octave = -1; }; // octave -1 = use chord default
struct NotePitch      { juce::String note; };          // "Bb4", "C#3"
struct DrumPitch      { juce::String drum; };           // "kick", "snare"
struct CentsPitch     { double cents = 0.0; };          // 702.0 = perfect fifth
struct SwaraPitch     { juce::String swara; };          // "Sa", "Re", "Ga"
struct MaqamDegreePitch { int degree = 1; double micro = 0.0; };
struct RatioPitch     { int numerator = 1; int denominator = 1; };
struct MidiPitch      { int midi = 60; };
struct GamelanPitch   { int tone = 1; juce::String tuning; }; // "slendro", "pelog"
struct FrequencyPitch { double frequency = 440.0; };

using Pitch = std::variant<
    DegreePitch,
    NotePitch,
    DrumPitch,
    CentsPitch,
    SwaraPitch,
    MaqamDegreePitch,
    RatioPitch,
    MidiPitch,
    GamelanPitch,
    FrequencyPitch
>;

// ============================================================================
// Cycle — rhythmic framework
// ============================================================================

struct Cycle {
    int beats = 4;
    juce::String name;             // "tintal", "rupak", etc. (empty = Western)
};

// ============================================================================
// Context3 — cascades down the tree
// ============================================================================

struct Context3 {
    juce::String modeSystem;       // "chord", "raga", "maqam", "pentatonic"
    juce::String mode;             // "BbMaj7", "Yaman", "Rast"
    Cycle cycle;
    juce::String tuningSystem;     // "12tet", "shruti", "slendro", "just"
    double referenceFrequency = 0.0;  // 0 = unset
    float velocity = -1.0f;       // negative = unset
    int channel = -1;             // negative = unset
    juce::var extras;              // tradition-specific extension point

    /** Merge: child values override parent where set */
    Context3 merge(const Context3& child) const;

    /** Check if a field has been explicitly set */
    bool hasVelocity() const { return velocity >= 0.0f; }
    bool hasChannel() const { return channel >= 0; }
    bool hasReferenceFrequency() const { return referenceFrequency > 0.0; }
    bool hasModeSystem() const { return modeSystem.isNotEmpty(); }
    bool hasMode() const { return mode.isNotEmpty(); }
    bool hasTuningSystem() const { return tuningSystem.isNotEmpty(); }
};

// ============================================================================
// Ornament — decorative event modification
// ============================================================================

struct Ornament {
    juce::String type;             // "trill", "mordent", "grace", "bend", "slide"
    juce::var params;              // type-specific parameters
};

// ============================================================================
// Event3 — abstract musical event (leaf node)
// ============================================================================

struct Event3 {
    Pitch pitch;
    double beat = 0.0;             // position within parent container
    double duration = 1.0;         // duration in beats
    float velocity = -1.0f;        // negative = inherit from context
    int channel = -1;              // negative = inherit from context
    std::vector<Ornament> ornaments;
    juce::var extras;              // extension point

    bool hasVelocity() const { return velocity >= 0.0f; }
    bool hasChannel() const { return channel >= 0; }
};

// ============================================================================
// ContainerMode
// ============================================================================

enum class ContainerMode {
    Sequential,
    Simultaneous
};

// ============================================================================
// Modification — a named operation on a node tree
// ============================================================================

struct Modification {
    juce::String operation;        // "add", "remove", "replace", "set", "scale", "transpose"
    juce::String target;           // path to target node/event (e.g., "bass.events[0]")
    juce::var value;               // operation-specific payload
};

struct ModificationChain {
    juce::String name;
    std::vector<Modification> operations;
};

// ============================================================================
// Node — the single recursive container
// ============================================================================

struct Node {
    juce::String name;
    ContainerMode mode = ContainerMode::Sequential;
    std::vector<Node> children;
    std::vector<Event3> events;
    juce::String ref;              // reference to a definition
    int repeat = 1;
    double beats = 0.0;           // minimum duration in beats (0 = computed from content)
    std::vector<juce::String> modify;  // names of modification chains to apply
    Context3 context;              // local override
    Context3 resolvedContext;      // filled by INHERIT stage
};

// ============================================================================
// Import — a library reference
// ============================================================================

struct Import {
    juce::String library;          // path like "jazz_bass" or "indian/ragas/yaman"
    juce::String alias;            // optional namespace alias (from "as" field)
};

// ============================================================================
// Document — output of LOAD stage
// ============================================================================

struct Document {
    juce::String version;
    Context3 context;              // top-level context
    std::map<juce::String, Node> definitions;
    std::map<juce::String, ModificationChain> modifications;
    std::vector<Import> imports;
    juce::String extends;          // library this document extends (for library files)
    Node root;
};

// ============================================================================
// ResolvedEvent — concrete output of RESOLVE stage
// ============================================================================

struct ResolvedEvent {
    double absoluteTimeBeat = 0.0;
    int midiNote = 60;
    double frequencyHz = 261.63;
    double pitchBendSemitones = 0.0;
    double durationBeats = 1.0;
    float velocity = 0.8f;
    int channel = 1;
    std::vector<ResolvedEvent> ornamentEvents;
};

// ============================================================================
// ResolvedPitch — intermediate output from PitchResolver
// ============================================================================

struct ResolvedPitch {
    int midiNote = 60;
    double frequencyHz = 261.63;
    double pitchBendSemitones = 0.0;
};

} // namespace mj3
} // namespace covertracks
