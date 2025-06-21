/*
  ==============================================================================
   AudioEngineInterface.h
   
   Tracktion Engine-based audio engine interface.
   
   This provides a clean interface to the TracktionAudioEngine singleton.
  ==============================================================================
*/

#pragma once

#include "TracktionAudioEngine.h"

namespace covertracks {

// Direct type alias for TracktionAudioEngine
typedef TracktionAudioEngine AudioEngineImpl;

/**
 * @brief Get the TracktionAudioEngine singleton instance
 */
inline TracktionAudioEngine& getAudioEngine() {
    return TracktionAudioEngine::getInstance();
}

/**
 * @brief Get TracktionAudioEngine instance as pointer for compatibility
 */
inline TracktionAudioEngine* getAudioEnginePtr() {
    return &TracktionAudioEngine::getInstance();
}

} // namespace covertracks