#include "PluginChainEditor.h"
#include <iostream>

namespace covertracks {

PluginChainEditor::PluginChainEditor()
{
    // Set up title
    titleLabel.setText("Plugin Chain Editor", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up flow label
    flowLabel.setText("MIDI In -> Plugin Chain -> Audio Out", juce::dontSendNotification);
    flowLabel.setFont(juce::FontOptions(12.0f).withStyle("Italic"));
    flowLabel.setJustificationType(juce::Justification::centred);
    flowLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(flowLabel);
    
    // Set up plugin list
    pluginListBox.setModel(this);
    pluginListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::darkgrey);
    pluginListBox.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(pluginListBox);
    
    // Set up buttons
    addPluginButton.setButtonText("Add Plugin");
    addPluginButton.addListener(this);
    addAndMakeVisible(addPluginButton);
    
    removePluginButton.setButtonText("Remove");
    removePluginButton.addListener(this);
    addAndMakeVisible(removePluginButton);
    
    moveUpButton.setButtonText("Move Up");
    moveUpButton.addListener(this);
    addAndMakeVisible(moveUpButton);
    
    moveDownButton.setButtonText("Move Down");
    moveDownButton.addListener(this);
    addAndMakeVisible(moveDownButton);
    
    editPluginButton.setButtonText("Edit");
    editPluginButton.addListener(this);
    addAndMakeVisible(editPluginButton);
    
    // Initialize plugin formats (following existing app pattern)
    formatManager.addDefaultFormats();
    
    // Start background plugin scanning
    startPluginScan();
    
    std::cout << "PluginChainEditor initialized with " << knownPluginList.getNumTypes() << " available plugins" << std::endl;
}

PluginChainEditor::~PluginChainEditor()
{
    pluginListBox.setModel(nullptr);
}

void PluginChainEditor::setPlayer(Player* player)
{
    currentPlayer = player;
    currentInstrument = player ? player->getInstrument() : nullptr;
    refreshPluginList();
}

void PluginChainEditor::setInstrument(Instrument* instrument)
{
    currentInstrument = instrument;
    currentPlayer = nullptr; // Direct instrument editing
    refreshPluginList();
}

void PluginChainEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw border
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), 2);
    
    // Draw chain flow diagram
    auto chainArea = getLocalBounds().removeFromTop(80).reduced(10);
    chainArea.removeFromTop(30); // Skip title
    chainArea.removeFromTop(20); // Skip flow label
    
    g.setColour(juce::Colours::lightblue);
    g.setFont(juce::FontOptions(11.0f));
    
    // Draw simple chain flow boxes
    auto boxWidth = 60;
    auto boxHeight = 20;
    auto spacing = 10;
    auto totalWidth = boxWidth * 3 + spacing * 4; // MIDI + Chain + Audio + spacing
    auto startX = (chainArea.getWidth() - totalWidth) / 2;
    
    // MIDI In box
    juce::Rectangle<int> midiBox(startX, 5, boxWidth, boxHeight);
    g.drawRect(midiBox);
    g.drawText("MIDI In", midiBox, juce::Justification::centred);
    
    // Arrow
    g.drawArrow(juce::Line<float>(midiBox.getRight() + 2, midiBox.getCentreY(),
                                midiBox.getRight() + spacing - 2, midiBox.getCentreY()), 2.0f, 6.0f, 6.0f);
    
    // Plugin Chain box
    juce::Rectangle<int> chainBox(midiBox.getRight() + spacing, 5, boxWidth, boxHeight);
    g.drawRect(chainBox);
    g.drawText("Plugins", chainBox, juce::Justification::centred);
    
    // Arrow
    g.drawArrow(juce::Line<float>(chainBox.getRight() + 2, chainBox.getCentreY(),
                                chainBox.getRight() + spacing - 2, chainBox.getCentreY()), 2.0f, 6.0f, 6.0f);
    
    // Audio Out box
    juce::Rectangle<int> audioBox(chainBox.getRight() + spacing, 5, boxWidth, boxHeight);
    g.drawRect(audioBox);
    g.drawText("Audio Out", audioBox, juce::Justification::centred);
}

void PluginChainEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title and flow description
    titleLabel.setBounds(area.removeFromTop(25));
    flowLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(35); // Space for chain diagram
    
    // Button strip at bottom
    auto buttonArea = area.removeFromBottom(40);
    auto buttonWidth = buttonArea.getWidth() / 5;
    
    addPluginButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    removePluginButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    moveUpButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    moveDownButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    editPluginButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    
    // Plugin list fills remaining space
    area.removeFromTop(10);
    pluginListBox.setBounds(area);
}

void PluginChainEditor::buttonClicked(juce::Button* button)
{
    if (button == &addPluginButton)
    {
        showPluginMenu();
    }
    else if (button == &removePluginButton)
    {
        removeSelectedPlugin();
    }
    else if (button == &moveUpButton)
    {
        moveSelectedPlugin(true);
    }
    else if (button == &moveDownButton)
    {
        moveSelectedPlugin(false);
    }
    else if (button == &editPluginButton)
    {
        editSelectedPlugin();
    }
}

int PluginChainEditor::getNumRows()
{
    if (!currentInstrument)
        return 0;
        
    return currentInstrument->getPluginCount();
}

void PluginChainEditor::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (!currentInstrument || rowNumber >= currentInstrument->getPluginCount())
        return;
        
    // Background
    if (rowIsSelected)
        g.fillAll(juce::Colours::lightblue.withAlpha(0.3f));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colours::lightgrey.withAlpha(0.1f));
        
    // Plugin info
    auto pluginNames = currentInstrument->getPluginNames();
    if (rowNumber < pluginNames.size())
    {
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(14.0f));
        
        // Draw plugin number and name
        juce::String displayText = juce::String(rowNumber + 1) + ". " + pluginNames[rowNumber];
        g.drawText(displayText, 10, 0, width - 20, height, juce::Justification::centredLeft);
    }
    
    // Border
    g.setColour(juce::Colours::grey);
    g.drawHorizontalLine(height - 1, 0, width);
}

void PluginChainEditor::listBoxItemClicked(int row, const juce::MouseEvent& /*e*/)
{
    pluginListBox.selectRow(row);
}

void PluginChainEditor::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (currentInstrument)
        if (auto* plugin = currentInstrument->getPlugin(row))
            plugin->windowState->showWindowExplicitly();
}

void PluginChainEditor::refreshPluginList()
{
    pluginListBox.updateContent();
    repaint();
}

void PluginChainEditor::showPluginMenu()
{
    if (!currentInstrument)
    {
        std::cout << "No instrument available for plugin addition" << std::endl;
        return;
    }
    
    juce::PopupMenu menu;
    
    // Use proper plugin scanning like the existing app
    addPluginsToMenu(menu);
    
    // Show menu with proper callback
    menu.showMenuAsync(juce::PopupMenu::Options(),
                      [this](int result) {
                          if (result == -2) // "Scan for plugins..." option
                          {
                              std::cout << "Rescanning for plugins..." << std::endl;
                              startPluginScan();
                              std::cout << "Plugin rescan complete. Found " << knownPluginList.getNumTypes() << " total plugins" << std::endl;
                          }
                          else if (result > 0)
                          {
                              auto chosen = getChosenType(result);
                              if (chosen.has_value())
                              {
                                  std::cout << "Attempting to add plugin: " << chosen->pluginDescription.name 
                                           << " (format: " << chosen->pluginDescription.pluginFormatName << ")" << std::endl;
                                  
                                  // Add plugin to instrument using the proper description
                                  std::cout << "PluginChainEditor: About to add plugin: " << chosen->pluginDescription.name 
                                           << " (format: " << chosen->pluginDescription.pluginFormatName << ")" << std::endl;
                                  std::cout << "Plugin ID: " << chosen->pluginDescription.fileOrIdentifier << std::endl;
                                  
                                  if (currentInstrument->addPlugin(chosen->pluginDescription))
                                  {
                                      std::cout << "PluginChainEditor: Successfully added plugin: " << chosen->pluginDescription.name << std::endl;
                                      refreshPluginList();
                                  }
                                  else
                                  {
                                      std::cout << "PluginChainEditor: Failed to add plugin: " << chosen->pluginDescription.name << std::endl;
                                  }
                              }
                              else
                              {
                                  std::cout << "Could not find plugin description for menu ID: " << result << std::endl;
                              }
                          }
                      });
}

