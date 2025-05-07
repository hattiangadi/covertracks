/**
 * @file JSONInterface_Tracktion.cpp
 * @brief Implementation of the JSONInterface class using Tracktion Engine
 */
 #include "JSONInterface.h"
 #include "TracktionIntegration.h"
 #include <iostream>
 
 JSONInterface::JSONInterface()
 {
     std::cout << "Creating JSONInterface (Tracktion implementation)" << std::endl;
     
     // Initialize the Tracktion Engine
     if (!TracktionIntegration::getInstance().initialize()) {
         std::cerr << "Failed to initialize Tracktion Engine" << std::endl;
     }
 }
 
 JSONInterface::~JSONInterface()
 {
     std::cout << "Destroying JSONInterface (Tracktion implementation)" << std::endl;
 }
 
 std::string JSONInterface::handleCommand(const std::string& jsonCommand)
 {
     // Log the command (shortened if too long)
     if (jsonCommand.length() > 200) {
         std::cout << "JSONInterface handling command: " << jsonCommand.substr(0, 200) << "..." << std::endl;
     } else {
         std::cout << "JSONInterface handling command: " << jsonCommand << std::endl;
     }
     
     // Forward the command to the Tracktion player interface
     auto* playerInterface = TracktionIntegration::getInstance().getPlayerInterface();
     if (!playerInterface) {
         nlohmann::json errorResponse;
         errorResponse["status"] = "error";
         errorResponse["message"] = "Tracktion player interface not initialized";
         return errorResponse.dump();
     }
     
     // Forward command and return response
     std::string response = playerInterface->handleCommand(jsonCommand);
     
     // Log the response (shortened if too long)
     if (response.length() > 200) {
         std::cout << "JSONInterface response: " << response.substr(0, 200) << "..." << std::endl;
     } else {
         std::cout << "JSONInterface response: " << response << std::endl;
     }
     
     return response;
 }
 
 void JSONInterface::setPatternFinishedCallback(
     std::function<void(const std::string&, const std::string&)> callback)
 {
     std::cout << "Setting pattern finished callback in JSONInterface" << std::endl;
     
     // Forward to Tracktion player interface
     auto* playerInterface = TracktionIntegration::getInstance().getPlayerInterface();
     if (playerInterface) {
         playerInterface->setPatternFinishedCallback(callback);
     } else {
         std::cerr << "Cannot set pattern finished callback - player interface not initialized" << std::endl;
     }
 }