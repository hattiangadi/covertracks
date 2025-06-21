// Simple test to verify the AudioEngineInterface works
#include "Source/Audio/AudioEngineInterface.h"
#include <iostream>

int main() {
    std::cout << "Testing AudioEngine interface..." << std::endl;
    
    try {
        auto& engine = covertracks::getAudioEngine();
        std::cout << "✅ AudioEngine interface works!" << std::endl;
        std::cout << "Sample rate: " << engine.getSampleRate() << std::endl;
        std::cout << "Buffer size: " << engine.getBufferSize() << std::endl;
        
#if USE_TRACKTION_ENGINE
        std::cout << "🎵 Using TracktionAudioEngine" << std::endl;
#else 
        std::cout << "🔧 Using original AudioEngine" << std::endl;
#endif
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}