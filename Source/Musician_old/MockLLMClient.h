#pragma once

#include "LLMClient.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace covertracks {

/**
 * @brief A mock LLM client for testing
 * 
 * This client doesn't require an actual LLM model, instead returning
 * hardcoded responses suitable for testing the Musician-Player architecture.
 */
class MockLLMClient : public LLMClient {
public:
    MockLLMClient() {
        std::cout << "Creating MockLLMClient" << std::endl;
    }
    
    ~MockLLMClient() override {
        std::cout << "Destroying MockLLMClient" << std::endl;
    }
    
    json generateResponse(const std::string& prompt) override {
        std::cout << "MockLLMClient generating response for prompt:" << std::endl;
        std::cout << prompt.substr(0, 150) << "..." << std::endl;
        
        // Check if this is a test pattern request
        if (prompt.find("twinkle") != std::string::npos) {
            return generateTwinkleTwinklePattern();
        }
        
        // Generate a simple response for testing
        return generateDefaultResponse();
    }
    
    void generateResponseAsync(const std::string& prompt,
                              std::function<void(const json&)> callback) override {
        std::cout << "MockLLMClient generating async response" << std::endl;
        
        // Simulate some processing time
        std::thread([prompt, callback, this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            json response;
            // Check if this is a test pattern request
            if (prompt.find("twinkle") != std::string::npos) {
                response = generateTwinkleTwinklePattern();
            } else {
                response = generateDefaultResponse();
            }
            
            callback(response);
        }).detach();
    }
    
    bool isReady() const override {
        return true;
    }
    
    std::string getModelName() const override {
        return "MockLLM";
    }
    
private:
    json generateTwinkleTwinklePattern() {
        std::cout << "Generating Twinkle Twinkle pattern" << std::endl;
        
        // Create MIDI events for Twinkle Twinkle Little Star
        json events = json::array();
        
        // Define the notes for Twinkle Twinkle Little Star melody
        // C C G G A A G, F F E E D D C
        const int notes[] = {
            60, 60, 67, 67, 69, 69, 67,
            65, 65, 64, 64, 62, 62, 60
        };
        
        // All notes are quarter notes (480 ticks)
        const int ticksPerQuarter = 480;
        
        // Generate note-on and note-off events
        for (int i = 0; i < 14; i++) {
            // Note-on at the start of each quarter note
            events.push_back({
                {"type", "note_on"},
                {"channel", 0},
                {"note", notes[i]},
                {"velocity", 100},
                {"time", i * ticksPerQuarter}
            });
            
            // Note-off at the end of each quarter note
            events.push_back({
                {"type", "note_off"},
                {"channel", 0},
                {"note", notes[i]},
                {"time", (i + 1) * ticksPerQuarter - 10}  // A bit before the next note
            });
        }
        
        return {{"events", events}};
    }
    
    json generateDefaultResponse() {
        std::cout << "Generating default pattern" << std::endl;
        
        // Create a simple C major scale
        json events = json::array();
        
        for (int i = 0; i < 8; i++) {
            int note = 60 + i;  // C4 to C5
            int startTime = i * 480;  // Quarter notes
            
            // Note-on
            events.push_back({
                {"type", "note_on"},
                {"channel", 0},
                {"note", note},
                {"velocity", 100},
                {"time", startTime}
            });
            
            // Note-off
            events.push_back({
                {"type", "note_off"},
                {"channel", 0},
                {"note", note},
                {"time", startTime + 450}  // Slightly shorter than quarter note
            });
        }
        
        return {{"events", events}};
    }
};

} // namespace covertracks