#include "JuceLibraryCode/JuceHeader.h"
#include "Source/Tests/AudioEngineTests.cpp"
#include "Source/Audio/AudioEngine.h"

/*
 * Simple test runner for AudioEngine tests
 * Compile with: clang++ -I./JuceLibraryCode -I./modules -std=c++17 -framework Foundation -framework AudioToolbox -framework CoreAudio run_tests.cpp -o run_tests
 */

class TestApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "AudioEngine Tests"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    
    void initialise(const juce::String&) override
    {
        // Run AudioEngine tests
        juce::UnitTestRunner runner;
        runner.runAllTests();
        
        // Exit with appropriate code
        auto numFailures = runner.getNumTestsFailedThisRun();
        std::cout << "\nTest Results: " << runner.getNumTestsRunThisRun() << " tests run, " 
                  << numFailures << " failures\n";
        
        setApplicationReturnValue(numFailures > 0 ? 1 : 0);
        quit();
    }
    
    void shutdown() override {}
};

START_JUCE_APPLICATION(TestApp)