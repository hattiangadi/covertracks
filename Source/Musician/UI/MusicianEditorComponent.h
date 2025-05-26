// Source/Musician/UI/MusicianEditorComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/Musician.h"
#include "../../LLM/Core/LLMClient.h"

namespace covertracks {

/**
 * @brief UI Component for editing and interacting with a Musician
 * 
 * This component provides an interface for configuring a Musician and
 * generating patterns through conversation with an LLM.
 */
class MusicianEditorComponent : public Component,
                              public Button::Listener,
                              public ComboBox::Listener,
                              private ChangeListener
{
public:
    /**
     * @brief Construct a new Musician Editor Component
     * 
     * @param musician The musician to edit
     * @param llmClient Shared pointer to the LLM client
     */
    MusicianEditorComponent(Musician* musician, std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Destroy the Musician Editor Component
     */
    ~MusicianEditorComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(Button* button) override;
    
    // ComboBox::Listener
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    
    // Musician access
    Musician* getMusician() const { return musician; }
    
private:
    // ChangeListener
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    // UI components
    Label styleLabel;
    ComboBox styleComboBox;
    
    Label apiKeyLabel;
    TextEditor apiKeyEditor;
    
    TextButton connectButton;
    TextButton disconnectButton;
    TextButton saveButton;
    
    // Status indicators
    Label statusLabel;
    
    // LLM connection state
    
    // References
    Musician* musician;
    std::shared_ptr<LLMClient> llmClient;
    
    // State
    bool connected;
    
    // Helper methods
    void updateConnectionStatus();
    void handleConnect();
    void handleDisconnect();
    void handleSave();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MusicianEditorComponent)
};

} // namespace covertracks
