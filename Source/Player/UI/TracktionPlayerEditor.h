#pragma once

#include <JuceHeader.h>
#include "../Core/TracktionPlayer.h"
#include "../Core/TracktionInstrument.h"
#include "PluginChainEditor.h"

namespace covertracks {

/**
 * @brief Simple editor for TracktionPlayer/TracktionInstrument
 * 
 * Focuses on the new clean architecture:
 * - TracktionPlayer metadata and orchestration settings
 * - TracktionInstrument plugin chain editing
 * - Simple, workable UI without complex graph editor
 */
class TracktionPlayerEditor : public juce::Component,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener
{
public:
    TracktionPlayerEditor();
    ~TracktionPlayerEditor() override;
    
    // Set the TracktionPlayer to edit
    void setPlayer(TracktionPlayer* player);
    TracktionPlayer* getPlayer() const { return currentPlayer; }
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener implementation
    void buttonClicked(juce::Button* button) override;
    
    // ComboBox::Listener implementation
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    // Listener interface for save/cancel events
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void tracktionPlayerEditorSaveRequested(TracktionPlayerEditor* editor) = 0;
        virtual void tracktionPlayerEditorCancelRequested(TracktionPlayerEditor* editor) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Data
    TracktionPlayer* currentPlayer = nullptr;
    
    // UI Components - Player metadata
    juce::Label titleLabel;
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::Label categoryLabel;
    juce::ComboBox categoryCombo;
    juce::Label descriptionLabel;
    juce::TextEditor descriptionEditor;
    
    // UI Components - Actions
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
    juce::TextButton testPlayerButton;
    
    // Plugin chain editor
    std::unique_ptr<PluginChainEditor> pluginChainEditor;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    // Update methods
    void updateUIFromPlayer();
    void updatePlayerFromUI();
    void testPlayer();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TracktionPlayerEditor)
};

} // namespace covertracks