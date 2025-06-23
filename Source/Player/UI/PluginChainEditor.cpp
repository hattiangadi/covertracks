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
    // Close all open plugin editors
    openEditors.clear();
    
    pluginListBox.setModel(nullptr);
}

void PluginChainEditor::setPlayer(TracktionPlayer* player)
{
    currentPlayer = player;
    currentInstrument = player ? player->getInstrument() : nullptr;
    refreshPluginList();
}

void PluginChainEditor::setInstrument(TracktionInstrument* instrument)
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

void PluginChainEditor::listBoxItemDoubleClicked(int row, const juce::MouseEvent& /*e*/)
{
    editPlugin(row);
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
        // Get the plugin before removing it
        auto* plugin = currentInstrument->getPlugin(selectedRow);
        
        if (currentInstrument->removePlugin(selectedRow))
        {
            std::cout << "Successfully removed plugin at index " << selectedRow << std::endl;
            
            // Close any open editor for this plugin
            if (plugin)
            {
                auto it = openEditors.find(plugin);
                if (it != openEditors.end())
                {
                    std::cout << "Closing editor for removed plugin: " << plugin->getName() << std::endl;
                    openEditors.erase(it);
                }
            }
            
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
        editPlugin(selectedRow);
    }
    else
    {
        std::cout << "No plugin selected for editing" << std::endl;
    }
}

