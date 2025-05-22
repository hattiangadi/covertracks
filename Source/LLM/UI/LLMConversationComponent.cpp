// Source/LLM/UI/LLMConversationComponent.cpp
#include "LLMConversationComponent.h"
#include "../../Musician/Core/PatternGenerator.h"
#include <iostream>

namespace covertracks {

LLMConversationComponent::LLMConversationComponent(std::shared_ptr<LLMClient> llmClientPtr, Musician* musicianPtr)
    : inputEditor("InputEditor"),
      sendButton("Send"),
      clearButton("Clear"),
      generateButton("Generate"),
      playButton("Play"),
      rockButton("Rock"),
      jazzButton("Jazz"),
      funkButton("Funk"),
      balladsButton("Ballad"),
      llmClient(llmClientPtr),
      musician(musicianPtr),
      waitingForResponse(false)
{
    // Set up the conversation display
    conversationViewport.setViewedComponent(&conversationPanel, false);
    conversationViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(conversationViewport);
    
    // Set up the input editor
    inputEditor.setMultiLine(true);
    inputEditor.setReturnKeyStartsNewLine(false);
    inputEditor.setReadOnly(false);
    inputEditor.setScrollbarsShown(true);
    inputEditor.setCaretVisible(true);
    inputEditor.setPopupMenuEnabled(true);
    inputEditor.addListener(this);
    addAndMakeVisible(inputEditor);
    
    // Set up the send button
    sendButton.addListener(this);
    addAndMakeVisible(sendButton);
    
    // Set up the clear button
    clearButton.addListener(this);
    addAndMakeVisible(clearButton);
    
    // Set up the generate button
    generateButton.addListener(this);
    addAndMakeVisible(generateButton);
    
    // Set up the play button
    playButton.addListener(this);
    playButton.setEnabled(false); // Disabled until we have a pattern
    addAndMakeVisible(playButton);
    
    // Set up the preset buttons
    rockButton.addListener(this);
    jazzButton.addListener(this);
    funkButton.addListener(this);
    balladsButton.addListener(this);
    
    addAndMakeVisible(rockButton);
    addAndMakeVisible(jazzButton);
    addAndMakeVisible(funkButton);
    addAndMakeVisible(balladsButton);
    
    // Listen for changes from the musician
    if (musician)
        musician->addChangeListener(this);
    
    // Initial message
    messages.add({ false, "Hello! I'm your music generation assistant. How can I help you create music today?" });
    updateConversationDisplay();
}

LLMConversationComponent::~LLMConversationComponent()
{
    // Remove listeners
    inputEditor.removeListener(this);
    sendButton.removeListener(this);
    clearButton.removeListener(this);
    generateButton.removeListener(this);
    playButton.removeListener(this);
    rockButton.removeListener(this);
    jazzButton.removeListener(this);
    funkButton.removeListener(this);
    balladsButton.removeListener(this);
    
    if (musician)
        musician->removeChangeListener(this);
}

void LLMConversationComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
    
    // Draw the conversation area
    auto conversationArea = getLocalBounds().withTrimmedBottom(120);
    g.setColour(juce::Colours::lightgrey);
    g.fillRect(conversationArea);
    g.setColour(juce::Colours::grey);
    g.drawRect(conversationArea, 1);
}

void LLMConversationComponent::resized()
{
    auto area = getLocalBounds();
    
    // Conversation display at the top
    auto conversationArea = area.withTrimmedBottom(120);
    conversationViewport.setBounds(conversationArea);
    conversationPanel.setBounds(0, 0, conversationArea.getWidth() - 20, conversationPanel.getHeight());
    
    // Input area at the bottom
    auto inputArea = area.withTop(area.getHeight() - 120);
    
    // Preset buttons at the top of the input area
    auto presetArea = inputArea.removeFromTop(40).reduced(5);
    int presetButtonWidth = (presetArea.getWidth() - 15) / 4;
    
    rockButton.setBounds(presetArea.removeFromLeft(presetButtonWidth));
    presetArea.removeFromLeft(5);
    jazzButton.setBounds(presetArea.removeFromLeft(presetButtonWidth));
    presetArea.removeFromLeft(5);
    funkButton.setBounds(presetArea.removeFromLeft(presetButtonWidth));
    presetArea.removeFromLeft(5);
    balladsButton.setBounds(presetArea);
    
    // Input editor and buttons
    auto bottomRow = inputArea.removeFromBottom(30);
    inputEditor.setBounds(inputArea.reduced(5));
    
    // Buttons in the bottom row
    int buttonWidth = (bottomRow.getWidth() - 20) / 4;
    
    sendButton.setBounds(bottomRow.removeFromLeft(buttonWidth).reduced(5));
    clearButton.setBounds(bottomRow.removeFromLeft(buttonWidth).reduced(5));
    generateButton.setBounds(bottomRow.removeFromLeft(buttonWidth).reduced(5));
    playButton.setBounds(bottomRow.reduced(5));
}

