#pragma once

#include <JuceHeader.h>
#include "../Core/Player.h"
#include "../Core/Instrument.h"
#include "PluginChainEditor.h"

namespace covertracks {

/**
 * @brief Simple editor for Player/Instrument
 * 
 * Focuses on the new clean architecture:
 * - Player metadata and orchestration settings
 * - Instrument plugin chain editing
 * - Simple, workable UI without complex graph editor
 */
class PlayerEditor : public juce::Component,
                             public juce::Button::Listener,
                             public juce::ComboBox::Listener
{
public:
    PlayerEditor();
    ~PlayerEditor() override;
    
    // Set the Player to edit
    void setPlayer(Player* player);
    Player* getPlayer() const { return currentPlayer; }
    
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
        virtual void playerEditorSaveRequested(PlayerEditor* editor) = 0;
        virtual void playerEditorCancelRequested(PlayerEditor* editor) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Data
    Player* currentPlayer = nullptr;
    
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
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerEditor)
};

} // namespace covertracks