#pragma once

#include <JuceHeader.h>

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
    
    // Test runner (not a component)
    std::unique_ptr<juce::UnitTestRunner> testRunner;
    
    void runTests();
    void displayResults();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestsComponent)
};

} // namespace covertracks