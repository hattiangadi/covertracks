#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace coverband {

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
    {"fill", "Drum fill transitioning between sections"}
};

// Guitar patterns
const std::map<std::string, std::string> guitarPatternsDesc = {
    {"whole", "One whole note chord (4 beats)"},
    {"half", "Two half note chords (2 beats each)"},
    {"quarter", "Four quarter note chords (1 beat each)"},
    {"strumming", "Strumming pattern (down-up-down-up)"},
    {"fingerpicking", "Fingerpicking arpeggio pattern"},
    {"comping", "Jazz comping with syncopated rhythm"},
    {"power", "Rock power chords on quarter notes"}
};

// Implementation of each pattern for piano
const std::map<std::string, std::string> pianoPatternsImpl = {
    {"whole", "$(CHORD)(w)"},
    {"half", "$(CHORD1)(h) $(CHORD2)(h)"},
    {"quarter", "$(CHORD1)(q) $(CHORD2)(q) $(CHORD3)(q) $(CHORD4)(q)"},
    {"syncopated", "$(CHORD1)(q) $(CHORD2)(h) $(CHORD3)(q)"},
    {"comping", "r(e) $(CHORD1)(e) r(q) $(CHORD2)(e) r(e) r(q) $(CHORD3)(e) r(e)"},
    {"bossa", "$(CHORD1)(e) r(e) $(CHORD1)(q) r(e) $(CHORD1)(e) r(e) $(CHORD1)(e)"},
    {"ballad", "$(CHORD1)(h) $(CHORD2)(q) $(CHORD2)(q)"},
    {"waltz", "$(CHORD1)(q) $(CHORD2)(q) $(CHORD3)(q)"}
};

// Implementation of each pattern for bass
const std::map<std::string, std::string> bassPatternsImpl = {
    {"whole", "$(CHORD:ROOT)2(w)"},
    {"half", "$(CHORD1:ROOT)2(h) $(CHORD2:ROOT)2(h)"},
    {"quarter", "$(CHORD1:ROOT)2(q) $(CHORD2:ROOT)2(q) $(CHORD3:ROOT)2(q) $(CHORD4:ROOT)2(q)"},
    {"walking", "$(CHORD:ROOT)2(q) $(CHORD:3RD)2(q) $(CHORD:5TH)2(q) $(CHORD:7TH)2(q)"},
    {"latin", "$(CHORD:ROOT)2(e) $(CHORD:5TH)2(e) $(CHORD:ROOT)2(q) $(CHORD:ROOT)2(e) $(CHORD:5TH)2(e) $(CHORD:ROOT)2(q)"},
    {"arco", "$(CHORD:ROOT)1(w)"},
    {"octaves", "$(CHORD:ROOT)1(q) $(CHORD:ROOT)2(q) $(CHORD:ROOT)1(q) $(CHORD:ROOT)2(q)"}
};

// Implementation of each pattern for drums
const std::map<std::string, std::string> drumPatternsImpl = {
    {"basic", "K:1,3 S:2,4 HH:1+,2+,3+,4+"},
    {"swing", "R:1,2,3,4 HH:2,4 K:1 S:3"},
    {"latin", "HH:1+,2+,3+,4+ S:2,4 K:1,3+"},
    {"waltz", "K:1 S:2 HH:1,2,3"},
    {"ballad", "R:1,2,3,4 S:2,4 K:1"},
    {"fill", "K:1 S:2 T1:3+ T2:3 T3:4+ T4:4"}
};

// Implementation of each pattern for guitar
const std::map<std::string, std::string> guitarPatternsImpl = {
    {"whole", "$(CHORD)(w)"},
    {"half", "$(CHORD1)(h) $(CHORD2)(h)"},
    {"quarter", "$(CHORD1)(q) $(CHORD2)(q) $(CHORD3)(q) $(CHORD4)(q)"},
    {"strumming", "$(CHORD)(d-du-udu)"},
    {"fingerpicking", "$(CHORD)(1-3-5-3-1-5)"},
    {"comping", "r(e) $(CHORD1)(e) r(q) $(CHORD2)(e) r(e) r(q) $(CHORD3)(e) r(e)"},
    {"power", "$(CHORD1)(q) $(CHORD2)(q) $(CHORD3)(q) $(CHORD4)(q)"}
};

// Templates for all instruments

// Piano jazz template
const std::string pianoJazzTemplate = R"(
Generate a jazz piano pattern using the following pattern-based format:

INSTRUCTIONS:
1. Create EXACTLY {{bars}} measures of jazz piano in {{key}} at {{tempo}} BPM
2. Choose from these patterns for each measure (each represents a full 4/4 measure):
   - whole: One whole note chord (4 beats)
   - half: Two half note chords (2 beats each)
   - quarter: Four quarter note chords (1 beat each) 
   - syncopated: Quarter, half, quarter rhythm
   - comping: Jazz comping with syncopated rhythm
   - bossa: Bossa nova rhythm

3. Specify both the pattern and the chords for each measure using this format:
   |pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|

4. Make each measure EXACTLY fit the pattern you choose
5. Follow the chord progression provided if available

