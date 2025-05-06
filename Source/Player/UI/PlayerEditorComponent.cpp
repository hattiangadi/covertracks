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
            // Only try to restore if the structure is valid
            graph->restoreFromXml(*player->graphState);
            
            // Make sure the graph panel updates to show the loaded configuration
            if (graphEditor->graphPanel != nullptr)
                graphEditor->graphPanel->updateComponents();
        }
        else
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
                
                // Use regular sleep instead of runDispatchLoopUntil
                Thread::sleep(500);
                
                // Create the default connections between components
                makeDefaultConnections(graph);
                
                // Ensure the graph panel updates to show the nodes
                if (graphEditor->graphPanel != nullptr)
                    graphEditor->graphPanel->updateComponents();
            }
        }
    }
    
    addAndMakeVisible(graphEditor.get());
    
    // Add a mouse listener to the graph panel to handle right-clicks
    if (graphEditor->graphPanel != nullptr)
        graphEditor->graphPanel->addMouseListener(this, false);
    
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
    
    // Remove mouse listener
    if (graphEditor != nullptr && graphEditor->graphPanel != nullptr)
        graphEditor->graphPanel->removeMouseListener(this);
}

void PlayerEditorComponent::makeDefaultConnections(PluginGraph* graph)
{
    // Find the nodes by their processor types
    AudioProcessorGraph::Node::Ptr audioInputNode = nullptr;
    AudioProcessorGraph::Node::Ptr audioOutputNode = nullptr;
    AudioProcessorGraph::Node::Ptr synthNode = nullptr;
    
    for (auto* node : graph->graph.getNodes())
    {
        String name = node->getProcessor()->getName();
        if (name == "Audio Input")
            audioInputNode = node;
        else if (name == "Audio Output")
            audioOutputNode = node;
        else if (name == "Sine Wave Synth") 
            synthNode = node;
    }
    
    // Make connections if we have all required nodes
    if (audioInputNode != nullptr && audioOutputNode != nullptr && synthNode != nullptr)
    {
        // Connect synth outputs to audio output inputs
        graph->graph.addConnection({
            {synthNode->nodeID, 0}, // Synth output channel 0
            {audioOutputNode->nodeID, 0}  // Audio output input channel 0
        });
        
        if (synthNode->getProcessor()->getNumOutputChannels() > 1 &&
            audioOutputNode->getProcessor()->getNumInputChannels() > 1)
        {
            graph->graph.addConnection({
                {synthNode->nodeID, 1}, // Synth output channel 1
                {audioOutputNode->nodeID, 1}  // Audio output input channel 1
            });
        }
    }
}

bool PlayerEditorComponent::mouseDownOnGraphEditor(const MouseEvent& e)
{
    // Check if right mouse button was pressed
    if (e.mods.isPopupMenu() && graphEditor != nullptr && graphEditor->graphPanel != nullptr)
    {
        // Convert the mouse position to graph panel coordinates
        Point<int> position = graphEditor->graphPanel->getLocalPoint(nullptr, e.getScreenPosition());
        
        // Show the plugin list popup menu
        graphEditor->graphPanel->showPopupMenu(position);
        return true;
    }
    
    return false;
}

void PlayerEditorComponent::mouseDown(const MouseEvent& e)
{
    // Handle right-click on graph editor
    auto* component = getComponentAt(e.getPosition());
    if (component != nullptr && (component == graphEditor.get() || 
        component->isParentOf(graphEditor.get()) ||
        (graphEditor != nullptr && graphEditor->isParentOf(component))))
    {
        // Convert event to graph editor coordinates and handle it
        mouseDownOnGraphEditor(e.getEventRelativeTo(graphEditor.get()));
    }
}

void PlayerEditorComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Header area
    auto headerArea = area.removeFromTop(40); // Header height
    
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