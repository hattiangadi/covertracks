// Source/Tests/MusicianAdvancedTests.h
#pragma once

#include <JuceHeader.h>
#include "../Musician/Core/Musician.h"

namespace covertracks {

/**
 * @brief Tests for Musician integration with PatternGenerator
 * Area 1: Integration Testing
 */
class MusicianPatternGeneratorTests : public juce::UnitTest {
public:
    MusicianPatternGeneratorTests() : juce::UnitTest("Musician PatternGenerator Integration") {}
    
    void runTest() override;
    
private:
    void testPromptDelegation();
    void testChartParsing();
    void testDataFlow();
};

/**
 * @brief Tests for chart format conversions
 * Area 5: Chart Format Testing  
 */
class MusicianChartFormatTests : public juce::UnitTest {
public:
    MusicianChartFormatTests() : juce::UnitTest("Musician Chart Format Conversions") {}
    
    void runTest() override;
    
private:
    void testFullPipeline();
    void testTempoHandling();
    void testComplexChartStructures();
    void testEdgeCases();
};

/**
 * @brief Tests for memory leak prevention
 * Area 3: Memory Management Testing
 */
class MusicianMemoryLeakTests : public juce::UnitTest {
public:
    MusicianMemoryLeakTests() : juce::UnitTest("Musician Memory Leak Prevention") {}
    
    void runTest() override;
    
private:
    void testRepeatedOperations();
    void testObjectAccumulation();
    void testChartCleanup();
};

/**
 * @brief Tests for concurrency and thread safety
 * Area 4: Concurrency Testing
 */
class MusicianConcurrencyTests : public juce::UnitTest {
public:
    MusicianConcurrencyTests() : juce::UnitTest("Musician Concurrency") {}
    
    void runTest() override;
    
private:
    void testConcurrentRequests();
    void testThreadSafety();
    void testDestructionDuringOperation();
};

// Global test instances
extern MusicianPatternGeneratorTests musicianPatternGeneratorTests;
extern MusicianChartFormatTests musicianChartFormatTests;
extern MusicianMemoryLeakTests musicianMemoryLeakTests;
extern MusicianConcurrencyTests musicianConcurrencyTests;

} // namespace covertracks