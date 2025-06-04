#include <JuceHeader.h>

// Include all test files
#include "PatternParserTests.cpp"

int main()
{
    // Run all unit tests
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false); // Don't crash on failures, just report them
    
    std::cout << "Running PatternParser tests...\n" << std::endl;
    
    // Run the pattern parser tests
    runner.runTests({&covertracks::patternParserTests});
    
    // Print results
    int numTests = runner.getNumResults();
    int numPassed = 0;
    int numFailed = 0;
    
    for (int i = 0; i < numTests; ++i)
    {
        auto result = runner.getResult(i);
        if (result->failures == 0)
            numPassed++;
        else
            numFailed++;
            
        std::cout << "\n" << result->unitTestName << ":" << std::endl;
        std::cout << "  Tests: " << result->tests << std::endl;
        std::cout << "  Passes: " << result->passes << std::endl;
        std::cout << "  Failures: " << result->failures << std::endl;
        
        if (result->failures > 0)
        {
            std::cout << "  Messages:" << std::endl;
            for (auto& message : result->messages)
            {
                std::cout << "    " << message << std::endl;
            }
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total test suites: " << numTests << std::endl;
    std::cout << "Passed: " << numPassed << std::endl;
    std::cout << "Failed: " << numFailed << std::endl;
    
    return numFailed == 0 ? 0 : 1;
}