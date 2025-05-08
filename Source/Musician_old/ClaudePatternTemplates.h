#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace covertracks {

/**
 * @brief Claude-optimized pattern templates for different instruments and styles
 * 
 * This namespace contains templates specially designed for Claude 3.7 Sonnet,
 * leveraging its musical knowledge and pattern generation capabilities.
 */
namespace ClaudePatternTemplates {

// Piano templates optimized for Claude
const std::map<std::string, std::string> piano = {
    // Default piano template with explicit instructions for Claude
    {"default", R"(
Generate a piano pattern in {{key}} at {{tempo}} BPM for the {{section}} section.

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars with appropriate voicings.
Ensure all notes have matching note_on and note_off events.
Use the piano range from A0 (21) to C8 (108).

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"},

    // Jazz piano template optimized for Claude
    {"jazz", R"(
Generate a jazz piano pattern in {{key}} at {{tempo}} BPM for the {{section}} section.

Create a piano part that includes:
- Jazz chord voicings with extensions (7ths, 9ths, 13ths)
- Subtle rhythmic comping in the left hand
- Occasional melodic fills in the right hand
- Swing feel with appropriate timing
- Voice leading between chord changes

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
Use the piano range from A0 (21) to C8 (108).

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"}
};

// Bass templates optimized for Claude
const std::map<std::string, std::string> bass = {
    // Default bass template with explicit instructions for Claude
    {"default", R"(
Generate a bass line pattern in {{key}} at {{tempo}} BPM for the {{section}} section.

The bass line should provide a solid foundation with:
- Clear root notes marking harmonic changes
- Appropriate walking or groove patterns
- Rhythmic stability and forward momentum
- Occasional fills at transition points

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 43, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 43, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
Use the bass range from E1 (28) to G3 (55).

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"},

    // Jazz bass template optimized for Claude
    {"jazz", R"(
Generate a jazz walking bass line in {{key}} at {{tempo}} BPM for the {{section}} section.

Create a bass line that includes:
- Quarter note walking patterns
- Strong beats on 1 and 3
- Chromatic approach notes
- Clear outlining of the chord progression
- Smooth voice leading between chords

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 43, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 43, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
Use the bass range from E1 (28) to G3 (55).

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"}
};

// Drum templates optimized for Claude
const std::map<std::string, std::string> drums = {
    // Default drum template with explicit instructions for Claude
    {"default", R"(
Generate a drum pattern at {{tempo}} BPM for the {{section}} section.

The pattern should use General MIDI drum mapping:
- Kick drum = note 36
- Snare drum = note 38
- Closed hi-hat = note 42
- Open hi-hat = note 46
- Ride cymbal = note 51
- Crash cymbal = note 49
- Toms = notes 41, 43, 45, 47

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 9, "note": 36, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 9, "note": 36, "time": 20},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
For drums, use short note durations (10-20 ticks).
Use channel 9 for all drum events.

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"},

    // Jazz drum template optimized for Claude
    {"jazz", R"(
Generate a jazz drum pattern at {{tempo}} BPM for the {{section}} section.

Create a jazz drum pattern with:
- Ride cymbal pattern with swing feel
- Hi-hat foot on beats 2 and 4
- Subtle kick drum on beat 1 and occasional accents
- Snare comping and accents
- Occasional brush or fill techniques

Use the General MIDI drum mapping:
- Kick drum = note 36
- Snare drum = note 38
- Closed hi-hat = note 42
- Open hi-hat = note 46
- Ride cymbal = note 51
- Crash cymbal = note 49
- Brush snare = note 40

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 9, "note": 51, "velocity": 90, "time": 0},
    {"type": "note_off", "channel": 9, "note": 51, "time": 20},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
For drums, use short note durations (10-20 ticks).
Use channel 9 for all drum events.

Incorporate swing by placing eighth notes slightly later than their strict position.

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"}
};

// Guitar templates optimized for Claude
const std::map<std::string, std::string> guitar = {
    // Default guitar template with explicit instructions for Claude
    {"default", R"(
Generate a guitar pattern in {{key}} at {{tempo}} BPM for the {{section}} section.

The pattern should include appropriate guitar techniques for the style:
- Chord strumming or arpeggios
- Single note melodies where appropriate
- Natural guitar phrasing and articulation
- Dynamics appropriate to the section

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 64, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 64, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
Use the guitar range from E2 (40) to E5 (76).

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"},

    // Jazz guitar template optimized for Claude
    {"jazz", R"(
Generate a jazz guitar pattern in {{key}} at {{tempo}} BPM for the {{section}} section.

Create a jazz guitar part that includes:
- Jazz chord voicings with extensions (7ths, 9ths, 13ths)
- Freddie Green style comping or modern jazz comping
- Appropriate voice leading between chord changes
- Occasional melodic fills or passing tones
- Swing feel with proper articulation

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 64, "velocity": 90, "time": 0},
    {"type": "note_off", "channel": 0, "note": 64, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.
Use the guitar range from E2 (40) to E5 (76).

For jazz guitar voicings, focus on rootless voicings (roots covered by bass) and use appropriate jazz tensions.

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"}
};

// Test templates for all instruments
const std::map<std::string, std::string> test = {
    // Melody template optimized for Claude
    {"melody", R"(
Generate a melodic pattern for {{instrument}} instrument in {{key}} at {{tempo}} BPM.

Create a pattern based on this melody: {{melody_notes}}
Adapt it appropriately for the {{instrument}} instrument.

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.

Instrument-specific adaptations:
- For piano: Add appropriate harmony and accompaniment
- For bass: Focus on the root notes with walking bass or groove patterns
- For drums: Create a rhythm that complements the melody's phrasing
- For guitar: Use either melody with light accompaniment or chord-based approach

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)"}
};

// Utility functions for template handling

/**
 * @brief Get template for instrument and style
 * 
 * @param instrumentType The instrument type
 * @param style The musical style (default: "default")
 * @return std::string The template text
 */
inline std::string getTemplate(const std::string& instrumentType, const std::string& style = "default") {
    // Select the appropriate template map based on instrument
    const std::map<std::string, std::string>* templateMap = nullptr;
    
    if (instrumentType == "piano") {
        templateMap = &piano;
    }
    else if (instrumentType == "bass") {
        templateMap = &bass;
    }
    else if (instrumentType == "drums") {
        templateMap = &drums;
    }
    else if (instrumentType == "guitar") {
        templateMap = &guitar;
    }
    else {
        // Default to test templates for unknown instruments
        templateMap = &test;
    }
    
    // Look for the specific style
    if (templateMap && templateMap->find(style) != templateMap->end()) {
        return templateMap->at(style);
    }
    
    // Fall back to default style if the requested style doesn't exist
    if (templateMap && templateMap->find("default") != templateMap->end()) {
        return templateMap->at("default");
    }
    
    // If all else fails, return a generic template
    return R"(
Generate a musical pattern for {{instrument}} in {{key}} at {{tempo}} BPM.

The pattern should be a JSON array of MIDI events with the following structure:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

Time is measured in MIDI ticks, where 480 ticks = 1 quarter note.
Create a pattern lasting exactly {{bars}} bars.
Ensure all notes have matching note_on and note_off events.

Respond ONLY with a valid JSON object containing the "events" array. No explanations or additional text.
)";
}

} // namespace ClaudePatternTemplates
} // namespace covertracks