void PluginChainEditor::removeSelectedPlugin()
{
    int selectedRow = pluginListBox.getSelectedRow();
    if (selectedRow >= 0 && currentInstrument)
    {
        if (currentInstrument->removePlugin(selectedRow))
        {
            std::cout << "Successfully removed plugin at index " << selectedRow << std::endl;
            
            refreshPluginList();
            
            // Update selection to stay within bounds
            int newPluginCount = currentInstrument->getPluginCount();
            if (newPluginCount > 0)
            {
                int newSelection = std::min(selectedRow, newPluginCount - 1);
                pluginListBox.selectRow(newSelection);
            }
        }
        else
        {
            std::cout << "Failed to remove plugin at index " << selectedRow << std::endl;
        }
    }
    else
    {
        std::cout << "No plugin selected for removal" << std::endl;
    }
}

void PluginChainEditor::moveSelectedPlugin(bool moveUp)
{
    int selectedRow = pluginListBox.getSelectedRow();
    if (selectedRow >= 0 && currentInstrument)
    {
        int pluginCount = currentInstrument->getPluginCount();
        int targetIndex = moveUp ? selectedRow - 1 : selectedRow + 1;
        
        // Check bounds
        if (targetIndex < 0 || targetIndex >= pluginCount)
        {
            std::cout << "Cannot move plugin: target index out of bounds" << std::endl;
            return;
        }
        
        if (currentInstrument->movePlugin(selectedRow, targetIndex))
        {
            std::cout << "Successfully moved plugin from index " << selectedRow << " to " << targetIndex << std::endl;
            refreshPluginList();
            
            // Update selection to follow the moved plugin
            pluginListBox.selectRow(targetIndex);
        }
        else
        {
            std::cout << "Failed to move plugin from index " << selectedRow << " to " << targetIndex << std::endl;
        }
    }
    else
    {
        std::cout << "No plugin selected for moving" << std::endl;
    }
}

void PluginChainEditor::editSelectedPlugin()
{
    int selectedRow = pluginListBox.getSelectedRow();
    if (selectedRow >= 0)
    {
        if (currentInstrument)
        {
            if (auto* plugin = currentInstrument->getPlugin(selectedRow))
            {
                // Check if it's an external plugin that's still initializing
                if (auto* externalPlugin = dynamic_cast<te::ExternalPlugin*>(plugin))
                {
                    if (externalPlugin->isInitialisingAsync())
                    {
                        std::cout << "Plugin is still initializing, please wait..." << std::endl;
                        // Could add a timer here to retry later, but for now just inform user
                        return;
                    }
                    
                    if (externalPlugin->getAudioPluginInstance() == nullptr)
                    {
                        std::cout << "External plugin AudioPluginInstance not available" << std::endl;
                        return;
                    }
                }
                
                plugin->windowState->showWindowExplicitly();
            }
        }
    }
    else
    {
        std::cout << "No plugin selected for editing" << std::endl;
    }
}

