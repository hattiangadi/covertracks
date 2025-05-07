#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

using json = nlohmann::json;

namespace coverband {

/**
 * @brief Central manager for all prompt templates
 * 
 * This class provides a single source of truth for all prompt templates
 * used in the CoverBand application.
 */
class PromptManager {
public:
    /**
     * @brief Get a prompt template for an instrument and style
     * 
     * @param instrumentType The instrument type (e.g., "piano", "bass")
     * @param style The musical style (e.g., "jazz", "rock")
     * @return std::string The prompt template
     */
    static std::string getPromptTemplate(
        const std::string& instrumentType, 
        const std::string& style = "default")
    {
        // Get the base template map
        const auto& templateMap = getTemplateMap(instrumentType);
        
        // Look for the specific style
        if (templateMap.find(style) != templateMap.end()) {
            return templateMap.at(style);
        }
        
        // Fall back to default style if the requested style doesn't exist
        if (templateMap.find("default") != templateMap.end()) {
            return templateMap.at("default");
        }
        
        // Final fallback to a generic template
        return getGenericTemplate();
    }
    
    /**
     * @brief Fill template variables with values from parameters
     * 
     * @param templateText The template text with variables like {{key}}
     * @param parameters JSON object with variable values
     * @param instrumentType The instrument type
     * @return std::string The filled template
     */
    static std::string fillTemplateVariables(
        const std::string& templateText, 
        const json& parameters, 
        const std::string& instrumentType) 
    {
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
                } else if (parameters[varName].is_number()) {
                    replacement = std::to_string(parameters[varName].get<double>());
                } else {
                    replacement = parameters[varName].dump();
                }
            } else {
                // Default values for common parameters
                if (varName == "tempo") replacement = "120";
                else if (varName == "key") replacement = "C";
                else if (varName == "instrument") replacement = instrumentType;
                else if (varName == "style") replacement = "jazz";
                else if (varName == "bars") replacement = "4";
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
     * @brief Create a complete prompt for the LLM
     * 
     * @param instrumentType The instrument type
     * @param parameters Parameters for the template
     * @param style The musical style
     * @return std::string The complete prompt
     */
    /**
 * @brief Create a complete prompt for the LLM
 * 
 * @param instrumentType The instrument type
 * @param parameters Parameters for the template
 * @param style The musical style
 * @return std::string The complete prompt
 */
static std::string createPrompt(
    const std::string& instrumentType,
    const json& parameters,
    const std::string& style = "default")
{
    // Get the template
    std::string templateText = getPromptTemplate(instrumentType, style);
    
    // Create a copy of parameters to enhance with length calculations
    json enhancedParams = parameters;
    enhancedParams["instrument"] = instrumentType;

    // Calculate bar and beat information if not already provided
    int bars = 0;
    if (parameters.contains("totalBars") && parameters["totalBars"].is_number()) {
        bars = parameters["totalBars"].get<int>();
        enhancedParams["bars"] = bars;
        if (!parameters.contains("beats")) {
            enhancedParams["beats"] = bars * 4; // Assuming 4/4 time signature
        }
    } else if (parameters.contains("bars") && parameters["bars"].is_number()) {
        bars = parameters["bars"].get<int>();
        enhancedParams["totalBars"] = bars; // Ensure totalBars is set for consistency
        if (!parameters.contains("beats")) {
            enhancedParams["beats"] = bars * 4; // Assuming 4/4 time signature
        }
    } else if (parameters.contains("beats") && parameters["beats"].is_number()) {
        int beats = parameters["beats"].get<int>();
        bars = (beats + 3) / 4; // Round up to nearest bar, assuming 4/4
        enhancedParams["bars"] = bars;
        enhancedParams["totalBars"] = bars; // Ensure totalBars is set for consistency
        // Keep original beats if specified, don't overwrite with bars*4
        if (!enhancedParams.contains("beats")) enhancedParams["beats"] = beats;
    } else {
        // Default to 4 bars if neither is specified
        bars = 4;
        enhancedParams["bars"] = bars;
        enhancedParams["totalBars"] = bars; // Ensure totalBars is set for consistency
        enhancedParams["beats"] = 16;
    }
    
    // Fill in template variables
    std::string prompt = fillTemplateVariables(templateText, enhancedParams, instrumentType);
    
    // Add song context information if available
    if (parameters.contains("title")) {
        std::ostringstream songContext;
        songContext << "\n\n### Song Information ###\n";
        songContext << "Title: " << parameters["title"].get<std::string>() << "\n";
        
        if (parameters.contains("songStructure")) {
            songContext << "Structure: " << parameters["songStructure"].get<std::string>();
            
            // Add bar count details if available
            if (parameters.contains("sectionLengths") && parameters["sectionLengths"].is_array()) {
                songContext << " (";
                size_t totalSections = parameters["songStructure"].get<std::string>().length();
                songContext << totalSections << " sections, ";
                
                // Calculate bars per section if possible
                if (parameters["sectionLengths"].size() > 0) {
                    int barsPerSection = parameters["sectionLengths"][0];
                    songContext << barsPerSection << " bars each, ";
                }
                
                songContext << parameters.value("totalBars", bars) << " bars total)";
            }
            songContext << "\n";
        }
        
        if (parameters.contains("feel")) {
            songContext << "Feel: " << parameters["feel"].get<std::string>();
            
            if (parameters.contains("swingAmount") && parameters["swingAmount"].is_number()) {
                double swing = parameters["swingAmount"].get<double>();
                songContext << " (Swing amount: " << swing << ")\n";
            } else {
                songContext << "\n";
            }
        }
        
        prompt += songContext.str();
    }
    
    // Add melody and chord progression context
    if (parameters.contains("melody_notes")) {
        // Check if we have section markers
        if (parameters.contains("songStructure") && parameters.contains("sectionLengths")) {
            prompt += "\n### Melody with Section Markers ###\n";
            
            // Add section markers if not already present in melody
            std::string melody = parameters["melody_notes"].get<std::string>();
            if (melody.find('[') == std::string::npos) {
                std::string structure = parameters["songStructure"].get<std::string>();
                std::vector<int> sectionLengths;
                
                // Extract section lengths
                if (parameters["sectionLengths"].is_array()) {
                    for (const auto& length : parameters["sectionLengths"]) {
                        sectionLengths.push_back(length);
                    }
                } else {
                    // If sectionLengths isn't an array, use default values
                    sectionLengths.resize(structure.length(), 2); // Default 2 bars per section
                }
                
                // Split melody into bars
                std::vector<std::string> bars;
                std::stringstream ss(melody);
                std::string bar;
                while (std::getline(ss, bar, '|')) {
                    if (!bar.empty()) {
                        bars.push_back(bar);
                    }
                }
                
                // Add section markers to melody
                std::string markedMelody;
                int barIndex = 0;
                for (size_t i = 0; i < structure.length() && barIndex < bars.size(); i++) {
                    markedMelody += "[" + std::string(1, structure[i]) + "] ";
                    
                    for (int j = 0; j < sectionLengths[i % sectionLengths.size()] && barIndex < bars.size(); j++) {
                        markedMelody += bars[barIndex] + " | ";
                        barIndex++;
                    }
                    
                    markedMelody += "\n";
                }
                
                prompt += markedMelody;
            } else {
                // If melody already has section markers, use as is
                prompt += melody + "\n";
            }
        } else {
            // No structure/section info, just add melody as is
            prompt += "\n### Melody ###\n" + parameters["melody_notes"].get<std::string>() + "\n";
        }
    }
    
    if (parameters.contains("chord_progression")) {
        // Check if we have section markers
        if (parameters.contains("songStructure") && parameters.contains("sectionLengths")) {
            prompt += "\n### Chord Progression with Section Markers and Bar Numbers ###\n";
            
            // Add section markers if not already present in chord progression
            std::string progression = parameters["chord_progression"].get<std::string>();
            if (progression.find('[') == std::string::npos) {
                std::string structure = parameters["songStructure"].get<std::string>();
                std::vector<int> sectionLengths;
                
                // Extract section lengths
                if (parameters["sectionLengths"].is_array()) {
                    for (const auto& length : parameters["sectionLengths"]) {
                        sectionLengths.push_back(length);
                    }
                } else {
                    // If sectionLengths isn't an array, use default values
                    sectionLengths.resize(structure.length(), 2); // Default 2 bars per section
                }
                
                // Split progression into bars/chords
                std::vector<std::string> chords;
                std::stringstream ss(progression);
                std::string chord;
                while (std::getline(ss, chord, '|')) {
                    if (!chord.empty()) {
                        chords.push_back(chord);
                    }
                }
                
                // Add section markers to chord progression
                std::string markedProgression;
                int barIndex = 0;
                int currentBar = 1;
                
                for (size_t i = 0; i < structure.length() && barIndex < chords.size(); i++) {
                    markedProgression += "[" + std::string(1, structure[i]) + "] ";
                    
                    for (int j = 0; j < sectionLengths[i % sectionLengths.size()] && barIndex < chords.size(); j++) {
                        markedProgression += "Bar " + std::to_string(currentBar) + ": " + chords[barIndex] + " | ";
                        barIndex++;
                        currentBar++;
                    }
                    
                    markedProgression += "\n";
                }
                
                prompt += markedProgression;
            } else {
                // If progression already has section markers, use as is
                prompt += progression + "\n";
            }
        } else {
            // No structure/section info, just add chord progression as is
            prompt += "\n### Chord Progression ###\n" + parameters["chord_progression"].get<std::string>() + "\n";
        }
    }
    
    // Add instrument-specific styling instructions
    if (parameters.contains("styleInstructions") && 
        parameters["styleInstructions"].is_object() &&
        parameters["styleInstructions"].contains(instrumentType)) {
        
        prompt += "\n### Performance Instructions ###\n" + 
                  parameters["styleInstructions"][instrumentType].get<std::string>() + "\n";
    }
    
    // Add reminder about expected format and bar count
    if (parameters.contains("totalBars")) {
        prompt += "\n\nYour response should include EXACTLY " + 
                 std::to_string(parameters["totalBars"].get<int>()) + 
                 " measures, one for each bar of the song.\n";
    }
    
    // Add reminder about using different patterns for different sections
    if (parameters.contains("songStructure")) {
        prompt += "Use different patterns for different sections to create musical interest.\n";
    }
    
    // Add reminder about section transitions
    prompt += "Add appropriate transitions between sections.\n";
    
    // Add final reminder to respond only with pattern format
    prompt += "\nRespond ONLY with the bar-based pattern format as shown in the examples. NO explanations or additional text.";
    
    return prompt;
}

private:
    // Generic template used as final fallback
    static std::string getGenericTemplate() {
        return R"(
Generate a musical pattern for {{instrument}} in {{key}} at {{tempo}} BPM.

The pattern should be formatted using bar-based notation with each measure
enclosed in | characters and specifying both the pattern and chords:

|pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|

Create a pattern lasting exactly {{bars}} bars.
)";
    }
    
