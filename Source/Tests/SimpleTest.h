#pragma once
#include <JuceHeader.h>

namespace covertracks {

/**
 * Simple test to verify CLI test infrastructure works
 */
class SimpleTest : public juce::UnitTest
{
public:
    SimpleTest() : UnitTest("Simple Infrastructure Test", "Infrastructure") {}
    
    void runTest() override
    {
        beginTest("Basic arithmetic");
        expect(2 + 2 == 4, "Two plus two should equal four");
        expect(10 * 3 == 30, "Ten times three should equal thirty");
        
        beginTest("String operations");
        juce::String hello = "Hello";
        juce::String world = "World";
        juce::String combined = hello + " " + world;
        expect(combined == "Hello World", "String concatenation should work");
        
        beginTest("JUCE basics");
        juce::Array<int> numbers;
        numbers.add(1);
        numbers.add(2);
        numbers.add(3);
        expect(numbers.size() == 3, "Array should contain 3 elements");
        expect(numbers[1] == 2, "Second element should be 2");
    }
};

// Register the test with JUCE's test runner
static SimpleTest simpleTest;

} // namespace covertracks