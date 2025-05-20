// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../UI/MainHostWindow.h"
#include <unordered_map>

namespace covertracks {

// Constructor
PlayerEditorComponent::PlayerEditorComponent(Player* playerToEdit, AudioEngine* engine)
    : player(playerToEdit),
      audioEngine(engine)
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
    if (player != nullptr && audioEngine != nullptr)
    {
        // Create GraphDocumentComponent with AudioEngine reference
        graphEditor = std::make_unique<GraphDocumentComponent>(
            audioEngine->getFormatManager(),
            audioEngine->getKnownPluginList(),
            *audioEngine);
        
        if (graphEditor->graphPanel)
            graphEditor->graphPanel->setOwnerPlayerEditorComponent(this);
        
        addAndMakeVisible(graphEditor.get());
        
        // Load player for editing - wait to ensure audio device is initialized
        startTimer(100);
        
        // Update the UI from the player data
        auto& metadata = player->getMetadata();
        nameEditor.setText(metadata.name);
        categoryEditor.setText(metadata.category);
        descriptionEditor.setText(metadata.description);
    }
}

PlayerEditorComponent::~PlayerEditorComponent()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
    loadGraphButton.removeListener(this);
}

// Add a static flag to track if we've already loaded this player for editing
static std::unordered_map<Player*, bool> playerLoadedMap;

void PlayerEditorComponent::loadPlayerForEditing()
{
    if (player != nullptr)
    {
        // Check if this player has already been loaded for editing
        if (playerLoadedMap.find(player) != playerLoadedMap.end() && playerLoadedMap[player])
            return;
        
        // Mark this player as loaded
        playerLoadedMap[player] = true;
        
        // Prepare the player for editing
        player->prepareForEditing();
        
        // If we have graph editor and the player has a graph state, load it
        if (graphEditor != nullptr && graphEditor->graph != nullptr)
        {
            auto graphState = player->getPluginGraphState();
            
            if (graphState != nullptr)
            {
                // Restore the graph from XML
                graphEditor->graph->restoreFromXml(*graphState);
                
                // Fix audio output nodes after restoring
                graphEditor->graph->fixAudioOutputNodes();
                
                // Force a complete UI update to show the graph
                if (graphEditor->graphPanel != nullptr)
                {
                    // Update the components
                    graphEditor->graphPanel->updateComponents();
                    graphEditor->graphPanel->repaint();
                    graphEditor->resized();
                    graphEditor->graphPanel->setVisible(true);
                    
                    // Schedule another update after a short delay to ensure display
                    startTimer(500);
                }
                
                // Connect the graph to the AudioEngine for playback during editing
                if (audioEngine != nullptr)
                {
                    // Make sure audio output nodes are properly configured before connecting
                    graphEditor->graph->fixAudioOutputNodes();
                    
                    // Get the processor graph
                    auto& graph = graphEditor->graph->graph;
                    
                    // Set the sample rate and block size to match the audio device
                    auto& deviceManager = audioEngine->getDeviceManager();
                    auto currentDevice = deviceManager.getCurrentAudioDevice();
                    if (currentDevice != nullptr)
                    {
                        double sampleRate = currentDevice->getCurrentSampleRate();
                        int blockSize = currentDevice->getCurrentBufferSizeSamples();
                        
                        // Prepare the graph for playback
                        graph.prepareToPlay(sampleRate, blockSize);
                    }
                    
                    // Register the processor graph with the AudioEngine
                    bool success = audioEngine->registerProcessor(&graph);
                    if (!success)
                    {
                        // If registration failed, try unregistering first (in case it's already registered)
                        audioEngine->unregisterProcessor(&graph);
                        success = audioEngine->registerProcessor(&graph);
                        
                        if (!success)
                        {
                            std::cerr << "Failed to register graph with AudioEngine" << std::endl;
                        }
                    }
                    
                    // Enable MIDI input for the graph
                    audioEngine->enableMidiInput(true);
                    
                    // Set the audio output volume to a reasonable level
                    audioEngine->setMasterGain(0.8f);
                    
                    // Connect the keyboard to the graph
                    connectMidiKeyboardToGraph();
                }
            }
        }
        
        // Update the UI from player metadata
        updateUIFromPlayer();
    }
}

