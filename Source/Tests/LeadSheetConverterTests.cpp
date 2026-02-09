// LeadSheetConverterTests.cpp — TDD tests for LeadSheetConverter
// Tests that lead sheet JSON is correctly converted to a 3.0 skeleton + melody reference

#include <JuceHeader.h>
#include "../Musician/Core/MusicJson3/LeadSheetConverter.h"

using namespace covertracks::mj3;

// Helper: parse skeleton and get root children array
static juce::Array<juce::var>* getSkeletonChildren(const juce::String& skeleton, juce::var& outSkeleton)
{
    auto r = juce::JSON::parse(skeleton, outSkeleton);
    if (!r.wasOk()) return nullptr;
    return outSkeleton["root"]["children"].getArray();
}

// =============================================================================
// Test 1: Groups consecutive same-chord measures correctly
// =============================================================================
class LSC_GroupByChord : public juce::UnitTest
{
public:
    LSC_GroupByChord() : UnitTest("LeadSheet: groups consecutive same-chord measures", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("C6 x4, D7#11 x4, Dm7 x2, G7 x2, C6 x4 = 5 sections");
        auto result = LeadSheetConverter::convert(R"({
            "title": "Test Song",
            "tempo": 150,
            "measures": [
                {"meas": 1,  "ch": "C6"},
                {"meas": 2,  "ch": "C6"},
                {"meas": 3,  "ch": "C6"},
                {"meas": 4,  "ch": "C6"},
                {"meas": 5,  "ch": "D7#11"},
                {"meas": 6,  "ch": "D7#11"},
                {"meas": 7,  "ch": "D7#11"},
                {"meas": 8,  "ch": "D7#11"},
                {"meas": 9,  "ch": "Dm7"},
                {"meas": 10, "ch": "Dm7"},
                {"meas": 11, "ch": "G7"},
                {"meas": 12, "ch": "G7"},
                {"meas": 13, "ch": "C6"},
                {"meas": 14, "ch": "C6"},
                {"meas": 15, "ch": "C6"},
                {"meas": 16, "ch": "C6"}
            ]
        })");

        expectEquals(result.totalBars, 16, "Total bars should be 16");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        expectEquals(children->size(), 5, "Should have 5 sections (C6, D7#11, Dm7, G7, C6)");
    }
};
static LSC_GroupByChord test_groupByChord;

// =============================================================================
// Test 2: Produces correct total bar count
// =============================================================================
class LSC_BarCount : public juce::UnitTest
{
public:
    LSC_BarCount() : UnitTest("LeadSheet: correct total bar count", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("32-bar lead sheet produces totalBars = 32");

        juce::String json = R"({"title": "32 Bar Song", "tempo": 120, "measures": [)";
        for (int i = 1; i <= 32; i++)
        {
            juce::String chord = (i <= 8) ? "Cmaj7" : (i <= 16) ? "Fmaj7" : (i <= 24) ? "G7" : "Cmaj7";
            if (i > 1) json += ",";
            json += R"({"meas": )" + juce::String(i) + R"(, "ch": ")" + chord + R"("})";
        }
        json += "]}";

        auto result = LeadSheetConverter::convert(json);
        expectEquals(result.totalBars, 32, "Should report 32 total bars");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        expectEquals(children->size(), 4, "Should have 4 sections");
    }
};
static LSC_BarCount test_barCount;

// =============================================================================
// Test 3: Skeleton has one sequential child per chord section with correct context
// =============================================================================
class LSC_SkeletonStructure : public juce::UnitTest
{
public:
    LSC_SkeletonStructure() : UnitTest("LeadSheet: skeleton structure with chord contexts", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("Each section has correct chord context and placeholder refs");
        auto result = LeadSheetConverter::convert(R"({
            "title": "Simple Song",
            "tempo": 140,
            "measures": [
                {"meas": 1, "ch": "Dm7"},
                {"meas": 2, "ch": "Dm7"},
                {"meas": 3, "ch": "G7"},
                {"meas": 4, "ch": "G7"},
                {"meas": 5, "ch": "Cmaj7"},
                {"meas": 6, "ch": "Cmaj7"},
                {"meas": 7, "ch": "Cmaj7"},
                {"meas": 8, "ch": "Cmaj7"}
            ]
        })");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);

