#pragma once

#include <JuceHeader.h>
#include <nlohmann/json.hpp>
#include <string>
#include <regex>
#include "MusicalShorthand.h"
#include "PatternProcessor.h"

using json = nlohmann::json;

namespace covertracks {

/**
 * @brief Processor for handling pattern-based musical notation conversion
 * 
 * This class provides a simplified interface for working with pattern-based
 * musical notation, acting as a bridge between the LLM output and the Player layer.
 */
class ShorthandProcessor {
public:
    /**
     * @brief Convert shorthand pattern notation to JSON format for the Player layer
     * 
     * @param patternText The shorthand notation text
     * @param instrumentType The instrument type (e.g., "piano", "bass")
     * @param playerId The player ID
     * @param patternId The pattern ID
     * @param tempo The tempo in BPM
     * @param expectedMeasures Expected number of measures (0 = don't validate)
     * @return json JSON object for the Player layer
     */
    static json shorthandToJson(
        const std::string& patternText,
        const std::string& instrumentType,
        const std::string& playerId,
        const std::string& patternId,
        double tempo = 120.0,
        int expectedMeasures = 0)
    {
        // Clean up the input to handle potential LLM formatting issues
        std::string cleanPattern = cleanupPatternText(patternText);

        // Log before forwarding to PatternProcessor
        std::cout << "--- ShorthandProcessor::shorthandToJson ---" << std::endl;
        std::cout << "Original patternText:" << std::endl << patternText << std::endl;
        std::cout << "Cleaned pattern:" << std::endl << cleanPattern << std::endl;
        std::cout << "Instrument: " << instrumentType << ", Player: " << playerId << ", Pattern: " << patternId << std::endl;
        
        // Extract measures to validate count before processing
        auto measures = PatternProcessor::extractMeasures(cleanPattern);
        
        // Only validate if expectedMeasures is provided (> 0)
        if (expectedMeasures > 0) {
            validateMeasureCount(measures, expectedMeasures);
        }
        
        // Forward to PatternProcessor
        json events = PatternProcessor::parsePatternToEvents(cleanPattern, instrumentType);
        
        std::cout << "Generated events:" << std::endl;
        std::cout << events.dump(2) << std::endl;
        std::cout << "------------------------------------" << std::endl;
        
        // Create JSON output for the player layer
        json outputJson = {
            {"command", "play_pattern"},
            {"player_id", playerId},
            {"pattern_id", patternId},
            {"tempo", tempo},
            {"events", events}
        };
        
        return outputJson;
    }
    
    /**
     * @brief Create a shorthand prompt for the LLM
     * 
     * @param instrumentType The instrument type (e.g., "piano", "bass")
     * @param parameters Parameters for the template (key, tempo, etc.)
     * @param style The style of music (e.g., "jazz", "bossa", "swing")
     * @return std::string The prompt to send to the LLM
     */
    static std::string createShorthandPrompt(
        const std::string& instrumentType,
        const json& parameters,
        const std::string& style = "default")
    {
        // Forward to the PromptManager
        return PromptManager::createPrompt(instrumentType, parameters, style);
    }

private:
    /**
     * @brief Validate that the pattern has the expected number of measures
     * 
     * @param measures Vector of extracted measures
     * @param expectedCount Expected number of measures
     * @return bool True if the count matches, false otherwise
     */
    static bool validateMeasureCount(const std::vector<std::string>& measures, int expectedCount) {
        if (measures.size() != expectedCount) {
            std::cerr << "WARNING: Expected " << expectedCount << " measures, but extracted " 
                     << measures.size() << " measures instead." << std::endl;
                     
            // Log the first few and last few measures for debugging
            std::cout << "First few measures:" << std::endl;
            for (size_t i = 0; i < std::min(measures.size(), size_t(3)); i++) {
                std::cout << "[" << i << "]: " << measures[i] << std::endl;
            }
            
            if (measures.size() > 6) {
                std::cout << "..." << std::endl;
                std::cout << "Last few measures:" << std::endl;
                for (size_t i = measures.size() - 3; i < measures.size(); i++) {
                    std::cout << "[" << i << "]: " << measures[i] << std::endl;
                }
            }
            
            return false;
        }
        
        return true;
    }

    /**
     * @brief Clean up pattern text from LLM output
     * 
     * @param patternText Raw pattern text from LLM
     * @return std::string Cleaned pattern text
     */
    static std::string cleanupPatternText(const std::string& patternText) {
        std::string result = patternText;
        
        // Handle code blocks
        try {
             std::regex codeBlockRegex(R"(```[a-zA-Z]*\s+([\s\S]*)\s*```)");
             std::smatch codeBlockMatch;
             if (std::regex_search(result, codeBlockMatch, codeBlockRegex) && codeBlockMatch.size() > 1) {
                result = codeBlockMatch[1].str();
             }
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR during codeBlockRegex: " << e.what() << std::endl;
        }
        
        // Replace escaped newlines with spaces
        try {
            result = std::regex_replace(result, std::regex("\\\\n"), " ");
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR during newline escape replacement: " << e.what() << std::endl;
        }
        
        // Replace actual newlines with spaces
        try {
            result = std::regex_replace(result, std::regex("\n"), " ");
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR during newline replacement: " << e.what() << std::endl;
        }
        
        // Normalize all pipe separators
        try {
            // Replace multiple spaces with a single space
            result = std::regex_replace(result, std::regex("\\s+"), " ");
            
            // Ensure clean spacing around pipe characters
            result = std::regex_replace(result, std::regex("\\s*\\|\\s*"), "|");
        } catch (const std::regex_error& e) {
            std::cerr << "REGEX_ERROR during pipe separator normalization: " << e.what() << std::endl;
        }
        
        // Clean up the pattern by ensuring proper pipe enclosures
        std::string cleanedResult;
        bool insideMeasure = false;
        std::string currentMeasure;
        
        for (char c : result) {
            if (c == '|') {
                if (insideMeasure) {
                    // End of measure found
                    if (!currentMeasure.empty()) {
                        cleanedResult += "|" + currentMeasure + "|";
                    }
                    currentMeasure.clear();
                } else {
                    // Start of measure found
                    insideMeasure = true;
                }
            } else if (insideMeasure) {
                currentMeasure += c;
            }
        }
        
        // Handle case where the last measure might not be closed
        if (insideMeasure && !currentMeasure.empty()) {
            cleanedResult += "|" + currentMeasure + "|";
        }
        
        // If nothing was processed, return the original (but with normalized whitespace)
        if (cleanedResult.empty()) {
            return result;
        }
        
        return cleanedResult;
    }
};

} // namespace covertracks