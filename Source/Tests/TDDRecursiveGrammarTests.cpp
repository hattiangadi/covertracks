#include <JuceHeader.h>
#include "../Musician/Core/MusicalContainer.h"
// #include "../Musician/Core/UniversalMusicParser.h"  // TODO: Implement in later TDD phase

namespace covertracks {

/**
 * TDD Tests for Recursive Grammar System
 * 
 * These tests define the behavior we WANT the new system to have.
 * They will FAIL initially because the implementation doesn't exist yet.
 * We'll implement the classes to make these tests pass.
 */

class TDDRecursiveGrammarTests : public juce::UnitTest
{
public:
    TDDRecursiveGrammarTests() : juce::UnitTest("TDD Recursive Grammar", "Music Pattern System") {}
    
    void runTest() override
    {
        testMusicalContainerBasics();
        testRecursiveLayerNesting();
        testContextInheritanceChain();
        testUnifiedVariationSystem();
        // TODO: Enable these tests when UniversalMusicParser is implemented
        // testJSONParsingToContainers();
        // testExpressionFeatures();
        // testShorthandNotation();
        // testAdvancedExamples();
    }
    
private:
    void testMusicalContainerBasics()
    {
        beginTest("MusicalContainer basic functionality");
        
        // Test 1: Empty container renders empty stream
        MusicalContainer container;
        Context emptyContext;
        auto stream = container.render(emptyContext, 1, 4);
        expect(stream.isEmpty(), "Empty container should render nothing");
        
        // Test 2: Container with direct events
        container.addEvent(Event{1.0, "kick", 0.8});
        container.addEvent(Event{3.0, "snare", 0.7});
        
        stream = container.render(emptyContext, 1, 4);
        expect(stream.size() == 2, "Should render 2 direct events");
        expect(stream.getEventAt(1.0).note == "kick", "First event should be kick");
        expect(stream.getEventAt(3.0).note == "snare", "Second event should be snare");
        
        // Test 3: Event properties preserved
        auto kickEvent = stream.getEventAt(1.0);
        expect(kickEvent.velocity == 0.8f, "Kick velocity should be preserved");
        expect(kickEvent.time == 1.0, "Kick timing should be preserved");
    }
    
    void testRecursiveLayerNesting()
    {
        beginTest("Recursive layer nesting");
        
        // Create: root -> drums layer -> kick sublayer -> events
        MusicalContainer root;
        
        auto& drumsLayer = root.addLayer("drums");
        auto& kickLayer = drumsLayer.addLayer("kick");
        kickLayer.addEvent(Event{1.0, "kick", 0.8});
        kickLayer.addEvent(Event{3.0, "kick", 0.8});
        
        auto& snareLayer = drumsLayer.addLayer("snare");
        snareLayer.addEvent(Event{2.0, "snare", 0.7});
        snareLayer.addEvent(Event{4.0, "snare", 0.7});
        
        Context context;
        auto stream = root.render(context, 1, 4);
        
        expect(stream.size() == 4, "Should render all nested events");
        expect(stream.hasEventAt(1.0), "Should have kick at beat 1");
        expect(stream.hasEventAt(2.0), "Should have snare at beat 2");
        expect(stream.hasEventAt(3.0), "Should have kick at beat 3");
        expect(stream.hasEventAt(4.0), "Should have snare at beat 4");
    }
    
    void testContextInheritanceChain()
    {
        beginTest("Context inheritance through hierarchy");
        
        // Global context
        Context global;
        global.octave = 4;
        global.velocity = 0.6f;  // mf
        global.chord = "C";
        
        // Root overrides velocity
        MusicalContainer root;
        root.context.velocity = 0.8f;  // f
        
        // Bass layer overrides octave
        auto& bassLayer = root.addLayer("bass");
        bassLayer.context.octave = 2;
        
        // Variation overrides chord
        auto& variation = bassLayer.addVariation("bar4");
        variation.context.chord = "Am";
        variation.addEvent(Event{4.0, "", 0.0, 1});  // Chord degree 1
        
        auto stream = root.render(global, 4, 4);  // Render bar 4 to trigger variation
        auto event = stream.getEventAt(4.0);
        
        // Event should inherit: octave=2, velocity=0.8, chord="Am"
        expect(event.resolvedOctave == 2, "Should inherit bass octave");
        expect(event.resolvedVelocity == 0.8f, "Should inherit root velocity");  
        expect(event.resolvedChord == "Am", "Should inherit variation chord");
        expect(event.resolvedNote == 57, "Chord degree 1 of Am in octave 2 = A2 = 57");
    }
    