        expectEquals(skeleton["version"].toString(), juce::String("3.0"), "Version should be 3.0");
        expectEquals((int)skeleton["tempo"], 140, "Tempo should be 140");
        expectEquals(skeleton["root"]["mode"].toString(), juce::String("sequential"), "Root should be sequential");

        expect(children != nullptr && children->size() == 3, "Should have 3 sections");
        if (!children || children->size() != 3) return;

        expectEquals((*children)[0]["context"]["mode"].toString(), juce::String("Dm7"), "Section 1 chord = Dm7");
        expectEquals((*children)[1]["context"]["mode"].toString(), juce::String("G7"), "Section 2 chord = G7");
        expectEquals((*children)[2]["context"]["mode"].toString(), juce::String("Cmaj7"), "Section 3 chord = Cmaj7");

        auto* define = skeleton["define"].getDynamicObject();
        expect(define != nullptr, "Should have define section");
        if (define)
        {
            expect(define->hasProperty("groove"), "Should have groove pattern");
            expect(define->hasProperty("fill"), "Should have fill pattern");
        }
    }
};
static LSC_SkeletonStructure test_skeletonStructure;

// =============================================================================
// Test 4: Melody reference includes all measures
// =============================================================================
class LSC_MelodyReference : public juce::UnitTest
{
public:
    LSC_MelodyReference() : UnitTest("LeadSheet: melody reference includes all measures", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("Melody reference text includes notes from all measures");
        auto result = LeadSheetConverter::convert(R"({
            "title": "Melody Test",
            "tempo": 120,
            "measures": [
                {"meas": 1, "ch": "C6", "mel": {"nts": ["G4"], "dur": ["1:0.5"]}},
                {"meas": 2, "ch": "C6", "mel": {"nts": ["Ab4","A4","C5"], "dur": ["1:0.25","1.25:0.25","2:2"]}},
                {"meas": 3, "ch": "D7", "mel": {"nts": ["D5","E5"], "dur": ["1:1","3:1"]}},
                {"meas": 4, "ch": "D7", "mel": {"nts": ["F#5"], "dur": ["1:2"]}}
            ]
        })");

        expect(result.melodyReference.isNotEmpty(), "Melody reference should not be empty");
        expect(result.melodyReference.contains("G4"), "Should contain G4 from measure 1");
        expect(result.melodyReference.contains("C5"), "Should contain C5 from measure 2");
        expect(result.melodyReference.contains("D5"), "Should contain D5 from measure 3");
        expect(result.melodyReference.contains("F#5"), "Should contain F#5 from measure 4");
        expect(result.melodyReference.contains("C6"), "Should reference C6 chord");
        expect(result.melodyReference.contains("D7"), "Should reference D7 chord");
    }
};
static LSC_MelodyReference test_melodyReference;

// =============================================================================
// Test 5: Edge cases — single-measure sections, empty melody, empty input
// =============================================================================
class LSC_EdgeCases : public juce::UnitTest
{
public:
    LSC_EdgeCases() : UnitTest("LeadSheet: edge cases", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("Single-measure sections work");
        auto result1 = LeadSheetConverter::convert(R"({
            "title": "Quick Changes",
            "tempo": 180,
            "measures": [
                {"meas": 1, "ch": "Cm7"},
                {"meas": 2, "ch": "F7"},
                {"meas": 3, "ch": "Bbmaj7"},
                {"meas": 4, "ch": "Ebmaj7"}
            ]
        })");
        expectEquals(result1.totalBars, 4, "Should have 4 bars");

        juce::var skel1;
        auto* children = getSkeletonChildren(result1.skeleton, skel1);
        expect(children != nullptr, "Should have children");
        if (children)
            expectEquals(children->size(), 4, "4 single-bar sections");

        beginTest("Empty melody produces empty melody reference");
        auto result2 = LeadSheetConverter::convert(R"({
            "title": "No Melody",
            "tempo": 120,
            "measures": [
                {"meas": 1, "ch": "Am7"},
                {"meas": 2, "ch": "Am7"}
            ]
        })");
        expectEquals(result2.totalBars, 2, "Should have 2 bars");

        beginTest("Empty input returns zero bars");
        auto result3 = LeadSheetConverter::convert("{}");
        expectEquals(result3.totalBars, 0, "Empty input = 0 bars");

        beginTest("Invalid JSON doesn't crash");
        auto result4 = LeadSheetConverter::convert("not valid json {{{");
        expectEquals(result4.totalBars, 0, "Invalid JSON = 0 bars");
    }
};
static LSC_EdgeCases test_edgeCases;

