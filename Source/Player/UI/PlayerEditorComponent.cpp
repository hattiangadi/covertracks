// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"

PlayerEditorComponent::PlayerEditorComponent(PlayerManager::Player* playerToEdit)
{
    // Initialize the player data
    if (playerToEdit != nullptr)
    {
        player = std::make_unique<PlayerManager::Player>();
        player->metadata = playerToEdit->metadata;
        
        if (playerToEdit->graphState != nullptr)
            player->graphState = std::make_unique<XmlElement>(*playerToEdit->graphState);
    }
    else
    {
        // Create a new player with default settings
        player = std::make_unique<PlayerManager::Player>();
        player->metadata.name = "New Player";
        player->metadata.category = "Keyboard";
        player->metadata.description = "A new player configuration";
        player->metadata.created = Time::getCurrentTime();
        player->metadata.lastModified = player->metadata.created;
        
        player->graphState = std::make_unique<XmlElement>("FILTERGRAPH");
    }
    
    // Set up name editor
    nameEditor.setText(player->metadata.name);
    nameEditor.setSelectAllWhenFocused(true);
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(nameEditor);
    
    // Set up category combo
    categoryCombo.addItem("Keyboard", 1);
    categoryCombo.addItem("Guitar", 2);
    categoryCombo.addItem("Bass", 3);
    categoryCombo.addItem("Drums", 4);
    categoryCombo.addItem("Vocals", 5);
    categoryCombo.addItem("Synth", 6);
    categoryCombo.addItem("Brass", 7);
    categoryCombo.addItem("Strings", 8);
    categoryCombo.addItem("Woodwind", 9);
    categoryCombo.addItem("Other", 10);
    
    categoryCombo.setText(player->metadata.category);
    addAndMakeVisible(categoryLabel);
    addAndMakeVisible(categoryCombo);
    
    // Set up description editor
    descriptionEditor.setText(player->metadata.description);
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.setSelectAllWhenFocused(true);
    addAndMakeVisible(descriptionLabel);
    addAndMakeVisible(descriptionEditor);
    
    // Set up plugin graph editor
    addAndMakeVisible(pluginLabel);
    
    // Initialize format manager with all needed formats
    AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();
    formatManager.addFormat(new InternalPluginFormat());
    
    // Create a default audio settings file if it doesn't exist
    File audioSettingsFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                             .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                             .getChildFile("AudioSettings.xml");
    
    std::unique_ptr<XmlElement> savedState;
    if (audioSettingsFile.existsAsFile())
    {
        savedState = XmlDocument::parse(audioSettingsFile);
    }
    else
    {
        // Create a default audio settings configuration
        savedState = std::make_unique<XmlElement>("DEVICESETUP");
        savedState->setAttribute("audioOutputDeviceName", "");
        savedState->setAttribute("audioInputDeviceName", "");
        savedState->setAttribute("audioDeviceRate", 44100.0);
        savedState->setAttribute("audioDeviceBufferSize", 512);
        
        // Save the default settings
        audioSettingsFile.create();
        FileOutputStream stream(audioSettingsFile);
        if (stream.openedOk())
            savedState->writeTo(stream);
    }
    
    // Create audio device manager
    auto* deviceManager = new AudioDeviceManager();
    deviceManager->initialise(2, 2, savedState.get(), true);
    
    // Get plugin list
    KnownPluginList knownPluginList;
    
    // Try to load plugin list from a file
    File pluginListFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                         .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                         .getChildFile("PluginList.xml");
    
    if (pluginListFile.existsAsFile())
    {
        if (auto savedPluginList = XmlDocument::parse(pluginListFile))
            knownPluginList.recreateFromXml(*savedPluginList);
    }
    else
    {
        // Create a default empty plugin list
        auto emptyList = std::make_unique<XmlElement>("KNOWNPLUGINS");
        pluginListFile.create();
        FileOutputStream stream(pluginListFile);
        if (stream.openedOk())
            emptyList->writeTo(stream);
    }
    
    // Create graph editor
    graphEditor = std::make_unique<GraphDocumentComponent>(formatManager, *deviceManager, knownPluginList);
    
    // Make the graph editor visually distinct
    graphEditor->setColour(ResizableWindow::backgroundColourId, Colours::lightgrey);
    
    // Initialize the graph with a safe approach
    if (auto* graph = graphEditor->graph.get())
    {
        // First clear the graph to start with a clean state
        graph->clear();
        
        // If we have an existing graph state, try to load it
        if (player->graphState != nullptr && player->graphState->hasTagName("FILTERGRAPH"))
        {
            // Check if the graph state has the necessary structure
            bool hasValidStructure = player->graphState->getChildByName("CONNECTIONS") != nullptr &&
                                    player->graphState->getChildByName("INPUTS") != nullptr &&
                                    player->graphState->getChildByName("OUTPUTS") != nullptr;
            
            if (hasValidStructure)
            {
                // Only try to restore if the structure is valid
                graph->restoreFromXml(*player->graphState);
                
                // Make sure the graph panel updates to show the loaded configuration
                if (graphEditor->graphPanel != nullptr)
                    graphEditor->graphPanel->updateComponents();
            }
            else
            {
                // Create default graph since the existing one isn't valid
                createDefaultGraph(graph);
            }
        }
        else
        {
            // Create a new default graph
            createDefaultGraph(graph);
        }
    }
    
    addAndMakeVisible(graphEditor.get());
    
    // We don't need to add a mouse listener since the GraphEditorPanel already handles right-clicks
    // Just make sure we set up the graph panel correctly
    if (graphEditor->graphPanel != nullptr)
        graphEditor->graphPanel->updateComponents();
    
    // Set up buttons
    addAndMakeVisible(saveButton);
    addAndMakeVisible(cancelButton);
    
    saveButton.addListener(this);
    cancelButton.addListener(this);
}

