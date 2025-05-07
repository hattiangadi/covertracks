#pragma once

#include <JuceHeader.h>
#include <nlohmann/json.hpp>
#include <string>
#include <regex>
#include <sstream>
#include <map>
#include <vector>
#include "MusicalShorthand.h"
#include "PatternTemplates.h"
#include "PromptManager.h" // Added include
#include <vector>
#include <utility>
#include <algorithm>
#include <numeric>

using json = nlohmann::json;

namespace coverband {

/**
 * @brief Processor for handling pattern-based musical notation
 * 
 * This class provides functionality for parsing pattern-based notation
 * and converting it to MIDI events.
 */
class PatternProcessor {
public:
    /**
     * @brief Create a pattern-based prompt for the LLM
     * 
     * @param instrumentType The instrument type
     * @param parameters Parameters for template variables
     * @param style The musical style (default: "jazz")
     * @return std::string The prompt for the LLM
     */
    static std::string createPatternPrompt(
        const std::string& instrumentType,
        const json& parameters,
        const std::string& style = "jazz")
    {
    // Use PromptManager to create the prompt
    return PromptManager::createPrompt(instrumentType, parameters, style);
}
    
    /**
     * @brief Parse pattern-based notation to MIDI events
     * 
     * @param patternText The pattern-based notation
     * @param instrumentType The instrument type
     * @param playerId The player ID
     * @param patternId The pattern ID
     * @param tempo The tempo in BPM
     * @return json JSON object for the player layer
     */
    static json patternToJson(
        const std::string& patternText,
        const std::string& instrumentType,
        const std::string& playerId,
        const std::string& patternId,
        double tempo = 120.0)
    {
        json outputJson;
        try {
            // Clean up the input text
            std::string cleanPattern = cleanupPatternText(patternText);

            // Parse the pattern to MIDI events
            json events = parsePatternToEvents(cleanPattern, instrumentType);

            // Create JSON output for the player layer
            outputJson = {
                {"command", "play_pattern"},
                {"player_id", playerId},
                {"pattern_id", patternId},
                {"tempo", tempo},
                {"events", events}
            };
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in PatternProcessor::patternToJson: " << e.what() << std::endl;
            // Fallback: empty events
            outputJson = {
                {"command", "play_pattern"},
                {"player_id", playerId},
                {"pattern_id", patternId},
                {"tempo", tempo},
                {"events", json::array()}
            };
        } catch (const std::exception& e) { // Catch other potential exceptions
            juce::Logger::writeToLog("EXCEPTION in PatternProcessor::patternToJson: " + juce::String(e.what()));
            outputJson = {
                {"command", "play_pattern"},
                {"player_id", playerId},
                {"pattern_id", patternId},
                {"tempo", tempo},
                {"events", json::array()}
            };
        }
        return outputJson;
    }
    
    /**
     * @brief Parse pattern-based notation to MIDI events
     * 
     * @param patternText The pattern-based notation
     * @param instrumentType The instrument type
     * @param ppq Pulses per quarter note (default: 480)
     * @return json MIDI events
     */
    static json parsePatternToEvents(
        const std::string& patternText,
        const std::string& instrumentType,
        int ppq = 480)
    {
        json events = json::array();
        
        // Extract measures from the pattern text
        std::vector<std::string> measures = extractMeasures(patternText);
        
        // Process each measure
        int currentTime = 0;
        const int ticksPerMeasure = ppq * 4; // Assuming 4/4 time
        
        for (const auto& measure : measures) {
            // Parse the measure to get pattern and chord information
            PatternInfo patternInfo = parseMeasurePattern(measure);
            
            // Generate events for this measure based on the pattern
            json measureEvents = generateEventsForPattern(
                patternInfo,
                instrumentType,
                currentTime,
                ppq
            );
            
            // Add measure events to the main event list
            for (const auto& event : measureEvents) {
                events.push_back(event);
            }
            
            // Advance time to the next measure
            currentTime += ticksPerMeasure;
        }
        
        return events;
    }
    
