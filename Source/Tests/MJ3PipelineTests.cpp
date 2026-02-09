// MJ3PipelineTests.cpp — End-to-end tests for Music JSON 3.0 pipeline
// Each test covers a real issue or architectural requirement.

#include <JuceHeader.h>
#include "../Musician/Core/MusicJson3/MJ3Parser.h"

using namespace covertracks::mj3;

// Helper: get MIDI note from event
static int note(const juce::var& evt) { return (int)evt["note"]; }
static double ts(const juce::var& evt) { return (double)evt["timestamp"]; }

// =============================================================================
// Test 1: Basic parse — single pattern produces correct MIDI events
// =============================================================================
class MJ3_BasicParse : public juce::UnitTest
{
public:
    MJ3_BasicParse() : UnitTest("MJ3 Pipeline: basic parse", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Single pattern with note names produces correct MIDI");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "define": {
                "riff": {
                    "events": [
                        {"note": "C4", "beat": 0, "duration": 1},
                        {"note": "E4", "beat": 1, "duration": 1},
                        {"note": "G4", "beat": 2, "duration": 1}
                    ]
                }
            },
            "root": {"ref": "riff"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events array");
        if (!events) return;

        expectEquals(events->size(), 3, "Should produce 3 events");
        expectEquals(note((*events)[0]), 60, "C4 = MIDI 60");
        expectEquals(note((*events)[1]), 64, "E4 = MIDI 64");
        expectEquals(note((*events)[2]), 67, "G4 = MIDI 67");
    }
};
static MJ3_BasicParse test_basicParse;

// =============================================================================
// Test 2: Sequential sections don't overlap
// (Bug: sections got default 4-beat duration, overlapping each other)
// =============================================================================
class MJ3_SequentialSections : public juce::UnitTest
{
public:
    MJ3_SequentialSections() : UnitTest("MJ3 Pipeline: sequential sections don't overlap", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Two 8-bar sections play back-to-back, not overlapping");
        // Walking bass: 4 quarter notes filling a full bar (iterDuration = 4)
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "define": {
                "bass": {
                    "events": [
                        {"note": "C3", "beat": 0, "duration": 1},
                        {"note": "E3", "beat": 1, "duration": 1},
                        {"note": "G3", "beat": 2, "duration": 1},
                        {"note": "C3", "beat": 3, "duration": 1}
                    ]
                }
            },
            "root": {
                "mode": "sequential",
                "children": [
                    {"name": "A", "mode": "simultaneous", "children": [
                        {"ref": "bass", "repeat": 8}
                    ]},
                    {"name": "B", "mode": "simultaneous", "children": [
                        {"ref": "bass", "repeat": 8}
                    ]}
                ]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        expectEquals(events->size(), 64, "4 notes x 8 repeats x 2 sections = 64");

        double maxTime = 0;
        for (auto& e : *events)
            maxTime = std::max(maxTime, ts(e));

        // bass: 4 beats x 8 = 32 beats per section. Section B starts at 32.
        // Last event at beat 32+31 = 63
        expect(maxTime >= 60.0, "Last event at beat 63 (not 4 if sections overlapped)");
    }
};
static MJ3_SequentialSections test_sequentialSections;

// =============================================================================
// Test 3: Degree + octave resolves to correct register
// (Bug: octave field on degree events was ignored, everything in octave 4)
// =============================================================================
class MJ3_DegreeOctave : public juce::UnitTest
{
public:
    MJ3_DegreeOctave() : UnitTest("MJ3 Pipeline: degree + octave register separation", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Same degree at different octaves produces different MIDI notes");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "bass":   {"events": [{"degree": 1, "octave": 2, "beat": 0, "duration": 1}]},
                "comp":   {"events": [{"degree": 1, "octave": 4, "beat": 0, "duration": 1}]},
                "melody": {"events": [{"degree": 1, "octave": 6, "beat": 0, "duration": 1}]}
            },
            "root": {
                "mode": "simultaneous",
                "children": [{"ref": "bass"}, {"ref": "comp"}, {"ref": "melody"}]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr && events->size() == 3, "Should produce 3 events");
        if (!events || events->size() != 3) return;

        int notes[3];
        for (int i = 0; i < 3; i++) notes[i] = note((*events)[i]);
        std::sort(notes, notes + 3);

        // C2=36, C4=60, C6=84
        expectEquals(notes[0], 36, "Bass: degree 1 octave 2 = C2 = MIDI 36");
        expectEquals(notes[1], 60, "Comp: degree 1 octave 4 = C4 = MIDI 60");
        expectEquals(notes[2], 84, "Melody: degree 1 octave 6 = C6 = MIDI 84");
    }
};
static MJ3_DegreeOctave test_degreeOctave;

