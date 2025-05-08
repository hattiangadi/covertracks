// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../UI/MainHostWindow.h"

namespace covertracks {

PlayerEditorComponent::PlayerEditorComponent(Player* playerToEdit)
    : player(playerToEdit)
{
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
    
    // Reserve space for the form at the top
    auto formArea = area.removeFromTop(150);
    
    // Layout the form fields
    auto labelWidth = 100;
    auto fieldHeight = 24;
    auto margin = 10;
    
    // Name field
    auto nameRow = formArea.removeFromTop(fieldHeight + margin);
    nameEditor.setBounds(nameRow.withTrimmedLeft(labelWidth));
    
    // Category field
    auto categoryRow = formArea.removeFromTop(fieldHeight + margin);
    categoryEditor.setBounds(categoryRow.withTrimmedLeft(labelWidth));
    
    // Description field
    auto descriptionRow = formArea.removeFromTop(fieldHeight * 2 + margin);
    descriptionEditor.setBounds(descriptionRow.withTrimmedLeft(labelWidth));
    
    // Reserve space for buttons at the bottom
    auto buttonArea = area.removeFromBottom(40);
    
    // Divide the button area into three equal parts
    auto buttonWidth = buttonArea.getWidth() / 3;
    
    saveButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    cancelButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    loadGraphButton.setBounds(buttonArea.reduced(5));
    
    // Use the remaining area for the graph editor
    if (graphEditor != nullptr)
    {
        graphEditor->setBounds(area);
    }
}

void PlayerEditorComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    g.setColour(Colours::white);
    g.setFont(14.0f);
    
    // Draw labels for the form fields
    auto labelWidth = 100;
    auto fieldHeight = 24;
    auto margin = 10;
    auto y = 0;
    
    // Name label
    g.drawText("Name:", 0, y, labelWidth, fieldHeight, Justification::centredRight, true);
    y += fieldHeight + margin;
    
    // Category label
    g.drawText("Category:", 0, y, labelWidth, fieldHeight, Justification::centredRight, true);
    y += fieldHeight + margin;
    
    // Description label
    g.drawText("Description:", 0, y, labelWidth, fieldHeight, Justification::centredRight, true);
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