    /**
     * @brief Extract measures from pattern text
     *
     * @param patternText The pattern text
     * @return std::vector<std::string> Vector of measure strings
     */
    /**
 * @brief Extract measures from pattern text
 *
 * @param patternText The pattern text
 * @return std::vector<std::string> Vector of measure strings
 */
static std::vector<std::string> extractMeasures(const std::string& patternText) {
    std::vector<std::string> measures;
    
    // Find all text between pipe characters
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    
    while ((start = patternText.find("|", start)) != std::string::npos) {
        // Move past the opening pipe
        start++;
        
        // Find the closing pipe
        end = patternText.find("|", start);
        
        // If no closing pipe, break
        if (end == std::string::npos) {
            break;
        }
        
        // Extract the measure content
        std::string measure = patternText.substr(start, end - start);
        
        // Trim whitespace
        measure = std::regex_replace(measure, std::regex("^\\s+|\\s+$"), "");
        
        if (!measure.empty()) {
            measures.push_back(measure);
        }
        
        // Move past the closing pipe for the next iteration
        start = end + 1;
    }
    
    // Debug output
    std::cout << "Extracted " << measures.size() << " measures from pattern text" << std::endl;
    
    return measures;
}
    
private:
    /**
     * @brief Structure to hold parsed pattern information
     */
    struct PatternInfo {
        std::string patternName;
        std::vector<std::string> chords;
        std::map<std::string, std::string> chordVoicings; // chord -> voicing
    };
    
    /**
     * @brief Fill template variables with values from parameters
     * 
     * @param templateText The template text with variables like {{key}}
     * @param parameters JSON object with variable values
     * @return std::string The filled template
     */
    static std::string fillTemplateVariables(const std::string& templateText, const json& parameters) {
        std::string result = templateText;
        
        // Replace {{variable}} with value from parameters
        std::regex varRegex(R"(\{\{(\w+)\}\})");
        std::string::const_iterator searchStart = result.cbegin();
        std::smatch matches;
        
        while (std::regex_search(searchStart, result.cend(), matches, varRegex)) {
            std::string varName = matches[1].str();
            std::string replacement;
            
            // Extract value from parameters
            if (parameters.contains(varName)) {
                if (parameters[varName].is_string()) {
                    replacement = parameters[varName].get<std::string>();
                } else {
                    replacement = parameters[varName].dump();
                }
            } else {
                // Default values for common parameters
                if (varName == "tempo") replacement = "120";
                else if (varName == "key") replacement = "C";
                else if (varName == "instrument") replacement = "instrument";
                else if (varName == "style") replacement = "jazz";
                else replacement = "[" + varName + "]";  // Placeholder for unknown variables
            }
            
            // Build regex for replacing this specific {{variable}}
            std::string replacePattern = "\\{\\{" + varName + "\\}\\}";
            result = std::regex_replace(result, std::regex(replacePattern), replacement);
            
            // Update search start position
            searchStart = result.cbegin();
        }
        
        return result;
    }
    
    /**
     * @brief Clean up pattern text for consistent parsing
     * 
     * @param patternText Original pattern text
     * @return std::string Cleaned pattern text
     */
    static std::string cleanupPatternText(const std::string& patternText) {
        std::string result = patternText;
        
        try {
            // Remove markdown code blocks if present
            std::regex codeBlockRegex(R"(```[a-zA-Z]*\s+([\s\S]*)\s*```)" );
            std::smatch codeBlockMatch;
            if (std::regex_search(result, codeBlockMatch, codeBlockRegex) && codeBlockMatch.size() > 1) {
                result = codeBlockMatch[1].str();
            }
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in codeBlockRegex: " << e.what() << std::endl;
        }
        
        // Replace newlines with spaces
        try {
            std::regex newlineRegex("\n");
            result = std::regex_replace(result, newlineRegex, " ");
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in newlineRegex: " << e.what() << std::endl;
        }
        
        // Remove "FORMATTED AS:" or similar headers (case-insensitive)
        try {
            std::regex headerRegex("FORMATTED AS:.*\\|", std::regex_constants::icase);
            result = std::regex_replace(result, headerRegex, "|");
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in headerRegex: " << e.what() << " (Code: " << e.code() << ")" << std::endl;
        }
        
        // Normalize spaces
        result = std::regex_replace(result, std::regex("\\s+"), " ");
        
        // Ensure consistent spacing around bar lines
        result = std::regex_replace(result, std::regex("\\s*\\|\\s*"), "|");
        
        // Trim leading and trailing whitespace
        result = std::regex_replace(result, std::regex("^\\s+|\\s+$"), "");
        
        // Add initial bar line if missing
        if (result.empty() || result[0] != '|') {
            result = "|" + result;
        }
        
        // Add final bar line if missing
        if (result.empty() || result[result.length() - 1] != '|') {
            result = result + "|";
        }
        
        return result;
    }
    
