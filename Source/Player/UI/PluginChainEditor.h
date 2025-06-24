#pragma once

#include <JuceHeader.h>
#include "../Core/Instrument.h"
#include "../Core/Player.h"
#include "../../Plugins/PluginGraph.h"  // For PluginDescriptionAndPreference
#include <optional>
#include <map>

namespace covertracks {

/**
 * @brief Simple plugin chain editor for Instrument
 * 
 * Displays plugins as a vertical list (chain) rather than a complex graph.
 * Automatically handles MIDI In -> Plugin Chain -> Audio Out flow.
 */
class PluginChainEditor : public juce::Component,
                         public juce::Button::Listener,
                         public juce::ListBoxModel
{
public:
    PluginChainEditor();
    ~PluginChainEditor() override;
    
    // Set the Player/Instrument to edit
    void setPlayer(Player* player);
    void setInstrument(Instrument* instrument);
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener implementation
    void buttonClicked(juce::Button* button) override;
    
    // ListBoxModel implementation
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    
private:
    // Data
    Player* currentPlayer = nullptr;
    Instrument* currentInstrument = nullptr;
    
    // UI Components
    juce::Label titleLabel;
    juce::Label flowLabel;
    juce::ListBox pluginListBox;
    juce::TextButton addPluginButton;
    juce::TextButton removePluginButton;
    juce::TextButton moveUpButton;
    juce::TextButton moveDownButton;
    juce::TextButton editPluginButton;
    
    // Plugin management
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    // External plugin mapping (for menu ID -> PluginDescription lookup)
    mutable std::map<int, juce::PluginDescription> externalPluginMap;
    

    
    void refreshPluginList();
    void showPluginMenu();
    void removeSelectedPlugin();
    void moveSelectedPlugin(bool moveUp);
    void editSelectedPlugin();
    void editPlugin(int index);
    void editTracktionPlugin(te::Plugin* plugin);
    
    // Plugin menu helpers (following existing app pattern)
    void addPluginsToMenu(juce::PopupMenu& menu);
    std::optional<PluginDescriptionAndPreference> getChosenType(int menuID) const;
    void startPluginScan();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainEditor)
};


} // namespace covertracks