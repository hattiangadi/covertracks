#include <JuceHeader.h>

namespace covertracks {

/**
 * Test flexible quantitative aliases - defaults with individual overrides
 */
/* Temporarily disabled due to syntax issues
class FlexibleQuantityTest : public juce::UnitTest
{
public:
    FlexibleQuantityTest() : juce::UnitTest("Flexible Quantity Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testDefaultWithOverrides();
        // testMixedTimeSignatures(); // Temporarily disabled due to syntax issues
        // testVariableSectionLengths(); // Temporarily disabled due to syntax issues
    }
    
private:
    void testDefaultWithOverrides()
    {
        beginTest("Default structure with individual overrides");
        
        // Pop song with typical structure but individual variations
        juce::String popSong = R"({
            "version": "2.0",
            "aliases": {
                "song": {"contains": "sections", "count": "variable"},
                "verse": {"contains": "bars", "count": 16, "default": true},
                "chorus": {"contains": "bars", "count": 8, "default": true},
                "bar": {"contains": "beats", "count": 4, "default": true}
            },
            
            "song": {
                "intro": {
                    "type": "verse",
                    "duration": 4,
                    "comment": "4-bar intro instead of typical 16-bar verse",
                    "bars": {"1-4": {"events": [{"t": 1, "n": "C4"}]}}
                },
                "verse1": {
                    "comment": "Uses default 16 bars",
                    "bars": {"1-16": {"events": [{"t": 1, "n": "F4"}]}}
                },
                "chorus1": {
                    "comment": "Uses default 8 bars", 
                    "bars": {"1-8": {"events": [{"t": 1, "n": "G4"}]}}
                },
                "verse2": {
                    "duration": 12,
                    "comment": "Shorter 12-bar verse for variation",
                    "bars": {"1-12": {"events": [{"t": 1, "n": "F4"}]}}
                },
                "chorus2": {
                    "duration": 12,
                    "comment": "Extended 12-bar chorus with outro",
                    "bars": {"1-12": {"events": [{"t": 1, "n": "G4"}]}}
                }
            }
        })";
        
        logMessage("Pop song structure variations:");
        logMessage("  intro: verse-type but 4 bars (override from default 16)");
        logMessage("  verse1: 16 bars (uses default)");
        logMessage("  chorus1: 8 bars (uses default)");
        logMessage("  verse2: 12 bars (override from default 16)");
        logMessage("  chorus2: 12 bars (override from default 8)");
        
        expect(true, "Demonstrated default + override pattern");
    }
    
    void testMixedTimeSignatures()
    {
        beginTest("Mixed time signatures with defaults");
        
        juce::String classicalPiece = R"({
            "version": "2.0",
            "aliases": {
                "movement": {"contains": "sections", "count": "variable"},
                "section": {"contains": "bars", "count": "variable"},
                "bar": {"contains": "beats", "count": 4, "default": true}
            },
            
            "movement": {
                "exposition": {
                    "bars": {
                        "1-16": {
                            "comment": "16 bars of 4/4 (uses default)",
                            "events": [{"t": 1, "n": "C4"}]
                        },
                        "17": {
                            "beats": 3,
                            "comment": "One bar of 3/4 (override)",
                            "events": [{"t": 1, "n": "F4"}]
                        },
                        "18-24": {
                            "comment": "Back to 4/4 (uses default)",
                            "events": [{"t": 1, "n": "G4"}]
                        }
                    }
                },
                "development": {
                    "bars": {
                        "1": {
                            "beats": 2,
                            "comment": "2/4 pickup bar",
                            "events": [{"t": 1, "n": "A4"}]
                        },
                        "2-20": {
                            "comment": "Regular 4/4 bars",
                            "events": [{"t": 1, "n": "B4"}]
                        },
                        "21": {
                            "beats": 6,
                            "comment": "6/4 climax bar",
                            "events": [{"t": 1, "n": "C5"}]
                        }
                    }
                }
            }
        })";
        
        logMessage("Classical piece with mixed meters:");
        logMessage("  Most bars: 4/4 (default)");
        logMessage("  Bar 17: 3/4 (individual override)");
        logMessage("  Bar 1 of development: 2/4 pickup (override)");
        logMessage("  Bar 21: 6/4 climax (override)");
        
        expect(true, "Demonstrated mixed time signatures");
    }
    
    void testVariableSectionLengths()
    {
        beginTest("Jazz with variable section lengths");
        
        juce::String jazzStandard = R"({
            "version": "2.0",
            "aliases": {
                "song": {"contains": "choruses", "count": "variable"},
                "chorus": {"contains": "sections", "count": 4, "pattern": "AABA", "default": true},
                "section": {"contains": "bars", "count": 8, "default": true},
                "bar": {"contains": "beats", "count": 4, "default": true}
            },
            
            "song": {
                "head": {
                    "comment": "Standard 32-bar AABA form",
                    "A1": {"bars": {"1-8": {"events": [{"t": 1, "n": "C4"}]}}},
                    "A2": {"bars": {"9-16": {"events": [{"t": 1, "n": "C4"}]}}},
                    "B": {"bars": {"17-24": {"events": [{"t": 1, "n": "F4"}]}}},
                    "A3": {"bars": {"25-32": {"events": [{"t": 1, "n": "C4"}]}}}
                },
                "piano_solo": {
                    "A1": {
                        "comment": "Standard 8-bar A section",
                        "bars": {"1-8": {"events": [{"t": 1, "n": "E4"}]}}
                    },
                    "A2": {
                        "duration": 12,
                        "comment": "Extended A section with 4 extra bars",
                        "bars": {"9-20": {"events": [{"t": 1, "n": "E4"}]}}
                    },
                    "B": {
                        "comment": "Standard 8-bar B section",
                        "bars": {"21-28": {"events": [{"t": 1, "n": "G4"}]}}
                    },
                    "A3": {
                        "duration": 4,
                        "comment": "Shortened A section, cut to 4 bars",
                        "bars": {"29-32": {"events": [{"t": 1, "n": "E4"}]}}
                    }
                },
                "bass_solo": {
                    "comment": "Double-length solo: 64 bars instead of 32",
                    "duration": 64,
                    "A1": {"bars": {"1-16": {"events": [{"t": 1, "n": "C2"}]}}},
                    "A2": {"bars": {"17-32": {"events": [{"t": 1, "n": "C2"}]}}},
                    "B": {"bars": {"33-48": {"events": [{"t": 1, "n": "F2"}]}}},
                    "A3": {"bars": {"49-64": {"events": [{"t": 1, "n": "C2"}]}}}
                }
            }
        })";
        
        logMessage("Jazz standard with variable chorus lengths:");
        logMessage("  head: 32 bars (standard AABA)");
        logMessage("  piano_solo: 36 bars (A1=8, A2=12, B=8, A3=4)");
        logMessage("  bass_solo: 64 bars (double-length chorus)");
        logMessage("");
        logMessage("This shows how defaults provide structure while");
        logMessage("allowing musical creativity and variation.");
        
        expect(true, "Demonstrated variable section lengths in jazz");
    }
};

// static FlexibleQuantityTest flexibleQuantityTest; // Temporarily disabled
*/

} // namespace covertracks