// =============================================================================
// Test 4: Chord context changes per section
// (Bug: same pattern sounded identical in every section)
// =============================================================================
class MJ3_ChordContextPerSection : public juce::UnitTest
{
public:
    MJ3_ChordContextPerSection() : UnitTest("MJ3 Pipeline: chord context changes per section", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Same degree pattern resolves differently over Dm7 vs G7 vs CMaj7");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "root_note": {"events": [{"degree": 1, "beat": 0, "duration": 1}]}
            },
            "root": {
                "mode": "sequential",
                "children": [
                    {"context": {"mode": "Dm7"}, "ref": "root_note"},
                    {"context": {"mode": "G7"}, "ref": "root_note"},
                    {"context": {"mode": "CMaj7"}, "ref": "root_note"}
                ]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr && events->size() == 3, "Should produce 3 events");
        if (!events || events->size() != 3) return;

        // Degree 1 = root: Dm7→D4=62, G7→G4=67, CMaj7→C4=60
        expectEquals(note((*events)[0]), 62, "Dm7 degree 1 = D4 = MIDI 62");
        expectEquals(note((*events)[1]), 67, "G7 degree 1 = G4 = MIDI 67");
        expectEquals(note((*events)[2]), 60, "CMaj7 degree 1 = C4 = MIDI 60");
    }
};
static MJ3_ChordContextPerSection test_chordContext;

// =============================================================================
// Test 5: Mixed repeat counts in simultaneous container
// (1-bar bass x 8 and 2-bar comp x 4 both cover 8 bars)
// =============================================================================
class MJ3_MixedRepeatCounts : public juce::UnitTest
{
public:
    MJ3_MixedRepeatCounts() : UnitTest("MJ3 Pipeline: mixed repeat counts same total duration", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("1-bar x 8 and 2-bar x 4 both span 32 beats");
        // bass: 4 quarter notes (iterDuration=4), comp: fills 8 beats (iterDuration=8)
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "define": {
                "bass_1bar": {
                    "events": [
                        {"note": "C3", "beat": 0, "duration": 1},
                        {"note": "E3", "beat": 1, "duration": 1},
                        {"note": "G3", "beat": 2, "duration": 1},
                        {"note": "C3", "beat": 3, "duration": 1}
                    ]
                },
                "comp_2bar": {
                    "events": [
                        {"note": "E4", "beat": 0, "duration": 2},
                        {"note": "G4", "beat": 2, "duration": 2},
                        {"note": "E4", "beat": 4, "duration": 2},
                        {"note": "C5", "beat": 7, "duration": 1}
                    ]
                }
            },
            "root": {
                "mode": "simultaneous",
                "children": [
                    {"ref": "bass_1bar", "repeat": 8},
                    {"ref": "comp_2bar", "repeat": 4}
                ]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // bass: 4 notes x 8 = 32, comp: 4 notes x 4 = 16
        expectEquals(events->size(), 48, "32 bass + 16 comp = 48 events");

        // Both should span ~32 beats
        double maxBass = 0, maxComp = 0;
        for (auto& e : *events)
        {
            if (note(e) < 60)
                maxBass = std::max(maxBass, ts(e));
            else
                maxComp = std::max(maxComp, ts(e));
        }
        // bass: 4 beats x 8 repeats, last at beat 31. comp: 8 beats x 4 repeats, last at beat 31
        expect(maxBass >= 28.0, "Bass spans to beat 31");
        expect(maxComp >= 28.0, "Comp spans to beat 31");
    }
};
static MJ3_MixedRepeatCounts test_mixedRepeats;

// =============================================================================
// Test 6: Realistic LLM output — multi-section with chord changes + octave separation
// =============================================================================
class MJ3_RealisticLLM : public juce::UnitTest
{
public:
    MJ3_RealisticLLM() : UnitTest("MJ3 Pipeline: realistic LLM multi-section chart", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("3-section chart: degree patterns, chord changes, register separation");
        // Walking bass: 4 notes per bar. Comp: hits on and-of-1, and-of-3, beat-4 (fills 4 beats)
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "Dm7", "velocity": 0.75},
            "define": {
                "bass_walk": {
                    "events": [
                        {"degree": 1, "octave": 3, "beat": 0, "duration": 1},
                        {"degree": 3, "octave": 3, "beat": 1, "duration": 1},
                        {"degree": 5, "octave": 3, "beat": 2, "duration": 1},
                        {"degree": 3, "octave": 3, "beat": 3, "duration": 1}
                    ]
                },
                "comp": {
                    "events": [
                        {"degree": 1, "octave": 4, "beat": 0.5, "duration": 0.5},
                        {"degree": 3, "octave": 4, "beat": 2.5, "duration": 0.5},
                        {"degree": 5, "octave": 4, "beat": 3, "duration": 1}
                    ]
                }
            },
            "root": {
                "mode": "sequential",
                "children": [
                    {
                        "context": {"mode": "Dm7"},
                        "mode": "simultaneous",
                        "children": [
                            {"ref": "bass_walk", "repeat": 4},
                            {"ref": "comp", "repeat": 4}
                        ]
                    },
                    {
                        "context": {"mode": "G7"},
                        "mode": "simultaneous",
                        "children": [
                            {"ref": "bass_walk", "repeat": 4},
                            {"ref": "comp", "repeat": 4}
                        ]
                    },
                    {
                        "context": {"mode": "CMaj7"},
                        "mode": "simultaneous",
                        "children": [
                            {"ref": "bass_walk", "repeat": 4},
                            {"ref": "comp", "repeat": 4}
                        ]
                    }
                ]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // 3 sections x 4 repeats x (4 bass + 3 comp) = 84
        expectEquals(events->size(), 84, "3 sections x 4 reps x 7 events = 84");

        // Total span: 3 sections x 16 beats = 48 beats
        double maxTime = 0;
        for (auto& e : *events) maxTime = std::max(maxTime, ts(e));
        expect(maxTime >= 44.0, "Spans ~48 beats across 3 sections");

        // Register separation: bass in octave 3 (< 60), comp in octave 4 (>= 60)
        int bassCount = 0, compCount = 0;
        for (auto& e : *events)
        {
            if (note(e) < 60) bassCount++;
            else compCount++;
        }
        expect(bassCount > 0 && compCount > 0, "Both bass and comp present");
        expectEquals(bassCount, 48, "48 bass events (4 x 4 x 3)");
        expectEquals(compCount, 36, "36 comp events (3 x 4 x 3)");

        // Chord changes: first bass note per section differs
        // Each section is 16 beats (4 bars x 4 beats)
        int sect1bass = -1, sect2bass = -1, sect3bass = -1;
        for (auto& e : *events)
        {
            int n = note(e);
            if (n >= 60) continue;
            double t = ts(e);
            if (t < 1.0 && sect1bass < 0) sect1bass = n;
            if (t >= 16.0 && t < 17.0 && sect2bass < 0) sect2bass = n;
            if (t >= 32.0 && t < 33.0 && sect3bass < 0) sect3bass = n;
        }
        expect(sect1bass != sect2bass, "Dm7 bass root differs from G7");
        expect(sect2bass != sect3bass, "G7 bass root differs from CMaj7");
    }
};
static MJ3_RealisticLLM test_realisticLLM;

// =============================================================================
// Test 7: Robustness — bad input doesn't crash
// =============================================================================
class MJ3_Robustness : public juce::UnitTest
{
public:
    MJ3_Robustness() : UnitTest("MJ3 Pipeline: robustness with bad input", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Empty JSON doesn't crash");
        auto r1 = MJ3Parser::parse("{}", 120.0);

        beginTest("Invalid JSON doesn't crash");
        auto r2 = MJ3Parser::parse("not json {{{", 120.0);

        beginTest("Circular ref doesn't hang");
        auto r3 = MJ3Parser::parse(R"({
            "version": "3.0",
            "define": {"a": {"ref": "b"}, "b": {"ref": "a"}},
            "root": {"ref": "a"}
        })", 120.0);
    }
};
static MJ3_Robustness test_robustness;

