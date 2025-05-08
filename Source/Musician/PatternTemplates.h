#pragma once

#include <JuceHeader.h>
#include <string>
#include <map>

namespace covertracks {

/**
 * @brief Pattern-based templates for different instruments and styles
 * 
 * This namespace contains predefined patterns and templates for generating
 * music using a structured pattern-based approach.
 */
namespace PatternTemplates {

// Define standard patterns for each instrument
// Each pattern represents a full measure (4 beats in 4/4 time)

// Piano patterns
const std::map<std::string, std::string> pianoPatternsDesc = {
    {"whole", "One whole note chord (4 beats)"},
    {"half", "Two half note chords (2 beats each)"},
    {"quarter", "Four quarter note chords (1 beat each)"},
    {"syncopated", "Quarter, half, quarter (syncopated rhythm)"},
    {"comping", "Jazz comping with syncopated rhythm"},
    {"bossa", "Bossa nova rhythm pattern"},
    {"ballad", "Smooth half note with anticipation"},
    {"waltz", "3/4 waltz feel with quarter notes"}
};

// Bass patterns
const std::map<std::string, std::string> bassPatternsDesc = {
    {"whole", "One whole note (4 beats)"},
    {"half", "Two half notes (2 beats each)"},
    {"quarter", "Four quarter notes (1 beat each)"},
    {"walking", "Walking bass with quarter notes (1-3-5-7 or similar)"},
    {"latin", "Latin bass pattern with syncopation"},
    {"arco", "Sustained arco (bowed) bass notes"},
    {"octaves", "Alternating between octaves"}
};

// Drum patterns
const std::map<std::string, std::string> drumPatternsDesc = {
    {"basic", "Basic rock beat with kick on 1,3 and snare on 2,4"},
    {"swing", "Jazz swing pattern with ride cymbal"},
    {"latin", "Latin groove with hi-hat and cross-stick"},
    {"waltz", "3/4 waltz pattern"},
    {"ballad", "Jazz ballad with brushes"},
    {"shuffle", "Shuffle pattern with ghost notes"},
    {"funk", "Funky sixteenth note pattern"},
    {"hiphop", "Hip-hop beat with open hi-hat"}
};

// Guitar patterns
const std::map<std::string, std::string> guitarPatternsDesc = {
    {"strumming", "Basic strumming pattern"},
    {"arpeggios", "Arpeggiated chords"},
    {"power_chords", "Power chords on quarter notes"},
    {"fingerpicking", "Fingerpicking pattern"},
    {"jazz_comping", "Jazz comping with seventh chords"},
    {"funk", "Funky sixteenth note rhythm"},
    {"lead", "Lead guitar melody line"}
};

/**
 * @brief Get the pattern description for an instrument
 * 
 * @param instrument The instrument type
 * @param patternName The pattern name
 * @return std::string The pattern description
 */
inline std::string getPatternDescription(
    const std::string& instrument,
    const std::string& patternName)
{
    if (instrument == "piano" && pianoPatternsDesc.find(patternName) != pianoPatternsDesc.end()) {
        return pianoPatternsDesc.at(patternName);
    }
    else if (instrument == "bass" && bassPatternsDesc.find(patternName) != bassPatternsDesc.end()) {
        return bassPatternsDesc.at(patternName);
    }
    else if (instrument == "drums" && drumPatternsDesc.find(patternName) != drumPatternsDesc.end()) {
        return drumPatternsDesc.at(patternName);
    }
    else if (instrument == "guitar" && guitarPatternsDesc.find(patternName) != guitarPatternsDesc.end()) {
        return guitarPatternsDesc.at(patternName);
    }
    
    return "Unknown pattern";
}

/**
 * @brief Get the implementation for a specific pattern
 * 
 * @param instrument The instrument type
 * @param patternName The pattern name
 * @return std::string The pattern implementation
 */
inline std::string getPatternImplementation(
    const std::string& instrument,
    const std::string& patternName)
{
    // Basic implementations for drum patterns
    if (instrument == "drums") {
        if (patternName == "basic") {
            return "K:1,3, S:2,4, HH:1,1+,2,2+,3,3+,4,4+";
        }
        else if (patternName == "swing") {
            return "K:1,3, S:2,4, R:1,1+,2,2+,3,3+,4,4+";
        }
        else if (patternName == "latin") {
            return "K:1,3+, S:2+,4, HH:1,1+,2,2+,3,3+,4,4+";
        }
    }
    
    // For other instruments, return an empty implementation for now
    return "";
}

/**
 * @brief Get all available patterns for an instrument
 * 
 * @param instrument The instrument type
 * @return var Available patterns as JUCE var/JSON
 */
inline var getAvailablePatterns(const std::string& instrument)
{
    DynamicObject* patternsObj = new DynamicObject();
    
    if (instrument == "piano") {
        for (const auto& pattern : pianoPatternsDesc) {
            patternsObj->setProperty(Identifier(pattern.first), String(pattern.second));
        }
    }
    else if (instrument == "bass") {
        for (const auto& pattern : bassPatternsDesc) {
            patternsObj->setProperty(Identifier(pattern.first), String(pattern.second));
        }
    }
    else if (instrument == "drums") {
        for (const auto& pattern : drumPatternsDesc) {
            patternsObj->setProperty(Identifier(pattern.first), String(pattern.second));
        }
    }
    else if (instrument == "guitar") {
        for (const auto& pattern : guitarPatternsDesc) {
            patternsObj->setProperty(Identifier(pattern.first), String(pattern.second));
        }
    }
    
    return var(patternsObj);
}

} // namespace PatternTemplates

} // namespace covertracks