PlayerEditorComponent::~PlayerEditorComponent()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
}

void PlayerEditorComponent::createDefaultGraph(PluginGraph* graph)
{
    // Set up a new default graph
    InternalPluginFormat internalFormat;
    auto internalTypes = internalFormat.getAllTypes();
    
    if (internalTypes.size() > 4)
    {
        // Add standard nodes: Audio Input, Audio Output, and Sine Wave Synth
        graph->addPlugin(PluginDescriptionAndPreference{internalTypes[0]}, {0.25, 0.1});  // Audio Input
        graph->addPlugin(PluginDescriptionAndPreference{internalTypes[2]}, {0.25, 0.9});  // Audio Output
        graph->addPlugin(PluginDescriptionAndPreference{internalTypes[4]}, {0.5, 0.5});   // Sine Wave Synth
        
        // Wait for plugins to initialize
        Thread::sleep(500);
        
        // Create the default connections between components
        makeDefaultConnections(graph);
        
        // Make sure the graph panel updates to show the nodes
        if (graphEditor->graphPanel != nullptr)
            graphEditor->graphPanel->updateComponents();
    }
}

void PlayerEditorComponent::makeDefaultConnections(PluginGraph* graph)
{
    // Find the nodes by their processor types
    AudioProcessorGraph::Node::Ptr audioInputNode;
    AudioProcessorGraph::Node::Ptr audioOutputNode;
    AudioProcessorGraph::Node::Ptr synthNode;
    
    // Iterate through all nodes in the graph
    for (auto* node : graph->graph.getNodes())
    {
        auto name = node->getProcessor()->getName();
        
        if (name == "Audio Input")
            audioInputNode = node;
        else if (name == "Audio Output")
            audioOutputNode = node;
        else if (name == "Sine Wave Synth")
            synthNode = node;
    }
    
    // Connect the nodes if they exist
    if (audioInputNode != nullptr && synthNode != nullptr && audioOutputNode != nullptr)
    {
        // Connect audio input to synth
        graph->graph.addConnection({
            {audioInputNode->nodeID, 0}, // Audio input output channel 0
            {synthNode->nodeID, 0}  // Synth input channel 0
        });
        
        // Connect synth to audio output
        graph->graph.addConnection({
            {synthNode->nodeID, 0}, // Synth output channel 0
            {audioOutputNode->nodeID, 0}  // Audio output input channel 0
        });
        
        // Connect second channel if available
        if (synthNode->getProcessor()->getTotalNumOutputChannels() > 1 &&
            audioOutputNode->getProcessor()->getTotalNumInputChannels() > 1)
        {
            graph->graph.addConnection({
                {synthNode->nodeID, 1}, // Synth output channel 1
                {audioOutputNode->nodeID, 1}  // Audio output input channel 1
            });
        }
    }
}