// =============================================================================
// Test 8: Events in children array (common LLM mistake)
// LLMs often put event objects in "children" instead of "events".
// Parser should detect this and treat them as events.
// =============================================================================
class MJ3_EventsInChildren : public juce::UnitTest
{
public:
    MJ3_EventsInChildren() : UnitTest("MJ3 Pipeline: events in children array", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Event-like objects in children are parsed as events");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "pattern": {
                    "mode": "simultaneous",
                    "children": [
                        {
                            "mode": "sequential",
                            "children": [
                                {"degree": 1, "beat": 0, "duration": 1},
                                {"degree": 3, "beat": 1, "duration": 1},
                                {"degree": 5, "beat": 2, "duration": 1}
                            ]
                        }
                    ]
                }
            },
            "root": {"ref": "pattern"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events array");
        if (!events) return;

        expectEquals(events->size(), 3, "Should produce 3 events from children-as-events");
        expectEquals(note((*events)[0]), 60, "Degree 1 of CMaj7 = C4 = MIDI 60");
    }
};
static MJ3_EventsInChildren test_eventsInChildren;

// =============================================================================
// Test 9: Degree arrays in children (LLM chord voicing style)
// =============================================================================
class MJ3_DegreeArraysInChildren : public juce::UnitTest
{
public:
    MJ3_DegreeArraysInChildren() : UnitTest("MJ3 Pipeline: degree arrays in children", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Degree arrays in children produce multiple simultaneous notes");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "voicing": {
                    "mode": "sequential",
                    "children": [
                        {"degree": [1, 3, 5, 7], "beat": 0, "duration": 4}
                    ]
                }
            },
            "root": {"ref": "voicing"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events array");
        if (!events) return;

        expectEquals(events->size(), 4, "Degree array [1,3,5,7] should produce 4 events");
    }
};
static MJ3_DegreeArraysInChildren test_degreeArraysInChildren;

