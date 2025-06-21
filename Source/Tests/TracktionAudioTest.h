#pragma once

#include <JuceHeader.h>
#include "../Audio/AudioEngineInterface.h"

// Include Tracktion Engine headers
#include "../../modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"

namespace te = tracktion;

namespace covertracks {

/**
 * @brief Simple test component for verifying basic Tracktion Engine audio functionality
 */
class TracktionAudioTest : public juce::Component,
                          public juce::Button::Listener
{
public:
    TracktionAudioTest();
    ~TracktionAudioTest() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    
private:
    void generateSineWave(float frequency, float duration);
    void generateTwoTones();
    void stopAllAudio();
    
    // UI Components
    juce::Label titleLabel;
    juce::TextButton sineWaveButton;
    juce::TextButton twoTonesButton;
    juce::TextButton stopButton;
    juce::Label statusLabel;
    
    // Audio test state
    std::unique_ptr<te::AudioTrack> testTrack;
    bool isPlaying = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TracktionAudioTest)
};

} // namespace covertracks