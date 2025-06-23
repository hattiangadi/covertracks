#pragma once

#include <JuceHeader.h>
#include "../Core/TracktionInstrument.h"
#include "../Core/TracktionPlayer.h"
#include "../../Plugins/PluginGraph.h"  // For PluginDescriptionAndPreference
#include <optional>
#include <map>

// Forward declare plugin editor window
namespace covertracks {
    class PluginEditorWindow;
}

namespace covertracks {

/**
 * @brief Simple plugin chain editor for TracktionInstrument
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
    
    // Set the TracktionPlayer/TracktionInstrument to edit
    void setPlayer(TracktionPlayer* player);
    void setInstrument(TracktionInstrument* instrument);
    
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
    TracktionPlayer* currentPlayer = nullptr;
    TracktionInstrument* currentInstrument = nullptr;
    
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
    
    // Plugin editor windows (keep track of open editors)
    std::map<te::Plugin*, std::unique_ptr<PluginEditorWindow>> openEditors;
    
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

/**
 * @brief Simple plugin editor window
 * 
 * Shows a plugin's editor UI in a resizable window
 */
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow(te::Plugin* plugin);
    PluginEditorWindow(juce::AudioProcessor* processor);
    ~PluginEditorWindow() override;
    
    // DocumentWindow overrides
    void closeButtonPressed() override;
    
    te::Plugin* getPlugin() const { return plugin; }
    
private:
    te::Plugin* plugin = nullptr;
    juce::AudioProcessor* processor = nullptr;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    
    void createEditor();
    void createJuceEditor();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

} // namespace covertracks