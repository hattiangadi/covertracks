#include "LeadSheetConverter.h"

namespace covertracks {
namespace mj3 {

ConvertedLeadSheet LeadSheetConverter::convert(const juce::String& leadSheetJson)
{
    ConvertedLeadSheet result;

    juce::var parsed;
    auto parseResult = juce::JSON::parse(leadSheetJson, parsed);
    if (!parseResult.wasOk() || !parsed.isObject())
        return result;

    // If no "measures" array, try converting from "sections" format (legacy image analysis)
    if (!parsed.hasProperty("measures") || !parsed["measures"].isArray())
    {
        parsed = convertSectionsToMeasures(parsed);
        if (!parsed.isObject())
            return result;
    }

    auto* measures = parsed["measures"].getArray();
    if (!measures || measures->isEmpty())
        return result;

    result.totalBars = measures->size();

    auto sections = groupMeasures(parsed["measures"]);
    result.skeleton = buildSkeleton(sections, parsed);
    result.melodyReference = buildMelodyReference(parsed["measures"], sections);

    return result;
}

juce::var LeadSheetConverter::convertSectionsToMeasures(const juce::var& parsed)
{
    // Convert legacy "sections" format to "measures" format:
    // Input:  {"sections": [{"name": "verse", "measures": 4, "chords": ["C", "Am", "F", "G"]}]}
    // Output: {"measures": [{"meas": 1, "ch": "C"}, {"meas": 2, "ch": "Am"}, ...]}

    auto* sections = parsed["sections"].getArray();
    if (!sections || sections->isEmpty())
        return {};

    auto* doc = new juce::DynamicObject();
    if (parsed.hasProperty("title"))
        doc->setProperty("title", parsed["title"]);
    if (parsed.hasProperty("tempo"))
        doc->setProperty("tempo", parsed["tempo"]);

    juce::Array<juce::var> measures;
    int measNum = 1;

    for (const auto& section : *sections)
    {
        int numMeasures = section.hasProperty("measures") ? (int)section["measures"] : 0;
        auto* chords = section["chords"].getArray();

        if (chords && !chords->isEmpty())
        {
            // If more chords than measures, use chord count as measure count
            int count = numMeasures > 0 ? numMeasures : chords->size();

            for (int i = 0; i < count; i++)
            {
                auto* m = new juce::DynamicObject();
                m->setProperty("meas", measNum++);

                // Assign chord: cycle through chords array
                juce::String chord = (*chords)[i % chords->size()].toString();
                m->setProperty("ch", chord);

                measures.add(juce::var(m));
            }
        }
        else if (numMeasures > 0)
        {
            // Sections with measure count but no chords
            for (int i = 0; i < numMeasures; i++)
            {
                auto* m = new juce::DynamicObject();
                m->setProperty("meas", measNum++);
                m->setProperty("ch", "");
                measures.add(juce::var(m));
            }
        }
    }

    doc->setProperty("measures", juce::var(measures));
    return juce::var(doc);
}

std::vector<LeadSheetConverter::Section> LeadSheetConverter::groupMeasures(const juce::var& measures)
{
    std::vector<Section> sections;
    auto* arr = measures.getArray();
    if (!arr || arr->isEmpty())
        return sections;

    const double beatsPerBar = 4.0;

    // First pass: resolve empty chords and split multi-chord bars into a flat
    // list of (chord, measureNumber, beats, isSecondHalf) entries.
    struct ChordEntry {
        juce::String chord;
        int measure;        // 1-based measure number
        double beats;
        bool isFirstHalf;   // true = first half of a split bar (don't advance source measure)
    };
    std::vector<ChordEntry> entries;

    juce::String prevChord;

    for (int i = 0; i < arr->size(); i++)
    {
        juce::String rawChord = (*arr)[i]["ch"].toString().trim();

        // Empty chord = inherit previous
        if (rawChord.isEmpty() && !prevChord.isEmpty())
            rawChord = prevChord;

        // Multi-chord bar (e.g. "D-7 G7"): use first chord for the whole bar.
        // Splitting creates extra sections that double the bar count.
        juce::StringArray tokens;
        tokens.addTokens(rawChord, " ", "");
        tokens.removeEmptyStrings();

        if (tokens.size() >= 2)
            rawChord = tokens[0];

        entries.push_back({rawChord, i + 1, beatsPerBar, false});
        if (rawChord.isNotEmpty())
            prevChord = rawChord;
    }

    // Second pass: group consecutive same-chord entries
    if (entries.empty())
        return sections;

    juce::String currentChord = entries[0].chord;
    int startMeasure = entries[0].measure;
    double totalBeats = entries[0].beats;
    int barCount = 1;
    // sourceMeasures: count distinct source measures consumed
    // First-half of a split bar contributes 0 source measures (the second half contributes 1)
    int srcMeasures = entries[0].isFirstHalf ? 0 : 1;

    for (size_t i = 1; i < entries.size(); i++)
    {
        if (entries[i].chord == currentChord)
        {
            totalBeats += entries[i].beats;
            if (entries[i].measure != entries[i-1].measure)
                barCount++;
            if (!entries[i].isFirstHalf)
                srcMeasures++;
        }
        else
        {
            sections.push_back({currentChord, startMeasure, barCount, totalBeats, srcMeasures});
            currentChord = entries[i].chord;
            startMeasure = entries[i].measure;
            totalBeats = entries[i].beats;
            barCount = 1;
            srcMeasures = entries[i].isFirstHalf ? 0 : 1;
        }
    }
    sections.push_back({currentChord, startMeasure, barCount, totalBeats, srcMeasures});

    return sections;
}

juce::String LeadSheetConverter::buildSkeleton(const std::vector<Section>& sections,
                                                const juce::var& leadSheet)
{
    using namespace juce;

    const double beatsPerBar = 4.0;

    auto* doc = new DynamicObject();
    doc->setProperty("version", "3.0");

    int tempo = leadSheet.hasProperty("tempo") ? (int)leadSheet["tempo"] : 120;
    if (tempo <= 0) tempo = 120;
    doc->setProperty("tempo", tempo);

    // Top-level context: use first non-empty, non-N.C. chord
    auto* ctx = new DynamicObject();
    ctx->setProperty("mode_system", "chord");
    for (const auto& s : sections)
    {
        if (s.chord.isNotEmpty() && s.chord != "N.C.")
        {
            ctx->setProperty("mode", s.chord);
            break;
        }
    }
    doc->setProperty("context", var(ctx));

    // Define section with multiple FILL placeholders for rhythmic variety
    auto* define = new DynamicObject();
    define->setProperty("groove",
        "FILL: create a 4-beat (1 bar) comping/groove pattern, degree-based. "
        "Syncopated rhythm. Events from beat 0 to beat 3.");
    define->setProperty("fill",
        "FILL: create a 4-beat (1 bar) fill/variation pattern, degree-based. "
        "DIFFERENT rhythm from groove — more motion or sparser. Events from beat 0 to beat 3.");
    doc->setProperty("define", var(define));

    // Root: sequential children, one per chord section
    // Alternate groove/fill for variety, with fill at transitions
    auto* root = new DynamicObject();
    root->setProperty("mode", "sequential");

    Array<var> children;
    int sectionIdx = 0;
    for (const auto& section : sections)
    {
        auto* child = new DynamicObject();

        // Context with chord (N.C. gets empty mode so parser doesn't choke)
        auto* childCtx = new DynamicObject();
        if (section.chord != "N.C.")
            childCtx->setProperty("mode", section.chord);
        child->setProperty("context", var(childCtx));

        child->setProperty("mode", "simultaneous");

        int repeatCount = std::max(1, section.numBars);

        // For multi-bar sections, use groove for most bars + fill for last bar
        // Wrap in sequential so they play in order within the simultaneous section
        Array<var> sectionChildren;
        if (repeatCount >= 2)
        {
            auto* seqWrapper = new DynamicObject();
            seqWrapper->setProperty("mode", "sequential");

            Array<var> seqChildren;
            auto* grooveRef = new DynamicObject();
            grooveRef->setProperty("ref", "groove");
            grooveRef->setProperty("repeat", repeatCount - 1);
            grooveRef->setProperty("beats", beatsPerBar);
            seqChildren.add(var(grooveRef));

            auto* fillRef = new DynamicObject();
            fillRef->setProperty("ref", "fill");
            fillRef->setProperty("repeat", 1);
            fillRef->setProperty("beats", beatsPerBar);
            seqChildren.add(var(fillRef));

            seqWrapper->setProperty("children", var(seqChildren));
            sectionChildren.add(var(seqWrapper));
        }
        else
        {
            // Single-bar sections: alternate groove and fill
            auto* patternRef = new DynamicObject();
            patternRef->setProperty("ref", (sectionIdx % 2 == 0) ? "groove" : "fill");
            patternRef->setProperty("repeat", 1);
            patternRef->setProperty("beats", beatsPerBar);
            sectionChildren.add(var(patternRef));
        }

        child->setProperty("children", var(sectionChildren));
        children.add(var(child));
        sectionIdx++;
    }

    root->setProperty("children", var(children));
    doc->setProperty("root", var(root));

    return JSON::toString(var(doc), false);
}

juce::String LeadSheetConverter::buildMelodyReference(const juce::var& measures,
                                                       const std::vector<Section>& sections)
{
    auto* arr = measures.getArray();
    if (!arr || arr->isEmpty())
        return {};

    // Check if any measure has melody data
    bool hasMelody = false;
    for (const auto& m : *arr)
    {
        if (m.hasProperty("mel"))
        {
            auto mel = m["mel"];
            auto* nts = mel["nts"].getArray();
            if (nts && !nts->isEmpty())
            {
                // Check it's not just rests
                for (const auto& n : *nts)
                {
                    if (n.toString() != "r")
                    {
                        hasMelody = true;
                        break;
                    }
                }
            }
            if (hasMelody) break;
        }
    }
    if (!hasMelody)
        return {};

    juce::String ref;

    // Build melody reference grouped by section
    // Track which measure index we're at in the original array
    int measIndex = 0;
    for (const auto& section : sections)
    {
        // sourceMeasures tells us how many source array entries this section covers
        // (0 for first half of a split bar, 1+ for normal sections)
        int measCount = section.sourceMeasures;

        int endMeas = measIndex + measCount;
        juce::String sectionNotes;

        for (int m = measIndex; m < endMeas && m < arr->size(); m++)
        {
            auto meas = (*arr)[m];
            if (!meas.hasProperty("mel"))
                continue;

            auto mel = meas["mel"];
            auto* notes = mel["nts"].getArray();
            auto* durs = mel["dur"].getArray();
            if (!notes || notes->isEmpty())
                continue;

            for (int n = 0; n < notes->size(); n++)
            {
                juce::String noteName = (*notes)[n].toString();
                if (noteName == "r") continue; // skip rests

                // Parse duration from "beat:dur" format
                juce::String durStr;
                if (durs && n < durs->size())
                {
                    juce::String durEntry = (*durs)[n].toString();
                    int colonIdx = durEntry.indexOf(":");
                    if (colonIdx >= 0)
                        durStr = durEntry.substring(colonIdx + 1);
                    else
                        durStr = durEntry;
                }

                if (sectionNotes.isNotEmpty())
                    sectionNotes += " ";
                sectionNotes += noteName;
                if (durStr.isNotEmpty())
                    sectionNotes += "(" + durStr + ")";
            }
        }

        if (sectionNotes.isNotEmpty())
        {
            int startM = section.startMeasure;
            int endM = section.startMeasure + measCount - 1;
            juce::String chordLabel = section.chord.isNotEmpty() ? section.chord : "?";
            if (startM == endM)
                ref += "  m" + juce::String(startM) + " (" + chordLabel + "): " + sectionNotes + "\n";
            else
                ref += "  m" + juce::String(startM) + "-" + juce::String(endM) + " (" + chordLabel + "): " + sectionNotes + "\n";
        }

        measIndex = endMeas;
    }

    return ref;
}

} // namespace mj3
} // namespace covertracks
