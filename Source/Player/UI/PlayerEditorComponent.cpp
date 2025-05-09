// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../UI/MainHostWindow.h"

namespace covertracks {

// Constructor
PlayerEditorComponent::PlayerEditorComponent(Player* playerToEdit, MainHostWindow* mainWindow)
    : player(playerToEdit)
{
    // Initialize and add metadata labels
    nameLabel.setText("Name:", dontSendNotification);
    nameLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(nameLabel);

    categoryLabel.setText("Category:", dontSendNotification);
    categoryLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(categoryLabel);

    descriptionLabel.setText("Description:", dontSendNotification);
    descriptionLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(descriptionLabel);

    // Initialize and add metadata editors
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.setReadOnly(false);
    nameEditor.setScrollbarsShown(true);
    nameEditor.setCaretVisible(true);
    nameEditor.setPopupMenuEnabled(true);
    addAndMakeVisible(nameEditor);

    categoryEditor.setMultiLine(false);
    categoryEditor.setReturnKeyStartsNewLine(false);
    categoryEditor.setReadOnly(false);
    categoryEditor.setScrollbarsShown(true);
    categoryEditor.setCaretVisible(true);
    categoryEditor.setPopupMenuEnabled(true);
    addAndMakeVisible(categoryEditor);

    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.setReadOnly(false);
    descriptionEditor.setScrollbarsShown(true);
    descriptionEditor.setCaretVisible(true);
    descriptionEditor.setPopupMenuEnabled(true);
    addAndMakeVisible(descriptionEditor);

    // Initialize and add buttons
    saveButton.setButtonText("Save");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);

    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);

    loadGraphButton.setButtonText("Load Graph");
    loadGraphButton.addListener(this);
    addAndMakeVisible(loadGraphButton);

    // Create a graph editor if we have a player
    if (player != nullptr)
    {
        // If mainWindow was provided, use its plugin environment
        if (mainWindow != nullptr)
        {
            DBG("Using MainHostWindow's plugin environment");
            graphEditor = std::make_unique<GraphDocumentComponent>(mainWindow->getFormatManager(),
                                                                mainWindow->getDeviceManager(),
                                                                mainWindow->getKnownPluginList());
        }
        else
        {
            // If no mainWindow was provided, try to find it
            auto* foundMainWindow = dynamic_cast<MainHostWindow*>(getTopLevelComponent());
            
            if (foundMainWindow != nullptr)
            {
                DBG("Found MainHostWindow, using its plugin environment");
                graphEditor = std::make_unique<GraphDocumentComponent>(foundMainWindow->getFormatManager(),
                                                                    foundMainWindow->getDeviceManager(),
                                                                    foundMainWindow->getKnownPluginList());
            }
            else
            {
                // Last resort: create a temporary environment
                DBG("WARNING: MainHostWindow not found, creating temporary plugin environment");
                AudioPluginFormatManager formatManager;
                formatManager.addDefaultFormats();
                formatManager.addFormat(new InternalPluginFormat());
                
                AudioDeviceManager deviceManager;
                KnownPluginList knownPluginList;
                
                graphEditor = std::make_unique<GraphDocumentComponent>(formatManager, deviceManager, knownPluginList);
            }
        }
        
        if (graphEditor->graphPanel)
            graphEditor->graphPanel->setOwnerPlayerEditorComponent(this);
        addAndMakeVisible(graphEditor.get());
        
        // Load the graph state if available
        auto graphState = player->getPluginGraphState();
        if (graphState != nullptr)
        {
            graphEditor->graph->restoreFromXml(*graphState);
        }
        
        // Update the UI from the player data
        updateUIFromPlayer();
    }
}

PlayerEditorComponent::~PlayerEditorComponent()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
    loadGraphButton.removeListener(this);
}