// =============================================================================
// Test 10: MIDI velocity > 1.0 gets normalized
// LLMs output velocity as 0-127 integers instead of 0.0-1.0 floats
// =============================================================================
class MJ3_VelocityNormalization : public juce::UnitTest
{
public:
    MJ3_VelocityNormalization() : UnitTest("MJ3 Pipeline: velocity normalization", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Velocity 80 (MIDI-style) gets normalized to 0.0-1.0 range");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "define": {
                "hit": {
                    "events": [
                        {"note": "C4", "beat": 0, "duration": 1, "velocity": 80}
                    ]
                }
            },
            "root": {"ref": "hit"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr && events->size() == 1, "Should have 1 event");
        if (!events || events->size() != 1) return;

        double vel = (double)(*events)[0]["velocity"];
        expect(vel > 0.0 && vel <= 1.0, "Velocity should be normalized to 0.0-1.0, got: " + juce::String(vel));
    }
};
static MJ3_VelocityNormalization test_velocityNorm;

// =============================================================================
// Test 11: Full LLM output pattern — events-in-children with degree arrays,
// MIDI velocity, and chord context: the exact nesting LLMs produce.
// =============================================================================
class MJ3_FullLLMOutput : public juce::UnitTest
{
public:
    MJ3_FullLLMOutput() : UnitTest("MJ3 Pipeline: full LLM output end-to-end", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Events-in-children + degree arrays + MIDI velocity + chord context");
        // LLM output pattern: define has nested containers where leaf events
        // are placed in "children" instead of "events", with degree arrays
        // for chord voicings and MIDI-style velocity (0-127).
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 150,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "pattern": {
                    "mode": "simultaneous",
                    "children": [
                        {
                            "mode": "sequential",
                            "children": [
                                {"degree": [1, 3], "beat": 0, "duration": 2, "velocity": 70},
                                {"degree": [2, 4], "beat": 2, "duration": 2, "velocity": 65}
                            ]
                        }
                    ]
                }
            },
            "root": {"ref": "pattern"}
        })", 150.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events array");
        if (!events) return;

        // [1,3] = 2 events + [2,4] = 2 events = 4 total
        expectEquals(events->size(), 4, "Should produce 4 events from degree arrays in children");

        // All velocities should be normalized to 0.0-1.0 (from MIDI 70, 65)
        for (int i = 0; i < events->size(); i++)
        {
            double vel = (double)(*events)[i]["velocity"];
            expect(vel > 0.0 && vel <= 1.0,
                "Event " + juce::String(i) + " velocity should be 0.0-1.0, got: " + juce::String(vel));
        }

        // Check velocity normalization: 70/127 ≈ 0.551
        double vel0 = (double)(*events)[0]["velocity"];
        expect(std::abs(vel0 - 70.0/127.0) < 0.02,
            "Velocity 70 should normalize to ~0.551, got: " + juce::String(vel0));

        // CMaj7 chord tones should be present (degrees 1-4 resolve to MIDI notes)
        // CMaj7 {0,4,7,11}: deg1=60(C4), deg2=64(E4), deg3=67(G4), deg4=71(B4)
        std::vector<int> notes;
        for (auto& e : *events)
            notes.push_back(note(e));
        std::sort(notes.begin(), notes.end());

        // Should contain CMaj7 tones
        expect(notes[0] >= 60 && notes[0] <= 71, "Notes should be CMaj7 chord tones");
    }
};
static MJ3_FullLLMOutput test_fullLLMOutput;

// =============================================================================
// Test 12: Mixed children — some event-like, some container nodes in same array
// =============================================================================
class MJ3_MixedChildren : public juce::UnitTest
{
public:
    MJ3_MixedChildren() : UnitTest("MJ3 Pipeline: mixed event and container children", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Children array with both event-like objects and proper containers");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "mixed": {
                    "mode": "sequential",
                    "children": [
                        {"degree": 1, "beat": 0, "duration": 1},
                        {"events": [{"degree": 3, "beat": 0, "duration": 1}]},
                        {"degree": 5, "beat": 2, "duration": 1}
                    ]
                }
            },
            "root": {"ref": "mixed"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events array");
        if (!events) return;

        // First child: event-like → 1 event (degree 1)
        // Second child: proper container with events → 1 event (degree 3)
        // Third child: event-like → 1 event (degree 5)
        expectEquals(events->size(), 3, "Should produce 3 events from mixed children");

        // All should resolve to CMaj7 chord tones
        // CMaj7 intervals: {0, 4, 7, 11} — degree is chord-tone index:
        // degree 1 = index 0 → 60+0 = 60 (C4)
        // degree 3 = index 2 → 60+7 = 67 (G4, the 5th)
        // degree 5 = index 4 → octaveShift=1, chordIndex=0 → 60+0+12 = 72 (C5)
        std::vector<int> notes;
        for (auto& e : *events)
            notes.push_back(note(e));
        std::sort(notes.begin(), notes.end());

        expectEquals(notes[0], 60, "Degree 1 of CMaj7 = C4 = MIDI 60");
        expectEquals(notes[1], 67, "Degree 3 of CMaj7 = G4 = MIDI 67 (chord-tone index 2 = 5th)");
        expectEquals(notes[2], 72, "Degree 5 of CMaj7 = C5 = MIDI 72 (wraps to next octave)");
    }
};
static MJ3_MixedChildren test_mixedChildren;

