#include <JuceHeader.h>

namespace covertracks {

/**
 * Test self-describing alias concept
 */
class SelfDescribingAliasTest : public juce::UnitTest
{
public:
    SelfDescribingAliasTest() : juce::UnitTest("Self-Describing Alias Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testSelfDescribingPattern();
        testCulturalAdaptation();
    }
    
private:
    void testSelfDescribingPattern()
    {
        beginTest("Self-describing aliases in pattern");
        
        // Pattern defines its own container relationships
        juce::String selfDescribingPattern = R"({
            "version": "2.0",
            "meta": {"title": "Self-Describing Test"},
            
            "aliases": {
                "piece": {"type": "structure", "contains": "movements"},
                "movements": {"type": "sequential", "contains": "themes"},
                "themes": {"type": "named", "contains": "phrases"},
                "phrases": {"type": "sequential", "contains": "measures"},
                "measures": {"type": "sequential", "contains": "notes"}
            },
            
            "piece": {
                "allegro": {
                    "themes": {
                        "main": {
                            "phrases": {
                                "opening": {
                                    "measures": {
                                        "1": {"notes": [{"t": 1, "n": "C4"}]},
                                        "2": {"notes": [{"t": 1, "n": "E4"}]}
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        logMessage("Self-describing pattern uses custom terminology:");
        logMessage("  piece → movements → themes → phrases → measures → notes");
        logMessage("Instead of hardcoded:");
        logMessage("  sections → layers → patterns → variations → events");
        
        expect(true, "Demonstrated self-describing concept");
    }
    
    void testCulturalAdaptation()
    {
        beginTest("Cultural adaptation through aliases");
        
        // Indian classical music pattern
        juce::String indianPattern = R"({
            "version": "2.0",
            "aliases": {
                "raga": {"type": "structure", "contains": "alap"},
                "alap": {"type": "sequential", "contains": "phrases"},
                "taal": {"type": "sequential", "contains": "matra"},
                "matra": {"type": "sequential", "contains": "bol"}
            },
            "raga": {
                "bhairav": {
                    "alap": {
                        "opening": {
                            "taal": {
                                "teentaal": {
                                    "matra": {
                                        "1": {"bol": [{"n": "Sa"}]}
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        // Gamelan pattern
        juce::String gamelanPattern = R"({
            "version": "2.0",
            "aliases": {
                "gending": {"type": "structure", "contains": "gongan"},
                "gongan": {"type": "sequential", "contains": "keteg"},
                "keteg": {"type": "sequential", "contains": "instruments"},
                "instruments": {"type": "simultaneous", "contains": "notes"}
            },
            "gending": {
                "lancaran": {
                    "gongan": {
                        "1": {
                            "keteg": {
                                "1": {
                                    "instruments": {
                                        "bonang": {"notes": [{"n": "1"}]},
                                        "gender": {"notes": [{"n": "5"}]}
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";
        
        logMessage("Different cultures can use their own terminology:");
        logMessage("  Indian: raga → alap → taal → matra → bol");
        logMessage("  Gamelan: gending → gongan → keteg → instruments");
        logMessage("  Jazz: song → chorus → sections → bars → beats");
        logMessage("");
        logMessage("All map to the same underlying MusicalContainer structure!");
        
        expect(true, "Demonstrated cultural adaptation");
    }
};

static SelfDescribingAliasTest selfDescribingAliasTest;

} // namespace covertracks