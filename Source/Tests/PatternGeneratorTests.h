// Source/Tests/PatternGeneratorTests.h
#pragma once

#include <JuceHeader.h>
#include "../Musician/Core/PatternGenerator.h"
#include "../Musician/Core/MusicPatternSystem.h"
#include "../LLM/Core/LLMClient.h"
#include "../LLM/Core/PromptManager.h"

namespace covertracks {

// Mock Response Data for testing
struct MockResponseData {
    bool shouldSucceed = true;
    String response = R"({
        "title": "Test Pattern",
        "tempo": 120,
        "primitives": {
            "drums": {
                "kick": 36,
                "snare": 38
            }
        },
        "sections": {
            "main": [
                [1.0, "kick"],
                [3.0, "snare"]
            ]
        }
    })";
};

// T1: PatternGenerator utility function tests  
class PatternGeneratorUtilityTests : public juce::UnitTest
{
public:
    PatternGeneratorUtilityTests() : juce::UnitTest("T1 PatternGenerator Utilities", "Pattern Generation System") {}

    void runTest() override;

private:
    void testBasicConstruction();
    void testPromptGeneration();
    void testResponseParsing();
    void testResponseValidation();
    void testSystemPrompt();
};

// T2: PatternGenerator integration tests
class PatternGeneratorIntegrationTests : public juce::UnitTest
{
public:
    PatternGeneratorIntegrationTests() : juce::UnitTest("T2 PatternGenerator Integration", "Pattern Generation System") {}

    void runTest() override;

private:
    void testCompleteWorkflow();
    void testErrorHandling();
    void testEdgeCases();
};

// External declarations for TestRunner
extern PatternGeneratorUtilityTests patternGeneratorUtilityTests;
extern PatternGeneratorIntegrationTests patternGeneratorIntegrationTests;

} // namespace covertracks