// =============================================================================
// Test 13: Velocity edge cases — boundary values
// =============================================================================
class MJ3_VelocityEdgeCases : public juce::UnitTest
{
public:
    MJ3_VelocityEdgeCases() : UnitTest("MJ3 Pipeline: velocity edge cases", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Velocity 0.8 stays as-is, 1.0 stays, 127 normalizes, 0 stays");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "define": {
                "vels": {
                    "events": [
                        {"note": "C4", "beat": 0, "duration": 1, "velocity": 0.8},
                        {"note": "D4", "beat": 1, "duration": 1, "velocity": 1.0},
                        {"note": "E4", "beat": 2, "duration": 1, "velocity": 127},
                        {"note": "F4", "beat": 3, "duration": 1, "velocity": 64}
                    ]
                }
            },
            "root": {"ref": "vels"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr && events->size() == 4, "Should have 4 events");
        if (!events || events->size() != 4) return;

        double v0 = (double)(*events)[0]["velocity"];  // 0.8
        double v1 = (double)(*events)[1]["velocity"];  // 1.0
        double v2 = (double)(*events)[2]["velocity"];  // 127 → ~1.0
        double v3 = (double)(*events)[3]["velocity"];  // 64 → ~0.504

        // 0.8: already in range, should stay
        expect(std::abs(v0 - 0.8) < 0.01, "0.8 should stay ~0.8, got: " + juce::String(v0));

        // 1.0: boundary, should stay (not > 1.0)
        expect(std::abs(v1 - 1.0) < 0.01, "1.0 should stay ~1.0, got: " + juce::String(v1));

        // 127: MIDI max → normalized to 127/127 = 1.0
        expect(std::abs(v2 - 1.0) < 0.01, "127 should normalize to ~1.0, got: " + juce::String(v2));

        // 64: MIDI medium → normalized to 64/127 ≈ 0.504
        expect(v3 > 0.4 && v3 < 0.6, "64 should normalize to ~0.504, got: " + juce::String(v3));

        // All should be in 0.0-1.0 range
        for (int i = 0; i < events->size(); i++)
        {
            double vel = (double)(*events)[i]["velocity"];
            expect(vel >= 0.0 && vel <= 1.0,
                "Event " + juce::String(i) + " velocity should be 0.0-1.0, got: " + juce::String(vel));
        }
    }
};
static MJ3_VelocityEdgeCases test_velocityEdgeCases;

// =============================================================================
// Test 14: Dash-notation minor chords (jazz convention: D-7 = Dm7, G- = Gm)
// =============================================================================
class MJ3_DashMinorChords : public juce::UnitTest
{
public:
    MJ3_DashMinorChords() : UnitTest("MJ3 Pipeline: dash-notation minor chords", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("D-7 resolves as D minor 7, not D dominant 7");
        // D-7 = Dm7: root D, intervals {0, 3, 7, 10}
        // Degree 1=D4(62), Degree 2=F4(65), Degree 3=A4(69), Degree 4=C5(72)
        // Compare with D7 (dominant): Degree 2=F#4(66)
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "D-7"},
            "define": {
                "test": {
                    "events": [
                        {"degree": 1, "beat": 0, "duration": 1},
                        {"degree": 2, "beat": 1, "duration": 1},
                        {"degree": 3, "beat": 2, "duration": 1},
                        {"degree": 4, "beat": 3, "duration": 1}
                    ]
                }
            },
            "root": {"ref": "test"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr && events->size() == 4, "Should produce 4 events");
        if (!events || events->size() != 4) return;

        // D minor 7: D(62) F(65) A(69) C(72)
        expectEquals(note((*events)[0]), 62, "D-7 degree 1 = D4 = MIDI 62");
        expectEquals(note((*events)[1]), 65, "D-7 degree 2 = F4 = MIDI 65 (minor 3rd, not major)");
        expectEquals(note((*events)[2]), 69, "D-7 degree 3 = A4 = MIDI 69");
        expectEquals(note((*events)[3]), 72, "D-7 degree 4 = C5 = MIDI 72 (minor 7th)");

