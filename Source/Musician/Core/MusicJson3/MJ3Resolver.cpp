#include "MJ3Resolver.h"
#include <cmath>

namespace covertracks {
namespace mj3 {

// ============================================================================
// WesternPitchResolver
// ============================================================================

bool WesternPitchResolver::canResolve(const Context3& ctx) const
{
    return !ctx.hasTuningSystem() ||
           ctx.tuningSystem == "12tet" ||
           ctx.tuningSystem.isEmpty();
}

ResolvedPitch WesternPitchResolver::resolve(const Pitch& pitch, const Context3& ctx)
{
    ResolvedPitch result;
    double refFreq = ctx.hasReferenceFrequency() ? ctx.referenceFrequency : 440.0;

    std::visit([&](auto&& p) {
        using T = std::decay_t<decltype(p)>;

        if constexpr (std::is_same_v<T, NotePitch>)
        {
            result.midiNote = noteNameToMidi(p.note);
            result.frequencyHz = midiToFrequency(result.midiNote, refFreq);
        }
        else if constexpr (std::is_same_v<T, DegreePitch>)
        {
            result.midiNote = resolveChordDegree(p.degree, ctx.mode);
            // Apply explicit octave if specified
            if (p.octave >= 0)
            {
                int noteInOctave = result.midiNote % 12;
                result.midiNote = (p.octave + 1) * 12 + noteInOctave;
            }
            result.frequencyHz = midiToFrequency(result.midiNote, refFreq);
        }
        else if constexpr (std::is_same_v<T, DrumPitch>)
        {
            result.midiNote = drumToMidi(p.drum);
            result.frequencyHz = 0.0;  // N/A for drums
        }
        else if constexpr (std::is_same_v<T, MidiPitch>)
        {
            result.midiNote = p.midi;
            result.frequencyHz = midiToFrequency(p.midi, refFreq);
        }
        else if constexpr (std::is_same_v<T, CentsPitch>)
        {
            // Cents from reference: 0 cents = reference frequency (A4 by default)
            double semitones = p.cents / 100.0;
            int nearestMidi = 69 + (int)std::round(semitones);  // A4 = 69
            result.midiNote = nearestMidi;
            result.frequencyHz = refFreq * std::pow(2.0, semitones / 12.0);
            result.pitchBendSemitones = semitones - std::round(semitones);
        }
        else if constexpr (std::is_same_v<T, FrequencyPitch>)
        {
            result.frequencyHz = p.frequency;
            // Find nearest MIDI note
            double semitones = 12.0 * std::log2(p.frequency / refFreq);
            result.midiNote = 69 + (int)std::round(semitones);
            result.pitchBendSemitones = semitones - std::round(semitones);
        }
        else if constexpr (std::is_same_v<T, RatioPitch>)
        {
            double ratio = (double)p.numerator / (double)p.denominator;
            result.frequencyHz = refFreq * ratio;
            double semitones = 12.0 * std::log2(ratio);
            result.midiNote = 69 + (int)std::round(semitones);
            result.pitchBendSemitones = semitones - std::round(semitones);
        }
        else
        {
            // Fallback for types this resolver doesn't handle
            result.midiNote = 60;
            result.frequencyHz = midiToFrequency(60, refFreq);
        }
    }, pitch);

    return result;
}

int WesternPitchResolver::noteNameToMidi(const juce::String& noteName) const
{
    if (noteName.isEmpty())
        return 60;

    int noteIndex = parseRootNote(noteName.substring(0, 1));

    int pos = 1;
    // Check for sharp/flat
    if (pos < noteName.length())
    {
        auto c = noteName[pos];
        if (c == '#' || c == 's') { noteIndex++; pos++; }
        else if (c == 'b') { noteIndex--; pos++; }
    }

    // Parse octave
    int octave = 4;  // default
    if (pos < noteName.length())
    {
        auto octStr = noteName.substring(pos);
        if (octStr.containsOnly("-0123456789"))
            octave = octStr.getIntValue();
    }

    return (octave + 1) * 12 + noteIndex;
}

int WesternPitchResolver::parseRootNote(const juce::String& noteName) const
{
    auto c = noteName[0];
    switch (c)
    {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: return 0;
    }
}

WesternPitchResolver::ChordInfo WesternPitchResolver::parseChord(const juce::String& chordName) const
{
    ChordInfo info;
    if (chordName.isEmpty())
    {
        info.rootMidi = 60;  // C
        info.intervals = {0, 4, 7};  // Major triad
        return info;
    }

    // Parse root note (e.g., "Bb" from "BbMaj7")
    juce::String root;
    int pos = 0;
    if (pos < chordName.length())
    {
        root += chordName[pos];
        pos++;
    }
    if (pos < chordName.length() && (chordName[pos] == 'b' || chordName[pos] == '#'))
    {
        root += chordName[pos];
        pos++;
    }

    // Compute root's chromatic offset from C
    int rootSemitone = parseRootNote(root.substring(0, 1));
    for (int i = 1; i < root.length(); ++i)
    {
        if (root[i] == '#') rootSemitone++;
        else if (root[i] == 'b') rootSemitone--;
    }
    rootSemitone = ((rootSemitone % 12) + 12) % 12;

    // Place in octave 4 (C4=60 through A4=69); Bb and B go to octave 3
    info.rootMidi = (rootSemitone <= 9) ? (60 + rootSemitone) : (48 + rootSemitone);

    // Parse chord quality
    auto quality = chordName.substring(pos).toLowerCase();

    if (quality.contains("maj7") || quality.contains("major7"))
        info.intervals = {0, 4, 7, 11};
    else if (quality.contains("m7") || quality.contains("min7") || quality.contains("minor7") || quality.contains("-7"))
        info.intervals = {0, 3, 7, 10};
    else if (quality.contains("7") || quality.contains("dom7"))
        info.intervals = {0, 4, 7, 10};
    else if (quality.contains("dim7"))
        info.intervals = {0, 3, 6, 9};
    else if (quality.contains("dim"))
        info.intervals = {0, 3, 6};
    else if (quality.contains("aug"))
        info.intervals = {0, 4, 8};
    else if (quality.contains("m") || quality.contains("min") || quality.contains("minor") || quality == "-")
        info.intervals = {0, 3, 7};
    else if (quality.contains("sus4"))
        info.intervals = {0, 5, 7};
    else if (quality.contains("sus2"))
        info.intervals = {0, 2, 7};
    else
        info.intervals = {0, 4, 7};  // Default: major triad

    return info;
}

int WesternPitchResolver::resolveChordDegree(int degree, const juce::String& chordName) const
{
    auto chord = parseChord(chordName);

    if (chord.intervals.empty())
        return chord.rootMidi;

    // Degree is 1-based: degree 1 = root, degree 3 = third, etc.
    // Map to chord intervals with octave wrapping
    int index = degree - 1;
    if (index < 0) index = 0;

    int octaveShift = index / (int)chord.intervals.size();
    int chordIndex = index % (int)chord.intervals.size();

    return chord.rootMidi + chord.intervals[(size_t)chordIndex] + (octaveShift * 12);
}

int WesternPitchResolver::drumToMidi(const juce::String& drumName) const
{
    // General MIDI drum map (channel 10)
    auto name = drumName.toLowerCase();

    if (name == "kick" || name == "bass_drum" || name == "bd")  return 36;
    if (name == "snare" || name == "sd")                         return 38;
    if (name == "rimshot" || name == "rim")                      return 37;
    if (name == "clap" || name == "handclap")                    return 39;
    if (name == "hihat_closed" || name == "hh" || name == "hihat") return 42;
    if (name == "hihat_open" || name == "hho")                   return 46;
    if (name == "hihat_pedal")                                    return 44;
    if (name == "tom_low" || name == "tom1")                     return 45;
    if (name == "tom_mid" || name == "tom2")                     return 47;
    if (name == "tom_high" || name == "tom3")                    return 50;
    if (name == "crash" || name == "crash1")                     return 49;
    if (name == "crash2")                                         return 57;
    if (name == "ride" || name == "ride1")                       return 51;
    if (name == "ride_bell")                                      return 53;
    if (name == "cowbell")                                        return 56;
    if (name == "tambourine")                                     return 54;
    if (name == "shaker")                                         return 70;
    if (name == "conga_high")                                     return 63;
    if (name == "conga_low")                                      return 64;
    if (name == "bongo_high")                                     return 60;
    if (name == "bongo_low")                                      return 61;
    if (name == "clave")                                          return 75;
    if (name == "woodblock")                                      return 76;

    return 36;  // Default: kick
}

double WesternPitchResolver::midiToFrequency(int midiNote, double refFreq) const
{
    return refFreq * std::pow(2.0, (midiNote - 69) / 12.0);
}

// ============================================================================
// MicrotonalPitchResolver
// ============================================================================

bool MicrotonalPitchResolver::canResolve(const Context3& /*ctx*/) const
{
    return true;  // Universal fallback
}

ResolvedPitch MicrotonalPitchResolver::resolve(const Pitch& pitch, const Context3& ctx)
{
    ResolvedPitch result;
    double refFreq = ctx.hasReferenceFrequency() ? ctx.referenceFrequency : 440.0;

    std::visit([&](auto&& p) {
        using T = std::decay_t<decltype(p)>;

        if constexpr (std::is_same_v<T, CentsPitch>)
        {
            double semitones = p.cents / 100.0;
            result.frequencyHz = refFreq * std::pow(2.0, semitones / 12.0);
            result.midiNote = frequencyToNearestMidi(result.frequencyHz);
            double nearestFreq = midiToFrequency(result.midiNote);
            if (nearestFreq > 0)
                result.pitchBendSemitones = 12.0 * std::log2(result.frequencyHz / nearestFreq);
        }
        else if constexpr (std::is_same_v<T, FrequencyPitch>)
        {
            result.frequencyHz = p.frequency;
            result.midiNote = frequencyToNearestMidi(p.frequency);
            double nearestFreq = midiToFrequency(result.midiNote);
            if (nearestFreq > 0)
                result.pitchBendSemitones = 12.0 * std::log2(p.frequency / nearestFreq);
        }
        else if constexpr (std::is_same_v<T, RatioPitch>)
        {
            double ratio = (double)p.numerator / (double)p.denominator;
            result.frequencyHz = refFreq * ratio;
            result.midiNote = frequencyToNearestMidi(result.frequencyHz);
            double nearestFreq = midiToFrequency(result.midiNote);
            if (nearestFreq > 0)
                result.pitchBendSemitones = 12.0 * std::log2(result.frequencyHz / nearestFreq);
        }
        else
        {
            // For types that need a Western resolver first, return defaults
            result.midiNote = 60;
            result.frequencyHz = midiToFrequency(60);
        }
    }, pitch);

    return result;
}

int MicrotonalPitchResolver::frequencyToNearestMidi(double freqHz) const
{
    if (freqHz <= 0) return 60;
    return (int)std::round(69.0 + 12.0 * std::log2(freqHz / 440.0));
}

double MicrotonalPitchResolver::midiToFrequency(int midiNote) const
{
    return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

// ============================================================================
// DefaultTimeResolver
// ============================================================================

double DefaultTimeResolver::resolveTime(double beat, const Cycle& /*cycle*/, double offset)
{
    return offset + beat;
}

double DefaultTimeResolver::cycleDuration(const Cycle& cycle)
{
    return (double)cycle.beats;
}

// ============================================================================
// DefaultOrnamentResolver
// ============================================================================

std::vector<ResolvedEvent> DefaultOrnamentResolver::resolve(
    const Ornament& orn, const ResolvedEvent& base, const Context3& ctx)
{
    if (orn.type == "trill")
        return resolveTrill(orn, base, ctx);
    if (orn.type == "grace")
        return resolveGrace(orn, base, ctx);

    // Unknown ornament: return base event unchanged
    return {base};
}

std::vector<ResolvedEvent> DefaultOrnamentResolver::resolveTrill(
    const Ornament& orn, const ResolvedEvent& base, const Context3& /*ctx*/)
{
    std::vector<ResolvedEvent> result;

    int interval = 2;  // default: whole step
    double speed = 0.125;  // default: 32nd note

    if (orn.params.isObject())
    {
        if (orn.params.hasProperty("interval"))
            interval = (int)orn.params["interval"];
        if (orn.params.hasProperty("speed"))
            speed = (double)orn.params["speed"];
    }

    double remaining = base.durationBeats;
    double time = base.absoluteTimeBeat;
    bool upper = false;

    while (remaining > 0.001)
    {
        ResolvedEvent note = base;
        note.absoluteTimeBeat = time;
        note.durationBeats = std::min(speed, remaining);
        note.midiNote = base.midiNote + (upper ? interval : 0);
        note.frequencyHz = 440.0 * std::pow(2.0, (note.midiNote - 69) / 12.0);
        result.push_back(note);

        time += speed;
        remaining -= speed;
        upper = !upper;
    }

    return result;
}

std::vector<ResolvedEvent> DefaultOrnamentResolver::resolveGrace(
    const Ornament& orn, const ResolvedEvent& base, const Context3& /*ctx*/)
{
    std::vector<ResolvedEvent> result;

    int interval = -2;  // default: whole step below
    double graceDuration = 0.0625;  // 64th note

    if (orn.params.isObject())
    {
        if (orn.params.hasProperty("interval"))
            interval = (int)orn.params["interval"];
        if (orn.params.hasProperty("duration"))
            graceDuration = (double)orn.params["duration"];
    }

    // Grace note
    ResolvedEvent grace = base;
    grace.absoluteTimeBeat = base.absoluteTimeBeat - graceDuration;
    grace.durationBeats = graceDuration;
    grace.midiNote = base.midiNote + interval;
    grace.velocity = base.velocity * 0.8f;
    result.push_back(grace);

    // Main note (slightly shortened)
    result.push_back(base);

    return result;
}

// ============================================================================
// MJ3Resolver — pipeline stage 5
// ============================================================================

MJ3Resolver::MJ3Resolver()
{
    pitchResolvers.push_back(std::make_unique<WesternPitchResolver>());
    pitchResolvers.push_back(std::make_unique<MicrotonalPitchResolver>());
    timeResolver = std::make_unique<DefaultTimeResolver>();
    ornamentResolver = std::make_unique<DefaultOrnamentResolver>();
}

void MJ3Resolver::addPitchResolver(std::unique_ptr<PitchResolver> resolver)
{
    // Insert before the microtonal fallback (last)
    if (pitchResolvers.size() > 1)
        pitchResolvers.insert(pitchResolvers.end() - 1, std::move(resolver));
    else
        pitchResolvers.push_back(std::move(resolver));
}

std::vector<ResolvedEvent> MJ3Resolver::resolve(const Document& doc)
{
    std::vector<ResolvedEvent> output;
    resolveNode(doc.root, 0.0, output);
    return output;
}

double MJ3Resolver::computeNodeDuration(const Node& node) const
{
    const auto& ctx = node.resolvedContext;
    double cycleDur = timeResolver->cycleDuration(ctx.cycle);
    double maxEnd = 0.0;

    // Check direct events
    for (const auto& evt : node.events)
    {
        double end = evt.beat + evt.duration;
        if (end > maxEnd) maxEnd = end;
    }

    // Check children
    if (node.mode == ContainerMode::Sequential)
    {
        double seqTotal = 0.0;
        for (const auto& child : node.children)
            seqTotal += computeNodeDuration(child);
        if (seqTotal > maxEnd) maxEnd = seqTotal;
    }
    else // Simultaneous
    {
        for (const auto& child : node.children)
        {
            double childDur = computeNodeDuration(child);
            if (childDur > maxEnd) maxEnd = childDur;
        }
    }

    // Empty node defaults to one cycle
    if (maxEnd == 0.0 && node.events.empty() && node.children.empty())
        maxEnd = cycleDur;

    // Apply minimum duration floor if set
    if (node.beats > 0.0)
        maxEnd = std::max(maxEnd, node.beats);

    return maxEnd;
}

void MJ3Resolver::resolveNode(const Node& node, double timeOffset,
                                std::vector<ResolvedEvent>& output)
{
    const auto& ctx = node.resolvedContext;
    double cycleDur = timeResolver->cycleDuration(ctx.cycle);

    // Resolve leaf events
    for (const auto& event : node.events)
    {
        auto resolved = resolveEvent(event, ctx, timeOffset);
        output.push_back(resolved);
    }

    // Recurse into children
    if (node.mode == ContainerMode::Sequential)
    {
        double childOffset = timeOffset;
        for (const auto& child : node.children)
        {
            resolveNode(child, childOffset, output);
            childOffset += computeNodeDuration(child);
        }
    }
    else // Simultaneous
    {
        for (const auto& child : node.children)
            resolveNode(child, timeOffset, output);
    }
}

ResolvedEvent MJ3Resolver::resolveEvent(const Event3& event, const Context3& ctx,
                                          double timeOffset)
{
    ResolvedEvent resolved;

    // Resolve pitch
    auto rp = resolvePitch(event.pitch, ctx);
    resolved.midiNote = rp.midiNote;
    resolved.frequencyHz = rp.frequencyHz;
    resolved.pitchBendSemitones = rp.pitchBendSemitones;

    // Resolve time
    resolved.absoluteTimeBeat = timeResolver->resolveTime(event.beat, ctx.cycle, timeOffset);

    // Duration
    resolved.durationBeats = event.duration;

    // Velocity: event override > context
    resolved.velocity = event.hasVelocity() ? event.velocity : ctx.velocity;

    // Channel: event override > context
    resolved.channel = event.hasChannel() ? event.channel : ctx.channel;

    // Handle drum channel convention
    if (std::holds_alternative<DrumPitch>(event.pitch) && !event.hasChannel())
        resolved.channel = 10;

    // Resolve ornaments
    for (const auto& orn : event.ornaments)
    {
        auto ornEvents = ornamentResolver->resolve(orn, resolved, ctx);
        for (auto& oe : ornEvents)
            resolved.ornamentEvents.push_back(oe);
    }

    return resolved;
}

ResolvedPitch MJ3Resolver::resolvePitch(const Pitch& pitch, const Context3& ctx)
{
    for (auto& resolver : pitchResolvers)
    {
        if (resolver->canResolve(ctx))
            return resolver->resolve(pitch, ctx);
    }

    // Should never reach here (microtonal is universal fallback)
    return ResolvedPitch{60, 261.63, 0.0};
}

} // namespace mj3
} // namespace covertracks
