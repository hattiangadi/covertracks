#pragma once
#include <iostream>
#include <JuceHeader.h>

namespace covertracks {

// Implementation for the test runner function (inline to avoid needing .cpp file)
inline void runAllTests() {
    std::cout << "=== Covertracks Unit Tests ===" << std::endl;
    std::cout << "Running MusicPatternSystem tests..." << std::endl;
    
    // Use JUCE's UnitTestRunner to run all registered tests
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    // Calculate results from test results
    int numTests = runner.getNumResults();
    int numFailures = 0;
    
    for (int i = 0; i < numTests; ++i) {
        auto* result = runner.getResult(i);
        if (result && result->failures > 0) {
            numFailures++;
        }
    }
    
    if (numFailures == 0) {
        std::cout << "PASS: All " << numTests << " unit tests passed!" << std::endl;
    } else {
        std::cout << "FAIL: " << numFailures << " out of " << numTests << " tests failed" << std::endl;
    }
    
    std::cout << "NOTE: Additional audio tests are available in the 'Tracktion Engine Audio Tests' section" << std::endl;
}

} // namespace covertracks