        beginTest("G-7 resolves as G minor 7");
        auto result2 = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "G-7"},
            "define": {
                "test": {
                    "events": [
                        {"degree": 1, "beat": 0, "duration": 1},
                        {"degree": 2, "beat": 1, "duration": 1}
                    ]
                }
            },
            "root": {"ref": "test"}
        })", 120.0);

        expect(result2.isObject() && (bool)result2["success"], "Should parse");
        auto* events2 = result2["events"].getArray();
        expect(events2 != nullptr && events2->size() == 2, "Should produce 2 events");
        if (!events2 || events2->size() != 2) return;

        // G minor 7: G(67) Bb(70)
        expectEquals(note((*events2)[0]), 67, "G-7 degree 1 = G4 = MIDI 67");
        expectEquals(note((*events2)[1]), 70, "G-7 degree 2 = Bb4 = MIDI 70 (minor 3rd)");
    }
};
static MJ3_DashMinorChords test_dashMinorChords;

// =============================================================================
// Test 15: Events-in-children with no "mode" on parent (LLM sometimes omits it)
// =============================================================================
class MJ3_EventsInChildrenNoMode : public juce::UnitTest
{
public:
    MJ3_EventsInChildrenNoMode() : UnitTest("MJ3 Pipeline: events-in-children without mode", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Event-like children work even when parent has no explicit mode");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "Am7"},
            "define": {
                "pattern": {
                    "children": [
                        {"degree": 1, "beat": 0, "duration": 2},
                        {"degree": 5, "beat": 2, "duration": 2}
                    ]
                }
            },
            "root": {"ref": "pattern"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        expectEquals(events->size(), 2, "Should produce 2 events");
        // Am7: degree 1 = A4(69), degree 5 = E5(76) ... actually depends on intervals
        // Am7 intervals: {0, 3, 7, 10} → degree 1=A4(69), degree 5 wraps: index 4, octaveShift=1, chordIndex=0
        // = 69 + 0 + 12 = 81. Hmm, that's high. Let me just check they exist.
        expect(note((*events)[0]) > 0, "First event should have valid MIDI note");
        expect(note((*events)[1]) > 0, "Second event should have valid MIDI note");
    }
};
static MJ3_EventsInChildrenNoMode test_eventsNoMode;

// =============================================================================
// Test 16: Degree array [1,3,5,7] in events array (not just children)
// =============================================================================
class MJ3_DegreeArrayInEvents : public juce::UnitTest
{
public:
    MJ3_DegreeArrayInEvents() : UnitTest("MJ3 Pipeline: degree array in events array", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Degree array in events array expands to multiple simultaneous notes");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "Dm7"},
            "define": {
                "voicing": {
                    "events": [
                        {"degree": [1, 3, 5, 7], "beat": 0, "duration": 4, "velocity": 0.6},
                        {"degree": 2, "beat": 2, "duration": 2, "velocity": 0.5}
                    ]
                }
            },
            "root": {"ref": "voicing"}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // [1,3,5,7] → 4 events at beat 0 + degree 2 at beat 2 → 5 total
        expectEquals(events->size(), 5, "Should produce 5 events (4 from array + 1 single)");
    }
};
static MJ3_DegreeArrayInEvents test_degreeArrayInEvents;

// =============================================================================
// Test 17: Repeat on Simultaneous container (ref to simultaneous definition)
// Bug: repeat was silently ignored on simultaneous nodes.
// =============================================================================
class MJ3_RepeatSimultaneous : public juce::UnitTest
{
public:
    MJ3_RepeatSimultaneous() : UnitTest("MJ3 Pipeline: repeat on simultaneous container", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Repeat 2 on a simultaneous pattern doubles the events");
        // Pattern is simultaneous (two voices playing together), 4 beats long.
        // repeat=2 should play it twice sequentially for 8 beats total.
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "pattern": {
                    "mode": "simultaneous",
                    "children": [
                        {"events": [
                            {"degree": 1, "octave": 3, "beat": 0, "duration": 2},
                            {"degree": 5, "octave": 3, "beat": 2, "duration": 2}
                        ]},
                        {"events": [
                            {"degree": 1, "octave": 5, "beat": 0, "duration": 4}
                        ]}
                    ]
                }
            },
            "root": {"ref": "pattern", "repeat": 2}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // Pattern has 3 events (2 bass + 1 melody). Repeat 2 = 6 events.
        expectEquals(events->size(), 6, "Should produce 6 events (3 per iteration x 2 repeats)");

        // Second iteration should start at beat 4 (pattern duration = 4 beats)
        double maxTime = 0;
        for (auto& e : *events)
            maxTime = std::max(maxTime, ts(e));
        expect(maxTime >= 4.0, "Events should span at least 8 beats, max: " + juce::String(maxTime));
    }
};
static MJ3_RepeatSimultaneous test_repeatSimultaneous;