void LLMConversationComponent::buttonClicked(juce::Button* button)
{
    if (button == &sendButton)
    {
        handleSendMessage();
    }
    else if (button == &clearButton)
    {
        handleClearConversation();
    }
    else if (button == &generateButton)
    {
        handleGeneratePattern();
    }
    else if (button == &playButton)
    {
        handlePlayPattern();
    }
    else if (button == &rockButton || button == &jazzButton || button == &funkButton || button == &balladsButton)
    {
        // Handle style preset buttons
        juce::String style = button->getButtonText().toLowerCase();
        handleStylePresetClicked(style);
    }
}

void LLMConversationComponent::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == &inputEditor)
    {
        if (juce::ModifierKeys::currentModifiers.isShiftDown())
        {
            // Shift+Enter: add a new line
            inputEditor.insertTextAtCaret("\n");
        }
        else
        {
            // Enter: send the message
            handleSendMessage();
        }
    }
}

void LLMConversationComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == musician)
    {
        // Enable the play button if we have a pattern
        playButton.setEnabled(musician->getPatternGenerator().isGenerating() == false);
    }
}

void LLMConversationComponent::updateConversationDisplay()
{
    // Calculate the height needed for all messages
    int totalHeight = 20; // Start with some padding
    juce::FontOptions fontOptions;
    auto fontWithHeight = fontOptions.withHeight(16.0f);
    juce::Font messageFont(fontWithHeight);
    
    for (const auto& message : messages)
    {
        // Calculate the text layout
        juce::String messageText = message.fromUser ? "You: " + message.message : "Assistant: " + message.message;
        juce::TextLayout textLayout;
        juce::AttributedString attributedText;
        attributedText.setText(messageText);
        attributedText.setFont(messageFont);
        attributedText.setColour(message.fromUser ? juce::Colours::black : juce::Colours::darkblue);
        textLayout.createLayout(attributedText, conversationPanel.getWidth() - 40.0f);
        
        // Add the height of this message plus some spacing
        totalHeight += static_cast<int>(textLayout.getHeight()) + 20;
    }
    
    // Resize the conversation panel
    conversationPanel.setSize(conversationPanel.getWidth(), totalHeight);
    
    // Update the messages in the panel
    conversationPanel.setMessages(messages);
    
    // Trigger a repaint
    conversationPanel.repaint();
    
    // Auto-scroll to the bottom
    conversationViewport.setViewPosition(0, totalHeight - conversationViewport.getHeight());
}

void LLMConversationComponent::handleSendMessage()
{
    // Get the message text
    juce::String message = inputEditor.getText().trim();
    
    if (message.isEmpty() || waitingForResponse)
        return;
    
    // Add the message to the conversation
    messages.add({ true, message });
    updateConversationDisplay();
    
    // Clear the input editor
    inputEditor.setText("");
    
    // Set waiting flag
    waitingForResponse = true;
    
    // Send the message to the LLM
    if (llmClient)
    {
        llmClient->sendMessage(message, [this](bool success, const juce::String& response) {
            // Add the response to the conversation
            messages.add({ false, success ? response : "Error: Failed to get a response from the LLM." });
            
            // Update the display
            updateConversationDisplay();
            
            // Reset waiting flag
            waitingForResponse = false;
        });
    }
    else
    {
        // No LLM client, just echo back
        messages.add({ false, "No LLM client connected. Please set up an API key." });
        updateConversationDisplay();
        waitingForResponse = false;
    }
}

