#pragma once

#include <JuceHeader.h>
#include <string>
#include <regex>
#include <sstream>
#include <map>
#include <vector>
#include <utility>
#include <algorithm>
#include <numeric>
#include "MusicalShorthand.h"
#include "PatternTemplates.h"

namespace covertracks {

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
     * @param parameters Parameters for template variables as JUCE var/JSON
     * @param style The musical style (default: "jazz")
     * @return std::string The prompt for the LLM
     */
    static std::string createPatternPrompt(
        const std::string& instrumentType,
        const var& parameters,
        const std::string& style = "jazz")
    {
        // Simple implementation that returns a template string
        String prompt = "Generate a " + String(instrumentType) + " pattern in " + String(style) + " style.";
        
        if (parameters.isObject()) {
            if (parameters.hasProperty("key"))
                prompt += "\nKey: " + parameters["key"].toString();
                
            if (parameters.hasProperty("tempo"))
                prompt += "\nTempo: " + parameters["tempo"].toString() + " BPM";
                
            if (parameters.hasProperty("chords"))
                prompt += "\nChords: " + parameters["chords"].toString();
        }
        
        prompt += "\n\nReturn a valid JSON object with MIDI events.";
        return prompt.toStdString();
    }
    
    /**
     * @brief Parse pattern-based notation to MIDI events
     * 
     * @param patternText The pattern-based notation
     * @param instrumentType The instrument type
     * @param playerId The player ID
     * @param patternId The pattern ID
     * @param tempo The tempo in BPM
     * @return var JSON object for the player layer as JUCE var/JSON
     */
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
            std::regex codeBlockRegex(R"(```[a-zA-Z]*\s+([\s\S]*)\s*```)");
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
        
        // Normalize spaces
        result = std::regex_replace(result, std::regex("\\s+"), " ");
        
        // Trim leading and trailing whitespace
        result = std::regex_replace(result, std::regex("^\\s+|\\s+$"), "");
        