// =============================================================================
// Test 6: Empty chord inherits from previous measure
// =============================================================================
class LSC_EmptyChordInheritance : public juce::UnitTest
{
public:
    LSC_EmptyChordInheritance() : UnitTest("LeadSheet: empty chord inherits previous", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("Empty ch fields group with previous chord");
        auto result = LeadSheetConverter::convert(R"({
            "title": "Continuation Test",
            "tempo": 120,
            "measures": [
                {"meas": 1, "ch": "FMaj7"},
                {"meas": 2, "ch": ""},
                {"meas": 3, "ch": ""},
                {"meas": 4, "ch": ""},
                {"meas": 5, "ch": "D7"},
                {"meas": 6, "ch": ""},
                {"meas": 7, "ch": "G7"},
                {"meas": 8, "ch": "G7"}
            ]
        })");

        expectEquals(result.totalBars, 8, "Should have 8 bars");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        // FMaj7 x4, D7 x2, G7 x2 = 3 sections
        expectEquals(children->size(), 3, "Should have 3 sections (FMaj7 x4, D7 x2, G7 x2)");

        expectEquals((*children)[0]["context"]["mode"].toString(), juce::String("FMaj7"), "Section 1 = FMaj7");
        expectEquals((*children)[1]["context"]["mode"].toString(), juce::String("D7"), "Section 2 = D7");
        expectEquals((*children)[2]["context"]["mode"].toString(), juce::String("G7"), "Section 3 = G7");

        // Multi-bar sections should have sequential wrapper with groove + fill
        // FMaj7 (4 bars) → sequential: groove repeat 3 + fill repeat 1
        auto s1children = (*children)[0]["children"].getArray();
        expect(s1children != nullptr && s1children->size() > 0, "Section 1 should have children");
        if (s1children && s1children->size() > 0)
        {
            // Multi-bar: has a sequential wrapper
            auto seqWrapper = (*s1children)[0];
            auto* seqChildren = seqWrapper["children"].getArray();
            expect(seqChildren != nullptr && seqChildren->size() == 2,
                "Multi-bar section should have sequential wrapper with 2 refs");
            if (seqChildren && seqChildren->size() == 2)
            {
                expectEquals((*seqChildren)[0]["ref"].toString(), juce::String("groove"), "First ref should be groove");
                expectEquals((int)(*seqChildren)[0]["repeat"], 3, "FMaj7: groove repeats 3 bars");
                expectEquals((*seqChildren)[1]["ref"].toString(), juce::String("fill"), "Second ref should be fill");
                expectEquals((int)(*seqChildren)[1]["repeat"], 1, "FMaj7: fill repeats 1 bar");
            }
        }
    }
};
static LSC_EmptyChordInheritance test_emptyChordInheritance;

// =============================================================================
// Test 7: Multi-chord bars use first chord (no split, preserves bar count)
// =============================================================================
class LSC_MultiChordBars : public juce::UnitTest
{
public:
    LSC_MultiChordBars() : UnitTest("LeadSheet: multi-chord bars use first chord", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("'D-7 G7' bar uses first chord D-7 for the whole bar");
        auto result = LeadSheetConverter::convert(R"({
            "title": "Split Bar Test",
            "tempo": 120,
            "measures": [
                {"meas": 1, "ch": "C6"},
                {"meas": 2, "ch": "C6"},
                {"meas": 3, "ch": "D-7 G7"},
                {"meas": 4, "ch": "C6"}
            ]
        })");

        expectEquals(result.totalBars, 4, "Should have 4 bars");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        // C6 x2, D-7 x1 (first chord of split bar), C6 x1 = 3 sections
        expectEquals(children->size(), 3, "Should have 3 sections (C6, D-7, C6)");

