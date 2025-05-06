// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../UI/MainHostWindow.h"

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
    
    // Set up format manager and internal format types
    InternalPluginFormat internalFormat;
    internalTypes = internalFormat.getAllTypes();
    
    // Create our own plugin system components
    formatManager.addDefaultFormats();
    formatManager.addFormat(new InternalPluginFormat());
    
    // Initialize the device manager
    deviceManager.initialise(2, 2, nullptr, true);
    
    // Create the graph editor with our components
    graphEditor = std::make_unique<GraphDocumentComponent>(formatManager, deviceManager, knownPluginList);
    
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
            bool hasValidStructure = player->graphState->getChildByName("CONNECTIONS") != nullptr;
            
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
    auto pluginTypes = internalFormat.getAllTypes();
    
    if (pluginTypes.size() > 4)
    {
        // Add standard nodes: Audio Input, Audio Output, and Sine Wave Synth
        graph->addPlugin(PluginDescriptionAndPreference{pluginTypes[0]}, {0.25, 0.1});  // Audio Input
        graph->addPlugin(PluginDescriptionAndPreference{pluginTypes[2]}, {0.25, 0.9});  // Audio Output
        graph->addPlugin(PluginDescriptionAndPreference{pluginTypes[4]}, {0.5, 0.5});   // Sine Wave Synth
        
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

void PlayerEditorComponent::createPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos)
{
    if (graphEditor != nullptr)
        graphEditor->createNewPlugin(desc, pos);
}



// Replace the old addPluginsToMenu method with this new version:

void PlayerEditorComponent::addPluginsToMenu(PopupMenu& m)
{
    if (graphEditor != nullptr)
    {
        int i = 0;
        
        // Use the class member internalTypes instead of creating a local variable
        for (auto& t : internalTypes)
            m.addItem(++i, t.name + " (" + t.pluginFormatName + ")");
    }
        
    m.addSeparator();
        
    // Get plugin list
    KnownPluginList tempKnownPluginList;
        
    // Try to load plugin list from a file
    File pluginListFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                         .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                         .getChildFile("PluginList.xml");
        
    if (pluginListFile.existsAsFile())
    {
        if (auto savedPluginList = XmlDocument::parse(pluginListFile))
            tempKnownPluginList.recreateFromXml(*savedPluginList);
    }
        
    auto pluginDescriptions = tempKnownPluginList.getTypes();
        
    // This avoids showing the internal types again later on in the list
    pluginDescriptions.removeIf([](PluginDescription& desc)
    {
        return desc.pluginFormatName == InternalPluginFormat::getIdentifier();
    });
        
    // Create a tree structure for the plugins
    auto tree = KnownPluginList::createTree(pluginDescriptions, KnownPluginList::sortByManufacturer);
    pluginDescriptionsAndPreference.clear();
        
    addToMenu(*tree, m, pluginDescriptions, pluginDescriptionsAndPreference);
}

void PlayerEditorComponent::addToMenu(const KnownPluginList::PluginTree& tree,
                                   PopupMenu& m,
                                   const Array<PluginDescription>& allPlugins,
                                   Array<PluginDescriptionAndPreference>& addedPlugins)
{
    for (auto* sub : tree.subFolders)
    {
        PopupMenu subMenu;
        addToMenu(*sub, subMenu, allPlugins, addedPlugins);
        
        m.addSubMenu(sub->folder, subMenu, true, nullptr, false, 0);
    }
    
    auto addPlugin = [&] (const auto& descriptionAndPreference, const auto& pluginName)
    {
        addedPlugins.add(descriptionAndPreference);
        const auto menuID = addedPlugins.size() + 1000; // Use 1000 as base ID for menu items
        m.addItem(menuID, pluginName, true, false);
    };
    
    for (auto& plugin : tree.plugins)
    {
        auto name = plugin.name;
        
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
            name << " (" << plugin.pluginFormatName << ')';
        
        addPlugin(PluginDescriptionAndPreference{plugin, PluginDescriptionAndPreference::UseARA::no}, name);
        
       #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
        if (plugin.hasARAExtension)
        {
            name << " (ARA)";
            addPlugin(PluginDescriptionAndPreference{plugin}, name);
        }
       #endif
    }
}

std::optional<PluginDescriptionAndPreference> PlayerEditorComponent::getChosenType(const int menuID) const
{
    const auto internalIndex = menuID - 1;
    
    if (isPositiveAndBelow(internalIndex, internalTypes.size()))
        return PluginDescriptionAndPreference{internalTypes[(size_t)internalIndex]};
    
    const auto externalIndex = menuID - 1000; // 1000 is the base ID for external plugins
    
    if (isPositiveAndBelow(externalIndex, pluginDescriptionsAndPreference.size()))
        return pluginDescriptionsAndPreference[externalIndex];
    
    return {};
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

void PlayerEditorComponent::mouseDown(const MouseEvent& e)
{
    // Handle right-click to show the plugin menu
    if (e.mods.isPopupMenu() && graphEditor != nullptr && graphEditor->graphPanel != nullptr)
    {
        // Convert the mouse position to the graph panel's coordinate space
        auto positionInGraph = e.getEventRelativeTo(graphEditor->graphPanel.get()).getPosition();
        graphEditor->graphPanel->showPopupMenu(positionInGraph, this);
    }
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