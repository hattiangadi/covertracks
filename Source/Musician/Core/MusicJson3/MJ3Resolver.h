#pragma once
#include "MJ3Types.h"
#include <memory>
#include <functional>

namespace covertracks {
namespace mj3 {

// ============================================================================
// Abstract resolver interfaces
// ============================================================================

class PitchResolver
{
public:
    virtual ~PitchResolver() = default;
    virtual ResolvedPitch resolve(const Pitch& pitch, const Context3& ctx) = 0;
    virtual bool canResolve(const Context3& ctx) const = 0;
};

class TimeResolver
{
public:
    virtual ~TimeResolver() = default;
    virtual double resolveTime(double beat, const Cycle& cycle, double offset) = 0;
    virtual double cycleDuration(const Cycle& cycle) = 0;
};

class OrnamentResolver
{
public:
    virtual ~OrnamentResolver() = default;
    virtual std::vector<ResolvedEvent> resolve(
        const Ornament& orn, const ResolvedEvent& base, const Context3& ctx) = 0;
};

// ============================================================================
// Western pitch resolver (12-TET, chord degrees, drums)
// ============================================================================

class WesternPitchResolver : public PitchResolver
{
public:
    ResolvedPitch resolve(const Pitch& pitch, const Context3& ctx) override;
    bool canResolve(const Context3& ctx) const override;

private:
    int noteNameToMidi(const juce::String& noteName) const;
    int resolveChordDegree(int degree, const juce::String& chordName) const;
    int drumToMidi(const juce::String& drumName) const;
    double midiToFrequency(int midiNote, double refFreq = 440.0) const;

    // Parse chord into root + intervals
    struct ChordInfo {
        int rootMidi;
        std::vector<int> intervals;  // semitone offsets from root
    };
    ChordInfo parseChord(const juce::String& chordName) const;
    int parseRootNote(const juce::String& noteName) const;
};

// ============================================================================
// Microtonal pitch resolver (cents, frequency, ratio — universal fallback)
// ============================================================================

class MicrotonalPitchResolver : public PitchResolver
{
public:
    ResolvedPitch resolve(const Pitch& pitch, const Context3& ctx) override;
    bool canResolve(const Context3& ctx) const override;

private:
    int frequencyToNearestMidi(double freqHz) const;
    double midiToFrequency(int midiNote) const;
};

// ============================================================================
// Default time resolver
// ============================================================================

class DefaultTimeResolver : public TimeResolver
{
public:
    double resolveTime(double beat, const Cycle& cycle, double offset) override;
    double cycleDuration(const Cycle& cycle) override;
};

// ============================================================================
// Default ornament resolver
// ============================================================================

class DefaultOrnamentResolver : public OrnamentResolver
{
public:
    std::vector<ResolvedEvent> resolve(
        const Ornament& orn, const ResolvedEvent& base, const Context3& ctx) override;

private:
    std::vector<ResolvedEvent> resolveTrill(
        const Ornament& orn, const ResolvedEvent& base, const Context3& ctx);
    std::vector<ResolvedEvent> resolveGrace(
        const Ornament& orn, const ResolvedEvent& base, const Context3& ctx);
};

// ============================================================================
// Stage 5: RESOLVE
// Convert abstract pitch/time to concrete values using pluggable resolvers
// ============================================================================

class MJ3Resolver
{
public:
    MJ3Resolver();

    /** Resolve all events in the document tree → ResolvedEvents */
    std::vector<ResolvedEvent> resolve(const Document& doc);

    /** Register a custom pitch resolver */
    void addPitchResolver(std::unique_ptr<PitchResolver> resolver);

private:
    std::vector<std::unique_ptr<PitchResolver>> pitchResolvers;
    std::unique_ptr<TimeResolver> timeResolver;
    std::unique_ptr<OrnamentResolver> ornamentResolver;

    void resolveNode(const Node& node, double timeOffset,
                     std::vector<ResolvedEvent>& output);
    double computeNodeDuration(const Node& node) const;
    ResolvedEvent resolveEvent(const Event3& event, const Context3& ctx,
                               double timeOffset);
    ResolvedPitch resolvePitch(const Pitch& pitch, const Context3& ctx);
};

} // namespace mj3
} // namespace covertracks
