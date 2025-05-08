#pragma once

#include <map>
#include <string>


namespace covertracks {

/**
 * @brief Default prompt templates for different instrument types and styles
 * 
 * This namespace contains predefined templates that can be loaded
 * during musician initialization.
 */
namespace PromptTemplates {

// Piano templates
const std::map<std::string, std::string> piano = {
    // Default piano template
    {"default", R"(
Generate a piano pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Style: {{style}}

Create appropriate piano voicings and melodic content that works well 
for the specified style and section. Use a mix of chords and melodic lines
with appropriate musical phrasing.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Jazz piano template
    {"jazz", R"(
Generate a jazz piano pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Chord Progression: {{chords}}

Create jazz piano voicings with appropriate extensions and alterations.
Include:
- Left hand comping with jazz chord voicings
- Right hand melodic lines or chord highlights
- Typical jazz phrasing with swing feel
- Voice leading between chords

For comping, use various jazz piano techniques like:
- Rootless voicings
- Shell voicings
- Quartal harmony
- Guide tones with appropriate extensions (9ths, 11ths, 13ths)

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},
  
    // Rock piano template
    {"rock", R"(
Generate a rock piano pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Energy: {{intensity}}

Create a rock piano part that might be played by artists like Billy Joel, 
Elton John or Ben Folds. Use techniques like:
- Strong rhythmic eighth note patterns
- Block chords for power
- Broken chord patterns
- Percussive approach for chorus sections
- Arpeggiated patterns for verses

Adjust the energy level based on the section:
- Verse: More sparse, broken chords, mid-range
- Chorus: Full chords, higher energy, wider range
- Bridge: Dynamic contrast, interesting harmonic movement

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Classical piano template
    {"classical", R"(
Generate a classical piano pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Period: {{period}}

Create a classical piano part in the style of {{period}} (Baroque, Classical, Romantic, etc.).
Use appropriate techniques for the specified period:
- Baroque: Clear counterpoint, ornaments, terraced dynamics
- Classical: Clear phrases, Alberti bass, balanced structures
- Romantic: Expressive dynamics, rubato implied, emotional range
- Impressionistic: Rich harmonies, whole tone scales, pedal effects

Consider a typical classical structure with:
- Left hand: Accompaniment pattern appropriate to the period
- Right hand: Melodic line with appropriate ornamentation

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"}
};

// Bass templates
const std::map<std::string, std::string> bass = {
    // Default bass template
    {"default", R"(
Generate a bass pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Style: {{style}}

Create a bass line that provides rhythmic and harmonic foundation.
Focus on root notes, fifths, and appropriate passing tones.
Use a consistent rhythmic pattern appropriate for the style.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
Use MIDI notes in the bass range (E1=28 to G3=55) and appropriate velocity for dynamics.
)"},

    // Jazz bass template - walking bass
    {"jazz", R"(
Generate a jazz walking bass pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Chord Progression: {{chords}}

Create a walking bass line that:
- Uses quarter notes as the primary rhythm (with occasional eighth notes for variation)
- Outlines the chord progression with strong chord tones on beats 1 and 3
- Uses chromatic approach notes, scalar passages, and arpeggios for movement
- Incorporates typical jazz bass techniques like enclosures and turnarounds
- Has good voice leading between chords with minimal jumps

Use notes primarily in the range of E1 (MIDI 28) to G3 (MIDI 55).
Create a smooth, flowing line that a jazz bassist would typically play.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Rock bass template
    {"rock", R"(
Generate a rock bass pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Energy: {{intensity}}

Create a rock bass line with:
- Strong rhythmic drive using eighth notes or sixteenth notes
- Focus on root-fifth patterns and power note combinations
- Occasional riffs or fills at transition points
- Dynamic variation based on section intensity
- Mix of fingerstyle and pick-style techniques

Adjust the pattern based on the section:
- Verse: More subdued, focus on groove and foundation
- Chorus: Higher energy, potentially more active
- Bridge: Contrast with previous sections, build tension

Use notes primarily in the range of E1 (MIDI 28) to C3 (MIDI 48).

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Funk bass template
    {"funk", R"(
Generate a funk bass pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}

Create a funk bass line with:
- Syncopated sixteenth note patterns
- Strong emphasis on the "one"
- Ghost notes and dead notes (low velocity notes)
- Slap and pop techniques (indicated by higher velocity)
- Chromatic approaches and fills
- Repeated motifs with slight variations

Use funk bass techniques like:
- Octave jumps
- Hammer-ons and pull-offs (indicated by short notes)
- Slides between notes
- Thumb slaps on root notes (higher velocity)
- Popped notes on higher strings (highest velocity)

Use notes primarily in the range of E1 (MIDI 28) to G3 (MIDI 55).

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"}
};

// Drum templates
const std::map<std::string, std::string> drums = {
    // Default drum template
    {"default", R"(
Generate a drum pattern with the following parameters:

Tempo: {{tempo}} BPM
Section: {{section}}
Style: {{style}}

Create a drum pattern using General MIDI mapping:
- Kick = note 36
- Snare = note 38
- Closed Hi-hat = note 42
- Open Hi-hat = note 46
- Ride cymbal = note 51
- Crash cymbal = note 49
- Tom-toms = notes 41, 43, 45, 47, 48, 50

Use appropriate rhythm patterns for the style.
Add fills at the end of phrases if appropriate.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Jazz drum template
    {"jazz", R"(
Generate a jazz drum pattern with the following parameters:

Tempo: {{tempo}} BPM
Section: {{section}}
Swing: {{swing}}

Create a jazz drum pattern using General MIDI mapping:
- Ride cymbal (51) - primary timekeeper with swing pattern
- Hi-hat foot (44) - on beats 2 and 4 (typical jazz)
- Kick drum (36) - sparse, emphasizing key points in phrase
- Snare drum (38) - light accents and comping
- Crash (49) - used for section starts or accents

Use a ride cymbal pattern with swing feel (longer first eighth note, shorter second).
"Feather" the bass drum subtly on some beats.
Add snare "comping" to complement the melodic phrases.
Add appropriate fills at phrase endings (usually 4 or 8 bar phrases).

Specify the swing amount from 0.5 (straight) to 0.67 (medium swing) to 0.75 (heavy swing).

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Rock drum template
    {"rock", R"(
Generate a rock drum pattern with the following parameters:

Tempo: {{tempo}} BPM
Section: {{section}}
Energy: {{intensity}}

Create a rock drum pattern using General MIDI mapping:
- Kick (36) - strong foundational beats
- Snare (38) - backbeats (typically beats 2 and 4)
- Closed Hi-hat (42) - eighth or sixteenth note patterns
- Open Hi-hat (46) - occasional accents
- Crash (49) - section starts and major accents
- Toms (41, 43, 45, 47, 48) - fills

Adjust the pattern based on the section:
- Verse: Standard rock beat with consistent hi-hat
- Chorus: More crash cymbals, potentially busier kick pattern
- Bridge: Contrasting pattern, possibly tom-focused

Add appropriate fills at transition points.
Use velocity to create dynamic emphasis (accents).

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Funk drum template
    {"funk", R"(
Generate a funk drum pattern with the following parameters:

Tempo: {{tempo}} BPM
Section: {{section}}
Groove: {{groove}}

Create a funk drum pattern using General MIDI mapping:
- Kick (36) - syncopated patterns, not just on downbeats
- Snare (38) - backbeats with ghost notes (low velocity)
- Closed Hi-hat (42) - sixteenth note patterns
- Open Hi-hat (46) - on upbeats for emphasis
- Ride (51) - alternative to hi-hat for variation
- Rim click (37) - for additional syncopation

Focus on groove elements:
- "Ghost notes" on snare (very low velocity notes)
- Syncopated kick patterns (not aligned with main beats)
- Linear patterns (avoiding simultaneous hits)
- Hi-hat variations (opening and closing)
- Intricate sixteenth note subdivisions

Specify a particular funk groove style: 
New Orleans, James Brown, P-Funk, Hip-Hop, etc.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"}
};

// Guitar templates
const std::map<std::string, std::string> guitar = {
    // Default guitar template
    {"default", R"(
Generate a guitar pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Style: {{style}}

Create a guitar part using appropriate techniques for the specified style.
Use strumming patterns, arpeggios, or riffs as needed.
Focus on the midrange of the guitar (E2=40 to E5=76).
Consider appropriate playing techniques like hammer-ons, pull-offs, etc.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Jazz guitar template - comping
    {"jazz", R"(
Generate a jazz guitar comping pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Chord Progression: {{chords}}

Create a jazz guitar comping part with:
- Chord voicings using jazz harmony (7ths, 9ths, 13ths, etc.)
- Rhythmic comping with syncopated patterns
- Freddie Green style quarter note comping or more modern approaches
- Voice leading between chord changes
- Shell voicings and drop 2/3 voicings when appropriate

Use jazz guitar techniques like:
- Guide tones and minimal voicings
- Rootless voicings (assuming bass player covers the root)
- Chromatic approach chords
- Passing chords and harmonic substitutions

Primarily use notes in typical jazz guitar range (E3=52 to G5=79).
Use appropriate MIDI velocity for dynamics and accents.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Rock guitar template - rhythm
    {"rock", R"(
Generate a rock rhythm guitar pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Energy: {{intensity}}

Create a rock rhythm guitar part with:
- Power chords for high-energy sections
- Open chords or arpeggios for verse sections
- Appropriate rhythmic strumming patterns
- Palm muting or accents as needed
- Occasional riffs or fills at transition points

Adjust the approach based on the section:
- Verse: More restrained, possibly arpeggiated or muted
- Chorus: Fuller sound, more aggressive strumming
- Bridge: Contrast with previous sections

Use standard guitar techniques like:
- Palm muting (reduced velocity)
- Accents (higher velocity)
- Chord strumming patterns
- Occasional single note runs

Primarily use notes in typical guitar range (E2=40 to E5=76).

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"},

    // Folk guitar template - fingerpicking
    {"folk", R"(
Generate a folk fingerpicking guitar pattern with the following parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Section: {{section}}
Pattern: {{pattern}}

Create a folk fingerpicking guitar part with:
- Alternating bass notes with melodic figures
- Travis picking or similar fingerstyle patterns
- Arpeggiated chord patterns
- Melodic fills between chord changes

Use common fingerpicking patterns:
- Travis picking (alternating bass with melody notes)
- Arpeggio patterns (ascending or descending)
- Roll patterns (forward, reverse, or mixed)
- Harp-like patterns with open strings

Primarily use notes in typical folk guitar range (E2=40 to B4=71).
Use moderate velocity for bass notes and slightly higher for melody notes.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"}
};

// Test patterns for all instruments - generic melody template
const std::map<std::string, std::string> test = {
    // Generic melody template for any song
    {"melody", R"(
Generate a pattern for "{{title}}" for {{type}} instrument with these parameters:

Tempo: {{tempo}} BPM
Key: {{key}}
Style: {{style}}

The melody follows this pattern:
{{melody_pattern}}

In {{key}} major, this would be:
{{melody_notes}}

Adapt this appropriately for {{type}} instrument, considering its specific range and role.
For piano, include simple harmony.
For bass, focus on supporting root notes.
For drums, create a simple beat to accompany the melody.
For guitar, use either melody or simple chords.

FORMAT REQUIREMENTS:
Your response MUST be a valid JSON object with the following structure exactly:
{
  "events": [
    {"type": "note_on", "channel": 0, "note": 60, "velocity": 100, "time": 0},
    {"type": "note_off", "channel": 0, "note": 60, "time": 480},
    ...additional events...
  ]
}

All events MUST include:
- "type": Either "note_on" or "note_off"
- "channel": Integer (typically 0)
- "time": Integer in ticks (480 ticks = 1 quarter note)
- For note_on: "note" (MIDI note number, where C4=60) and "velocity" (1-127)
- For note_off: "note" (same MIDI number as corresponding note_on)

Every note_on MUST have a matching note_off event.
)"}
};

// Load templates for a specific instrument type
inline var getTemplates(const std::string& instrumentType) {
    DynamicObject* templatesObj = new DynamicObject();
    
    // Add test templates for all instruments
    for (const auto& pair : test) {
        templatesObj->setProperty(Identifier(pair.first), String(pair.second));
    }
    
    // Add instrument-specific templates
    if (instrumentType == "piano") {
        for (const auto& pair : piano) {
            templatesObj->setProperty(Identifier(pair.first), String(pair.second));
        }
    }
    else if (instrumentType == "bass") {
        for (const auto& pair : bass) {
            templatesObj->setProperty(Identifier(pair.first), String(pair.second));
        }
    }
    else if (instrumentType == "drums") {
        for (const auto& pair : drums) {
            templatesObj->setProperty(Identifier(pair.first), String(pair.second));
        }
    }
    else if (instrumentType == "guitar") {
        for (const auto& pair : guitar) {
            templatesObj->setProperty(Identifier(pair.first), String(pair.second));
        }
    }
    
    return var(templatesObj);
}

} // namespace PromptTemplates
} // namespace covertracks