void PlayerEditorComponent::saveAndReleaseResources()
{
    if (player != nullptr)
    {
        // Update player from UI
        updatePlayerFromUI();
        
        // Save the graph state if we have a graph editor
        if (graphEditor != nullptr && graphEditor->graph != nullptr)
        {
            // Disconnect the graph from the AudioEngine
            if (audioEngine != nullptr)
            {
                std::cout << "Disconnecting graph from AudioEngine after editing" << std::endl;
                audioEngine->unregisterProcessor(&graphEditor->graph->graph);
            }
            
            // Save the current graph state to the player
            auto graphState = graphEditor->graph->createXml();
            if (graphState != nullptr)
            {
                std::cout << "Saving graph state with " << graphEditor->graph->graph.getConnections().size() << " connections" << std::endl;
                
                // Count nodes and connections for logging
                int midiConnections = 0;
                int audioConnections = 0;
                    
                for (auto* e : graphState->getChildWithTagNameIterator("CONNECTION"))
                {
                    auto srcChannel = e->getIntAttribute("srcChannel");
                    bool isMidi = (srcChannel == 4096);
                    
                    if (isMidi)
                        midiConnections++;
                    else
                        audioConnections++;
                }
                
                std::cout << "Graph state contains " << (midiConnections + audioConnections) 
                          << " connections (MIDI: " << midiConnections 
                          << ", Audio: " << audioConnections << ")" << std::endl;
                
                // Save directly to the player's graph file
                player->savePluginGraphToFile(*graphState);
            }
        }
        
        // Disconnect from AudioEngine if connected
        if (player->isConnectedToAudioEngine() && audioEngine != nullptr)
        {
            player->disconnectFromAudioEngine();
        }
        
        // Remove the player from our tracking map
        if (playerLoadedMap.find(player) != playerLoadedMap.end())
        {
            playerLoadedMap.erase(player);
        }
        
        // Release resources
        player->releaseResources();
        
        std::cout << "Player resources released after editing" << std::endl;
    }
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
        if (graphEditor != nullptr && graphEditor->graph != nullptr)
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
    if (player == nullptr)
    {
        std::cout << "PlayerEditorComponent::updateUIFromPlayer - player is null" << std::endl;
        return;
    }
    
    // Update the UI components
    auto& metadata = player->getMetadata();
    nameEditor.setText(metadata.name);
    categoryEditor.setText(metadata.category);
    descriptionEditor.setText(metadata.description);
    
    // Start timer to load the graph state after a short delay
    // This ensures the audio device is fully initialized
    startTimer(100);
}

void PlayerEditorComponent::timerCallback()
{
    static int updateCount = 0;
    
    // Stop the timer
    stopTimer();
    
    // If this is the first call (from constructor), load the player
    if (updateCount == 0)
    {
        loadPlayerForEditing();
        updateCount++;
    }
    // If this is a subsequent call (scheduled after loading), force a UI update
    else
    {
        // Force another UI update after a short delay
        if (graphEditor != nullptr && graphEditor->graphPanel != nullptr && player != nullptr)
        {
            // Load the graph state from the player
            auto graphStateXml = player->getPluginGraphState();
            if (graphStateXml != nullptr)
            {
                std::cout << "Loading graph state for editing player: " << player->getMetadata().name << std::endl;
                graphEditor->graph->restoreFromXml(*graphStateXml);
                
                // Register the processor with the AudioEngine
                if (audioEngine != nullptr && graphEditor->graph != nullptr)
                {
                    // Register the graph processor with the AudioEngine
                    audioEngine->registerProcessor(&graphEditor->graph->graph);
                    std::cout << "Registered processor for player: " << player->getMetadata().name << std::endl;
                }
            }
            graphEditor->graph->fixAudioOutputNodes();
            graphEditor->graphPanel->updateComponents();
            graphEditor->graphPanel->repaint();
            graphEditor->resized();
        }
        
        updateCount++;
        
        // If we've done less than 3 updates, schedule another one
        if (updateCount < 3)
            startTimer(500);
    }
}

void PlayerEditorComponent::connectMidiKeyboardToGraph()
{
    // Ensure MIDI input is enabled in the AudioEngine
    if (audioEngine != nullptr && graphEditor != nullptr)
    {
        // Enable MIDI input
        audioEngine->enableMidiInput(true);
        
        // The GraphDocumentComponent itself implements MidiInputCallback
        // We need to use the raw pointer, not the unique_ptr
        audioEngine->registerMidiInput("", graphEditor.get());
        
        // Register the processor with the AudioEngine to ensure MIDI messages are routed properly
        if (graphEditor->graph != nullptr)
        {
            // Register the graph processor with the AudioEngine
            audioEngine->registerProcessor(&graphEditor->graph->graph);
            std::cout << "Registered processor for player in connectMidiKeyboardToGraph" << std::endl;
        }
        
        std::cout << "Connected MIDI keyboard to graph for player" << std::endl;
    }
}

void PlayerEditorComponent::handleIncomingMidiMessage(MidiInput* /*source*/, const MidiMessage& /*message*/)
{
    // We don't need to handle external MIDI messages directly
    // The AudioEngine will route them to the graph automatically
}

void PlayerEditorComponent::handleNoteOn(MidiKeyboardState* /*source*/, int /*midiChannel*/, int /*midiNoteNumber*/, float /*velocity*/)
{
    // GraphDocumentComponent now handles MIDI keyboard state directly
    // No action needed here
}

void PlayerEditorComponent::handleNoteOff(MidiKeyboardState* /*source*/, int /*midiChannel*/, int /*midiNoteNumber*/, float /*velocity*/)
{
    // GraphDocumentComponent now handles MIDI keyboard state directly
    // No action needed here
}