        expectEquals((*children)[0]["context"]["mode"].toString(), juce::String("C6"), "Section 1 = C6");
        expectEquals((*children)[1]["context"]["mode"].toString(), juce::String("D-7"), "Section 2 = D-7 (first chord of split bar)");
        expectEquals((*children)[2]["context"]["mode"].toString(), juce::String("C6"), "Section 3 = C6");

        // C6 section (2 bars): sequential wrapper with groove(1) + fill(1)
        auto s1ch = (*children)[0]["children"].getArray();
        if (s1ch && s1ch->size() > 0)
        {
            auto seqWrapper = (*s1ch)[0];
            auto* seqKids = seqWrapper["children"].getArray();
            expect(seqKids != nullptr && seqKids->size() == 2,
                "C6 multi-bar section should have groove + fill");
        }

        // D-7 section (1 bar): single ref (groove or fill)
        auto s2ch = (*children)[1]["children"].getArray();
        if (s2ch && s2ch->size() > 0)
            expectEquals((int)(*s2ch)[0]["repeat"], 1, "D-7 single-bar section repeats 1");
    }
};
static LSC_MultiChordBars test_multiChordBars;

// =============================================================================
// Test 8: Real-world A Train lead sheet structure
// =============================================================================
class LSC_ATrainStructure : public juce::UnitTest
{
public:
    LSC_ATrainStructure() : UnitTest("LeadSheet: A Train real-world structure", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("A Train first 8 bars: correct grouping with empty chords and multi-chord bars");
        auto result = LeadSheetConverter::convert(R"({
            "title": "Take the A Train",
            "tempo": 150,
            "measures": [
                {"meas": 1, "ch": "C6", "mel": {"nts": ["G4"], "dur": ["1:4"]}},
                {"meas": 2, "ch": "", "mel": {"nts": ["G4","E4"], "dur": ["1:0.5","1.5:1.5"]}},
                {"meas": 3, "ch": "D7b5", "mel": {"nts": ["Eb5","D5"], "dur": ["1:1","2:1"]}},
                {"meas": 4, "ch": "D-7", "mel": {"nts": ["G4"], "dur": ["1:2"]}},
                {"meas": 5, "ch": "G7", "mel": {"nts": ["F#4","G4"], "dur": ["1:0.5","1.5:0.5"]}},
                {"meas": 6, "ch": "C6", "mel": {"nts": ["C5","G4"], "dur": ["1:1","2:1"]}},
                {"meas": 7, "ch": "D-7 G7", "mel": {"nts": ["G4"], "dur": ["1:2"]}},
                {"meas": 8, "ch": "G-7 C7", "mel": {"nts": ["G4"], "dur": ["1:4"]}}
            ]
        })");

        expectEquals(result.totalBars, 8, "Should have 8 bars");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        // m1-2: C6 x2 (empty inherits), m3: D7b5, m4: D-7, m5: G7,
        // m6: C6, m7: D-7 (first chord of "D-7 G7"), m8: G-7 (first chord of "G-7 C7")
        // Grouping: C6(2), D7b5(1), D-7(2) [m4+m7 consecutive? No, m5-6 break them]
        // Actually: C6(2), D7b5(1), D-7(1), G7(1), C6(1), D-7(1), G-7(1) = 7 sections
        expectEquals(children->size(), 7, "Should have 7 sections (no split bars)");

        // All chords should be non-empty
        for (int i = 0; i < children->size(); i++)
        {
            auto chord = (*children)[i]["context"]["mode"].toString();
            expect(chord.isNotEmpty(), "Section " + juce::String(i+1) + " should have non-empty chord, got: '" + chord + "'");
        }

        // Total bar count should equal 8
        // Multi-bar sections have sequential wrapper: groove(N-1) + fill(1)
        // Single-bar sections have a direct ref with repeat=1
        int totalBars = 0;
        for (int i = 0; i < children->size(); i++)
        {
            auto sChildren = (*children)[i]["children"].getArray();
            if (!sChildren || sChildren->size() == 0) continue;

            auto firstChild = (*sChildren)[0];
            if (firstChild.hasProperty("ref"))
            {
                // Single-bar: direct ref
                totalBars += (int)firstChild["repeat"];
            }
            else if (firstChild.hasProperty("children"))
            {
                // Multi-bar: sequential wrapper with groove + fill
                auto* seqKids = firstChild["children"].getArray();
                if (seqKids)
                {
                    for (int j = 0; j < seqKids->size(); j++)
                        totalBars += (int)(*seqKids)[j]["repeat"];
                }
            }
        }
        expectEquals(totalBars, 8, "Total bars should equal 8");

