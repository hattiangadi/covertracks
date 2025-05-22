// Source/Musician/Core/PatternGenerator.cpp
#include "PatternGenerator.h"
#include "PatternParser.h"
#include <iostream>

namespace covertracks {

PatternGenerator::PatternGenerator(MusicPatternSystem& system,
                                  std::shared_ptr<LLMClient> client,
                                  PromptManager& manager)
    : Thread("PatternGeneratorThread"),
      patternSystem(system),
      llmClient(client),
      promptManager(manager),
      parser(std::make_unique<PatternParser>(system)),
      generating(false),
      waitEvent(true) // Auto reset
{
    startThread();
}

PatternGenerator::~PatternGenerator()
{
    stopThread(5000);
}

void PatternGenerator::generateSong(const String& style,
                                  const StringArray& instruments,
                                  int tempo,
                                  const String& description,
                                  PatternGenerationCallback callback)
{
    if (generating)
    {
        if (callback)
            callback(false, nullptr, "Pattern generation already in progress");
        return;
    }
    
    generating = true;
    
    // Create the prompt
    currentPrompt = promptManager.createMusicGenerationPrompt(style, instruments, tempo, description);
    currentCallback = std::move(callback);
    
    // Signal the thread to start generation
    waitEvent.signal();
}

void PatternGenerator::modifyPattern(const String& currentPattern,
                                   const String& modification,
                                   PatternGenerationCallback callback)
{
    if (generating)
    {
        if (callback)
            callback(false, nullptr, "Pattern generation already in progress");
        return;
    }
    
    generating = true;
    
    // Create the prompt
    currentPrompt = promptManager.createPatternModificationPrompt(currentPattern, modification);
    currentCallback = std::move(callback);
    
    // Signal the thread to start generation
    waitEvent.signal();
}

void PatternGenerator::generateSection(const String& section,
                                     const String& style,
                                     const StringArray& instruments,
                                     const String& description,
                                     PatternGenerationCallback callback)
{
    if (generating)
    {
        if (callback)
            callback(false, nullptr, "Pattern generation already in progress");
        return;
    }
    
    generating = true;
    
    // Create the prompt
    currentPrompt = promptManager.createSectionGenerationPrompt(section, style, instruments, description);
    currentCallback = std::move(callback);
    
    // Signal the thread to start generation
    waitEvent.signal();
}

void PatternGenerator::run()
{
    while (!threadShouldExit())
    {
        // Wait for a generation request
        waitEvent.wait(-1);
        
        if (threadShouldExit())
            break;
        
        if (llmClient && generating)
        {
            // Send the prompt to the LLM
            String prompt = currentPrompt;
            
            // Add the system prompt if provided
            if (systemPrompt.isNotEmpty())
            {
                // The system prompt will be handled by prepending it to the user's first message
                if (conversationHistory.isEmpty())
                {
                    prompt = systemPrompt + "\n\n" + prompt;
                }
            }
            
            // Use conversation history if available
            llmClient->sendMessageWithHistory(prompt, conversationHistory, 
                [this](bool success, const String& response) {
                    // Parse the response
                    std::shared_ptr<Song> song;
                    
                    if (success)
                    {
                        // Add to conversation history
                        conversationHistory.add({currentPrompt, response});
                        
                        // Parse the response
                        song = parser->parseLLMResponse(response);
                    }
                    
                    // Call the callback on the message thread
                    auto callback = currentCallback;
                    MessageManager::callAsync([callback, success, song, response]() {
                        if (callback)
                            callback(success, song, response);
                    });
                    
                    // Reset state
                    generating = false;
                });
        }
        else
        {
            // LLM client not available or not generating
            auto callback = currentCallback;
            MessageManager::callAsync([callback]() {
                if (callback)
                    callback(false, nullptr, "LLM client not available");
            });
            
            generating = false;
        }
    }
}

} // namespace covertracks