void PlayerEditorComponent::resized()
{
    auto area = getLocalBounds();

    // --- Side panel for metadata and buttons ---
    const int sidePanelWidth = 320;
    auto sidePanel = area.removeFromLeft(sidePanelWidth);

    // Consistent margins and spacing
    const int panelMargin = 16;
    const int labelWidth = 80;
    const int fieldHeight = 28;
    const int fieldSpacing = 12;
    const int labelFieldGap = 8;
    int fieldWidth = sidePanel.getWidth() - 2 * panelMargin - labelWidth - labelFieldGap;
    int y = sidePanel.getY() + panelMargin;

    // Name label and field
    nameLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    nameEditor.setBounds(nameLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Category label and field
    categoryLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    categoryEditor.setBounds(categoryLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Description label and field
    descriptionLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    descriptionEditor.setBounds(descriptionLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight * 3);
    y += fieldHeight * 3 + fieldSpacing * 2;

    // Buttons
    const int buttonWidth = (fieldWidth + labelWidth + labelFieldGap - 2 * panelMargin) / 3;
    const int buttonHeight = 30;
    const int buttonY = sidePanel.getBottom() - panelMargin - buttonHeight;

    saveButton.setBounds(sidePanel.getX() + panelMargin, buttonY, buttonWidth, buttonHeight);
    cancelButton.setBounds(sidePanel.getX() + panelMargin + buttonWidth + panelMargin, buttonY, buttonWidth, buttonHeight);
    loadGraphButton.setBounds(sidePanel.getX() + panelMargin + 2 * (buttonWidth + panelMargin), buttonY, buttonWidth, buttonHeight);

    // --- Main area for graph editor ---
    if (graphEditor != nullptr)
    {
        graphEditor->setBounds(area);
        DBG("[PlayerEditorComponent::resized] graphEditor bounds: " << graphEditor->getBounds().toString());
    }
}

void PlayerEditorComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    // Draw separator for side panel
    const int sidePanelWidth = 320;
    g.setColour(Colours::darkgrey.withAlpha(0.4f));
    g.drawLine((float)sidePanelWidth, 0.0f, (float)sidePanelWidth, (float)getHeight(), 2.0f);
}


void PlayerEditorComponent::mouseDown(const MouseEvent& e)
{
    // Handle mouse clicks in the graph editor
}

void PlayerEditorComponent::buttonClicked(Button* button)
{
    if (button == &saveButton)
    {
        // Update the player data from the UI
        updatePlayerFromUI();
        
        // Notify listeners that save has been requested
        listeners.call(&Listener::playerEditorSaveRequested, this);
    }
    else if (button == &cancelButton)
    {
        // Notify listeners that cancel has been requested
        listeners.call(&Listener::playerEditorCancelRequested, this);
    }
    else if (button == &loadGraphButton)
    {
        // Show a file chooser to load a graph file
        FileChooser chooser("Select a graph file to load...",
                            File::getSpecialLocation(File::userHomeDirectory),
                            "*.filtergraph");
        
        // Use launchAsync instead of browseForFileToOpen
        chooser.launchAsync(FileBrowserComponent::openMode, [this](const FileChooser& fc)
        {
            auto file = fc.getResult();
            
            if (file.existsAsFile())
            {
                // Load the graph file
                auto xml = XmlDocument::parse(file);
                
                if (xml != nullptr && graphEditor != nullptr)
                {
                    // Update the graph editor
                    graphEditor->graph->restoreFromXml(*xml);
                    
                    // Update the player's graph state by loading it
                    if (player != nullptr)
                    {
                        player->loadPluginGraph(*xml);
                    }
                }
            }
        });
    }
}

void PlayerEditorComponent::createPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos)
{
    // Create a plugin in the graph editor
    if (graphEditor != nullptr && graphEditor->graph != nullptr)
    {
        // Log plugin description details for debugging
        DBG("Attempting to create plugin: " + desc.pluginDescription.name);
        DBG("  Format: " + desc.pluginDescription.pluginFormatName);
        DBG("  File: " + desc.pluginDescription.fileOrIdentifier);
        
        // Debug available formats
        if (auto* mainWindow = dynamic_cast<MainHostWindow*>(getTopLevelComponent()))
        {
            DBG("Available formats in MainHostWindow:");
            for (int i = 0; i < mainWindow->getFormatManager().getNumFormats(); ++i)
            {
                auto* format = mainWindow->getFormatManager().getFormat(i);
                DBG("  " + String(i) + ": " + format->getName());
            }
            
            // Check if the format exists in the MainHostWindow's format manager
            bool formatFound = false;
            for (int i = 0; i < mainWindow->getFormatManager().getNumFormats(); ++i)
            {
                auto* format = mainWindow->getFormatManager().getFormat(i);
                if (format->getName() == desc.pluginDescription.pluginFormatName)
                {
                    formatFound = true;
                    DBG("Format found in MainHostWindow's format manager");
                    break;
                }
            }
            
            if (!formatFound)
            {
                DBG("WARNING: Format '" + desc.pluginDescription.pluginFormatName + "' not found in MainHostWindow's format manager");
            }
        }
        else
        {
            DBG("WARNING: Could not find MainHostWindow to check formats");
        }
        
        // Try to add the plugin directly to the graph
        graphEditor->graph->addPlugin(desc, {pos.x / (double)graphEditor->getWidth(), 
                                           pos.y / (double)graphEditor->getHeight()});
    }
    else
    {
        DBG("Cannot create plugin: graphEditor or graph is null");
    }
}