    // Get the template map for a specific instrument
    static const std::map<std::string, std::string>& getTemplateMap(const std::string& instrumentType)
    {
        static const std::map<std::string, std::map<std::string, std::string>> templates = {
            // Piano templates
            {"piano", {
                {"default", R"(
Generate a piano pattern in {{key}} at {{tempo}} BPM for "{{title}}".

The pattern should be formatted using bar-based notation with each measure
enclosed in | characters and specifying both the pattern and chords:

|pattern: whole, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|

Choose from these patterns for each measure (each represents a full measure):
- whole: One whole note chord (4 beats)
- half: Two half note chords (2 beats each)
- quarter: Four quarter note chords (1 beat each)
- syncopated: Quarter, half, quarter rhythm
- comping: Jazz comping with syncopated rhythm
- bossa: Bossa nova rhythm

Create a pattern lasting exactly {{bars}} bars that accompanies the melody and supports the chord progression.
)"},
{"jazz", R"(
    Generate a jazz piano pattern in {{key}} at {{tempo}} BPM for "{{title}}".
    
    Create a piano part that follows the {{songStructure}} structure ({{totalBars}} bars total) with:
    - Jazz chord voicings with extensions (7ths, 9ths, 13ths)
    - Subtle rhythmic comping in the left hand
    - Voice leading between chord changes
    - Syncopation and swing feel appropriate for jazz
    
    Format the pattern using bar-based notation with each measure
    enclosed in | characters and specifying both the pattern and chords:
    
    |pattern: comping, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
    
    Choose from these patterns for each measure (each represents a full measure):
    - whole: One whole note chord (4 beats)
    - half: Two half note chords (2 beats each)
    - quarter: Four quarter note chords (1 beat each)
    - syncopated: Quarter, half, quarter rhythm
    - comping: Jazz comping with syncopated rhythm
    - bossa: Bossa nova rhythm
    
    Create a cohesive pattern that spans exactly {{totalBars}} bars ({{totalBars}} measures).
    Vary your patterns based on the structure to create interest and development.
    
    Your response should include EXACTLY {{totalBars}} measures, one for each bar of the song.
    Use different patterns for different sections to create musical interest.
    Add appropriate transitions between sections.
    
    Respond ONLY with the bar-based pattern format as shown in the examples. NO explanations or additional text.
    )"}
            }},
            
            // Bass templates
            {"bass", {
                {"default", R"(
Generate a bass line pattern in {{key}} at {{tempo}} BPM for "{{title}}".

The bass line should provide a solid foundation with clear root notes 
and appropriate walking or groove patterns.

Format the pattern using bar-based notation with each measure
enclosed in | characters and specifying both the pattern and chords:

|pattern: walking, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|

Choose from these patterns for each measure (each represents a full measure):
- whole: One whole note (4 beats)
- half: Two half notes (2 beats each)
- quarter: Four quarter notes (1 beat each)
- walking: Walking bass with quarter notes (1-3-5-7 or similar)
- latin: Latin bass pattern with syncopation

Create a pattern lasting exactly {{bars}} bars that complements the melody and emphasizes the chord changes.
)"},
{"jazz", R"(
    Generate a jazz bass line in {{key}} at {{tempo}} BPM for "{{title}}".
    
    Create a bass part that follows the {{songStructure}} structure ({{totalBars}} bars total) with:
    - Quarter note walking patterns with occasional syncopation
    - Strong beats on 1 and 3, with emphasis on chord roots
    - Chromatic approach notes at transition points
    - Smooth voice leading between chords
    - Swing feel with appropriate timing
    
    Format the pattern using bar-based notation with each measure
    enclosed in | characters and specifying both the pattern and chords:
    
    |pattern: walking, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
    
    Choose from these patterns for each measure (each represents a full measure):
    - whole: One whole note (4 beats)
    - half: Two half notes (2 beats each)
    - quarter: Four quarter notes (1 beat each)
    - walking: Walking bass with quarter notes (1-3-5-7 or similar)
    - latin: Latin bass pattern with syncopation
    
    Create a cohesive pattern that spans exactly {{totalBars}} bars ({{totalBars}} measures).
    Vary your patterns based on the structure to create interest and development.
    
    Your response should include EXACTLY {{totalBars}} measures, one for each bar of the song.
    Use different patterns for different sections to create musical interest.
    Add appropriate transitions between sections.
    
    Respond ONLY with the bar-based pattern format as shown in the examples. NO explanations or additional text.
    )"}
            }},
            
            // Drums templates
            {"drums", {
                {"default", R"(
Generate a drum pattern at {{tempo}} BPM for "{{title}}".

Format the pattern using bar-based notation with each measure
enclosed in | characters and specifying the pattern type:

|pattern: basic| pattern: swing| pattern: fill|

Choose from these patterns for each measure (each represents a full measure):
- basic: Basic rock beat with kick on 1,3 and snare on 2,4
- swing: Jazz swing pattern with ride cymbal
- latin: Latin groove with hi-hat and cross-stick
- fill: Drum fill transitioning between sections

Create a pattern lasting exactly {{bars}} bars that supports the melody and enhances the feel of the song.
Add appropriate fills to mark section transitions and phrase endings.
)"},
{"jazz", R"(
    Generate a jazz drum pattern at {{tempo}} BPM for "{{title}}".
    
    Create a drum part that follows the {{songStructure}} structure ({{totalBars}} bars total) with:
    - Ride cymbal patterns with swing feel
    - Hi-hat on beats 2 and 4
    - Subtle kick drum accents
    - Light snare comping
    - Brushes technique for softer sections
    - Fills to mark phrase endings and section transitions
    
    Format the pattern using bar-based notation with each measure
    enclosed in | characters and specifying the pattern type:
    
    |pattern: swing| pattern: brushes| pattern: fill|
    
    Choose from these patterns for each measure (each represents a full measure):
    - swing: Jazz swing pattern with ride cymbal
    - brushes: Light brush work for softer sections
    - latin: Latin-influenced pattern with cross-stick
    - fill: Drum fill for transitions
    - basic: Simple time keeping with minimal embellishment
    - accented: Stronger dynamics with punctuated accents
    
    Create a cohesive pattern that spans exactly {{totalBars}} bars ({{totalBars}} measures).
    Vary your patterns based on the structure to create interest and development.
    
    Your response should include EXACTLY {{totalBars}} measures, one for each bar of the song.
    Use different patterns for different sections to create musical interest.
    Every section transition should have a fill in the last bar.
    
    Respond ONLY with the bar-based pattern format as shown in the examples. NO explanations or additional text.
    )"}
            }},
            
            // Guitar templates
            {"guitar", {
                {"default", R"(
Generate a guitar pattern in {{key}} at {{tempo}} BPM for "{{title}}".

Format the pattern using bar-based notation with each measure
enclosed in | characters and specifying both the pattern and chords:

|pattern: strumming, chord: Cmaj7| pattern: fingerpicking, chords: G7, Cmaj7|

Choose from these patterns for each measure (each represents a full measure):
- whole: One whole note chord (4 beats)
- half: Two half note chords (2 beats each)
- quarter: Four quarter note chords (1 beat each)
- strumming: Strumming pattern (down-up-down-up)
- fingerpicking: Fingerpicking arpeggio pattern
- comping: Jazz comping with syncopated rhythm

Create a pattern lasting exactly {{bars}} bars that complements the melody and supports the chord progression.
)"},
{"jazz", R"(
    Generate a jazz guitar pattern in {{key}} at {{tempo}} BPM for "{{title}}".
    
    Create a guitar part that follows the {{songStructure}} structure ({{totalBars}} bars total) with:
    - Jazz chord voicings with extensions (7ths, 9ths, 13ths)
    - Freddie Green style comping or modern jazz comping
    - Smooth voice leading between chord changes
    - Rhythmic syncopation and swing feel
    - Occasional melodic fills at phrase endings
    
    Format the pattern using bar-based notation with each measure
    enclosed in | characters and specifying both the pattern and chords:
    
    |pattern: comping, chord: Cmaj7| pattern: half, chords: G7, Cmaj7|
    
    Choose from these patterns for each measure (each represents a full measure):
    - whole: One whole note chord (4 beats)
    - half: Two half note chords (2 beats each)
    - quarter: Four quarter note chords (1 beat each)
    - comping: Jazz comping with syncopated rhythm
    - chord-melody: Blend of chords and melody notes
    - walking: Walking guitar line (for solo sections)
    - freddie: Freddie Green style quarter note comping
    - bossa: Bossa nova style with syncopated accents
    
    Create a cohesive pattern that spans exactly {{totalBars}} bars ({{totalBars}} measures).
    Vary your patterns based on the structure to create interest and development.
    
    Your response should include EXACTLY {{totalBars}} measures, one for each bar of the song.
    Use different patterns for different sections to create musical interest.
    Add appropriate transitions between sections.
    
    Respond ONLY with the bar-based pattern format as shown in the examples. NO explanations or additional text.
    )"}
            }}
        };
        
        // Find the instrument map
        if (templates.find(instrumentType) != templates.end()) {
            return templates.at(instrumentType);
        }
        
        // Return empty map if not found
        static const std::map<std::string, std::string> emptyMap;
        return emptyMap;
    }
};

} // namespace coverband