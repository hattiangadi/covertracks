#include "JSONInterface.h"

JSONInterface::JSONInterface()
{
    std::cout << "Creating JSONInterface" << std::endl;
}

JSONInterface::~JSONInterface()
{
    std::cout << "Destroying JSONInterface" << std::endl;
}

std::string JSONInterface::handleCommand(const std::string& jsonCommand)
{
    try {
        // Parse JSON
        nlohmann::json json = nlohmann::json::parse(jsonCommand);
        
        // Extract command
        std::string command = json.at("command");
        
        // Dispatch to appropriate handler
        if (command == "create_player") {
            return handleCreatePlayer(json);
        }
        else if (command == "play_pattern") {
            return handlePlayPattern(json);
        }
        else if (command == "stop") {
            return handleStopPlayer(json);
        }
        else if (command == "shutdown") {
            return handleShutdown(json);
        }
        else {
            return createErrorResponse("Unknown command: " + command);
        }
    }
    catch (const nlohmann::json::exception& e) {
        return createErrorResponse("JSON error: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        return createErrorResponse("Error: " + std::string(e.what()));
    }
}

void JSONInterface::setPatternFinishedCallback(
    std::function<void(const std::string&, const std::string&)> callback)
{
    std::cout << "[DEBUG] JSONInterface::setPatternFinishedCallback - Setting pattern finished callback" << std::endl;
    
    // Store the callback for future players
    patternFinishedCallback = callback;
    
    // For each existing player, set the callback
    std::vector<std::string> playerIds = controller.getPlayerIds();
    std::cout << "[DEBUG] Setting callback for " << playerIds.size() << " existing players" << std::endl;
    
    for (const auto& playerId : playerIds) {
        std::cout << "[DEBUG] Setting pattern finished callback for player: " << playerId << std::endl;
        controller.setPatternFinishedCallback(playerId, callback);
    }
    
    std::cout << "[DEBUG] Pattern finished callback setup complete" << std::endl;
}

std::string JSONInterface::handleCreatePlayer(const nlohmann::json& json)
{
    try {
        // Extract required parameters
        std::string playerId = json.at("player_id");
        
        // Extract or create config object
        nlohmann::json config;
        
        // If there's a config field, use it
        if (json.contains("config") && json["config"].is_object()) {
            config = json["config"];
        }
        
        // Copy essential fields from command to config
        for (const auto& key : {"instrument_type", "plugin_id", "sample_name"}) {
            if (json.contains(key)) {
                config[key] = json[key];
            }
        }
        
        // Ensure instrument_type is present
        if (!config.contains("instrument_type")) {
            return createErrorResponse("Missing required field: instrument_type");
        }
        
        // Create player
        std::string result = controller.createPlayer(playerId, config);
        
        // Check for error
        if (result.find("Error:") == 0) {
            return createErrorResponse(result);
        }
        
        // If we have a stored callback, set it for the new player
        if (patternFinishedCallback) {
            std::cout << "Setting pattern finished callback for newly created player: " << playerId << std::endl;
            controller.setPatternFinishedCallback(playerId, patternFinishedCallback);
        }
        
        return createSuccessResponse(result);
    }
    catch (const nlohmann::json::exception& e) {
        return createErrorResponse("Invalid player creation parameters: " + std::string(e.what()));
    }
}

std::string JSONInterface::handlePlayPattern(const nlohmann::json& json)
{
    try {
        // Extract parameters
        std::string playerId = json.at("player_id");
        std::string patternId = json.at("pattern_id");
        nlohmann::json events = json.at("events");
        
        std::cout << "[DEBUG] JSONInterface::handlePlayPattern - Processing pattern: " << patternId 
                  << " for player: " << playerId << std::endl;
        std::cout << "[DEBUG] Number of events in pattern: " << events.size() << std::endl;
        
        // Log a few events to verify content
        if (!events.empty()) {
            std::cout << "[DEBUG] First few events in pattern:" << std::endl;
            int count = std::min(5, static_cast<int>(events.size()));
            for (int i = 0; i < count; i++) {
                std::cout << "[DEBUG] Event " << i << ": " << events[i].dump() << std::endl;
            }
        }
        
        // Convert JSON events to MIDI sequence
        juce::MidiMessageSequence sequence = convertJSONToMIDISequence(events);
        
        std::cout << "[DEBUG] Converted to MIDI sequence with " << sequence.getNumEvents() << " events" << std::endl;
        
        // Play pattern
        std::cout << "[DEBUG] Sending pattern to player controller..." << std::endl;
        std::string result = controller.playPattern(playerId, patternId, sequence);
        std::cout << "[DEBUG] Player controller result: " << result << std::endl;
        
        // Check for error
        if (result.find("Error:") == 0) {
            std::cout << "[ERROR] Failed to play pattern: " << result << std::endl;
            return createErrorResponse(result);
        }
        
        return createSuccessResponse(result);
    }
    catch (const nlohmann::json::exception& e) {
        std::cout << "[ERROR] JSON exception in handlePlayPattern: " << e.what() << std::endl;
        return createErrorResponse("Invalid pattern parameters: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        std::cout << "[ERROR] Standard exception in handlePlayPattern: " << e.what() << std::endl;
        return createErrorResponse("Exception: " + std::string(e.what()));
    }
    catch (...) {
        std::cout << "[ERROR] Unknown exception in handlePlayPattern" << std::endl;
        return createErrorResponse("Unknown exception occurred");
    }
}

std::string JSONInterface::handleStopPlayer(const nlohmann::json& json)
{
    try {
        // Extract parameters
        std::string playerId = json.at("player_id");
        
        // Stop player
        std::string result = controller.stopPlayer(playerId);
        
        // Check for error
        if (result.find("Error:") == 0) {
            return createErrorResponse(result);
        }
        
        return createSuccessResponse(result);
    }
    catch (const nlohmann::json::exception& e) {
        return createErrorResponse("Invalid stop parameters: " + std::string(e.what()));
    }
}

std::string JSONInterface::handleShutdown(const nlohmann::json& json)
{
    try {
        // Extract scope (optional)
        std::string scope = json.value("scope", "system");
        
        // Shutdown
        std::string result;
        if (scope == "system") {
            result = controller.shutdown();
        }
        else {
            result = createErrorResponse("Invalid shutdown scope: " + scope);
        }
        
        // Check for error
        if (result.find("Error:") == 0) {
            return createErrorResponse(result);
        }
        
        return createSuccessResponse(result);
    }
    catch (const nlohmann::json::exception& e) {
        return createErrorResponse("Invalid shutdown parameters: " + std::string(e.what()));
    }
}

std::string JSONInterface::createSuccessResponse(const std::string& message)
{
    nlohmann::json response;
    response["status"] = "success";
    response["message"] = message;
    
    return response.dump();
}

std::string JSONInterface::createErrorResponse(const std::string& error)
{
    nlohmann::json response;
    response["status"] = "error";
    response["message"] = error;
    
    return response.dump();
}

juce::MidiMessageSequence JSONInterface::convertJSONToMIDISequence(const nlohmann::json& events)
{
    juce::MidiMessageSequence sequence;
    std::cout << "[DEBUG] Converting JSON events to MIDI sequence..." << std::endl;
    std::cout << "[DEBUG] Number of JSON events to convert: " << events.size() << std::endl;
    
    int noteOnCount = 0;
    int noteOffCount = 0;
    int ccCount = 0;
    int programChangeCount = 0;
    int unknownCount = 0;
    
    try {
        for (size_t i = 0; i < events.size(); ++i) {
            const auto& event = events[i];
            std::string type = event.at("type");
            int channel = event.value("channel", 0);
            double timestamp = event.value("time", 0.0);
            
            if (type == "note_on") {
                int note = event.at("note");
                int velocity = event.at("velocity");
                
                std::cout << "[DEBUG] Converting note_on: note=" << note 
                          << ", velocity=" << velocity 
                          << ", channel=" << channel 
                          << ", time=" << timestamp << std::endl;
                
                sequence.addEvent(juce::MidiMessage::noteOn(channel + 1, note, static_cast<juce::uint8>(velocity)),
                                timestamp);
                noteOnCount++;
            }
            else if (type == "note_off") {
                int note = event.at("note");
                
                std::cout << "[DEBUG] Converting note_off: note=" << note 
                          << ", channel=" << channel 
                          << ", time=" << timestamp << std::endl;
                
                sequence.addEvent(juce::MidiMessage::noteOff(channel + 1, note),
                                timestamp);
                noteOffCount++;
            }
            else if (type == "cc") {
                int controllerValue = event.at("controller");
                int value = event.at("value");
                
                std::cout << "[DEBUG] Converting cc: controller=" << controllerValue 
                          << ", value=" << value 
                          << ", channel=" << channel 
                          << ", time=" << timestamp << std::endl;
                
                sequence.addEvent(juce::MidiMessage::controllerEvent(channel + 1, controllerValue, value),
                                timestamp);
                ccCount++;
            }
            else if (type == "program_change") {
                int program = event.at("program");
                
                std::cout << "[DEBUG] Converting program_change: program=" << program 
                          << ", channel=" << channel 
                          << ", time=" << timestamp << std::endl;
                
                sequence.addEvent(juce::MidiMessage::programChange(channel + 1, program),
                                timestamp);
                programChangeCount++;
            }
            else {
                std::cout << "[WARNING] Unknown event type: " << type << std::endl;
                unknownCount++;
            }
        }
        
        // Update matched pairs to link note-ons with note-offs
        sequence.updateMatchedPairs();
        
        // Log summary
        std::cout << "[DEBUG] MIDI sequence conversion complete. Events: " 
                  << sequence.getNumEvents() << " total (" 
                  << noteOnCount << " note-on, " 
                  << noteOffCount << " note-off, " 
                  << ccCount << " cc, " 
                  << programChangeCount << " program-change, " 
                  << unknownCount << " unknown)" << std::endl;
        
        // Check for note-on/note-off mismatch
        if (noteOnCount != noteOffCount) {
            std::cout << "[WARNING] Mismatch between note-on and note-off events! " 
                      << "This may cause stuck notes or other playback issues." << std::endl;
        }
        
        // Check if sequence is empty
        if (sequence.getNumEvents() == 0) {
            std::cout << "[ERROR] Created an empty MIDI sequence! No events were converted." << std::endl;
        }
    }
    catch (const nlohmann::json::exception& e) {
        std::cout << "[ERROR] JSON exception in convertJSONToMIDISequence: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "[ERROR] Standard exception in convertJSONToMIDISequence: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "[ERROR] Unknown exception in convertJSONToMIDISequence" << std::endl;
    }
    
    return sequence;
}