ADDITIONAL GUIDELINES:
- Use appropriate jazz chord voicings
- You can specify exact voicings with chord(notes) format: Cmaj7(C3,E3,G3,B3)
- Each measure MUST be enclosed in | characters
- Each pattern MUST be listed by name (whole, half, quarter, etc.)

FORMATTED AS:
|pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
)";

// Bass jazz template
const std::string bassJazzTemplate = R"(
Generate a jazz bass pattern using the following pattern-based format:

INSTRUCTIONS:
1. Create EXACTLY {{bars}} measures of jazz bass in {{key}} at {{tempo}} BPM
2. Choose from these patterns for each measure (each represents a full 4/4 measure):
   - whole: One whole note (4 beats)
   - half: Two half notes (2 beats each)
   - quarter: Four quarter notes (1 beat each)
   - walking: Walking bass with quarter notes (1-3-5-7 or similar)
   - latin: Latin bass pattern with syncopation

3. Specify both the pattern and the chords for each measure using this format:
   |pattern: whole, chord: C| pattern: walking, chord: G7|

4. Make each measure EXACTLY fit the pattern you choose
5. Follow the chord progression provided if available

ADDITIONAL GUIDELINES:
- Use appropriate bass range (E1-G3)
- Focus on root, fifth, and approach tones
- Each measure MUST be enclosed in | characters
- For walking bass patterns, create a smooth voice leading between chords

FORMATTED AS:
|pattern: whole, chord: C| pattern: walking, chord: G7|
)";

// Drum jazz template
const std::string drumJazzTemplate = R"(
Generate a jazz drum pattern using the following pattern-based format:

INSTRUCTIONS:
1. Create EXACTLY {{bars}} measures of jazz drums at {{tempo}} BPM
2. Choose from these patterns for each measure (each represents a full 4/4 measure):
   - swing: Jazz swing pattern with ride cymbal
   - latin: Latin groove with hi-hat and cross-stick
   - ballad: Jazz ballad with brushes
   - fill: Drum fill transitioning between sections

3. Specify the pattern for each measure using this format:
   |pattern: swing| pattern: latin|

4. For drums, no chord information is needed

ADDITIONAL GUIDELINES:
- Use appropriate jazz drumming techniques
- Each measure MUST be enclosed in | characters
- Add variation to keep the pattern interesting
- Add fills at the end of 4 or 8 bar phrases

FORMATTED AS:
|pattern: swing| pattern: swing| pattern: latin| pattern: fill|
)";

// Guitar jazz template
const std::string guitarJazzTemplate = R"(
Generate a jazz guitar pattern using the following pattern-based format:

INSTRUCTIONS:
1. Create EXACTLY {{bars}} measures of jazz guitar in {{key}} at {{tempo}} BPM
2. Choose from these patterns for each measure (each represents a full 4/4 measure):
   - whole: One whole note chord (4 beats)
   - half: Two half note chords (2 beats each)
   - quarter: Four quarter note chords (1 beat each)
   - comping: Jazz comping with syncopated rhythm
   - fingerpicking: Fingerpicking arpeggio pattern

3. Specify both the pattern and the chords for each measure using this format:
   |pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|

4. Make each measure EXACTLY fit the pattern you choose
5. Follow the chord progression provided if available

ADDITIONAL GUIDELINES:
- Use appropriate jazz chord voicings
- You can specify exact voicings with chord(notes) format: Cmaj7(E3,G3,B3,E4)
- Each measure MUST be enclosed in | characters
- Each pattern MUST be listed by name (whole, half, comping, etc.)

FORMATTED AS:
|pattern: comping, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
)";

// Get template for instrument and style
inline std::string getTemplate(const std::string& instrumentType, const std::string& style [[maybe_unused]] = "jazz") 
{    if (instrumentType == "piano") {
        return pianoJazzTemplate;
    }
    else if (instrumentType == "bass") {
        return bassJazzTemplate;
    }
    else if (instrumentType == "drums") {
        return drumJazzTemplate;
    }
    else if (instrumentType == "guitar") {
        return guitarJazzTemplate;
    }
    else {
        // Default to piano for unknown instruments
        return pianoJazzTemplate;
    }
}

// Get pattern implementation based on instrument and pattern name
inline std::string getPatternImplementation(
    const std::string& instrumentType, 
    const std::string& patternName) 
{
    if (instrumentType == "piano") {
        if (pianoPatternsImpl.find(patternName) != pianoPatternsImpl.end()) {
            return pianoPatternsImpl.at(patternName);
        }
    }
    else if (instrumentType == "bass") {
        if (bassPatternsImpl.find(patternName) != bassPatternsImpl.end()) {
            return bassPatternsImpl.at(patternName);
        }
    }
    else if (instrumentType == "drums") {
        if (drumPatternsImpl.find(patternName) != drumPatternsImpl.end()) {
            return drumPatternsImpl.at(patternName);
        }
    }
    else if (instrumentType == "guitar") {
        if (guitarPatternsImpl.find(patternName) != guitarPatternsImpl.end()) {
            return guitarPatternsImpl.at(patternName);
        }
    }
    
    // Default implementation (quarter notes)
    return "$(CHORD1)(q) $(CHORD2)(q) $(CHORD3)(q) $(CHORD4)(q)";
}

} // namespace PatternTemplates
} // namespace coverband