void PlayerEditorComponent::addPluginsToMenu(PopupMenu& menu)
{
    // Get the main window to access the plugin list
    if (auto* mainWindow = dynamic_cast<MainHostWindow*>(getTopLevelComponent()))
    {
        mainWindow->addPluginsToMenu(menu);
    }
}

std::optional<PluginDescriptionAndPreference> PlayerEditorComponent::getChosenType(int menuID) const
{
    // Get the main window to access the plugin descriptions
    if (auto* mainWindow = dynamic_cast<MainHostWindow*>(getTopLevelComponent()))
    {
        return mainWindow->getChosenType(menuID);
    }
    
    return std::nullopt;
}

void PlayerEditorComponent::addToMenu(const KnownPluginList::PluginTree& tree,
                                     PopupMenu& m,
                                     const Array<PluginDescription>& allPlugins,
                                     Array<PluginDescriptionAndPreference>& addedPlugins)
{
    // For simplicity, just add all plugins in a flat list
    for (int i = 0; i < allPlugins.size(); ++i)
    {
        const auto& plugin = allPlugins.getReference(i);
        addedPlugins.add(PluginDescriptionAndPreference(plugin));
        m.addItem(addedPlugins.size() + 1000, plugin.name + " (" + plugin.pluginFormatName + ")");
    }
}

void PlayerEditorComponent::takeOwnershipOfPlayer(std::unique_ptr<Player> newPlayer)
{
    // Take ownership of the player
    ownedPlayer = std::move(newPlayer);
    player = ownedPlayer.get();
    
    // Update the UI
    updateUIFromPlayer();
}

void PlayerEditorComponent::updatePlayerFromUI()
{
    if (player != nullptr)
    {
        // Update metadata
        auto& metadata = player->getMetadata();
        metadata.name = nameEditor.getText();
        metadata.category = categoryEditor.getText();
        metadata.description = descriptionEditor.getText();
        metadata.lastModified = Time::getCurrentTime();
        
        // Update graph state
        if (graphEditor != nullptr)
        {
            auto graphXml = graphEditor->graph->createXml();
            if (graphXml != nullptr)
            {
                player->loadPluginGraph(*graphXml);
            }
        }
    }
}

void PlayerEditorComponent::updateUIFromPlayer()
{
    if (player != nullptr)
    {
        // Update UI components
        auto& metadata = player->getMetadata();
        nameEditor.setText(metadata.name);
        categoryEditor.setText(metadata.category);
        descriptionEditor.setText(metadata.description);
    }
}

void PlayerEditorComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PlayerEditorComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace covertracks
