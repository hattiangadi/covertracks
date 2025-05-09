// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../UI/MainHostWindow.h"

namespace covertracks {

PlayerEditorComponent::PlayerEditorComponent(Player* playerToEdit)
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

    // Set up UI components
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.setReadOnly(false);
    nameEditor.setScrollbarsShown(true);
    nameEditor.setCaretVisible(true);
    nameEditor.setPopupMenuEnabled(true);
    
    categoryEditor.setMultiLine(false);
    categoryEditor.setReturnKeyStartsNewLine(false);
    categoryEditor.setReadOnly(false);
    categoryEditor.setScrollbarsShown(true);
    categoryEditor.setCaretVisible(true);
    categoryEditor.setPopupMenuEnabled(true);
    
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.setReadOnly(false);
    descriptionEditor.setScrollbarsShown(true);
    descriptionEditor.setCaretVisible(true);
    descriptionEditor.setPopupMenuEnabled(true);
    
    saveButton.setButtonText("Save");
    cancelButton.setButtonText("Cancel");
    loadGraphButton.setButtonText("Load Graph");
    
    addAndMakeVisible(nameEditor);
    addAndMakeVisible(categoryEditor);
    addAndMakeVisible(descriptionEditor);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(cancelButton);
    addAndMakeVisible(loadGraphButton);
    
    saveButton.addListener(this);
    cancelButton.addListener(this);
    loadGraphButton.addListener(this);
    
    // Initialize the graph editor if we have a player
    if (player != nullptr)
    {
        // Create a graph document component
        AudioPluginFormatManager formatManager;
        AudioDeviceManager deviceManager;
        KnownPluginList knownPluginList;
        graphEditor = std::make_unique<GraphDocumentComponent>(formatManager, deviceManager, knownPluginList);
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
    nameEditor.setBounds(sidePanel.getX() + panelMargin + labelWidth + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Category label and field
    categoryLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    categoryEditor.setBounds(sidePanel.getX() + panelMargin + labelWidth + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Description label and field (taller)
    int descHeight = fieldHeight * 2;
    descriptionLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    descriptionEditor.setBounds(sidePanel.getX() + panelMargin + labelWidth + labelFieldGap, y, fieldWidth, descHeight);
    y += descHeight + fieldSpacing;

    // Buttons at the bottom of side panel (unchanged)
    int buttonHeight = 32;
    int buttonWidth = (sidePanel.getWidth() - 4 * panelMargin) / 3;
    int buttonY = sidePanel.getBottom() - buttonHeight - panelMargin;
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
    if (graphEditor != nullptr)
    {
        // Use the provided PluginDescriptionAndPreference directly
        graphEditor->createNewPlugin(desc, pos);
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
