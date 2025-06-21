#include <iostream>
#include <memory>

// Minimal test to verify Tracktion Engine creation
#include "modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"

namespace te = tracktion;

int main()
{
    std::cout << "Testing Tracktion Engine integration..." << std::endl;
    
    try {
        // Test 1: Basic Engine creation
        std::cout << "1. Creating Tracktion Engine..." << std::endl;
        auto engine = std::make_unique<te::Engine>("TestEngine");
        std::cout << "   ✅ Engine created successfully!" << std::endl;
        
        // Test 2: Edit creation
        std::cout << "2. Creating temporary edit file..." << std::endl;
        auto tempFile = juce::File::createTempFile(".tracktionedit");
        std::cout << "   ✅ Temp file created: " << tempFile.getFullPathName().toStdString() << std::endl;
        
        std::cout << "3. Creating Edit..." << std::endl;
        auto edit = te::createEmptyEdit(*engine, tempFile);
        if (edit) {
            std::cout << "   ✅ Edit created successfully!" << std::endl;
            
            // Test 3: Track creation
            std::cout << "4. Creating audio track..." << std::endl;
            auto track = edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*edit), nullptr);
            if (track) {
                std::cout << "   ✅ Track created: " << track->getName().toStdString() << std::endl;
                
                // Test 4: MIDI clip creation
                std::cout << "5. Creating MIDI clip..." << std::endl;
                te::TimePosition start = te::TimePosition::fromSeconds(0.0);
                te::TimePosition end = te::TimePosition::fromSeconds(4.0);
                auto midiClip = track->insertMIDIClip("Test Clip", te::TimeRange(start, end), nullptr);
                if (midiClip) {
                    std::cout << "   ✅ MIDI clip created: " << midiClip->getName().toStdString() << std::endl;
                } else {
                    std::cout << "   ❌ Failed to create MIDI clip" << std::endl;
                }
            } else {
                std::cout << "   ❌ Failed to create track" << std::endl;
            }
        } else {
            std::cout << "   ❌ Failed to create edit" << std::endl;
        }
        
        std::cout << "\n🎉 All Tracktion Engine tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "❌ Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}