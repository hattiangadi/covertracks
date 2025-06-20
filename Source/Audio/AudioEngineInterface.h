/*
  ==============================================================================
   AudioEngineInterface.h
   
   Interface for switching between original AudioEngine and TracktionAudioEngine.
   
   This allows for a seamless replacement strategy where we can switch between
   implementations by changing a single typedef.
  ==============================================================================
*/

#pragma once

// Enable this to use Tracktion Engine, comment out to use original
#define USE_TRACKTION_ENGINE 0

#if USE_TRACKTION_ENGINE
    #include "TracktionAudioEngine.h"
    namespace covertracks {
        // Type alias for seamless replacement
        using AudioEngineImpl = TracktionAudioEngine;
    }
#else
    #include "AudioEngine.h" 
    namespace covertracks {
        // Type alias for seamless replacement
        using AudioEngineImpl = AudioEngine;
    }
#endif

namespace covertracks {

/**
 * @brief Unified interface for getting the audio engine instance
 * 
 * This function provides a consistent way to access the audio engine
 * regardless of which implementation is currently active.
 */
inline AudioEngineImpl& getAudioEngine() {
    return AudioEngineImpl::getInstance();
}

} // namespace covertracks