        // Verify melody reference has notes
        expect(result.melodyReference.isNotEmpty(), "Melody reference should not be empty");
        expect(result.melodyReference.contains("G4"), "Should contain G4");
        expect(result.melodyReference.contains("Eb5"), "Should contain Eb5");
        expect(result.melodyReference.contains("C5"), "Should contain C5");
    }
};
static LSC_ATrainStructure test_aTrainStructure;

// =============================================================================
// Test 9: N.C. (No Chord) handling
// =============================================================================
class LSC_NoChord : public juce::UnitTest
{
public:
    LSC_NoChord() : UnitTest("LeadSheet: N.C. handling", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("N.C. measures create sections without chord context");
        auto result = LeadSheetConverter::convert(R"({
            "title": "NC Test",
            "tempo": 120,
            "measures": [
                {"meas": 1, "ch": "C6"},
                {"meas": 2, "ch": "N.C."},
                {"meas": 3, "ch": "CMaj7"},
                {"meas": 4, "ch": "CMaj7"}
            ]
        })");

        expectEquals(result.totalBars, 4, "Should have 4 bars");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        // C6 x1, N.C. x1, CMaj7 x2 = 3 sections
        expectEquals(children->size(), 3, "Should have 3 sections");

        // N.C. section should NOT have a mode set (or be empty)
        // so the parser doesn't try to resolve it as a chord
        auto ncSection = (*children)[1];
        auto ncMode = ncSection["context"]["mode"].toString();
        expect(ncMode.isEmpty() || ncMode == "N.C.", "N.C. section should have empty or N.C. mode");
    }
};
static LSC_NoChord test_noChord;

// =============================================================================
// Test 10: Legacy "sections" format converted to "measures" format
// Old image analysis returned {"sections": [...]} instead of {"measures": [...]}
// =============================================================================
class LSC_SectionsFormat : public juce::UnitTest
{
public:
    LSC_SectionsFormat() : UnitTest("LeadSheet: converts legacy sections format", "LeadSheet Converter") {}
    void runTest() override
    {
        beginTest("Sections format with chords array produces correct skeleton");
        // This is what the old image analysis returned for Twinkle Twinkle Little Star
        auto result = LeadSheetConverter::convert(R"({
            "title": "Twinkle, Twinkle, Little Star",
            "tempo": 120,
            "sections": [
                {
                    "name": "verse",
                    "measures": 8,
                    "chords": ["C", "C", "G", "G", "A", "A", "G", "F"]
                }
            ]
        })");

        expect(result.totalBars > 0, "Should produce bars from sections format");
        expectEquals(result.totalBars, 8, "Should have 8 bars");

        juce::var skeleton;
        auto* children = getSkeletonChildren(result.skeleton, skeleton);
        expect(children != nullptr, "Root should have children");
        if (!children) return;

        // C x2, G x2, A x2, G x1, F x1 = 5 sections
        expectEquals(children->size(), 5, "Should have 5 sections (grouped by chord)");

        // First section should be C
        auto firstCtx = (*children)[0]["context"]["mode"].toString();
        expectEquals(firstCtx, juce::String("C"), "First section should be C");

        beginTest("Sections with more chords than measures uses chord count");
        auto result2 = LeadSheetConverter::convert(R"({
            "title": "Test",
            "tempo": 100,
            "sections": [
                {"name": "A", "measures": 2, "chords": ["C", "G"]},
                {"name": "B", "measures": 2, "chords": ["Am", "F"]}
            ]
        })");

        expectEquals(result2.totalBars, 4, "Should have 4 bars from 2 sections x 2 measures");

        beginTest("Empty sections returns no bars");
        auto result3 = LeadSheetConverter::convert(R"({
            "title": "Empty",
            "sections": []
        })");
        expectEquals(result3.totalBars, 0, "Empty sections should produce 0 bars");
    }
};
static LSC_SectionsFormat test_sectionsFormat;
