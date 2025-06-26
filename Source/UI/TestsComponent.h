#pragma once

#include <JuceHeader.h>
#include "../Audio/AudioEngineInterface.h"
#include "../Player/Core/Player.h"
#include "../Player/Core/Instrument.h"

namespace covertracks {

/**
 * @brief Component for running unit tests and testing new architecture within the application
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
    juce::ComboBox testSuiteSelector;
    juce::Label testSuiteLabel;
    juce::TextButton runButton;
    juce::TextButton clearButton;
    juce::TextEditor resultsText;
    
    // Audio test components (now using new architecture)
    juce::Label audioTestLabel;
    juce::TextButton sineWaveButton;
    juce::TextButton twoTonesButton;
    juce::TextButton stopAudioButton;
    juce::TextButton pluginEditorButton;
    juce::Label audioStatusLabel;
    
    // Test methods
    void runTests();
    
    // Audio test methods using new Player/Instrument architecture
    void generateSineWave(float frequency, float duration);
    void generateTwoTones();
    void stopAllAudio();
    void showPluginEditor();
    void updateAudioStatus(const juce::String& message);
    
    // New architecture components
    std::unique_ptr<Instrument> testInstrument;
    std::unique_ptr<Player> testPlayer1;
    std::unique_ptr<Player> testPlayer2;
    
    // Shared plugin format manager and known plugin list for new architecture
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestsComponent)
};

} // namespace covertracks