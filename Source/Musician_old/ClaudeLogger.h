#pragma once

#include <JuceHeader.h>
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace coverband {

/**
 * @brief Simple logger for Claude interactions
 * 
 * This class provides logging functions for debugging Claude API requests and responses.
 */
class ClaudeLogger {
public:
    /**
     * @brief Initialize the logger
     * 
     * @param logDir Directory to store log files
     */
    static void initialize(const juce::String& logDir = "logs") {
        // Create logs directory if it doesn't exist
        juce::File logsDirectory(logDir);
        if (!logsDirectory.exists()) {
            logsDirectory.createDirectory();
        }
        
        // Store the log directory
        logDirectory = logDir;
        
        // Create a timestamp for this session
        sessionTimestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        
        // Write initialization message
        std::cout << "Claude logger initialized. Logs will be stored in " << logDir << std::endl;
    }
    
    /**
     * @brief Log a prompt sent to Claude
     * 
     * @param instrumentType The instrument type
     * @param prompt The prompt text
     */
    static void logPrompt(const std::string& instrumentType, const std::string& prompt) {
        if (logDirectory.isEmpty()) {
            initialize(); // Auto-initialize if not done already
        }
        
        // Create filename
        juce::String filename = logDirectory + "/claude_prompt_" + sessionTimestamp + "_" + instrumentType + ".txt";
        
        // Log to file
        std::ofstream file(filename.toStdString(), std::ios::app);
        if (file.is_open()) {
            file << "=== PROMPT SENT TO CLAUDE AT " << juce::Time::getCurrentTime().formatted("%H:%M:%S") << " ===" << std::endl;
            file << prompt << std::endl;
            file << "=== END PROMPT ===" << std::endl << std::endl;
            file.close();
        }
        
        // Log to console
        std::cout << "Claude prompt for " << instrumentType << " logged to " << filename << std::endl;
    }
    
    /**
     * @brief Log a response received from Claude
     * 
     * @param instrumentType The instrument type
     * @param response The response JSON
     */
    static void logResponse(const std::string& instrumentType, const json& response) {
        if (logDirectory.isEmpty()) {
            initialize(); // Auto-initialize if not done already
        }
        
        // Create filename
        juce::String filename = logDirectory + "/claude_response_" + sessionTimestamp + "_" + instrumentType + ".json";
        
        // Log to file
        std::ofstream file(filename.toStdString(), std::ios::app);
        if (file.is_open()) {
            file << "=== RESPONSE RECEIVED FROM CLAUDE AT " << juce::Time::getCurrentTime().formatted("%H:%M:%S") << " ===" << std::endl;
            file << response.dump(2) << std::endl;
            file << "=== END RESPONSE ===" << std::endl << std::endl;
            file.close();
        }
        
        // Log to console
        std::cout << "Claude response for " << instrumentType << " logged to " << filename << std::endl;
    }
    
    /**
     * @brief Log the pattern generated for the player
     * 
     * @param instrumentType The instrument type
     * @param pattern The pattern JSON
     */
    static void logPattern(const std::string& instrumentType, const json& pattern) {
        if (logDirectory.isEmpty()) {
            initialize(); // Auto-initialize if not done already
        }
        
        // Create filename
        juce::String filename = logDirectory + "/claude_pattern_" + sessionTimestamp + "_" + instrumentType + ".json";
        
        // Log to file
        std::ofstream file(filename.toStdString(), std::ios::app);
        if (file.is_open()) {
            file << "=== PATTERN GENERATED AT " << juce::Time::getCurrentTime().formatted("%H:%M:%S") << " ===" << std::endl;
            file << pattern.dump(2) << std::endl;
            file << "=== END PATTERN ===" << std::endl << std::endl;
            file.close();
        }
        
        // Log to console
        std::cout << "Claude pattern for " << instrumentType << " logged to " << filename << std::endl;
    }
    
private:
    static juce::String logDirectory;
    static juce::String sessionTimestamp;
};


} // namespace coverband