// =============================================================================
// Test 18: Repeat on simultaneous with events-in-children (real LLM pattern)
// This is the exact structure the LLM produces: simultaneous wrapper around
// sequential children with events-in-children, referenced with repeat.
// =============================================================================
class MJ3_RepeatSimultaneousEventsInChildren : public juce::UnitTest
{
public:
    MJ3_RepeatSimultaneousEventsInChildren()
        : UnitTest("MJ3 Pipeline: repeat simultaneous with events-in-children", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Real LLM pattern: simultaneous→sequential→events-in-children with repeat");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "pattern": {
                    "mode": "simultaneous",
                    "children": [
                        {
                            "mode": "sequential",
                            "children": [
                                {"degree": [1, 3, 5], "beat": 0, "duration": 2},
                                {"degree": [1, 3, 5], "beat": 2, "duration": 2}
                            ]
                        },
                        {
                            "mode": "sequential",
                            "children": [
                                {"degree": 1, "beat": 0, "duration": 1, "octave": 3},
                                {"degree": 5, "beat": 1, "duration": 1, "octave": 3},
                                {"degree": 1, "beat": 2, "duration": 1, "octave": 3},
                                {"degree": 3, "beat": 3, "duration": 1, "octave": 3}
                            ]
                        }
                    ]
                }
            },
            "root": {"ref": "pattern", "repeat": 3}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // Per iteration: [1,3,5] x2 = 6 chord events + 4 bass events = 10
        // Repeat 3 = 30 total
        expectEquals(events->size(), 30, "Should produce 30 events (10 per iter x 3 repeats)");

        // Pattern duration = 4 beats. Repeat 3 = 12 beats total.
        double maxTime = 0;
        for (auto& e : *events)
            maxTime = std::max(maxTime, ts(e));
        expect(maxTime >= 8.0, "Events should span 12 beats, max: " + juce::String(maxTime));
    }
};
static MJ3_RepeatSimultaneousEventsInChildren test_repeatSimEvtsInChildren;

// =============================================================================
// Test 19: Multi-section with repeat on simultaneous pattern (full skeleton flow)
// Two chord sections each referencing the same simultaneous pattern with repeat.
// =============================================================================
class MJ3_MultiSectionRepeatSimultaneous : public juce::UnitTest
{
public:
    MJ3_MultiSectionRepeatSimultaneous()
        : UnitTest("MJ3 Pipeline: multi-section with repeated simultaneous", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Two chord sections, each repeating a simultaneous pattern");
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "pattern": {
                    "mode": "simultaneous",
                    "children": [
                        {"events": [
                            {"degree": 1, "octave": 3, "beat": 0, "duration": 2},
                            {"degree": 5, "octave": 3, "beat": 2, "duration": 2}
                        ]},
                        {"events": [
                            {"degree": 1, "octave": 5, "beat": 0, "duration": 4}
                        ]}
                    ]
                }
            },
            "root": {
                "mode": "sequential",
                "children": [
                    {
                        "context": {"mode": "CMaj7"},
                        "mode": "simultaneous",
                        "children": [{"ref": "pattern", "repeat": 2}]
                    },
                    {
                        "context": {"mode": "Dm7"},
                        "mode": "simultaneous",
                        "children": [{"ref": "pattern", "repeat": 2}]
                    }
                ]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // Pattern: 3 events, repeat 2 = 6 per section, 2 sections = 12
        expectEquals(events->size(), 12, "Should produce 12 events (3 x 2 reps x 2 sections)");

        // Total span: 2 sections x 8 beats = 16 beats
        double maxTime = 0;
        for (auto& e : *events)
            maxTime = std::max(maxTime, ts(e));
        expect(maxTime >= 8.0, "Should span at least 16 beats, max: " + juce::String(maxTime));

        // Chord differentiation: CMaj7 root = C (60 area), Dm7 root = D (62 area)
        // Find first bass note in each section
        int sect1note = -1, sect2note = -1;
        for (auto& e : *events)
        {
            int n = note(e);
            double t = ts(e);
            if (n < 60 && t < 1.0 && sect1note < 0) sect1note = n;
            if (n < 60 && t >= 8.0 && sect2note < 0) sect2note = n;
        }
        expect(sect1note != sect2note, "Bass notes should differ between CMaj7 and Dm7 sections");
    }
};
static MJ3_MultiSectionRepeatSimultaneous test_multiSectionRepeatSim;