void PlayerEditorComponent::createPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos)
{
    // Create a plugin in the graph editor
    if (graphEditor != nullptr && graphEditor->graph != nullptr)
    {
        // Log plugin description details for debugging
        DBG("Attempting to create plugin: " + desc.pluginDescription.name);
        DBG("  Format: " + desc.pluginDescription.pluginFormatName);
        
        // Add the plugin to the graph
        graphEditor->graph->addPlugin(desc, {pos.x / (double)graphEditor->getWidth(), 
                                          pos.y / (double)graphEditor->getHeight()});
    }
    else
    {
        DBG("Cannot create plugin: graphEditor or graph is null");
    }
}

void PlayerEditorComponent::buttonClicked(Button* button)
{
    if (button == &saveButton)
    {
        // Update the player data from UI
        updatePlayerFromUI();
        
        // Notify listeners that save has been requested
        listeners.call(&Listener::playerEditorSaveRequested, this);
    }
    else if (button == &cancelButton)
    {
        // Release resources before canceling
        saveAndReleaseResources();
        
        // Notify listeners that cancel has been requested
        listeners.call(&Listener::playerEditorCancelRequested, this);
    }
    else if (button == &loadGraphButton)
    {
        // Use the FileBasedDocument method to load a file asynchronously
        if (graphEditor != nullptr && graphEditor->graph != nullptr)
        {
            graphEditor->graph->loadFromUserSpecifiedFileAsync(true, [this](Result result)
            {
                if (result.wasOk())
                {
                    // Get the XML representation of the loaded graph
                    auto xml = graphEditor->graph->createXml();
                    
                    if (xml != nullptr)
                    {
                        // Force the graph panel to update
                        if (graphEditor != nullptr && graphEditor->graphPanel != nullptr)
                        {
                            graphEditor->graphPanel->updateComponents();
                            graphEditor->repaint();
                        }
                        
                        // Update the player's graph state
                        if (player != nullptr)
                        {
                            player->loadPluginGraph(*xml);
                            
                            // Explicitly save the player to ensure the graph state is persisted
                            if (auto* mainWindow = dynamic_cast<MainHostWindow*>(getTopLevelComponent()))
                            {
                                if (mainWindow->playerManager != nullptr)
                                {
                                    mainWindow->playerManager->savePlayer(player);
                                }
                            }
                        }
                    }
                }
            });
        }
    }
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


void PlayerEditorComponent::mouseDown(const MouseEvent& /*e*/)
{
    // Handle mouse down events in the graph editor
}





void covertracks::PlayerEditorComponent::addPluginsToMenu(PopupMenu& menu)
{
    // Use AudioEngine to access the plugin list
    if (audioEngine != nullptr)
    {
        // Get available plugin descriptions from the AudioEngine
        auto& pluginList = audioEngine->getKnownPluginList();
        
        // Create a submenu for each plugin category
        KnownPluginList::addToMenu(menu, pluginList.getTypes(), KnownPluginList::sortByManufacturer);
    }
}

std::optional<covertracks::PluginDescriptionAndPreference> covertracks::PlayerEditorComponent::getChosenType(int menuID) const
{
    // Use AudioEngine to access the plugin descriptions
    if (audioEngine != nullptr)
    {
        // Get available plugin descriptions from the AudioEngine
        auto& pluginList = audioEngine->getKnownPluginList();
        
        // Debug the menu ID and available plugins
        DBG("PlayerEditorComponent::getChosenType - Menu ID: " + String(menuID));
        DBG("Available plugins: " + String(pluginList.getNumTypes()));
        
        // Use JUCE's method to find the plugin description by menu ID
        // This is the proper way to handle the menu IDs generated by KnownPluginList::addToMenu
        int pluginIndex = KnownPluginList::getIndexChosenByMenu(pluginList.getTypes(), menuID);
        
        if (pluginIndex >= 0 && pluginIndex < pluginList.getTypes().size())
        {
            const auto& desc = pluginList.getTypes()[pluginIndex];
            DBG("Found plugin: " + desc.name);
            return PluginDescriptionAndPreference { desc };
        }
        
        // If we get here, no plugin was found
        DBG("No plugin found for menu ID: " + String(menuID));
    }
    
    return std::nullopt;
}

// This method has been removed as it's not declared in the header file

void covertracks::PlayerEditorComponent::takeOwnershipOfPlayer(std::unique_ptr<Player> newPlayer)
{
    // Take ownership of the player
    ownedPlayer = std::move(newPlayer);
    player = ownedPlayer.get();
    
    // Update the UI
    updateUIFromPlayer();
}

std::unique_ptr<Player> covertracks::PlayerEditorComponent::releaseOwnedPlayer()
{
    // If we don't own a player, return nullptr
    if (!ownedPlayer)
        return nullptr;
    
    // Remove the player from our tracking map
    if (playerLoadedMap.find(ownedPlayer.get()) != playerLoadedMap.end())
    {
        playerLoadedMap.erase(ownedPlayer.get());
    }
    
    // Get a copy of the player pointer before we release ownership
    player = nullptr;
    
    // Return ownership to the caller
    return std::move(ownedPlayer);
}





void covertracks::PlayerEditorComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void covertracks::PlayerEditorComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}



} // namespace covertracks
