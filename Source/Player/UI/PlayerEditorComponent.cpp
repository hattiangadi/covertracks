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
        
        // Copy the graph state
        if (playerToEdit->graphState != nullptr)
            player->graphState = std::make_unique<XmlElement>(*playerToEdit->graphState);
            
        // Copy the graph file path
        player->graphFilePath = playerToEdit->graphFilePath;
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
        
        // Create an empty graph state
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
    
    // Create our own plugin system components
    formatManager.addDefaultFormats();
    formatManager.addFormat(new InternalPluginFormat());
    
    // Initialize internal plugin types
    InternalPluginFormat internalFormat;
    internalTypes = internalFormat.getAllTypes();
    
    // Load the plugin list from file
    File pluginListFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                          .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                          .getChildFile("PluginList.xml");
    
    if (pluginListFile.existsAsFile())
    {
        if (auto savedPluginList = XmlDocument::parse(pluginListFile))
            knownPluginList.recreateFromXml(*savedPluginList);
    }
    
    // Initialize the device manager
    deviceManager.initialise(2, 2, nullptr, true);
    
    // Create the graph editor with our components
    graphEditor = std::make_unique<GraphDocumentComponent>(formatManager, deviceManager, knownPluginList);
    
    // Make the graph editor visually distinct
    graphEditor->setColour(ResizableWindow::backgroundColourId, Colours::lightgrey);
    
    // Initialize the graph with a safe approach
    if (auto* graph = graphEditor->graph.get())
    {
        // First clear the graph to start with a clean state - this removes any default plugins
        // that might have been added by the PluginGraph::newDocument() method
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
    addAndMakeVisible(loadGraphButton);
    addAndMakeVisible(saveGraphButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(cancelButton);
    
    loadGraphButton.addListener(this);
    saveGraphButton.addListener(this);
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
    // Clear existing graph
    graph->clear();
    
    // Set up a new minimal default graph
    InternalPluginFormat internalFormat;
    auto pluginTypes = internalFormat.getAllTypes();
    
    // Add just the three essential components
    graph->addPlugin(PluginDescriptionAndPreference{pluginTypes[1]}, {0.25, 0.2});  // MIDI Input
    graph->addPlugin(PluginDescriptionAndPreference{pluginTypes[4]}, {0.5, 0.5});   // Sine Wave Synth
    graph->addPlugin(PluginDescriptionAndPreference{pluginTypes[2]}, {0.75, 0.8});  // Audio Output
    
    // Wait for plugins to initialize
    Thread::sleep(500);
    
    // Make proper connections between them
    makeDefaultConnections(graph);
}

void PlayerEditorComponent::makeDefaultConnections(PluginGraph* graph)
{
    AudioProcessorGraph::Node::Ptr midiInputNode = nullptr;
    AudioProcessorGraph::Node::Ptr audioOutputNode = nullptr;
    AudioProcessorGraph::Node::Ptr synthNode = nullptr;
    
    // Find the nodes by their processor types
    for (auto* node : graph->graph.getNodes())
    {
        auto name = node->getProcessor()->getName();
        
        if (name == "MIDI Input")
            midiInputNode = node;
        else if (name == "Audio Output")
            audioOutputNode = node;
        else if (name == "Sine Wave Synth")
            synthNode = node;
    }
    
    // Connect the nodes if they exist
    if (midiInputNode != nullptr && synthNode != nullptr && audioOutputNode != nullptr)
    {
        // Connect MIDI input to synth
        graph->graph.addConnection({
            {midiInputNode->nodeID, AudioProcessorGraph::midiChannelIndex}, // MIDI input output channel
            {synthNode->nodeID, AudioProcessorGraph::midiChannelIndex}  // Synth MIDI input channel
        });
        
        // Connect synth to audio output
        graph->graph.addConnection({
            {synthNode->nodeID, 0}, // Synth output channel 0 (left)
            {audioOutputNode->nodeID, 0}  // Audio output input channel 0 (left)
        });
        
        // Connect synth to second audio output channel
        graph->graph.addConnection({
            {synthNode->nodeID, 1}, // Synth output channel 1 (right)
            {audioOutputNode->nodeID, 1}  // Audio output input channel 1 (right)
        });
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
    // Clear any previous plugin descriptions
    pluginDescriptionsAndPreference.clear();
    
    // Add internal plugins first
    int menuID = menuIDBase;
    
    // Create a submenu for internal plugins
    PopupMenu internalPluginsMenu;
    
    for (auto& type : internalTypes)
    {
        pluginDescriptionsAndPreference.add(PluginDescriptionAndPreference{type});
        internalPluginsMenu.addItem(menuID++, type.name);
    }
    
    m.addSubMenu("Internal Plugins", internalPluginsMenu);
    m.addSeparator();
    
    // Add external plugins from the known plugin list
    auto pluginDescriptions = knownPluginList.getTypes();
    
    // Remove internal plugins from this list to avoid duplication
    pluginDescriptions.removeIf([](PluginDescription& desc)
    {
        return desc.pluginFormatName == InternalPluginFormat::getIdentifier();
    });
    
    // Create a tree structure for the plugins
    auto tree = KnownPluginList::createTree(pluginDescriptions, KnownPluginList::sortByManufacturer);
    
    // Add the tree to the menu
    addToMenu(*tree, m, pluginDescriptions, pluginDescriptionsAndPreference);
    
    // Add option to scan for plugins
    m.addSeparator();
    m.addItem(-1, "Edit the list of available plugins...");
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

std::optional<PluginDescriptionAndPreference> PlayerEditorComponent::getChosenType(int menuID) const
{
    // If it's a regular plugin from the tree
    if (menuID >= 1000)
    {
        const int i = menuID - 1000;
        
        if (isPositiveAndBelow(i, pluginDescriptionsAndPreference.size()))
            return pluginDescriptionsAndPreference[i];
    }
    // If it's an internal plugin
    else if (menuID >= menuIDBase && menuID < menuIDBase + internalTypes.size())
    {
        const int i = menuID - menuIDBase;
        
        if (isPositiveAndBelow(i, internalTypes.size()))
            return PluginDescriptionAndPreference{internalTypes[i]};
    }
    
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
    saveButton.setBounds(buttonArea.removeFromLeft(120).withHeight(buttonHeight));
    buttonArea.removeFromLeft(10); // Spacing
    saveGraphButton.setBounds(buttonArea.removeFromLeft(120).withHeight(buttonHeight));
    buttonArea.removeFromLeft(10); // Spacing
    loadGraphButton.setBounds(buttonArea.removeFromLeft(120).withHeight(buttonHeight));
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
        // Use our own showPopupMenu method instead of the graph panel's
        showPopupMenu(positionInGraph);
    }
}

void PlayerEditorComponent::showPopupMenu(Point<int> position)
{
    PopupMenu menu;
    addPluginsToMenu(menu);
    
    menu.showMenuAsync({},
                    ModalCallbackFunction::create([this, position](int r)
                    {
                        if (auto chosen = getChosenType(r))
                            createPlugin(*chosen, position);
                    }));
}

void PlayerEditorComponent::buttonClicked(Button* button)
{
    if (button == &loadGraphButton)
    {
        // Prompt user to select a graph file
        fileChooser = std::make_unique<FileChooser>("Load Graph File", 
                                                  File::getSpecialLocation(File::userDocumentsDirectory),
                                                  "*.filtergraph", true);
        
        auto loadGraphCallback = [safeThis = SafePointer<PlayerEditorComponent>(this)] (const FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;
                
            File graphFile = chooser.getResult();
            if (graphFile == File())
                return;
                
            // Load the graph file directly into the graph editor
            if (auto* graph = safeThis->graphEditor->graph.get())
            {
                // Clear the graph first
                graph->clear();
                
                // Load the XML file
                std::unique_ptr<XmlElement> xml = parseXML(graphFile);
                if (xml != nullptr && xml->hasTagName("FILTERGRAPH"))
                {
                    // Restore the graph from XML
                    graph->restoreFromXml(*xml);
                    
                    // Update the player's graph state
                    if (safeThis->player != nullptr)
                    {
                        safeThis->player->graphState = std::move(xml);
                        safeThis->player->graphFilePath = graphFile;
                    }
                    
                    // Make sure the graph panel updates to show the loaded configuration
                    if (safeThis->graphEditor->graphPanel != nullptr)
                        safeThis->graphEditor->graphPanel->updateComponents();
                }
                else
                {
                    // Show error if loading failed
                    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                                   "Graph Load Error",
                                                   "Failed to load the selected graph file.");
                }
            }
        };
        
        fileChooser->launchAsync(FileBrowserComponent::openMode, loadGraphCallback);
    }
    else if (button == &saveGraphButton)
    {
        // Save just the graph to a .filtergraph file
        if (auto* graph = graphEditor->graph.get())
        {
            // Create XML from the current graph state
            std::unique_ptr<XmlElement> graphXml = graph->createXml();
            
            // Prompt user to choose where to save the graph
            fileChooser = std::make_unique<FileChooser>("Save Graph As", 
                                                      player->graphFilePath.existsAsFile() ? player->graphFilePath : 
                                                      File::getSpecialLocation(File::userDocumentsDirectory),
                                                      "*.filtergraph");
            
            // We need to use a shared_ptr here because the lambda will be copied when passed to launchAsync
            auto sharedXml = std::make_shared<XmlElement>(*graphXml);
            
            auto saveGraphCallback = [safeThis = SafePointer<PlayerEditorComponent>(this), sharedXml] 
                                      (const FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;
                    
                File file = chooser.getResult();
                if (file == File())
                    return;
                    
                if (!file.getFileExtension().equalsIgnoreCase(".filtergraph"))
                    file = file.withFileExtension("filtergraph");
                
                // Save the XML to file
                MemoryBlock block;
                MemoryOutputStream stream(block, false);
                sharedXml->writeTo(stream);
                
                if (file.replaceWithData(block.getData(), block.getSize()))
                {
                    // Update the player's graph file path
                    if (safeThis->player != nullptr)
                        safeThis->player->graphFilePath = file;
                }
                else
                {
                    // Show error if saving failed
                    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                                   "Graph Save Error",
                                                   "Failed to save the graph file.");
                }
            };
            
            fileChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, 
                                   saveGraphCallback);
        }
    }
    else if (button == &saveButton)
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