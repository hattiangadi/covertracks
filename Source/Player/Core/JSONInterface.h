#pragma once

#include <JuceHeader.h>
#include <string>

namespace covertracks {

/**
 * @brief Interface for JSON communication between components
 * 
 * This class provides a standard interface for components to exchange
 * data using JUCE's JSON functionality.
 */
class JSONInterface {
public:
    virtual ~JSONInterface() = default;
    
    /**
     * @brief Process a JSON command string
     * 
     * @param commandString JSON command as a string
     * @return std::string Response as a JSON string
     */
    virtual std::string processJSONCommand(const std::string& commandString) = 0;
    
    /**
     * @brief Process a JSON command
     * 
     * @param command Command as JUCE var/JSON
     * @return var Response as JUCE var/JSON
     */
    virtual var processCommand(const var& command) = 0;
};

} // namespace covertracks