void PluginChainEditor::addPluginsToMenu(juce::PopupMenu& menu)
{
    // Clear previous mappings
    externalPluginMap.clear();
    
    // Add built-in Tracktion plugins first
    menu.addSectionHeader("Built-in Synthesizers");
    menu.addItem(1, "4-Oscillator Synth");
    menu.addItem(2, "Basic Sampler");
    
    menu.addSeparator();
    menu.addSectionHeader("Built-in Effects");
    menu.addItem(10, "Reverb");
    menu.addItem(11, "Delay");
    menu.addItem(12, "Low Pass Filter");
    
    // Add external plugins from Engine's KnownPluginList
    auto& audioEngine = covertracks::getAudioEngine();
    auto& tracktionEngine = audioEngine.getTracktionEngine();
    auto pluginTypes = tracktionEngine.getPluginManager().knownPluginList.getTypes();
    
    juce::Array<juce::PluginDescription> externalPlugins;
    for (const auto& desc : pluginTypes)
    {
        // Skip internal/built-in formats to avoid duplication
        if (desc.pluginFormatName != "Internal" && desc.pluginFormatName != te::PluginManager::builtInPluginFormatName)
        {
            externalPlugins.add(desc);
        }
    }
    
    if (externalPlugins.size() > 0)
    {
        menu.addSeparator();
        menu.addSectionHeader("External Plugins (" + juce::String(externalPlugins.size()) + ")");
        
        // Start external plugin IDs from 100 to avoid conflicts with built-ins
        int menuItemId = 100;
        
        for (const auto& desc : externalPlugins)
        {
            juce::String menuText = desc.name;
            if (desc.manufacturerName.isNotEmpty())
                menuText += " (" + desc.manufacturerName + ")";
            
            // Add format info to help identify plugin types
            menuText += " [" + desc.pluginFormatName + "]";
                
            menu.addItem(menuItemId, menuText);
            
            // Store mapping for later retrieval
            externalPluginMap[menuItemId] = desc;
            menuItemId++;
        }
    }
    else
    {
        menu.addSeparator();
        menu.addSectionHeader("External Plugins");
        menu.addItem(-1, "No external plugins found", false, false);
        menu.addItem(-2, "Scan for plugins...", true, false);
    }
}

std::optional<PluginDescriptionAndPreference> PluginChainEditor::getChosenType(int menuID) const
{
    // Handle built-in plugins
    if (menuID <= 50)  // Built-in range
    {
        juce::PluginDescription desc;
        desc.pluginFormatName = te::PluginManager::builtInPluginFormatName;
        
        switch (menuID)
        {
            case 1:
                desc.name = "4-Oscillator Synth";
                desc.category = te::FourOscPlugin::xmlTypeName;
                desc.isInstrument = true;
                break;
            case 2:
                desc.name = "Basic Sampler";
                desc.category = te::SamplerPlugin::xmlTypeName;
                desc.isInstrument = true;
                break;
            case 10:
                desc.name = "Reverb";
                desc.category = te::ReverbPlugin::xmlTypeName;
                desc.isInstrument = false;
                break;
            case 11:
                desc.name = "Delay";
                desc.category = te::DelayPlugin::xmlTypeName;
                desc.isInstrument = false;
                break;
            case 12:
                desc.name = "Low Pass Filter";
                desc.category = te::LowPassPlugin::xmlTypeName;
                desc.isInstrument = false;
                break;
            default:
                return std::nullopt;
        }
        
        return PluginDescriptionAndPreference{desc};
    }
    
    // Handle external plugins
    auto it = externalPluginMap.find(menuID);
    if (it != externalPluginMap.end())
    {
        return PluginDescriptionAndPreference{it->second};
    }
    
    return std::nullopt;
}

void PluginChainEditor::startPluginScan()
{
    // Use the Engine's built-in plugin management system
    auto& audioEngine = covertracks::getAudioEngine();
    auto& tracktionEngine = audioEngine.getTracktionEngine();
    
    auto& pluginManager = tracktionEngine.getPluginManager();
    
    // Trigger scanning through the Engine's plugin system
    std::cout << "Starting plugin scan using Tracktion Engine..." << std::endl;
    auto pluginFormats = pluginManager.pluginFormatManager.getFormats();
    std::cout << "Found " << pluginFormats.size() << " plugin formats" << std::endl;
    
    for (auto* format : pluginFormats)
    {
        std::cout << "Scanning format: " << format->getName() << std::endl;
        auto defaultLocations = format->getDefaultLocationsToSearch();
        
        // Use JUCE's built-in scanner with the Engine's KnownPluginList
        juce::PluginDirectoryScanner scanner(pluginManager.knownPluginList, *format, defaultLocations, true, juce::File());
        
        juce::String pluginBeingScanned;
        int numScanned = 0;
        while (scanner.scanNextFile(false, pluginBeingScanned) && numScanned < 20)
        {
            std::cout << "  Scanning: " << pluginBeingScanned << std::endl;
            numScanned++;
        }
        
        std::cout << "  Format scan complete" << std::endl;
    }
    
    std::cout << "Plugin scan complete. Total plugins found: " << pluginManager.knownPluginList.getNumTypes() << std::endl;
}

} // namespace covertracks