    void testUnifiedVariationSystem()
    {
        beginTest("Unified variation system - same code for patterns and layers");
        
        // Test 1: Pattern with variations
        MusicalContainer pattern;
        pattern.addEvent(Event{1.0, "kick", 0.8});
        
        auto& patternVar = pattern.addVariation("bar4");
        patternVar.addEvent(Event{4.0, "crash", 1.0});
        
        Context context;
        auto patternStream = pattern.render(context, 4, 4);
        expect(patternStream.hasEventAt(4.0), "Pattern variation should apply on bar 4");
        expect(patternStream.getEventAt(4.0).note == "crash", "Should have crash from variation");
        
        // Test 2: Layer with variations (same structure!)
        MusicalContainer layer;
        layer.setReference("basicPattern");
        
        auto& layerVar = layer.addVariation("bar8");
        layerVar.addEvent(Event{4.0, "fill", 0.9});
        
        auto layerStream = layer.render(context, 8, 8);
        expect(layerStream.hasEventAt(4.0), "Layer variation should apply on bar 8");
        expect(layerStream.getEventAt(4.0).note == "fill", "Should have fill from variation");
        
        // Both should use identical processing logic
        expect(patternVar.getClass() == layerVar.getClass(), "Variations should be same type");
    }
    
    void testJSONParsingToContainers()
    {
        beginTest("JSON parsing to recursive containers");
        
        String jsonPattern = R"({
            "version": "2.0",
            "meta": {"title": "Test", "tempo": 120},
            "sections": {
                "verse": {
                    "bars": 4,
                    "context": {"velocity": "mf"},
                    "layers": {
                        "drums": {
                            "pattern": "rock",
                            "variations": {
                                "bar4": {
                                    "events": [{"t": 4, "n": "crash"}]
                                }
                            }
                        }
                    }
                }
            },
            "patterns": {
                "rock": [
                    {"t": 1, "n": "kick"},
                    {"t": 3, "n": "snare"}
                ]
            }
        })";
        
        // TODO: Implement when UniversalMusicParser is ready
        // auto container = UniversalMusicParser::parseJSON(jsonPattern);
        // expect(container != nullptr, "Should parse JSON successfully");
    }
    
    void testExpressionFeatures()
    {
        beginTest("Expression features integration");
        
        String jsonPattern = R"({
            "version": "2.0", 
            "context": {"dynamics": "mf", "articulation": "legato"},
            "events": [
                {
                    "t": 1, "n": "C4",
                    "expression": {
                        "dynamics": "ff",
                        "articulation": "staccato",
                        "vibrato": {"rate": 5, "depth": 0.2}
                    }
                },
                {
                    "t": 2, "n": "D4",
                    "processors": {
                        "reverb": {"room": 0.5, "decay": 3.0}
                    }
                }
            ]
        })";
        
        // TODO: Implement when UniversalMusicParser is ready
        // auto container = UniversalMusicParser::parseJSON(jsonPattern);
    }
    
    void testShorthandNotation()
    {
        beginTest("Shorthand notation expansion");
        
        String jsonPattern = R"({
            "version": "2.0",
            "shortcuts": {
                "k": "kick",
                "s": "snare",
                "h": "hihat",
                "basic4": "k..s..k..s"
            },
            "events": [
                {"t": 0, "pattern": "basic4", "repeat": 2}
            ]
        })";
        
        // TODO: Implement when UniversalMusicParser is ready
        // auto container = UniversalMusicParser::parseJSON(jsonPattern);
    }
    
    void testAdvancedExamples()
    {
        beginTest("Advanced recursive examples");
        
        // Test the mind-bending fractal pattern example
        String fractalPattern = R"({
            "version": "2.0",
            "fractals": {
                "layers": {
                    "level1": {
                        "events": [{"t": 1, "n": "kick"}, {"t": 3, "n": "snare"}],
                        "layers": {
                            "level2": {
                                "events": [{"t": 1.5, "n": "hihat"}, {"t": 3.5, "n": "hihat"}],
                                "layers": {
                                    "level3": {
                                        "events": [{"t": 1.25, "n": "ghost"}, {"t": 3.25, "n": "ghost"}]
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        // TODO: Implement when UniversalMusicParser is ready
        // auto container = UniversalMusicParser::parseJSON(fractalPattern);
    }
};

// This test will FAIL initially because the classes don't exist
// That's exactly what we want in TDD!
static TDDRecursiveGrammarTests tddRecursiveGrammarTests;

} // namespace covertracks