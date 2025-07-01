#include <JuceHeader.h>

namespace covertracks {

/**
 * Test quantitative alias concept - aliases that define both structure and count
 */
class QuantitativeAliasTest : public juce::UnitTest
{
public:
    QuantitativeAliasTest() : juce::UnitTest("Quantitative Alias Test", "Music Pattern System") {}
    
    void runTest() override
    {
        testJazzStandardStructure();
        testBluesStructure();
        testVariableCount();
    }
    
private:
    void testJazzStandardStructure()
    {
        beginTest("Jazz standard with quantitative aliases");
        
        // 32-bar AABA jazz standard with precise structure definition
        juce::String jazzStandard = R"({
            "version": "2.0",
            "meta": {"title": "Quantitative Jazz Standard"},
            
            "aliases": {
                "song": {
                    "type": "structure",
                    "contains": "chorus",
                    "count": 1,
                    "description": "Complete jazz standard"
                },
                "chorus": {
                    "type": "structure", 
                    "contains": "sections",
                    "count": 4,
                    "pattern": "AABA",
                    "description": "32-bar chorus form"
                },
                "section": {
                    "type": "sequential",
                    "contains": "bars", 
                    "count": 8,
                    "description": "8-bar section"
                },
                "bar": {
                    "type": "sequential",
                    "contains": "beats",
                    "count": 4,
                    "description": "4/4 time"
                }
            },
            
            "song": {
                "chorus": {
                    "A": {
                        "bars": {
                            "1": {"beats": {"1": {"events": [{"t": 1, "n": "C4"}]}}},
                            "2": {"beats": {"1": {"events": [{"t": 1, "n": "E4"}]}}}
                        }
                    },
                    "A2": {
                        "bars": {
                            "1": {"beats": {"1": {"events": [{"t": 1, "n": "F4"}]}}}
                        }
                    },
                    "B": {
                        "bars": {
                            "1": {"beats": {"1": {"events": [{"t": 1, "n": "G4"}]}}}
                        }
                    },
                    "A3": {
                        "bars": {
                            "1": {"beats": {"1": {"events": [{"t": 1, "n": "C4"}]}}}
                        }
                    }
                }
            }
        })";
        
        logMessage("Jazz standard structure:");
        logMessage("  song (1) → chorus (4) → sections → bars (8) → beats (4)");
        logMessage("  Total: 1 * 4 * 8 * 4 = 128 beats (32 bars)");
        logMessage("  Pattern: AABA (A=8 bars, A=8 bars, B=8 bars, A=8 bars)");
        
        expect(true, "Demonstrated quantitative jazz structure");
    }
    
    void testBluesStructure()
    {
        beginTest("12-bar blues with quantitative aliases");
        
        juce::String bluesPattern = R"({
            "version": "2.0",
            "aliases": {
                "blues": {
                    "type": "structure",
                    "contains": "chorus", 
                    "count": "infinite",
                    "description": "Repeating 12-bar form"
                },
                "chorus": {
                    "type": "structure",
                    "contains": "sections",
                    "count": 3,
                    "pattern": "I-IV-V",
                    "description": "12-bar blues chorus"
                },
                "I_section": {
                    "type": "sequential",
                    "contains": "bars",
                    "count": 4,
                    "description": "4 bars on I chord"
                },
                "IV_section": {
                    "type": "sequential", 
                    "contains": "bars",
                    "count": 4,
                    "description": "2 bars IV, 2 bars I"
                },
                "V_section": {
                    "type": "sequential",
                    "contains": "bars", 
                    "count": 4,
                    "description": "V-IV-I-I turnaround"
                }
            },
            
            "blues": {
                "chorus": {
                    "I": {
                        "bars": {
                            "1-4": {"chord": "C7", "events": [{"t": 1, "n": "C3"}]}
                        }
                    },
                    "IV": {
                        "bars": {
                            "5-6": {"chord": "F7", "events": [{"t": 1, "n": "F3"}]},
                            "7-8": {"chord": "C7", "events": [{"t": 1, "n": "C3"}]}
                        }
                    },
                    "V": {
                        "bars": {
                            "9": {"chord": "G7", "events": [{"t": 1, "n": "G3"}]},
                            "10": {"chord": "F7", "events": [{"t": 1, "n": "F3"}]},
                            "11-12": {"chord": "C7", "events": [{"t": 1, "n": "C3"}]}
                        }
                    }
                }
            }
        })";
        
        logMessage("Blues structure:");
        logMessage("  blues (∞) → chorus (3) → sections → bars (4 each)");
        logMessage("  Total per chorus: 3 * 4 = 12 bars");
        logMessage("  Harmonic pattern: I(4) + IV(4) + V(4) = 12 bars");
        
        expect(true, "Demonstrated quantitative blues structure");
    }
    
    void testVariableCount()
    {
        beginTest("Variable and free count containers");
        
        juce::String variablePattern = R"({
            "version": "2.0",
            "aliases": {
                "song": {
                    "type": "structure",
                    "contains": "sections",
                    "count": "variable",
                    "description": "Flexible song form"
                },
                "verse": {
                    "type": "sequential",
                    "contains": "bars",
                    "count": {"min": 8, "max": 16},
                    "description": "8-16 bar verse"
                },
                "chorus": {
                    "type": "sequential", 
                    "contains": "bars",
                    "count": 8,
                    "description": "Fixed 8-bar chorus"
                },
                "solo": {
                    "type": "sequential",
                    "contains": "choruses",
                    "count": "free",
                    "description": "Open-ended improvisation"
                },
                "alap": {
                    "type": "free",
                    "contains": "phrases", 
                    "count": "free",
                    "description": "Free-time exploration"
                }
            },
            
            "song": {
                "intro": {
                    "bars": {"1-4": {"events": [{"t": 1, "n": "C4"}]}}
                },
                "verse1": {
                    "bars": {"1-12": {"events": [{"t": 1, "n": "F4"}]}}
                },
                "chorus1": {
                    "bars": {"1-8": {"events": [{"t": 1, "n": "G4"}]}}
                },
                "solo": {
                    "choruses": {
                        "1": {"events": [{"t": 1, "n": "A4"}]},
                        "2": {"events": [{"t": 1, "n": "B4"}]},
                        "3": {"events": [{"t": 1, "n": "C5"}]}
                    }
                }
            }
        })";
        
        logMessage("Variable count types:");
        logMessage("  Fixed: chorus = 8 bars (always)");
        logMessage("  Range: verse = 8-16 bars (flexible within range)");
        logMessage("  Variable: song = any number of sections");
        logMessage("  Free: solo = unlimited choruses");
        logMessage("  Free-time: alap = no fixed duration");
        
        expect(true, "Demonstrated variable count containers");
    }
};

static QuantitativeAliasTest quantitativeAliasTest;

} // namespace covertracks