// Source/Tests/PatternGeneratorTests.cpp
#include "PatternGeneratorTests.h"

namespace covertracks {

void PatternGeneratorUtilityTests::runTest()
{
    testBasicConstruction();
    testPromptGeneration();
    testResponseParsing();
    testResponseValidation();
    testSystemPrompt();
}

void PatternGeneratorUtilityTests::testBasicConstruction()
{
    beginTest("Basic PatternGenerator construction and destruction");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    
    // Should construct without issues using new 2-parameter constructor
    {
        PatternGenerator generator(patternSystem, promptManager);
        
        // Test that system prompt is initialized
        expect(generator.getSystemPrompt().isNotEmpty(), "System prompt should be initialized");
        expect(generator.getSystemPrompt().contains("music pattern generator"), "System prompt should contain expected content");
    }
    
    // Should destruct cleanly
    logMessage("PatternGenerator constructed and destructed successfully");
}

void PatternGeneratorUtilityTests::testPromptGeneration()
{
    beginTest("Prompt generation utilities");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    
    // Test song prompt generation
    StringArray instruments;
    instruments.add("drums");
    instruments.add("bass");
    
    String songPrompt = generator.createSongPrompt("rock", instruments, 120, "energetic test pattern");
    expect(songPrompt.isNotEmpty(), "Song prompt should not be empty");
    expect(songPrompt.contains("rock") || songPrompt.contains("120"), "Song prompt should contain style or tempo");
    
    // Test modification prompt generation
    String currentPattern = "{\"tempo\": 120, \"sections\": {}}";
    String modificationPrompt = generator.createModificationPrompt(currentPattern, "make it faster");
    expect(modificationPrompt.isNotEmpty(), "Modification prompt should not be empty");
    expect(modificationPrompt.contains("make it faster"), "Modification prompt should contain the modification request");
    
    // Test section prompt generation
    String sectionPrompt = generator.createSectionPrompt("verse", "jazz", instruments, "smooth section");
    expect(sectionPrompt.isNotEmpty(), "Section prompt should not be empty");
    expect(sectionPrompt.contains("verse") || sectionPrompt.contains("jazz"), "Section prompt should contain section or style");
    
    logMessage("All prompt generation utilities working correctly");
}

void PatternGeneratorUtilityTests::testResponseParsing()
{
    beginTest("Response parsing utilities");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    MockResponseData mockData;
    
    // Test successful parsing of valid JSON
    auto chart = generator.parseResponse(mockData.response);
    expect(chart != nullptr, "Should successfully parse valid JSON response");
    
    if (chart)
    {
        expect(chart->getTempo() > 0, "Parsed chart should have a valid tempo");
        expect(chart->getNumSections() > 0, "Parsed chart should have sections");
    }
    
    // Test parsing of invalid JSON
    auto invalidChart = generator.parseResponse("invalid json content");
    expect(invalidChart == nullptr, "Should return nullptr for invalid JSON");
    
    // Test parsing of empty response
    auto emptyChart = generator.parseResponse("");
    expect(emptyChart == nullptr, "Should return nullptr for empty response");
    
    logMessage("Response parsing utilities working correctly");
}

void PatternGeneratorUtilityTests::testResponseValidation()
{
    beginTest("Response validation utilities");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    MockResponseData mockData;
    
    // Test validation of valid JSON
    expect(generator.validateResponse(mockData.response), "Should validate correct JSON response");
    
    // Test validation of responses with pattern keywords
    expect(generator.validateResponse("tempo: 120, sections: {}"), "Should validate response with pattern keywords");
    
    // Test validation of invalid responses
    expect(!generator.validateResponse(""), "Should reject empty response");
    expect(!generator.validateResponse("just plain text"), "Should reject plain text without structure");
    expect(!generator.validateResponse("no musical content here"), "Should reject non-musical content");
    
    logMessage("Response validation utilities working correctly");
}

void PatternGeneratorUtilityTests::testSystemPrompt()
{
    beginTest("System prompt management");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    
    // Test initial system prompt
    String initialPrompt = generator.getSystemPrompt();
    expect(initialPrompt.isNotEmpty(), "Initial system prompt should not be empty");
    
    // Test setting custom system prompt
    String customPrompt = "Custom test prompt for pattern generation";
    generator.setSystemPrompt(customPrompt);
    expect(generator.getSystemPrompt() == customPrompt, "Should update system prompt correctly");
    
    // Test setting empty system prompt
    generator.setSystemPrompt("");
    expect(generator.getSystemPrompt().isEmpty(), "Should allow empty system prompt");
    
    logMessage("System prompt management working correctly");
}

void PatternGeneratorIntegrationTests::runTest()
{
    testCompleteWorkflow();
    testErrorHandling();
    testEdgeCases();
}

void PatternGeneratorIntegrationTests::testCompleteWorkflow()
{
    beginTest("Complete workflow integration");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    MockResponseData mockData;
    
    // Test complete workflow: prompt -> parse -> validate
    StringArray instruments;
    instruments.add("piano");
    
    // 1. Generate prompt
    String prompt = generator.createSongPrompt("classical", instruments, 80, "peaceful melody");
    expect(prompt.isNotEmpty(), "Workflow step 1: prompt generation should work");
    
    // 2. Validate response
    bool isValid = generator.validateResponse(mockData.response);
    expect(isValid, "Workflow step 2: response validation should work");
    
    // 3. Parse response
    auto chart = generator.parseResponse(mockData.response);
    expect(chart != nullptr, "Workflow step 3: response parsing should work");
    
    logMessage("Complete workflow integration successful");
}

void PatternGeneratorIntegrationTests::testErrorHandling()
{
    beginTest("Error handling integration");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    
    // Test handling malformed JSON
    String malformedJson = "{\"tempo\": 120, \"sections\": {unclosed";
    expect(!generator.validateResponse(malformedJson), "Should reject malformed JSON");
    expect(generator.parseResponse(malformedJson) == nullptr, "Should return null for malformed JSON");
    
    // Test handling empty input
    expect(!generator.validateResponse(""), "Should reject empty input");
    expect(generator.parseResponse("") == nullptr, "Should return null for empty input");
    
    logMessage("Error handling integration successful");
}

void PatternGeneratorIntegrationTests::testEdgeCases()
{
    beginTest("Edge cases integration");
    
    MusicPatternSystem patternSystem;
    PromptManager promptManager;
    PatternGenerator generator(patternSystem, promptManager);
    
    // Test with empty instrument arrays
    StringArray emptyInstruments;
    String prompt = generator.createSongPrompt("rock", emptyInstruments, 120, "test");
    expect(prompt.isNotEmpty(), "Should handle empty instruments array");
    
    // Test with very long descriptions
    String longDescription = String::repeatedString("very long description ", 100);
    String longPrompt = generator.createSongPrompt("jazz", emptyInstruments, 140, longDescription);
    expect(longPrompt.isNotEmpty(), "Should handle very long descriptions");
    
    // Test with special characters in style
    String specialPrompt = generator.createSectionPrompt("verse@#$%", "rock&roll", emptyInstruments, "test");
    expect(specialPrompt.isNotEmpty(), "Should handle special characters");
    
    logMessage("Edge cases integration successful");
}

// Test registry - global instances for TestRunner access
PatternGeneratorUtilityTests patternGeneratorUtilityTests;
PatternGeneratorIntegrationTests patternGeneratorIntegrationTests;

} // namespace covertracks