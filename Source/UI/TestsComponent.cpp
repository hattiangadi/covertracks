#include "TestsComponent.h"
#include "../Tests/PatternParserTests.cpp"

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
    
    // Create the unit test runner (not a component, just for running tests)
    testRunner = std::make_unique<juce::UnitTestRunner>();
    testRunner->setAssertOnFailure(false); // Don't crash on failures
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
        resultsText.setText("Click 'Run Tests' to execute PatternParser unit tests...");
    }
}

void TestsComponent::runTests()
{
    resultsText.setText("Running tests...\n");
    
    // Run the tests
    testRunner->runAllTests();
    
    // Display results
    displayResults();
}

void TestsComponent::displayResults()
{
    juce::String results = "=== PATTERN PARSER TEST RESULTS ===\n\n";
    
    int numResults = testRunner->getNumResults();
    int totalTests = 0;
    int totalPasses = 0;
    int totalFailures = 0;
    
    for (int i = 0; i < numResults; ++i)
    {
        auto result = testRunner->getResult(i);
        
        // JUCE TestResult has passes and failures, tests = passes + failures
        int tests = result->passes + result->failures;
        totalTests += tests;
        totalPasses += result->passes;
        totalFailures += result->failures;
        
        results += "Test Suite: " + result->unitTestName + "\n";
        results += "  Tests: " + juce::String(tests) + "\n";
        results += "  Passes: " + juce::String(result->passes) + "\n";
        results += "  Failures: " + juce::String(result->failures) + "\n";
        
        if (result->failures > 0)
        {
            results += "  Error Messages:\n";
            for (auto& message : result->messages)
            {
                results += "    " + message + "\n";
            }
        }
        results += "\n";
    }
    
    results += "=== SUMMARY ===\n";
    results += "Total Tests: " + juce::String(totalTests) + "\n";
    results += "Total Passes: " + juce::String(totalPasses) + "\n";
    results += "Total Failures: " + juce::String(totalFailures) + "\n";
    
    if (totalFailures == 0)
    {
        results += "\nALL TESTS PASSED!\n";
    }
    else
    {
        results += "\n" + juce::String(totalFailures) + " test(s) failed.\n";
    }
    
    resultsText.setText(results);
}

} // namespace covertracks