void PlayerEditorComponent::mouseDown(const MouseEvent& e)
{
    // Check if this is a right-click on the graph editor area
    if (e.mods.isPopupMenu() && graphEditor != nullptr && graphEditor->graphPanel != nullptr)
    {
        // Find the component that was clicked
        auto* component = getComponentAt(e.getPosition());
        
        // Check if the click was on the graph editor or one of its children
        if (component == graphEditor.get() || 
            (component != nullptr && graphEditor->isParentOf(component)))
        {
            // Convert the mouse position to graph panel coordinates
            auto graphPanelPos = graphEditor->graphPanel->getLocalPoint(this, e.getPosition());
            
            // Show the plugin menu
            showPluginMenu(graphPanelPos);
        }
    }
}

void PlayerEditorComponent::showPluginMenu(Point<int> position)
{
    if (graphEditor == nullptr || graphEditor->graphPanel == nullptr)
        return;
    
    PopupMenu menu;
    
    // Add internal plugins
    InternalPluginFormat internalFormat;
    auto internalTypes = internalFormat.getAllTypes();
    
    // First add internal plugins directly to the menu
    int menuID = 1;
    for (auto& type : internalTypes)
    {
        menu.addItem(menuID++, type.name + " (" + type.pluginFormatName + ")");
    }
    
    menu.addSeparator();
    
    // Get the plugin list from the AudioPluginFormatManager
    KnownPluginList knownPluginList;
    
    // Try to load plugin list from a file
    File pluginListFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                         .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                         .getChildFile("PluginList.xml");
    
    if (pluginListFile.existsAsFile())
    {
        if (auto savedPluginList = XmlDocument::parse(pluginListFile))
            knownPluginList.recreateFromXml(*savedPluginList);
    }
    
    auto pluginDescriptions = knownPluginList.getTypes();
    
    // This avoids showing the internal types again later on in the list
    pluginDescriptions.removeIf([](PluginDescription& desc)
    {
        return desc.pluginFormatName == InternalPluginFormat::getIdentifier();
    });
    
    // Create a tree structure for the plugins
    auto tree = KnownPluginList::createTree(pluginDescriptions, KnownPluginList::sortByCategory);
    Array<PluginDescriptionAndPreference> pluginDescriptionsAndPreference;
    
    // Add the plugins to the menu
    addPluginsToMenu(*tree, menu, pluginDescriptions, pluginDescriptionsAndPreference, menuID);
    
    menu.showMenuAsync(PopupMenu::Options(),
                      [this, position, internalTypes, pluginDescriptionsAndPreference](int result)
                      {
                          // Handle internal plugins (first in the menu)
                          int internalIndex = result - 1;
                          
                          if (isPositiveAndBelow(internalIndex, internalTypes.size()))
                          {
                              // Add the selected internal plugin to the graph
                              if (graphEditor != nullptr && graphEditor->graph != nullptr)
                              {
                                  graphEditor->createNewPlugin(PluginDescriptionAndPreference{internalTypes[(size_t)internalIndex]}, position);
                              }
                              return;
                          }
                          
                          // Handle external plugins
                          int externalIndex = result - (int)internalTypes.size() - 1;
                          
                          if (isPositiveAndBelow(externalIndex, pluginDescriptionsAndPreference.size()))
                          {
                              // Add the selected external plugin to the graph
                              if (graphEditor != nullptr && graphEditor->graph != nullptr)
                              {
                                  graphEditor->createNewPlugin(pluginDescriptionsAndPreference[externalIndex], position);
                              }
                          }
                      });
}

void PlayerEditorComponent::addPluginsToMenu(const KnownPluginList::PluginTree& tree,
                                      PopupMenu& menu,
                                      const Array<PluginDescription>& allPlugins,
                                      Array<PluginDescriptionAndPreference>& addedPlugins,
                                      int& nextMenuID)
{
    // Add subfolders first
    for (auto* subFolder : tree.subFolders)
    {
        PopupMenu subMenu;
        addPluginsToMenu(*subFolder, subMenu, allPlugins, addedPlugins, nextMenuID);
        
        menu.addSubMenu(subFolder->folder, subMenu, true, nullptr, false, 0);
    }
    
    // Then add plugins in this folder
    for (const auto& plugin : tree.plugins)
    {
        // Create a descriptive name for the plugin
        String name = plugin.name;
        
        // Check if there are duplicate names in this folder
        bool hasDuplicates = false;
        for (const auto& otherPlugin : tree.plugins)
        {
            if (&otherPlugin != &plugin && otherPlugin.name == plugin.name)
            {
                hasDuplicates = true;
                break;
            }
        }
        
        // If there are duplicates, add the format name to distinguish them
        if (hasDuplicates)
            name << " (" << plugin.pluginFormatName << ")";
        
        // Add the plugin to the menu and store its description
        menu.addItem(nextMenuID, name);
        addedPlugins.add(PluginDescriptionAndPreference{plugin});
        ++nextMenuID;
    }
}

void PlayerEditorComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Header area
    area.removeFromTop(40); // Header height
    
    area.removeFromTop(10); // Spacing after header
    
    // Form fields section - limit this to a reasonable height
    auto formArea = area.removeFromTop(150); // Fixed height for form fields
    
    // Name field
    auto nameArea = formArea.removeFromTop(24);
    nameLabel.setBounds(nameArea.removeFromLeft(100));
    nameEditor.setBounds(nameArea);
    
    formArea.removeFromTop(10); // Spacing
    
    // Category field
    auto categoryArea = formArea.removeFromTop(24);
    categoryLabel.setBounds(categoryArea.removeFromLeft(100));
    categoryCombo.setBounds(categoryArea);
    
    formArea.removeFromTop(10); // Spacing
    
    // Description field
    auto descriptionArea = formArea.removeFromTop(72); // Larger area for description
    descriptionLabel.setBounds(descriptionArea.removeFromLeft(100).withHeight(24));
    descriptionEditor.setBounds(descriptionArea);
    
    // Plugin graph section - give this MUCH more space
    area.removeFromTop(20); // Spacing
    
    // Plugin label
    pluginLabel.setBounds(area.removeFromTop(24));
    
    // Buttons at the bottom
    auto buttonHeight = 30;
    auto buttonArea = area.removeFromBottom(buttonHeight + 10);
    
    // Graph editor gets all remaining space
    graphEditor->setBounds(area);
    
    // Position buttons
    cancelButton.setBounds(buttonArea.removeFromLeft(100).withHeight(buttonHeight));
    buttonArea.removeFromLeft(10); // Spacing
    saveButton.setBounds(buttonArea.removeFromLeft(100).withHeight(buttonHeight));
}

void PlayerEditorComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw the header
    g.setColour(Colours::black);
    FontOptions f(Font::getDefaultSansSerifFontName(), 18.0f, Font::bold);
    g.setFont(f);
    g.drawText("Player Details", getLocalBounds().reduced(10).removeFromTop(30),
               Justification::centredLeft, true);
    
    // Draw a separator line
    g.drawLine(10, 40, getWidth() - 10, 40, 1.0f);
    
    // Draw a border around the graph editor area to make it more prominent
    auto graphBounds = graphEditor->getBounds().expanded(2);
    g.setColour(Colours::darkgrey);
    g.drawRect(graphBounds, 2);
    
    // Add an instructional label
    g.setColour(Colours::black);
    FontOptions labelFont(Font::getDefaultSansSerifFontName(), 14.0f, Font::plain);
    g.setFont(labelFont);
    
    auto instructionRect = Rectangle<int>(graphBounds.getX(), graphBounds.getY() - 30, 
                                         graphBounds.getWidth(), 25);
    g.drawText("Right-click to add plugins, drag between pins to connect them",
               instructionRect, Justification::centred, true);
}

void PlayerEditorComponent::buttonClicked(Button* button)
{
    if (button == &saveButton)
    {
        // Update player data from UI
        player->metadata.name = nameEditor.getText();
        player->metadata.category = categoryCombo.getText();
        player->metadata.description = descriptionEditor.getText();
        player->metadata.lastModified = Time::getCurrentTime();
        
        // Save the graph state including plugin states
        if (auto* graph = graphEditor->graph.get())
        {
            // This will include both the graph structure and each plugin's state
            player->graphState = graph->createXml();
        }
        
        // Notify listeners
        listeners.call(&Listener::playerEditorSaveRequested, this);
    }
    else if (button == &cancelButton)
    {
        // Notify listeners
        listeners.call(&Listener::playerEditorCancelRequested, this);
    }
}

std::unique_ptr<PlayerManager::Player> PlayerEditorComponent::getPlayer()
{
    return std::move(player);
}