    /**
     * @brief Parse a measure to extract pattern and chord information
     * 
     * @param measure The measure text
     * @return PatternInfo Structure with pattern name and chords
     */
  /**
   * @brief Parse a measure to extract pattern and chord information
   *
   * @param measure The measure text
   * @return PatternInfo Structure with pattern name and chords
   */
/**
 * @brief Parse a measure to extract pattern and chord information
 * 
 * @param measure The measure text
 * @return PatternInfo Structure with pattern name and chords
 */
 static PatternInfo parseMeasurePattern(const std::string& measure) {
    PatternInfo info;
    info.patternName = "quarter"; // Default

    try {
        // Extract pattern name
        std::regex patternRegex(R"(pattern:\s*([a-zA-Z0-9-]+))");
        std::smatch patternMatch;
        if (std::regex_search(measure, patternMatch, patternRegex) && patternMatch.size() > 1) {
            info.patternName = patternMatch[1].str();
        }

        // Try to extract multiple chords first
        std::regex chordsRegex(R"(chords:\s*([^|,]+(?:,\s*[^|,]+)*))");
        std::smatch chordsMatch;
        if (std::regex_search(measure, chordsMatch, chordsRegex) && chordsMatch.size() > 1) {
            std::string chordList = chordsMatch[1].str();
            std::regex chordSplitRegex(R"(,\s*)");
            
            std::sregex_token_iterator iter(chordList.begin(), chordList.end(), chordSplitRegex, -1);
            std::sregex_token_iterator end;
            
            while (iter != end) {
                std::string chord = *iter++;
                // Trim whitespace
                chord = std::regex_replace(chord, std::regex("^\\s+|\\s+$"), "");
                if (!chord.empty()) {
                    info.chords.push_back(chord);
                }
            }
        } else {
            // Try to extract a single chord
            std::regex chordRegex(R"(chord:\s*([^|,]+))");
            std::smatch chordMatch;
            if (std::regex_search(measure, chordMatch, chordRegex) && chordMatch.size() > 1) {
                std::string chord = chordMatch[1].str();
                // Trim whitespace
                chord = std::regex_replace(chord, std::regex("^\\s+|\\s+$"), "");
                if (!chord.empty()) {
                    info.chords.push_back(chord);
                }
            }
        }

        // Extract any chord voicings if present
        std::regex voicingRegex(R"(([A-G][b#]?[^(]*)\(([^)]*)\))");
        std::smatch voicingMatch;
        std::string::const_iterator searchStart = measure.begin();
        
        while (std::regex_search(searchStart, measure.end(), voicingMatch, voicingRegex)) {
            if (voicingMatch.size() > 2) {
                std::string chordName = voicingMatch[1].str();
                std::string voicing = voicingMatch[2].str();
                
                // Trim whitespace
                chordName = std::regex_replace(chordName, std::regex("^\\s+|\\s+$"), "");
                
                if (!chordName.empty()) {
                    info.chordVoicings[chordName] = "(" + voicing + ")";
                }
            }
            
            searchStart = voicingMatch.suffix().first;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing measure pattern: " << e.what() << std::endl;
        std::cerr << "Measure: " << measure << std::endl;
    }

    // If we didn't find any chords, add a default C chord
    if (info.chords.empty()) {
        info.chords.push_back("C");
    }

    return info;
}
    
    /**
     * @brief Generate MIDI events for a pattern
     * 
     * @param patternInfo Pattern information
     * @param instrumentType Instrument type
     * @param startTime Start time in MIDI ticks
     * @param ppq Pulses per quarter note
     * @return json MIDI events for the pattern
     */
    static json generateEventsForPattern(
        const PatternInfo& patternInfo,
        const std::string& instrumentType,
        int startTime,
        int ppq)
    {
        json events = json::array();
        
        // Special handling for drums
        if (instrumentType == "drums") {
            return generateDrumEvents(patternInfo.patternName, startTime, ppq);
        }
        
        // If no chord information available, add defaults
        std::vector<std::string> chords = patternInfo.chords;
        if (chords.empty()) {
            chords.push_back("C");
        }
        
        // Get the implementation for this pattern
        std::string patternImpl = PatternTemplates::getPatternImplementation(
            instrumentType, patternInfo.patternName);
        
        // Parse pattern implementation
        std::vector<std::pair<std::string, std::string>> elements = parsePatternImplementation(patternImpl);
        
        // Calculate number of slots needed
        int numSlots = std::count(patternImpl.begin(), patternImpl.end(), '$') / 2;
        
        // Handle case where there aren't enough chords
        std::vector<std::string> usedChords = patternInfo.chords;
        if (usedChords.size() < numSlots) {
            // Repeat the last chord if needed
            while (usedChords.size() < numSlots && !usedChords.empty()) {
                usedChords.push_back(usedChords.back());
            }
        }
        
        // Calculate time per element to ensure 4/4 measure
        const int measureDuration = ppq * 4; // 4 beats
        
        // Process each element in the pattern
        int currentOffset = 0;
        
        for (size_t i = 0; i < elements.size(); i++) {
            std::string elementType = elements[i].first;
            std::string rhythmValue = elements[i].second;
            
            // Calculate duration for this element
            int duration = MusicalShorthand::parseRhythm(rhythmValue, ppq);
            
            // Skip rest elements
            if (elementType == "r") {
                currentOffset += duration;
                continue;
            }
            
            // Check if this is a chord element
            if (elementType.find("CHORD") != std::string::npos) {
                // Extract chord index if present
                int chordIndex = 0;
                std::regex chordIndexRegex(R"(CHORD(\d*))");
                std::smatch indexMatch;
                
                if (std::regex_search(elementType, indexMatch, chordIndexRegex) && indexMatch.size() > 1) {
                    std::string indexStr = indexMatch[1].str();
                    if (!indexStr.empty()) {
                        chordIndex = std::stoi(indexStr) - 1; // Convert to 0-based index
                    }
                }
                
                // Ensure chord index is within bounds
                if (chordIndex >= 0 && chordIndex < static_cast<int>(usedChords.size())) {
                    std::string chordName = usedChords[chordIndex];
                    
                    // Check if we need to extract a specific chord component (ROOT, 3RD, etc.)
                    std::string chordComponent = "FULL";
                    size_t colonPos = elementType.find(':');
                    if (colonPos != std::string::npos) {
                        chordComponent = elementType.substr(colonPos + 1);
                    }
                    
                    // Generate events for this chord
                    std::vector<int> notes;
                    
                    if (chordComponent == "ROOT") {
                        // Just the root note
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (!chordNotes.empty()) {
                            notes.push_back(chordNotes[0]); // Root note
                        }
                    }
                    else if (chordComponent == "3RD") {
                        // Just the third
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (chordNotes.size() > 1) {
                            notes.push_back(chordNotes[1]); // Third
                        }
                    }
                    else if (chordComponent == "5TH") {
                        // Just the fifth
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (chordNotes.size() > 2) {
                            notes.push_back(chordNotes[2]); // Fifth
                        }
                    }
                    else if (chordComponent == "7TH") {
                        // Just the seventh if available
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (chordNotes.size() > 3) {
                            notes.push_back(chordNotes[3]); // Seventh
                        }
                    }
                    else {
                        // Full chord
                        // Check if this chord has a specified voicing
                        if (patternInfo.chordVoicings.find(chordName) != patternInfo.chordVoicings.end()) {
                            // Parse the voicing
                            std::string voicing = patternInfo.chordVoicings.at(chordName);
                            
                            // Remove parentheses
                            voicing = std::regex_replace(voicing, std::regex("[()]"), "");
                            
                            // Check if it's a list of notes
                            if (voicing.find(',') != std::string::npos) {
                                // Parse individual notes
                                std::istringstream notesStream(voicing);
                                std::string noteStr;
                                
                                while (std::getline(notesStream, noteStr, ',')) {
                                    // Trim whitespace
                                    noteStr = std::regex_replace(noteStr, std::regex("^\\s+|\\s+$"), "");
                                    
                                    // Parse note
                                    int note = MusicalShorthand::parseNote(noteStr);
                                    if (note != -1) {
                                        notes.push_back(note);
                                    }
                                }
                            }
                            else {
                                // It could be a rhythm or pattern indicator, ignore for note generation
                            }
                        }
                        else {
                            // Generate standard chord voicing
                            notes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        }
                    }
                    
                    // Create note events
                    for (int note : notes) {
                        // Add note-on event
                        events.push_back({
                            {"type", "note_on"},
                            {"channel", 0},
                            {"note", note},
                            {"velocity", 100},
                            {"time", startTime + currentOffset}
                        });
                        
                        // Add note-off event
                        events.push_back({
                            {"type", "note_off"},
                            {"channel", 0},
                            {"note", note},
                            {"time", startTime + currentOffset + duration}
                        });
                    }
                }
            }
            
            // Advance time
            currentOffset += duration;
        }
        
        return events;
    }
    
    /**
     * @brief Parse pattern implementation into elements
     * 
     * @param patternImpl Pattern implementation string
     * @return std::vector<std::pair<std::string, std::string>> Vector of (elementType, rhythmValue) pairs
     */
    static std::vector<std::pair<std::string, std::string>> parsePatternImplementation(const std::string& patternImpl) {
        std::vector<std::pair<std::string, std::string>> elements;
        
        // Regex to match $(ELEMENT)OCTAVE(RHYTHM) patterns (allows for missing OCTAVE)
        std::regex elementRegex(R"(\$\(([^)]+)\)\d*\(([^)]+)\))"); // Corrected Regex
        std::string::const_iterator searchStart = patternImpl.cbegin();
        std::smatch matches;
        
        while (std::regex_search(searchStart, patternImpl.cend(), matches, elementRegex)) {
            if (matches.size() > 2) {
                std::string elementType = matches[1].str();
                std::string rhythmValue = matches[2].str();
                
                elements.push_back(std::make_pair(elementType, rhythmValue));
            }
            
            searchStart = matches.suffix().first;
        }
        
        return elements;
    }
    
    /**
     * @brief Generate drum events for a specific pattern
     * 
     * @param patternName The drum pattern name
     * @param startTime Start time in MIDI ticks
     * @param ppq Pulses per quarter note
     * @return json MIDI events for the drum pattern
     */
    static json generateDrumEvents(
        const std::string& patternName,
        int startTime,
        int ppq)
    {
        json events = json::array();
        
        // Get the implementation for this drum pattern
        std::string patternImpl = PatternTemplates::getPatternImplementation("drums", patternName);
        
        // Define drum MIDI note mapping
        static const std::map<std::string, int> drumMap = {
            {"K", 36},   // Kick
            {"S", 38},   // Snare
            {"HH", 42},  // Closed hi-hat
            {"OH", 46},  // Open hi-hat
            {"R", 51},   // Ride
            {"C", 49},   // Crash
            {"T1", 41},  // Tom 1
            {"T2", 43},  // Tom 2
            {"T3", 45},  // Tom 3
            {"T4", 47}   // Tom 4
        };
        
        // Parse the drum pattern implementation
        std::regex instrRegex;
        try {
            instrRegex = std::regex(R"(([A-Za-z0-9]+):([^,\s]+(,[^,\s]+)*))");
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in instrRegex: " << e.what() << std::endl;
            return events;
        }
        auto searchStart = patternImpl.cbegin();
        std::smatch instrMatches;
        
        while (std::regex_search(searchStart, patternImpl.cend(), instrMatches, instrRegex)) {
            std::string instrName = instrMatches[1].str();
            std::string beatStr = instrMatches[2].str();
            
            // Find the MIDI note for this instrument
            if (drumMap.find(instrName) != drumMap.end()) {
                int midiNote = drumMap.at(instrName);
                
                // Split beat positions by comma
                std::vector<std::string> beatPositions;
                std::istringstream beatStream(beatStr);
                std::string pos;
                
                // Split by commas
                while (std::getline(beatStream, pos, ',')) {
                    beatPositions.push_back(pos);
                }
                
                // Process each beat position
                for (const auto& position : beatPositions) {
                    // Check if it has a '+' (eighth note offset)
                    bool hasPlus = position.find('+') != std::string::npos;
                    
                    // Extract the beat number
                    std::string beatNumStr = position;
                    if (hasPlus) {
                        beatNumStr = position.substr(0, position.find('+'));
                    }
                    
                    // Convert to int
                    int beatNum = std::stoi(beatNumStr);
                    
                    // Calculate time in MIDI ticks
                    int time = startTime + (beatNum - 1) * ppq;  // Convert 1-based to 0-based
                    if (hasPlus) {
                        time += ppq / 2;  // Add eighth note offset
                    }
                    
                    // Add note-on event
                    events.push_back({
                        {"type", "note_on"},
                        {"channel", 9},  // Drum channel
                        {"note", midiNote},
                        {"velocity", 100},
                        {"time", time}
                    });
                    
                    // Add note-off event (short duration for drums)
                    events.push_back({
                        {"type", "note_off"},
                        {"channel", 9},
                        {"note", midiNote},
                        {"time", time + 10}  // Short duration
                    });
                }
            }
            
            searchStart = instrMatches.suffix().first;
        }
        
        return events;
    }
};
}