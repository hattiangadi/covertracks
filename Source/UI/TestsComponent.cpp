#include "TestsComponent.h"
#include "../Tests/TestRunner.h"
#include <sstream>
#include <iostream>

namespace covertracks {

TestsComponent::TestsComponent()
    : titleLabel("titleLabel", "Unit Tests"),
      runButton("Run Tests"),
      clearButton("Clear Results")
{
    // Set up title label
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("bold")));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up buttons
    runButton.addListener(this);
    addAndMakeVisible(runButton);
    
    clearButton.addListener(this);
    addAndMakeVisible(clearButton);
    
    // Set up results text area
    resultsText.setMultiLine(true);
    resultsText.setReadOnly(true);
    resultsText.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(12.0f)));
    resultsText.setText("Click 'Run Tests' to execute PatternParser unit tests...");
    addAndMakeVisible(resultsText);
    
    // Test runner is now handled by the runAllTests() function
}

TestsComponent::~TestsComponent()
{
    runButton.removeListener(this);
    clearButton.removeListener(this);
}

void TestsComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw a border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void TestsComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Position title label at the top
    titleLabel.setBounds(bounds.removeFromTop(30));
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position buttons
    auto buttonRow = bounds.removeFromTop(30);
    runButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(10); // spacing
    clearButton.setBounds(buttonRow.removeFromLeft(100));
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position results text area to fill the remaining space
    resultsText.setBounds(bounds);
}

void TestsComponent::buttonClicked(juce::Button* button)
{
    if (button == &runButton)
    {
        runTests();
    }
    else if (button == &clearButton)
    {
        resultsText.setText("Click 'Run Tests' to execute PatternParser and PatternGenerator unit tests...");
    }
}

void TestsComponent::runTests()
{
    resultsText.setText("Running tests...\n");
    
    // Capture console output for display
    std::ostringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());
    
    // Run all tests (this will print to our captured output)
    covertracks::runAllTests();
    
    // Restore console output
    std::cout.rdbuf(old_cout);
    
    // Display the captured output
    resultsText.setText(output.str());
}

} // namespace covertracks