        return result;
    }
    
    static var patternToJson(
        const std::string& patternText,
        const std::string& instrumentType,
        const std::string& playerId,
        const std::string& patternId,
        double tempo = 120.0)
    {
        DynamicObject* outputObj = new DynamicObject();
        try {
            // Clean up the input text
            std::string cleanPattern = cleanupPatternText(patternText);

            // Parse the pattern to MIDI events
            var events = parsePatternToEvents(cleanPattern, instrumentType);

            // Create JSON output for the player layer
            outputObj->setProperty("command", "play_pattern");
            outputObj->setProperty("player_id", String(playerId));
            outputObj->setProperty("pattern_id", String(patternId));
            outputObj->setProperty("tempo", tempo);
            outputObj->setProperty("events", events);
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in PatternProcessor::patternToJson: " << e.what() << std::endl;
            // Fallback: empty events
            outputObj->setProperty("command", "play_pattern");
            outputObj->setProperty("player_id", String(playerId));
            outputObj->setProperty("pattern_id", String(patternId));
            outputObj->setProperty("tempo", tempo);
            outputObj->setProperty("events", var(Array<var>()));
        } catch (const std::exception& e) { // Catch other potential exceptions
            juce::Logger::writeToLog("EXCEPTION in PatternProcessor::patternToJson: " + juce::String(e.what()));
            outputObj->setProperty("command", "play_pattern");
            outputObj->setProperty("player_id", String(playerId));
            outputObj->setProperty("pattern_id", String(patternId));
            outputObj->setProperty("tempo", tempo);
            outputObj->setProperty("events", var(Array<var>()));
        }
        return var(outputObj);
    }
    
    /**
     * @brief Parse pattern-based notation to MIDI events
     * 
     * @param patternText The pattern-based notation
     * @param instrumentType The instrument type
     * @param ppq Pulses per quarter note (default: 480)
     * @return var MIDI events as JUCE var/JSON
     */
    static var parsePatternToEvents(
        const std::string& patternText,
        const std::string& instrumentType,
        int ppq = 480)
    {
        var events = var(Array<var>());
        
        // Extract measures from the pattern text
        std::vector<std::string> measures = extractMeasures(patternText);
        
        // Process based on instrument type
        if (instrumentType == "piano" || instrumentType == "guitar" || instrumentType == "bass") {
            // For melodic instruments, use MusicalShorthand to parse
            events = MusicalShorthand::parseNoteSequence(patternText, ppq);
        } else if (instrumentType == "drums") {
            // For drums, use drum-specific parsing
            events = parseDrumPattern(patternText, ppq);
        }
        
        // If no events were generated by the instrument-specific parsers,
        // we'll try a generic approach
        if (events.getArray()->isEmpty()) {
            // Process each measure
            int currentTime = 0;
            const int ticksPerMeasure = ppq * 4; // Assuming 4/4 time
            
            for (const auto& measure : measures) {
                // For now, just use MusicalShorthand to parse any notes in the measure
                var measureEvents = MusicalShorthand::parseNoteSequence(measure, ppq);
                
                // Adjust the timing of events to account for measure position
                if (!measureEvents.getArray()->isEmpty()) {
                    for (int i = 0; i < measureEvents.getArray()->size(); ++i) {
                        var& event = measureEvents.getArray()->getReference(i);
                        if (event.hasProperty("time")) {
                            int time = event["time"];
                            event.getDynamicObject()->setProperty("time", time + currentTime);
                        }
                    }
                    
                    // Add the measure events to the overall events
                    for (int i = 0; i < measureEvents.getArray()->size(); ++i) {
                        events.append(measureEvents.getArray()->getReference(i));
                    }
                }
                
                // Move to the next measure
                currentTime += ticksPerMeasure;
            }
        }
        
        return events;
    }
    
    /**
     * @brief Extract measures from pattern text
     * 
     * @param patternText The pattern-based notation
     * @return std::vector<std::string> Vector of measures
     */
    static std::vector<std::string> extractMeasures(const std::string& patternText) {
        std::vector<std::string> measures;
        
        try {
            // Split by bar lines or newlines
            std::regex measureRegex(R"([|\n]+)");
            std::sregex_token_iterator iter(patternText.begin(), patternText.end(), measureRegex, -1);
            std::sregex_token_iterator end;
            
            while (iter != end) {
                std::string measure = *iter++;
                if (!measure.empty()) {
                    // Trim whitespace
                    measure = std::regex_replace(measure, std::regex("^\\s+|\\s+$"), "");
                    if (!measure.empty()) {
                        measures.push_back(measure);
                    }
                }
            }
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in extractMeasures: " << e.what() << std::endl;
            // Fallback: treat the whole pattern as one measure
            measures.push_back(patternText);
        }
        
        return measures;
    }
    
    /**
     * @brief Parse drum pattern to MIDI events
     * 
     * @param patternText The drum pattern notation
     * @param ppq Pulses per quarter note
     * @return var MIDI events as JUCE var/JSON
     */
    static var parseDrumPattern(const std::string& patternText, int ppq = 480) {
        var events = var(Array<var>());
        
        // Define drum mappings (GM standard)
        std::map<std::string, int> drumMap = {
            {"bd", 36},  // Bass drum
            {"sn", 38},  // Snare
            {"hh", 42},  // Closed hi-hat
            {"ho", 46},  // Open hi-hat
            {"cy", 49},  // Crash cymbal
            {"rd", 51},  // Ride cymbal
            {"tm", 45},  // Tom
            {"lt", 41},  // Low tom
            {"mt", 47},  // Mid tom
            {"ht", 50}   // High tom
        };
        
        try {
            // Split into lines for different drum parts
            std::vector<std::string> lines;
            std::stringstream ss(patternText);
            std::string line;
            
            while (std::getline(ss, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            
            // Process each line
            for (const auto& line : lines) {
                // Extract drum type and pattern
                std::regex drumLineRegex(R"(([a-zA-Z]+)\s*[:\-]\s*([xXoO\-\s]+))");
                std::smatch drumLineMatch;
                
                if (std::regex_search(line, drumLineMatch, drumLineRegex) && drumLineMatch.size() > 2) {
                    std::string drumType = drumLineMatch[1].str();
                    std::string pattern = drumLineMatch[2].str();
                    
                    // Convert to lowercase
                    std::transform(drumType.begin(), drumType.end(), drumType.begin(), ::tolower);
                    
                    // Find drum note in map
                    int note = -1;
                    if (drumMap.find(drumType) != drumMap.end()) {
                        note = drumMap[drumType];
                    } else {
                        // Try to find a partial match
                        for (const auto& pair : drumMap) {
                            if (drumType.find(pair.first) != std::string::npos) {
                                note = pair.second;
                                break;
                            }
                        }
                    }
                    
                    if (note != -1) {
                        // Process the pattern
                        // Remove all whitespace
                        pattern = std::regex_replace(pattern, std::regex("\\s+"), "");
                        
                        // Calculate ticks per step
                        int steps = pattern.length();
                        int ticksPerStep = (ppq * 4) / steps; // Assuming 4/4 time
                        
                        // Create events for each hit
                        for (int i = 0; i < steps; i++) {
                            char c = pattern[i];
                            if (c == 'x' || c == 'X' || c == 'o' || c == 'O') {
                                // Determine velocity based on character
                                int velocity = (c == 'x' || c == 'X') ? 100 : 70;
                                
                                // Add note-on event
                                DynamicObject* noteOnObj = new DynamicObject();
                                noteOnObj->setProperty("type", "note_on");
                                noteOnObj->setProperty("channel", 9); // Drum channel (10-1)
                                noteOnObj->setProperty("note", note);
                                noteOnObj->setProperty("velocity", velocity);
                                noteOnObj->setProperty("time", i * ticksPerStep);
                                events.append(var(noteOnObj));
                                
                                // Add note-off event (short duration)
                                DynamicObject* noteOffObj = new DynamicObject();
                                noteOffObj->setProperty("type", "note_off");
                                noteOffObj->setProperty("channel", 9); // Drum channel (10-1)
                                noteOffObj->setProperty("note", note);
                                noteOffObj->setProperty("time", i * ticksPerStep + ticksPerStep/2);
                                events.append(var(noteOffObj));
                            }
                        }
                    }
                }
            }
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR in parseDrumPattern: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("EXCEPTION in parseDrumPattern: " + juce::String(e.what()));
        }
        
        return events;
    }
    
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
    static std::string fillTemplateVariables(const std::string& templateText, const var& parameters) {
        std::string result = templateText;
        
        // Replace {{variable}} with value from parameters
        std::regex varRegex(R"(\{\{(\w+)\}\})");
        std::string::const_iterator searchStart = result.cbegin();
        std::smatch matches;
        
        while (std::regex_search(searchStart, result.cend(), matches, varRegex)) {
            std::string varName = matches[1].str();
            std::string replacement;
            
            // Extract value from parameters
            if (parameters.isObject() && parameters.hasProperty(varName)) {
                replacement = parameters[varName].toString().toStdString();
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
                    info.chords.append(chord);
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
                    info.chords.append(chord);
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
        info.chords.append("C");
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
     * @return var MIDI events for the pattern
     */
    static var generateEventsForPattern(
        const PatternInfo& patternInfo,
        const std::string& instrumentType,
        int startTime,
        int ppq)
    {
        var events = var(Array<var>());
        
        // Special handling for drums
        if (instrumentType == "drums") {
            return generateDrumEvents(patternInfo.patternName, startTime, ppq);
        }
        
        // If no chord information available, add defaults
        std::vector<std::string> chords = patternInfo.chords;
        if (chords.empty()) {
            chords.append("C");
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
                usedChords.append(usedChords.back());
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
                            notes.append(chordNotes[0]); // Root note
                        }
                    }
                    else if (chordComponent == "3RD") {
                        // Just the third
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (chordNotes.size() > 1) {
                            notes.append(chordNotes[1]); // Third
                        }
                    }
                    else if (chordComponent == "5TH") {
                        // Just the fifth
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (chordNotes.size() > 2) {
                            notes.append(chordNotes[2]); // Fifth
                        }
                    }
                    else if (chordComponent == "7TH") {
                        // Just the seventh if available
                        auto chordNotes = MusicalShorthand::parseChord(chordName, instrumentType == "bass" ? 2 : 4);
                        if (chordNotes.size() > 3) {
                            notes.append(chordNotes[3]); // Seventh
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
                                        notes.append(note);
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
                        events.append({
                            {"type", "note_on"},
                            {"channel", 0},
                            {"note", note},
                            {"velocity", 100},
                            {"time", startTime + currentOffset}
                        });
                        
                        // Add note-off event
                        events.append({
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
                
                elements.append(std::make_pair(elementType, rhythmValue));
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
     * @return var MIDI events for the drum pattern
     */
    static var generateDrumEvents(
        const std::string& patternName,
        int startTime,
        int ppq)
    {
        var events = var(Array<var>());
        
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
                    beatPositions.append(pos);
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
                    events.append({
                        {"type", "note_on"},
                        {"channel", 9},  // Drum channel
                        {"note", midiNote},
                        {"velocity", 100},
                        {"time", time}
                    });
                    
                    // Add note-off event (short duration for drums)
                    events.append({
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