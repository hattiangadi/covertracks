// PlayerEditorComponent.cpp - Update to properly save and load plugin states

#include "PlayerEditorComponent.h"

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
    
    // Techniques section has been removed
    
    // Set up plugin graph editor
    addAndMakeVisible(pluginLabel);
    
    // Set up the graph editor
    AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();
    
    // Get application properties for device manager
    PropertiesFile* appProperties = nullptr;
    std::unique_ptr<XmlElement> savedState;
    
    // Try to get audio device state from a file instead
    File audioSettingsFile = File::getSpecialLocation(File::userApplicationDataDirectory)
                             .getChildFile(JUCEApplication::getInstance()->getApplicationName())
                             .getChildFile("AudioSettings.xml");
    
    if (audioSettingsFile.existsAsFile())
        savedState = XmlDocument::parse(audioSettingsFile);
    
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
    
    // Create graph editor
    graphEditor = std::make_unique<GraphDocumentComponent>(formatManager, *deviceManager, knownPluginList);
    
    // If we have an existing graph state, load it
    if (player->graphState != nullptr && player->graphState->hasTagName("FILTERGRAPH"))
    {
        if (auto* graph = graphEditor->graph.get())
        {
            // Restore the graph including plugin states
            graph->restoreFromXml(*player->graphState);
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

void PlayerEditorComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Header area
    area.removeFromTop(40); // Header height
    
    area.removeFromTop(10); // Spacing after header
    
    // Name field
    auto nameArea = area.removeFromTop(24);
    nameLabel.setBounds(nameArea.removeFromLeft(100));
    nameEditor.setBounds(nameArea);
    
    area.removeFromTop(10); // Spacing
    
    // Category field
    auto categoryArea = area.removeFromTop(24);
    categoryLabel.setBounds(categoryArea.removeFromLeft(100));
    categoryCombo.setBounds(categoryArea);
    
    area.removeFromTop(10); // Spacing
    
    // Description field
    auto descriptionArea = area.removeFromTop(72); // Larger area for description
    descriptionLabel.setBounds(descriptionArea.removeFromLeft(100).withHeight(24));
    descriptionEditor.setBounds(descriptionArea);
    
    area.removeFromTop(20); // Spacing
    
    // Techniques section has been removed
    // No need for extra spacing here
    
    // Plugin graph editor section
    auto pluginLabelArea = area.removeFromTop(24);
    pluginLabel.setBounds(pluginLabelArea);
    
    // Graph editor takes the remaining space except for button area
    auto buttonHeight = 30;
    auto buttonArea = area.removeFromBottom(buttonHeight);
    graphEditor->setBounds(area);
    
    // Buttons at the bottom
    auto buttonWidth = 100;
    
    cancelButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(10); // Spacing
    saveButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
}

void PlayerEditorComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw the header
    g.setColour(Colours::black);
    Font f;
    f.setHeight(18.0f);
    f.setBold(true);
    g.setFont(f);
    g.drawText("Player Details", getLocalBounds().reduced(10).removeFromTop(30),
               Justification::centredLeft, true);
    
    // Draw a separator line
    g.drawLine(10, 40, getWidth() - 10, 40, 1.0f);
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
        
        // Capabilities have been removed
        
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