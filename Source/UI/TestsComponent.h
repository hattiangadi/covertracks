#pragma once

#include <JuceHeader.h>
#include "../Audio/AudioEngineInterface.h"

// Include Tracktion Engine headers
#include "../../modules/tracktion_engine/modules/tracktion_engine/tracktion_engine.h"

namespace te = tracktion;

namespace covertracks {

/**
 * @brief Component for running unit tests within the application
 */
class TestsComponent : public juce::Component,
                       private juce::Button::Listener
{
public:
    TestsComponent();
    ~TestsComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener implementation
    void buttonClicked(juce::Button* button) override;
    
private:
    // UI components
    juce::Label titleLabel;
    juce::TextButton runButton;
    juce::TextButton clearButton;
    juce::TextEditor resultsText;
    
    // Audio test components
    juce::Label audioTestLabel;
    juce::TextButton sineWaveButton;
    juce::TextButton twoTonesButton;
    juce::TextButton stopAudioButton;
    juce::Label audioStatusLabel;
    
    // Audio test methods
    void runTests();
    void generateSineWave(float frequency, float duration);
    void generateTwoTones();
    void stopAllAudio();
    
    // Audio test state
    te::AudioTrack* testTrack = nullptr;
    bool isAudioPlaying = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestsComponent)
};

} // namespace covertracks