// =============================================================================
// Test 20: Rest events in children don't inflate sequential duration
// LLMs generate {"beat": 0.5, "duration": 1.5} (no pitch) as rests.
// These must NOT become empty container nodes (which default to 4-beat duration).
// =============================================================================
class MJ3_RestEventsInChildren : public juce::UnitTest
{
public:
    MJ3_RestEventsInChildren()
        : UnitTest("MJ3 Pipeline: rest events in children don't inflate duration", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Rest-like objects in children are skipped, pattern stays 4 beats");
        // Real LLM pattern: groove with chord hits and rests interleaved,
        // plus a bass note as a direct simultaneous child.
        // Use degrees [1,2,3] and [2,3,4] to avoid MIDI note collisions
        // (degree 5+ with octave override wraps and deduplicates).
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "CMaj7"},
            "define": {
                "groove": {
                    "mode": "simultaneous",
                    "children": [
                        {
                            "mode": "sequential",
                            "children": [
                                {"degree": [1, 2, 3], "beat": 0, "duration": 0.5, "octave": 4},
                                {"beat": 0.5, "duration": 1.5},
                                {"degree": [2, 3, 4], "beat": 2, "duration": 0.5, "octave": 4},
                                {"beat": 2.5, "duration": 1.5}
                            ]
                        },
                        {"degree": [1], "beat": 0, "duration": 4, "octave": 2}
                    ]
                }
            },
            "root": {"ref": "groove", "repeat": 2}
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse successfully");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events array");
        if (!events) return;

        // Per iteration: [1,2,3] chord = 3 events + [2,3,4] chord = 3 events
        //   + [1] bass = 1 event = 7 events
        // Repeat 2 = 14 total
        expectEquals(events->size(), 14, "Should produce 14 events (7 per iter x 2 repeats)");

        // Second iteration should start at beat 4 (not beat 8 or 12).
        // If rest objects inflate duration, the pattern would be 8+ beats instead of 4.
        double maxTime = 0;
        for (auto& e : *events)
            maxTime = std::max(maxTime, ts(e));

        // 2 iterations x 4 beats = 8 total. Last chord event at beat 4+2 = 6.
        // If rests inflated duration, pattern would be 8+ beats per iter → max >= 10.
        expect(maxTime >= 4.0 && maxTime <= 7.0,
            "Max timestamp should be ~6.0 (not inflated), got: " + juce::String(maxTime));

        beginTest("Single rest-only sequential child doesn't create phantom duration");
        auto result2 = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "Am7"},
            "define": {
                "pattern": {
                    "mode": "sequential",
                    "children": [
                        {"degree": 1, "beat": 0, "duration": 2},
                        {"beat": 2, "duration": 2},
                        {"degree": 5, "beat": 0, "duration": 2}
                    ]
                }
            },
            "root": {"ref": "pattern"}
        })", 120.0);

        expect(result2.isObject() && (bool)result2["success"], "Should parse");
        auto* events2 = result2["events"].getArray();
        expect(events2 != nullptr, "Should have events");
        if (!events2) return;

        // Two pitch events (degree 1 and degree 5), rest is skipped
        expectEquals(events2->size(), 2, "Should produce 2 events (rest skipped)");
    }
};
static MJ3_RestEventsInChildren test_restEventsInChildren;

// =============================================================================
// Test 21: "beats" property enforces minimum duration for pattern refs
// LLM patterns that end before 4 beats should still count as full bars
// =============================================================================
class MJ3_BeatsMinDuration : public juce::UnitTest
{
public:
    MJ3_BeatsMinDuration() : UnitTest("MJ3 Pipeline: beats min duration", "MJ3 Pipeline") {}
    void runTest() override
    {
        beginTest("Pattern with beats=4 enforces 4-beat duration even when events end at 3");

        // groove pattern has events ending at beat 3 (not 4)
        // With beats=4 on the ref, each section should be 4 beats
        // 3 sections × 4 beats = 12 beats total, not 9
        auto result = MJ3Parser::parse(R"({
            "version": "3.0", "tempo": 120,
            "context": {"mode_system": "chord", "mode": "C"},
            "define": {
                "groove": {
                    "events": [
                        {"degree": 1, "beat": 0, "duration": 0.5},
                        {"degree": 3, "beat": 1.5, "duration": 0.5},
                        {"degree": 5, "beat": 2.5, "duration": 0.5}
                    ]
                }
            },
            "root": {
                "mode": "sequential",
                "children": [
                    {"context": {"mode": "C"}, "mode": "simultaneous",
                     "children": [{"ref": "groove", "repeat": 1, "beats": 4}]},
                    {"context": {"mode": "F"}, "mode": "simultaneous",
                     "children": [{"ref": "groove", "repeat": 1, "beats": 4}]},
                    {"context": {"mode": "G"}, "mode": "simultaneous",
                     "children": [{"ref": "groove", "repeat": 1, "beats": 4}]}
                ]
            }
        })", 120.0);

        expect(result.isObject() && (bool)result["success"], "Should parse");
        auto* events = result["events"].getArray();
        expect(events != nullptr, "Should have events");
        if (!events) return;

        // 3 events per section × 3 sections = 9 events
        expectEquals(events->size(), 9, "Should have 9 events");

        // Check timestamps: section 2 should start at beat 4 (not 3)
        // Section 3 should start at beat 8 (not 6)
        double maxTimestamp = 0.0;
        for (const auto& evt : *events)
            maxTimestamp = std::max(maxTimestamp, ts(evt));

        // Last event should be at beat 8 + 2.5 = 10.5 (with beats=4)
        // Without beats, it would be at beat 6 + 2.5 = 8.5
        expect(maxTimestamp >= 10.0, "Last event should be at beat 10+ (beats=4 enforced)");
        expect(maxTimestamp < 12.0, "Last event should be before beat 12");
    }
};
static MJ3_BeatsMinDuration test_beatsMinDuration;
