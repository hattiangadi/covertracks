// Source/Tests/MusicianTests.h
#pragma once

#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"
#include "../Musician/Core/MusicPatternSystem.h"
#include "../LLM/Core/PromptManager.h"

namespace covertracks {

/**
 * @brief Test data for Musician tests
 */
struct MockMusicianData {
    String validJsonResponse = R"({
        "tempo": 120,
        "sections": {
            "A": {
                "bars": [
                    {
                        "primitives": ["kick", "snare"],
                        "duration": 1.0
                    }
                ]
            }
        }
    })";
    
    String invalidJsonResponse = "not valid json at all";
    
    String malformedJsonResponse = R"({"tempo": 120, "sections": {unclosed)";
};

/**
 * @brief Basic Musician functionality tests
 */
class MusicianBasicTests : public juce::UnitTest {
public:
    MusicianBasicTests() : juce::UnitTest("Musician Basic Tests") {}
    
    void runTest() override;
    
private:
    void testConstruction();
    void testThreadLifecycle();
    void testBasicProperties();
    void testLLMClientConnection();
};

/**
 * @brief Musician chart conversion tests
 */
class MusicianChartTests : public juce::UnitTest {
public:
    MusicianChartTests() : juce::UnitTest("Musician Chart Tests") {}
    
    void runTest() override;
    
private:
    void testChartConversion();
    void testInvalidChartHandling();
    void testMidiEventGeneration();
};

/**
 * @brief Musician async operation tests
 */
class MusicianAsyncTests : public juce::UnitTest {
public:
    MusicianAsyncTests() : juce::UnitTest("Musician Async Tests") {}
    
    void runTest() override;
    
private:
    void testGenerateChartAsync();
    void testMultipleRequests();
    void testErrorHandling();
};

// Global test instances for TestRunner access
extern MusicianBasicTests musicianBasicTests;
extern MusicianChartTests musicianChartTests;
extern MusicianAsyncTests musicianAsyncTests;

} // namespace covertracks