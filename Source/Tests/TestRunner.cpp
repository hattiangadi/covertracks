#include <JuceHeader.h>
#include "TestRunner.h"

// Include all test files
#include "PatternParserTests.cpp"
#include "PatternGeneratorTests.h"
#include "MusicianTests.cpp"
#include "MusicianAdvancedTests.cpp"
#include "PlayerTests.cpp"
#include "AudioEngineTests.cpp"

namespace covertracks {

void runAllTests()
{
    // Run all unit tests
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false); // Don't crash on failures, just report them
    
    std::cout << "Running PatternParser tests...\n" << std::endl;
    
    // Run the pattern parser tests
    runner.runTests({&basicPatternTests, &chordPatternTests, 
                     &layerPatternTests, &layerChordPatternTests,
                     &multiSectionSongTests, &patternReferenceTests,
                     &patternVariationTests});
    
    std::cout << "\nRunning PatternGenerator tests...\n" << std::endl;
    
    // Run the pattern generator tests  
    runner.runTests({&patternGeneratorUtilityTests, &patternGeneratorIntegrationTests});
    
    std::cout << "\nRunning Musician tests...\n" << std::endl;
    
    // Run the basic musician tests
    runner.runTests({&musicianBasicTests, &musicianChartTests, &musicianAsyncTests});
    
    std::cout << "\nRunning Musician advanced tests...\n" << std::endl;
    
    // Run the advanced musician tests  
    runner.runTests({&musicianPatternGeneratorTests, &musicianChartFormatTests, 
                     &musicianMemoryLeakTests, &musicianConcurrencyTests});
    
    std::cout << "\nRunning Player tests...\n" << std::endl;
    
    // Run the player tests
    runner.runTests({&playerBasicTests, &playerResourceTests, &playerChartTests, &playerPlaybackTests});
    
    std::cout << "\nRunning Player advanced tests...\n" << std::endl;
    
    // Run the advanced player tests
    runner.runTests({&playerAsyncTests, &playerPerformanceTests, &playerErrorTests, &playerIntegrationTests});
    
    std::cout << "\nRunning AudioEngine tests...\n" << std::endl;
    
    // Run the AudioEngine tests
    runner.runTests({&audioEngineTests});
    
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
}

} // namespace covertracks