void PluginChainEditor::editPlugin(int index)
{
    if (!currentInstrument)
    {
        std::cout << "No instrument available for plugin editing" << std::endl;
        return;
    }
    
    // Try to get either a Tracktion plugin or JUCE AudioProcessor
    auto* plugin = currentInstrument->getPlugin(index);
    auto* juceProcessor = currentInstrument->getJucePlugin(index);
    
    if (!plugin && !juceProcessor)
    {
        std::cout << "Invalid plugin index for editing: " << index << std::endl;
        return;
    }
    
    // Check for JUCE processor (external plugins)
    if (juceProcessor)
    {
        std::cout << "Creating editor for JUCE AudioProcessor: " << juceProcessor->getName() << std::endl;
        
        // Create editor window directly from JUCE AudioProcessor
        try 
        {
            auto editorWindow = std::make_unique<PluginEditorWindow>(juceProcessor);
            if (editorWindow)
            {
                editorWindow->setVisible(true);
                // Note: We're not tracking JUCE editors in openEditors map for now - could be improved
                std::cout << "Opened JUCE plugin editor for: " << juceProcessor->getName() << std::endl;
            }
            else
            {
                std::cout << "Failed to create editor window for JUCE processor: " << juceProcessor->getName() << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "Error creating JUCE plugin editor: " << e.what() << std::endl;
        }
    }
    else if (plugin)
    {
        // For external plugins where wrapped processor isn't available yet, 
        // try to force initialization first
        if (auto* externalPlugin = dynamic_cast<te::ExternalPlugin*>(plugin))
        {
            std::cout << "External plugin detected, attempting to ensure initialization..." << std::endl;
            
            // Give the plugin a moment to initialize if it's still loading
            juce::Timer::callAfterDelay(100, [this, plugin, index]()
            {
                auto* delayedProcessor = currentInstrument->getJucePlugin(index);
                if (delayedProcessor)
                {
                    std::cout << "External plugin processor now available: " << delayedProcessor->getName() << std::endl;
                    
                    try 
                    {
                        auto editorWindow = std::make_unique<PluginEditorWindow>(delayedProcessor);
                        if (editorWindow)
                        {
                            editorWindow->setVisible(true);
                            std::cout << "Opened delayed JUCE plugin editor for: " << delayedProcessor->getName() << std::endl;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cout << "Error creating delayed JUCE plugin editor: " << e.what() << std::endl;
                    }
                }
                else
                {
                    std::cout << "External plugin processor still not available after delay" << std::endl;
                    // Fall back to Tracktion plugin editor
                    editTracktionPlugin(plugin);
                }
            });
            
            return; // Don't continue to Tracktion editor creation
        }
        else
        {
            editTracktionPlugin(plugin);
        }
    }
}

void PluginChainEditor::editTracktionPlugin(te::Plugin* plugin)
{
    // Check if we already have an editor open for this Tracktion plugin
    auto it = openEditors.find(plugin);
    if (it != openEditors.end())
    {
        // Bring existing editor to front
        it->second->toFront(true);
        std::cout << "Bringing existing editor to front for: " << plugin->getName() << std::endl;
        return;
    }
    
    // Create new editor window for Tracktion plugin
    try 
    {
        auto editorWindow = std::make_unique<PluginEditorWindow>(plugin);
        if (editorWindow)
        {
            editorWindow->setVisible(true);
            openEditors[plugin] = std::move(editorWindow);
            std::cout << "Opened Tracktion plugin editor for: " << plugin->getName() << std::endl;
        }
        else
        {
            std::cout << "Failed to create editor window for Tracktion plugin: " << plugin->getName() << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "Error creating Tracktion plugin editor: " << e.what() << std::endl;
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
    
    // Add external plugins from KnownPluginList
    auto pluginTypes = knownPluginList.getTypes();
    
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
    // Load any previously scanned plugins
    juce::File pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                    .getChildFile("Covertracks")
                                    .getChildFile("PluginList.xml");
    
    if (pluginListFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(pluginListFile);
        if (xml != nullptr)
        {
            knownPluginList.recreateFromXml(*xml);
            std::cout << "Loaded " << knownPluginList.getNumTypes() << " previously scanned plugins" << std::endl;
        }
    }
    
    // Start background scanning for new plugins
    std::cout << "Starting plugin scan..." << std::endl;
    auto pluginFormats = formatManager.getFormats();
    std::cout << "Found " << pluginFormats.size() << " plugin formats" << std::endl;
    
    for (auto* format : pluginFormats)
    {
        std::cout << "Scanning format: " << format->getName() << std::endl;
        auto defaultLocations = format->getDefaultLocationsToSearch();
        std::cout << "  Search paths: " << defaultLocations.getNumPaths() << std::endl;
        
        // Use JUCE's built-in scanner for better reliability
        juce::PluginDirectoryScanner scanner(knownPluginList, *format, defaultLocations, true, juce::File());
        
        juce::String pluginBeingScanned;
        while (scanner.scanNextFile(false, pluginBeingScanned))
        {
            std::cout << "  Scanning: " << pluginBeingScanned << std::endl;
        }
        
        std::cout << "  Format scan complete" << std::endl;
    }
    
    // Save the updated plugin list
    pluginListFile.getParentDirectory().createDirectory();
    auto xml = knownPluginList.createXml();
    if (xml != nullptr)
    {
        xml->writeTo(pluginListFile);
        std::cout << "Plugin scan complete. Found " << knownPluginList.getNumTypes() << " total plugins" << std::endl;
        
        // Debug: List found plugins
        for (const auto& desc : knownPluginList.getTypes())
        {
            std::cout << "  Found: " << desc.name << " (" << desc.pluginFormatName << ")" << std::endl;
        }
    }
}

// =========================================================================
// PluginEditorWindow Implementation
// =========================================================================

PluginEditorWindow::PluginEditorWindow(te::Plugin* p)
    : juce::DocumentWindow(p ? p->getName() : "Plugin Editor", 
                          juce::Colours::lightgrey, 
                          juce::DocumentWindow::allButtons),
      plugin(p)
{
    if (!plugin)
    {
        std::cout << "PluginEditorWindow: Invalid plugin pointer" << std::endl;
        return;
    }
    
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    
    createEditor();
    
    // Center on screen
    centreWithSize(getWidth(), getHeight());
}

PluginEditorWindow::PluginEditorWindow(juce::AudioProcessor* proc)
    : juce::DocumentWindow(proc ? proc->getName() : "Plugin Editor", 
                          juce::Colours::lightgrey, 
                          juce::DocumentWindow::allButtons),
      processor(proc)
{
    if (!processor)
    {
        std::cout << "PluginEditorWindow: Invalid AudioProcessor pointer" << std::endl;
        return;
    }
    
    setUsingNativeTitleBar(true);
    setResizable(true, false);
    
    createJuceEditor();
    
    // Center on screen
    centreWithSize(getWidth(), getHeight());
}

PluginEditorWindow::~PluginEditorWindow()
{
    // Clean up editor before window is destroyed
    setContentOwned(nullptr, false);
    editor.reset();
}

void PluginEditorWindow::createEditor()
{
    if (!plugin)
    {
        std::cout << "PluginEditorWindow: No plugin available for editor creation" << std::endl;
        return;
    }
    
    // Try to get the plugin's AudioProcessor (for external plugins)
    auto* processor = plugin->getWrappedAudioProcessor();
    
    std::cout << "PluginEditorWindow: Creating editor for " << plugin->getName() 
              << " (wrapped processor: " << (processor ? "available" : "none") << ")" << std::endl;
    
    if (processor)
    {
        // External plugin - use JUCE AudioProcessor editor creation (same as GraphEditorPanel)
        std::cout << "PluginEditorWindow: External plugin detected, using JUCE editor creation" << std::endl;
        
        juce::AudioProcessorEditor* ui = nullptr;
        
        // Try to create native editor first
        if (processor->hasEditor())
        {
            std::cout << "PluginEditorWindow: Processor has editor, calling createEditorIfNeeded()" << std::endl;
            try
            {
                ui = processor->createEditorIfNeeded();
                if (ui)
                {
                    std::cout << "PluginEditorWindow: Native editor created successfully" << std::endl;
                    setContentOwned(ui, true);
                    setResizable(ui->isResizable(), false);
                    
                    // Size window to fit editor
                    setSize(ui->getWidth(), ui->getHeight() + getTitleBarHeight());
                    return;
                }
            }
            catch (const std::exception& e)
            {
                std::cout << "PluginEditorWindow: Exception creating native editor: " << e.what() << std::endl;
            }
        }
        
        // Fallback to generic editor
        std::cout << "PluginEditorWindow: Creating GenericAudioProcessorEditor for: " << plugin->getName() << std::endl;
        try
        {
            auto* genericEditor = new juce::GenericAudioProcessorEditor(*processor);
            genericEditor->setResizeLimits(200, 300, 1000, 800);
            setContentOwned(genericEditor, true);
            setResizable(true, false);
            setSize(400, 300);
            std::cout << "PluginEditorWindow: Generic editor created successfully" << std::endl;
            return;
        }
        catch (const std::exception& e)
        {
            std::cout << "PluginEditorWindow: Exception creating generic editor: " << e.what() << std::endl;
        }
    }
    
    // Built-in Tracktion plugin or fallback case
    std::cout << "PluginEditorWindow: Built-in Tracktion plugin or fallback case for: " << plugin->getName() << std::endl;
    
    // Create an informative display
    auto infoComponent = std::make_unique<juce::Component>();
    auto infoLabel = std::make_unique<juce::Label>();
    
    juce::String message;
    if (processor)
    {
        message = "Plugin: " + plugin->getName() + "\n\nThis plugin doesn't provide a custom editor interface.\nGeneric parameter editor creation failed.";
    }
    else
    {
        message = "Plugin: " + plugin->getName() + "\n\nThis is a built-in Tracktion plugin.\nParameter editing interface will be available in a future update.";
    }
    
    infoLabel->setText(message, juce::dontSendNotification);
    infoLabel->setJustificationType(juce::Justification::centred);
    infoLabel->setBounds(10, 10, 380, 280);
    
    infoComponent->addAndMakeVisible(infoLabel.release());
    infoComponent->setSize(400, 300);
    
    setContentOwned(infoComponent.release(), true);
    setSize(400, 350);
}

void PluginEditorWindow::createJuceEditor()
{
    if (!processor)
    {
        std::cout << "PluginEditorWindow: No AudioProcessor available for editor creation" << std::endl;
        return;
    }
    
    std::cout << "PluginEditorWindow: Creating JUCE editor for: " << processor->getName() << std::endl;
    
    juce::AudioProcessorEditor* ui = nullptr;
    
    // Try to create native editor first
    if (processor->hasEditor())
    {
        std::cout << "PluginEditorWindow: AudioProcessor has editor, calling createEditorIfNeeded()" << std::endl;
        try
        {
            ui = processor->createEditorIfNeeded();
            if (ui)
            {
                std::cout << "PluginEditorWindow: Native editor created successfully" << std::endl;
                setContentOwned(ui, true);
                setResizable(ui->isResizable(), false);
                
                // Size window to fit editor
                setSize(ui->getWidth(), ui->getHeight() + getTitleBarHeight());
                return;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "PluginEditorWindow: Exception creating native editor: " << e.what() << std::endl;
        }
    }
    
    // Fallback to generic editor
    std::cout << "PluginEditorWindow: Creating GenericAudioProcessorEditor for: " << processor->getName() << std::endl;
    try
    {
        auto* genericEditor = new juce::GenericAudioProcessorEditor(*processor);
        genericEditor->setResizeLimits(200, 300, 1000, 800);
        setContentOwned(genericEditor, true);
        setResizable(true, false);
        setSize(400, 300);
        std::cout << "PluginEditorWindow: Generic editor created successfully" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "PluginEditorWindow: Exception creating generic editor: " << e.what() << std::endl;
        
        // Final fallback
        auto infoLabel = std::make_unique<juce::Label>();
        infoLabel->setText("Failed to create editor for: " + processor->getName(), juce::dontSendNotification);
        infoLabel->setJustificationType(juce::Justification::centred);
        infoLabel->setSize(300, 100);
        
        setContentOwned(infoLabel.release(), true);
        setSize(300, 150);
    }
}

void PluginEditorWindow::closeButtonPressed()
{
    // Window will be cleaned up by the PluginChainEditor
    setVisible(false);
}

} // namespace covertracks