#pragma once

#include <JuceHeader.h>
#include <string>
#include <map>
#include <vector>
#include <regex>

namespace covertracks {

/**
 * @brief Utility class for musical note and rhythm parsing
 * 
 * This class provides core utilities for parsing musical notation elements,
 * such as notes, rhythms, and chords. It serves as a foundation for the
 * pattern-based musical notation system.
 */
class MusicalShorthand {
public:
    /**
     * @brief Parses a note string to MIDI note number
     * 
     * @param noteStr Note string (e.g., "C4", "Eb3", "F#5")
     * @return int MIDI note number or -1 if invalid
     */
    static int parseNote(const std::string& noteStr) {
        static const std::map<char, int> noteValues = {
            {'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}
        };
        
        if (noteStr.empty() || noteStr == "r") {
            return -1;  // Rest or empty
        }
        
        // Extract note name, accidental, and octave
        std::regex noteRegex(R"(([A-G])(b|#)?([\d]+))");
        std::smatch matches;
        
        if (!std::regex_match(noteStr, matches, noteRegex)) {
            return -1;
        }
        
        char noteName = matches[1].str()[0];
        std::string accidental = matches[2].str();
        int octave = std::stoi(matches[3].str());
        
        // Calculate MIDI note number
        int noteValue = noteValues.at(noteName);
        
        if (accidental == "b") {
            noteValue--;
        } else if (accidental == "#") {
            noteValue++;
        }
        
        return (octave + 1) * 12 + noteValue;
    }
    
    /**
     * @brief Converts MIDI note to note string
     * 
     * @param midiNote MIDI note number
     * @return std::string Note string (e.g., "C4")
     */
    static std::string midiNoteToString(int midiNote) {
        static const std::vector<std::string> noteNames = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        
        if (midiNote < 0 || midiNote > 127) {
            return "?";
        }
        
        int octave = (midiNote / 12) - 1;
        int noteIndex = midiNote % 12;
        
        return noteNames[noteIndex] + std::to_string(octave);
    }
    
    /**
     * @brief Parse a rhythm value to MIDI ticks
     * 
     * @param rhythmStr Rhythm string (e.g., "q", "h", "e.")
     * @param ppq Pulses per quarter note (default: 480)
     * @return int Duration in MIDI ticks
     */
    static int parseRhythm(const std::string& rhythmStr, int ppq = 480) {
        if (rhythmStr.empty()) {
            return ppq;  // Default to quarter note
        }
        
        char baseRhythm = rhythmStr[0];
        bool isDotted = rhythmStr.length() > 1 && rhythmStr[1] == '.';
        
        int baseDuration = 0;
        
        switch (baseRhythm) {
            case 'w': baseDuration = ppq * 4; break;  // Whole note
            case 'h': baseDuration = ppq * 2; break;  // Half note
            case 'q': baseDuration = ppq; break;      // Quarter note
            case 'e': baseDuration = ppq / 2; break;  // Eighth note
            case 's': baseDuration = ppq / 4; break;  // Sixteenth note
            case 't': baseDuration = ppq / 3; break;  // Triplet eighth
            default: baseDuration = ppq; break;       // Default to quarter
        }
        
        // Add half duration for dotted notes
        if (isDotted) {
            baseDuration += baseDuration / 2;
        }
        
        return baseDuration;
    }
    
    /**
     * @brief Parse a chord symbol to component notes
     * 
     * @param chordStr Chord string (e.g., "Cmaj7", "G7", "Ebm7b5")
     * @param octave Base octave for the chord
     * @return std::vector<int> MIDI note numbers for the chord tones
     */
    static std::vector<int> parseChord(const std::string& chordStr, int octave = 4) {
        std::regex chordRegex(R"(([A-G][b#]?)([^/]*)(\/([A-G][b#]?))?)",
                             std::regex_constants::icase);
        std::smatch matches;
        
        if (!std::regex_match(chordStr, matches, chordRegex)) {
            return {};
        }
        
        std::string root = matches[1].str();
        std::string quality = matches[2].str();
        std::string bassNote = matches[4].str();
        
        // Parse root note
        int rootNote = parseNote(root + std::to_string(octave));
        if (rootNote == -1) {
            return {};
        }
        
        std::vector<int> notes;
        notes.push_back(rootNote);
        
        // Add chord tones based on quality
        if (quality == "m" || quality == "min" || quality == "minor") {
            notes.push_back(rootNote + 3);  // Minor third
            notes.push_back(rootNote + 7);  // Perfect fifth
        }
        else if (quality == "7") {
            notes.push_back(rootNote + 4);  // Major third
            notes.push_back(rootNote + 7);  // Perfect fifth
            notes.push_back(rootNote + 10); // Minor seventh
        }
        else if (quality == "maj7" || quality == "M7") {
            notes.push_back(rootNote + 4);  // Major third
            notes.push_back(rootNote + 7);  // Perfect fifth
            notes.push_back(rootNote + 11); // Major seventh
        }
        else if (quality == "m7" || quality == "min7") {
            notes.push_back(rootNote + 3);  // Minor third
            notes.push_back(rootNote + 7);  // Perfect fifth
            notes.push_back(rootNote + 10); // Minor seventh
        }
        else if (quality == "m7b5" || quality == "min7b5") {
            notes.push_back(rootNote + 3);  // Minor third
            notes.push_back(rootNote + 6);  // Diminished fifth
            notes.push_back(rootNote + 10); // Minor seventh
        }
        else {  // Default to major triad
            notes.push_back(rootNote + 4);  // Major third
            notes.push_back(rootNote + 7);  // Perfect fifth
        }
        
        // Handle slash chords
        if (!bassNote.empty()) {
            int bassNoteNum = parseNote(bassNote + std::to_string(octave - 1));
            if (bassNoteNum != -1) {
                notes[0] = bassNoteNum;  // Replace root with bass note
            }
        }
        
        return notes;
    }
   
    // Add this enhanced function to MusicalShorthand class to handle more complex chord types

/**
 * @brief Parse a chord symbol to component notes with extended voicings
 * 
 * @param chordStr Chord string (e.g., "Cmaj7", "G7", "Ebm7b5", "G13b9")
 * @param octave Base octave for the chord
 * @return std::vector<int> MIDI note numbers for the chord tones
 */
static std::vector<int> parseExtendedChord(const std::string& chordStr, int octave = 4) {
    std::regex chordRegex(R"(([A-G][b#]?)([^/]*)(\/([A-G][b#]?))?)",
                         std::regex_constants::icase);
    std::smatch matches;
    
    if (!std::regex_match(chordStr, matches, chordRegex)) {
        return {};
    }
    
    std::string root = matches[1].str();
    std::string quality = matches[2].str();
    std::string bassNote = matches[4].str();
    
    // Parse root note
    int rootNote = parseNote(root + std::to_string(octave));
    if (rootNote == -1) {
        return {};
    }
    
    std::vector<int> notes;
    notes.push_back(rootNote);
    
    // Basic triad notes (used by most chords)
    int third = -1;
    int fifth = -1;
    int seventh = -1;
    int ninth = -1;
    int eleventh = -1;
    int thirteenth = -1;
    
    // Determine third (major or minor)
    if (quality.find("m") == 0 || quality.find("min") == 0 || 
        quality.find("-") == 0 || quality.find("dim") == 0 || 
        quality.find("o") == 0 || quality.find("ø") == 0) {
        // Minor third
        third = rootNote + 3;
    } else {
        // Major third
        third = rootNote + 4;
    }
    
    // Determine fifth (perfect, diminished, or augmented)
    if (quality.find("dim") != std::string::npos || 
        quality.find("o") != std::string::npos || 
        quality.find("ø") != std::string::npos ||
        quality.find("b5") != std::string::npos) {
        // Diminished fifth
        fifth = rootNote + 6;
    } else if (quality.find("+") != std::string::npos || 
              quality.find("aug") != std::string::npos ||
              quality.find("#5") != std::string::npos) {
        // Augmented fifth
        fifth = rootNote + 8;
    } else {
        // Perfect fifth
        fifth = rootNote + 7;
    }
    
    // Determine seventh (major, minor, or diminished)
    if (quality.find("maj7") != std::string::npos || 
        quality.find("M7") != std::string::npos ||
        quality.find("maj9") != std::string::npos ||
        quality.find("maj13") != std::string::npos) {
        // Major seventh
        seventh = rootNote + 11;
    } else if (quality.find("7") != std::string::npos || 
              quality.find("9") != std::string::npos ||
              quality.find("11") != std::string::npos ||
              quality.find("13") != std::string::npos ||
              quality.find("m7") != std::string::npos ||
              quality.find("min7") != std::string::npos) {
        // Minor seventh
        seventh = rootNote + 10;
    } else if (quality.find("dim7") != std::string::npos) {
        // Diminished seventh
        seventh = rootNote + 9;
    }
    
    // Determine extensions (9th, 11th, 13th)
    if (quality.find("9") != std::string::npos) {
        ninth = rootNote + 14; // Major 9th
        if (quality.find("b9") != std::string::npos) {
            ninth = rootNote + 13; // Flat 9th
        } else if (quality.find("#9") != std::string::npos) {
            ninth = rootNote + 15; // Sharp 9th
        }
    }
    
    if (quality.find("11") != std::string::npos) {
        eleventh = rootNote + 17; // Perfect 11th
        if (quality.find("#11") != std::string::npos) {
            eleventh = rootNote + 18; // Sharp 11th
        }
    }
    
    if (quality.find("13") != std::string::npos) {
        thirteenth = rootNote + 21; // Major 13th
        if (quality.find("b13") != std::string::npos) {
            thirteenth = rootNote + 20; // Flat 13th
        }
    }
    
    // Now build the chord based on all these components
    // Always include the root note (already added)
    
    // Add third if determined
    if (third != -1) {
        notes.push_back(third);
    }
    
    // Add fifth if determined
    if (fifth != -1) {
        notes.push_back(fifth);
    }
    
    // Add seventh if determined
    if (seventh != -1) {
        notes.push_back(seventh);
    }
    
    // Add ninth if applicable
    if (ninth != -1) {
        notes.push_back(ninth);
    }
    
    // Add eleventh if applicable
    if (eleventh != -1) {
        notes.push_back(eleventh);
    }
    
    // Add thirteenth if applicable
    if (thirteenth != -1) {
        notes.push_back(thirteenth);
    }
    
    // Handle slash chords
    if (!bassNote.empty()) {
        int bassNoteNum = parseNote(bassNote + std::to_string(octave - 1));
        if (bassNoteNum != -1) {
            // Remove the original root if it exists
            auto it = std::find(notes.begin(), notes.end(), rootNote);
            if (it != notes.end()) {
                notes.erase(it);
            }
            // Add the bass note
            notes.insert(notes.begin(), bassNoteNum);
        }
    }
    
    // Handle special voicing adjustments
    if (quality.find("maj9") != std::string::npos || quality.find("9") != std::string::npos) {
        // For ninth chords, we might want to omit the root in upper voicings
        if (notes.size() > 4) {
            auto it = std::find(notes.begin() + 1, notes.end(), rootNote + 12);
            if (it != notes.end()) {
                notes.erase(it);
            }
        }
    }
    
    // For 11th and 13th chords, we often omit the 5th and sometimes the 9th to avoid muddiness
    if (quality.find("11") != std::string::npos || quality.find("13") != std::string::npos) {
        if (notes.size() > 4) {
            auto it = std::find(notes.begin(), notes.end(), fifth);
            if (it != notes.end()) {
                notes.erase(it);
            }
        }
    }
    
    return notes;
}

    /**
     * @brief Parse a chord with explicit voicing
     * 
     * @param voicingStr Voicing string (e.g., "Cmaj7[C3,E3,G3,B3]")
     * @return std::vector<int> MIDI note numbers for the voicing
     */
    static std::vector<int> parseVoicing(const std::string& voicingStr) {
        std::regex voicingRegex(R"(([A-G][b#]?[^[]*)\[(.*)\])", 
                               std::regex_constants::icase);
        std::smatch matches;
        
        if (!std::regex_match(voicingStr, matches, voicingRegex)) {
            return {};
        }
        
        std::string chordName = matches[1].str();
        std::string voicingNotes = matches[2].str();
        
        // Parse individual notes in the voicing
        std::vector<int> notes;
        std::regex noteRegex(R"([A-G][b#]?\d+)");
        std::string::const_iterator searchStart = voicingNotes.cbegin();
        std::smatch noteMatches;
        
        while (std::regex_search(searchStart, voicingNotes.cend(), noteMatches, noteRegex)) {
            std::string noteStr = noteMatches[0].str();
            int note = parseNote(noteStr);
            if (note != -1) {
                notes.push_back(note);
            }
            searchStart = noteMatches.suffix().first;
        }
        
        return notes;
    }
    
    /**
     * @brief Parse a sequence of notes and durations
     * 
     * @param noteSequence String of note+duration pairs (e.g., "C4q D4q E4e F4e")
     * @param ppq Pulses per quarter note (default: 480)
     * @return var MIDI events for the note sequence as JUCE var/JSON
     */
    static var parseNoteSequence(const std::string& noteSequence, int ppq = 480) {
        var events = var(Array<var>());
        
        // Regex to match notes with duration
        std::regex noteRegex(R"(([A-Ga-g][b#]?\d+|r)([a-z\.]*))");
        std::string::const_iterator searchStart = noteSequence.cbegin();
        std::smatch noteMatches;
        
        int currentTime = 0;
        
        while (std::regex_search(searchStart, noteSequence.cend(), noteMatches, noteRegex)) {
            std::string noteStr = noteMatches[1].str();
            std::string rhythmStr = noteMatches[2].str();
            
            // Parse rhythm first
            int duration = parseRhythm(rhythmStr, ppq);
            
            // Check if it's a rest
            if (noteStr == "r") {
                // For rests, we just advance the time
                currentTime += duration;
            } else {
                // Parse note
                int note = parseNote(noteStr);
                if (note != -1) {
                    // Add note-on event
                    DynamicObject* noteOnObj = new DynamicObject();
                    noteOnObj->setProperty("type", "note_on");
                    noteOnObj->setProperty("channel", 0);
                    noteOnObj->setProperty("note", note);
                    noteOnObj->setProperty("velocity", 100);
                    noteOnObj->setProperty("time", currentTime);
                    events.append(var(noteOnObj));
                    
                    // Add note-off event
                    DynamicObject* noteOffObj = new DynamicObject();
                    noteOffObj->setProperty("type", "note_off");
                    noteOffObj->setProperty("channel", 0);
                    noteOffObj->setProperty("note", note);
                    noteOffObj->setProperty("time", currentTime + duration);
                    events.append(var(noteOffObj));
                }
                
                // Advance time
                currentTime += duration;
            }
            
            // Skip bar line if present
            std::string remainder = noteMatches.suffix();
            if (!remainder.empty() && remainder[0] == '|') {
                // Skip the bar line character
                searchStart = noteMatches.suffix().first + 1;
            } else {
                searchStart = noteMatches.suffix().first;
            }
        }
        
        return events;
    }
    
    /**
     * @brief Find chord name from a set of MIDI notes
     * 
     * @param notes Vector of MIDI note numbers
     * @return std::string The chord name (e.g., "Cmaj7", "Gm")
     */
    static std::string findChordName(const std::vector<int>& notes) {
        // Sort notes and convert to pitch classes
        std::vector<int> pitchClasses;
        for (int note : notes) {
            pitchClasses.push_back(note % 12);
        }
        std::sort(pitchClasses.begin(), pitchClasses.end());
        
        // Remove duplicates
        pitchClasses.erase(std::unique(pitchClasses.begin(), pitchClasses.end()), pitchClasses.end());
        
        // Find the root note (assume lowest note is root)
        int root = pitchClasses[0];
        std::string rootName;
        
        switch (root) {
            case 0: rootName = "C"; break;
            case 1: rootName = "C#"; break;
            case 2: rootName = "D"; break;
            case 3: rootName = "D#"; break;
            case 4: rootName = "E"; break;
            case 5: rootName = "F"; break;
            case 6: rootName = "F#"; break;
            case 7: rootName = "G"; break;
            case 8: rootName = "G#"; break;
            case 9: rootName = "A"; break;
            case 10: rootName = "A#"; break;
            case 11: rootName = "B"; break;
            default: rootName = "?"; break;
        }
        
        // Find intervals relative to root
        std::vector<int> intervals;
        for (int pc : pitchClasses) {
            intervals.push_back((pc - root + 12) % 12);
        }
        
        // Identify chord type based on intervals
        std::string chordType;
        
        // Check for major chord (root, major third, fifth)
        if (std::find(intervals.begin(), intervals.end(), 4) != intervals.end() &&
            std::find(intervals.begin(), intervals.end(), 7) != intervals.end()) {
            
            // Check for seventh chords
            if (std::find(intervals.begin(), intervals.end(), 10) != intervals.end()) {
                chordType = "7"; // Dominant seventh
            }
            else if (std::find(intervals.begin(), intervals.end(), 11) != intervals.end()) {
                chordType = "maj7"; // Major seventh
            }
            else {
                chordType = ""; // Major triad
            }
        }
        // Check for minor chord (root, minor third, fifth)
        else if (std::find(intervals.begin(), intervals.end(), 3) != intervals.end() &&
                std::find(intervals.begin(), intervals.end(), 7) != intervals.end()) {
            
            // Check for seventh chords
            if (std::find(intervals.begin(), intervals.end(), 10) != intervals.end()) {
                chordType = "m7"; // Minor seventh
            }
            else if (std::find(intervals.begin(), intervals.end(), 11) != intervals.end()) {
                chordType = "mM7"; // Minor-major seventh
            }
            else {
                chordType = "m"; // Minor triad
            }
        }
        // Check for diminished chord (root, minor third, diminished fifth)
        else if (std::find(intervals.begin(), intervals.end(), 3) != intervals.end() &&
                std::find(intervals.begin(), intervals.end(), 6) != intervals.end()) {
            
            // Check for seventh
            if (std::find(intervals.begin(), intervals.end(), 9) != intervals.end()) {
                chordType = "dim7"; // Diminished seventh
            }
            else if (std::find(intervals.begin(), intervals.end(), 10) != intervals.end()) {
                chordType = "m7b5"; // Half-diminished seventh
            }
            else {
                chordType = "dim"; // Diminished triad
            }
        }
        // Check for augmented chord (root, major third, augmented fifth)
        else if (std::find(intervals.begin(), intervals.end(), 4) != intervals.end() &&
                std::find(intervals.begin(), intervals.end(), 8) != intervals.end()) {
            chordType = "aug"; // Augmented triad
        }
        else {
            // If we can't identify the chord, just use "?"
            chordType = "?";
        }
        
        return rootName + chordType;
    }
};

} // namespace covertracks