void LLMConversationComponent::handleGeneratePattern()
{
    if (waitingForResponse || !musician)
        return;
    
    // Generate a default pattern
    juce::String prompt = "Create a simple musical pattern using keys that demonstrates a C major scale.";
    
    // Add the message to the conversation
    messages.add({ true, prompt });
    updateConversationDisplay();
    
    // Send the message to the musician
    waitingForResponse = true;
    
    // Add a placeholder for the response
    messages.add({ false, "Generating pattern..." });
    updateConversationDisplay();
    
    // Create parameters for pattern generation
    juce::DynamicObject* params = new juce::DynamicObject();
    params->setProperty("style", "classical");
    params->setProperty("instruments", "keys");
    params->setProperty("tempo", 120);
    params->setProperty("description", "C major scale demonstration");
    
    // Generate the pattern
    musician->generatePattern(juce::var(params), [this](bool success, const juce::var& pattern) {
        // Remove the "Generating pattern..." message
        if (!messages.isEmpty() && !messages.getLast().fromUser && 
            messages.getLast().message == "Generating pattern...")
        {
            messages.removeLast();
        }
        
        // Add the response
        if (success)
        {
            // Store the pattern
            if (pattern.hasProperty("response"))
            {
                messages.add({ false, pattern["response"].toString() });
            }
            else
            {
                messages.add({ false, "Pattern generated successfully! You can now play it using the Play button." });
            }
            
            // Enable the play button
            playButton.setEnabled(true);
        }
        else
        {
            messages.add({ false, "Sorry, I had trouble generating a pattern. Please try again." });
            playButton.setEnabled(false);
        }
        
        // Update the display
        updateConversationDisplay();
        
        // No longer waiting for a response
        waitingForResponse = false;
    });
}

void LLMConversationComponent::handleClearConversation()
{
    // Clear all messages except the initial greeting
    messages.clear();
    messages.add({ false, "Hello! I'm your music generation assistant. How can I help you create music today?" });
    updateConversationDisplay();
    
    // Clear the pattern generator's conversation history
    if (musician)
        musician->getPatternGenerator().clearConversationHistory();
    
    // Disable the play button
    playButton.setEnabled(false);
}

void LLMConversationComponent::handlePlayPattern()
{
    if (!musician)
        return;
    
    // Play the most recently generated pattern
    musician->playPattern(juce::var());
}

void LLMConversationComponent::handleStylePresetClicked(const juce::String& style)
{
    if (waitingForResponse)
        return;
    
    // Create a prompt based on the style
    juce::String prompt;
    
    if (style == "rock")
    {
        prompt = "Create a rock song with drums, bass, and guitar. Include a verse, chorus, and an exciting guitar solo in the bridge.";
    }
    else if (style == "jazz")
    {
        prompt = "Generate a jazz piece with keys, bass, and drums. Use complex chord progressions and include a swing feel.";
    }
    else if (style == "funk")
    {
        prompt = "Create a funky groove with drums, bass, and keys. Make it syncopated with a strong emphasis on beat 1.";
    }
    else if (style == "ballad")
    {
        prompt = "Compose a gentle ballad with piano and bass. Make it emotional with flowing arpeggios and a simple melody.";
    }
    else
    {
        prompt = "Generate a " + style + " style musical pattern.";
    }
    
    // Add the message to the conversation
    messages.add({ true, prompt });
    updateConversationDisplay();
    
    // Clear the input editor
    inputEditor.setText("");
    
    // Set waiting flag
    waitingForResponse = true;
    
    // Send the message to the LLM
    if (llmClient)
    {
        llmClient->sendMessage(prompt, [this](bool success, const juce::String& response) {
            // Add the response to the conversation
            messages.add({ false, success ? response : "Error: Failed to get a response from the LLM." });
            
            // Update the display
            updateConversationDisplay();
            
            // Reset waiting flag
            waitingForResponse = false;
        });
    }
    else
    {
        // No LLM client, just echo back
        messages.add({ false, "No LLM client connected. Please set up an API key." });
        updateConversationDisplay();
        waitingForResponse = false;
    }
}

} // namespace covertracks