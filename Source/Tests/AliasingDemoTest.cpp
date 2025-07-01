#include <JuceHeader.h>

namespace covertracks {

/**
 * Demonstration of container aliasing concept
 */
class AliasingDemoTest : public juce::UnitTest
{
public:
    AliasingDemoTest() : juce::UnitTest("Aliasing Demo Test", "Music Pattern System") {}
    
    void runTest() override
    {
        beginTest("Container aliasing demonstration");
        
        // This demonstrates how the same MusicalContainer structure
        // can be aliased with semantic names for clarity
        
        // Traditional approach - type-specific fields
        juce::String traditional = R"({
            "sections": {
                "intro": {
                    "layers": {
                        "piano": {
                            "variations": {
                                "bar1": {"events": [{"t": 1, "n": "C4"}]},
                                "bar2": {"events": [{"t": 1, "n": "D4"}]}
                            }
                        }
                    }
                }
            }
        })";
        
        // Aliased approach - semantic hierarchy
        juce::String aliased = R"({
            "arrangement": {
                "intro": {
                    "duration": 4,
                    "phrases": {
                        "opening": {
                            "bars": {
                                "1": {"beats": {"1": {"events": [{"t": 1, "n": "C4"}]}}},
                                "2": {"beats": {"1": {"events": [{"t": 1, "n": "D4"}]}}}
                            }
                        }
                    }
                }
            }
        })";
        
        // Unified approach - all containers, different relationships
        juce::String unified = R"({
            "container": {
                "type": "arrangement",
                "children": {
                    "intro": {
                        "type": "section",
                        "duration": 4,
                        "children": {
                            "opening": {
                                "type": "phrase",
                                "children": {
                                    "1": {
                                        "type": "bar",
                                        "children": {
                                            "1": {
                                                "type": "beat",
                                                "events": [{"t": 1, "n": "C4"}]
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        logMessage("Traditional approach uses type-specific fields:");
        logMessage("  sections → layers → variations");
        logMessage("");
        logMessage("Aliased approach uses semantic hierarchy:");
        logMessage("  arrangement → section → phrase → bar → beat");
        logMessage("");
        logMessage("Unified approach makes container nature explicit:");
        logMessage("  All are containers with type and children");
        logMessage("");
        logMessage("Benefits of aliasing:");
        logMessage("  1. Semantic clarity - musicians understand 'verse' better than 'container'");
        logMessage("  2. Universal parsing - one method handles all types");
        logMessage("  3. Flexible nesting - any level can contain any other");
        logMessage("  4. Natural duration specification at each level");
        
        expect(true, "Demonstration complete");
    }
};

static AliasingDemoTest aliasingDemoTest;

} // namespace covertracks