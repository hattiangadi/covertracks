// Source/LLM/UI/LLMConversationComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/LLMClient.h"
#include "../../Musician/Core/Musician.h"

namespace covertracks {

/**
 * @brief UI Component for conversing with an LLM
 * 
 * This component provides a chat-like interface for interacting with an LLM
 * to generate and modify musical patterns.
 */
class LLMConversationComponent : public juce::Component, 
                                public juce::Button::Listener,
                                public juce::TextEditor::Listener,
                                private juce::ChangeListener
{
public:
    /**
     * @brief Construct a new LLM Conversation Component
     * 
     * @param llmClient Shared pointer to the LLM client
     * @param musician Pointer to the musician (not owned)
     */
    LLMConversationComponent(std::shared_ptr<LLMClient> llmClient, Musician* musician);
    
    /**
     * @brief Destroy the LLM Conversation Component
     */
    ~LLMConversationComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    
    // Musician access
    Musician* getMusician() const { return musician; }
    
private:
    // ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
    // UI components
    juce::TextEditor inputEditor;
    juce::TextButton sendButton;
    juce::TextButton clearButton;
    juce::TextButton generateButton;
    juce::TextButton playButton;
    
    // Conversation display
    struct ConversationMessage {
        bool fromUser;
        juce::String message;
    };
    
    // Custom panel for displaying conversation
    class ConversationPanel : public juce::Component
    {
    public:
        ConversationPanel() { setOpaque(true); }
        
        void paint(juce::Graphics& g) override
        {
            g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
            
            int y = 10;
            juce::FontOptions fontOptions;
            auto fontWithHeight = fontOptions.withHeight(16.0f);
            juce::Font messageFont(fontWithHeight);
            
            for (const auto& message : messages)
            {
                // Draw the message
                g.setColour(message.fromUser ? juce::Colours::black : juce::Colours::darkblue);
                g.setFont(messageFont);
                
                // Calculate the text layout
                juce::String messageText = message.fromUser ? "You: " + message.message : "Assistant: " + message.message;
                juce::TextLayout textLayout;
                juce::AttributedString attributedText;
                attributedText.setText(messageText);
                attributedText.setFont(messageFont);
                attributedText.setColour(message.fromUser ? juce::Colours::black : juce::Colours::darkblue);
                textLayout.createLayout(attributedText, getWidth() - 40.0f);
                
                // Draw the text
                textLayout.draw(g, juce::Rectangle<float>(20, y, getWidth() - 40, textLayout.getHeight()));
                
                // Move down for the next message
                y += static_cast<int>(textLayout.getHeight()) + 20;
            }
        }
        
        void setMessages(const juce::Array<ConversationMessage>& msgs)
        {
            messages = msgs;
            repaint();
        }
        
    private:
        juce::Array<ConversationMessage> messages;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConversationPanel)
    };
    
    juce::Array<ConversationMessage> messages;
    ConversationPanel conversationPanel;
    juce::Viewport conversationViewport;
    
    // Template buttons for common prompts
    juce::TextButton rockButton;
    juce::TextButton jazzButton;
    juce::TextButton funkButton;
    juce::TextButton balladsButton;
    
    // References to LLM and Musician
    std::shared_ptr<LLMClient> llmClient;
    Musician* musician;
    
    // State
    bool waitingForResponse;
    
    // UI helpers
    void updateConversationDisplay();
    void handleSendMessage();
    void handleGeneratePattern();
    void handleClearConversation();
    void handlePlayPattern();
    
    // Add preset buttons for common music styles/types
    void addStylePresetButton(const String& text, const String& style);
    void handleStylePresetClicked(const String& style);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LLMConversationComponent)
};

} // namespace covertracks