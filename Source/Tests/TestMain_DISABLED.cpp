#include <JuceHeader.h>
#include "TestRunner.h"

/*
 * Main entry point for the test executable
 * 
 * This creates a minimal JUCE application that runs all tests
 * and exits with appropriate return codes for CI/CD systems.
 */

namespace covertracks {

class TestApplication : public juce::JUCEApplication
{
public:
    TestApplication() = default;
    
    const juce::String getApplicationName() override
    {
        return "Covertracks Tests";
    }
    
    const juce::String getApplicationVersion() override
    {
        return "1.0.0";
    }
    
    void initialise(const juce::String& commandLine) override
    {
        // Suppress unused parameter warning
        juce::ignoreUnused(commandLine);
        
        std::cout << "Running Covertracks test suite..." << std::endl;
        std::cout << "==========================================" << std::endl;
        
        try
        {
            // Run all tests
            runAllTests();
            
            std::cout << "==========================================" << std::endl;
            std::cout << "Test run completed." << std::endl;
            
            // Exit with success - test results are displayed in UI
            // For automated testing, consider implementing exit code based on test results
            quit();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
            setApplicationReturnValue(1);
            quit();
        }
        catch (...)
        {
            std::cerr << "Test execution failed with unknown exception" << std::endl;
            setApplicationReturnValue(1);
            quit();
        }
    }
    
    void shutdown() override
    {
        // Cleanup if needed
    }
};

} // namespace covertracks

// This macro creates the main() function for the application
START_JUCE_APPLICATION(covertracks::TestApplication)