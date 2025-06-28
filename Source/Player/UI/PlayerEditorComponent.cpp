// PlayerEditorComponent.cpp with fixes
#include "PlayerEditorComponent.h"
#include "../../Plugins/InternalPlugins.h"
#include "../../UI/MainHostWindow.h"
#include <unordered_map>

namespace covertracks {

// Constructor
PlayerEditorComponent::PlayerEditorComponent(Player* playerToEdit, AudioEngineImpl* engine)
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

    // Set up category combo box with instrument types
    categoryEditor.addItem("Keyboard", 1);
    categoryEditor.addItem("Drums", 2);
    categoryEditor.addItem("Bass", 3);
    categoryEditor.addItem("Guitar", 4);
    categoryEditor.addItem("Other", 5);
    categoryEditor.setTextWhenNothingSelected("Select category");
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


    // Setup instrument capabilities UI
    setupCapabilitiesUI();

    // Setup the new Player system instead of the old graph editor
    if (player != nullptr && audioEngine != nullptr)
    {
        setupNewPlayerSystem();
        
        // Update the UI from the player data
        auto& metadata = player->getMetadata();
        nameEditor.setText(metadata.name);
        // Set the category combo box based on the metadata
        if (metadata.category == "Keyboard")
            categoryEditor.setSelectedId(1, dontSendNotification);
        else if (metadata.category == "Drums")
            categoryEditor.setSelectedId(2, dontSendNotification);
        else if (metadata.category == "Bass")
            categoryEditor.setSelectedId(3, dontSendNotification);
        else if (metadata.category == "Guitar")
            categoryEditor.setSelectedId(4, dontSendNotification);
        else
            categoryEditor.setSelectedId(5, dontSendNotification); // Other
        descriptionEditor.setText(metadata.description);
        
        // Update capabilities UI from player
        updateCapabilitiesInUI();
    }
}

PlayerEditorComponent::~PlayerEditorComponent()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
    instrumentTypeCombo.removeListener(this);
    capabilityTypeCombo.removeListener(this);
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
        
        // TODO: Replace with Tracktion track editing interface
        // Old GraphEditorPanel code removed - using Tracktion tracks now
        /*
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
                    
                    // The new AudioEngine registers players, not individual processors
                    // Processor registration is handled automatically when player is registered
                    bool success = true; // Assume success for now
                    if (!success)
                    {
                        // If registration failed, try unregistering first (in case it's already registered)
                        // Note: unregisterProcessor/registerProcessor not needed in new AudioEngine
                        success = true;
                        
                        if (!success)
                        {
                            std::cerr << "Failed to register graph with AudioEngine" << std::endl;
                        }
                    }
                    
                    // Enable MIDI input for the graph
                    audioEngine->enableMidiInput(true);
                    
                    // Note: Volume control now handled via ensemble/performance level settings
                    
                    // Connect the keyboard to the graph
                    connectMidiKeyboardToGraph();
                }
            }
        }
        */
        
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
        
        // Save plugin chain state (Tracktion-based approach)
        if (player && player->getInstrument())
        {
            std::cout << "Saving plugin chain state for player: " << player->getName() << std::endl;
            // Plugin state is automatically saved in Tracktion track
        }
        
        // Stop player if playing (Tracktion-based approach)
        if (player->isPlaying())
        {
            player->stop();
        }
        
        // Remove the player from our tracking map
        if (playerLoadedMap.find(player) != playerLoadedMap.end())
        {
            playerLoadedMap.erase(player);
        }
        
        // Resources are automatically managed by Tracktion
        
        std::cout << "Player resources released after editing" << std::endl;
    }
}

void PlayerEditorComponent::updatePlayerFromUI()
{
    if (player != nullptr)
    {
        // Get category from the combo box
        juce::String category;
        int categoryId = categoryEditor.getSelectedId();
        switch (categoryId) {
            case 1: category = "Keyboard"; break;
            case 2: category = "Drums"; break;
            case 3: category = "Bass"; break;
            case 4: category = "Guitar"; break;
            case 5: category = "Other"; break;
            default: category = "Other"; break;
        }
        
        // Use backend method to update metadata
        player->updateMetadataFromUI(nameEditor.getText(), category, descriptionEditor.getText());
        
        // Use backend method to update capabilities
        player->updateCapabilitiesFromUI(
            instrumentTypeCombo.getText(),
            capabilityTypeCombo.getSelectedId(),
            (int)minNoteSlider.getValue(),
            (int)maxNoteSlider.getValue(),
            soundMappingEditor.getText()
        );
        
        // Transfer plugin state from playerInstance to player using backend method
        if (playerInstance != nullptr)
        {
            std::cout << "Transferring plugin state from playerInstance to player..." << std::endl;
            
            if (player->transferPluginStateFrom(playerInstance.get()))
            {
                std::cout << "Plugin state transfer completed successfully" << std::endl;
            }
            else
            {
                std::cerr << "Failed to transfer plugin state to main player" << std::endl;
            }
        }
        
        // TODO: Replace with Tracktion track plugin updates
        // Old GraphEditorPanel update code removed
        /*
        if (graphEditor != nullptr && graphEditor->graph != nullptr)
        {
            auto graphXml = graphEditor->graph->createXml();
            if (graphXml != nullptr)
            {
                player->loadPluginGraph(*graphXml);
            }
        }
        */
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
    
    // Set the category combo box based on the metadata
    if (metadata.category == "Keyboard")
        categoryEditor.setSelectedId(1, dontSendNotification);
    else if (metadata.category == "Drums")
        categoryEditor.setSelectedId(2, dontSendNotification);
    else if (metadata.category == "Bass")
        categoryEditor.setSelectedId(3, dontSendNotification);
    else if (metadata.category == "Guitar")
        categoryEditor.setSelectedId(4, dontSendNotification);
    else
        categoryEditor.setSelectedId(5, dontSendNotification); // Other
        
    descriptionEditor.setText(metadata.description);
    
    // Update capabilities UI
    updateCapabilitiesInUI();
    
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
        // TODO: Replace with Tracktion track UI updates
        // Old timer-based GraphEditorPanel update code removed
        /*
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
                    // Note: registerProcessor not needed in new AudioEngine
                    std::cout << "Registered processor for player: " << player->getMetadata().name << std::endl;
                }
            }
            graphEditor->graph->fixAudioOutputNodes();
            graphEditor->graphPanel->updateComponents();
            graphEditor->graphPanel->repaint();
            graphEditor->resized();
        }
        */
        
        updateCount++;
        
        // If we've done less than 3 updates, schedule another one
        if (updateCount < 3)
            startTimer(500);
    }
}

void PlayerEditorComponent::connectMidiKeyboardToGraph()
{
    // Connect MIDI keyboard to player using Tracktion approach
    if (audioEngine != nullptr && player != nullptr)
    {
        // Enable MIDI input
        audioEngine->enableMidiInput(true);
        
        // MIDI routing is handled automatically by Tracktion tracks
        std::cout << "MIDI keyboard connected to player via Tracktion track" << std::endl;
        
        std::cout << "Connected MIDI keyboard to graph for player" << std::endl;
    }
}

void PlayerEditorComponent::handleIncomingMidiMessage(MidiInput* /*source*/, const MidiMessage& /*message*/)
{
    // We don't need to handle external MIDI messages directly
    // The AudioEngine will route them to the graph automatically
}

void PlayerEditorComponent::handleNoteOn(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
    if (playerInstance)
    {
        auto message = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
        playerInstance->processMidiMessage(message);
        std::cout << "MIDI Note On: " << midiNoteNumber << " velocity: " << velocity << std::endl;
    }
}

void PlayerEditorComponent::handleNoteOff(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float /*velocity*/)
{
    if (playerInstance)
    {
        auto message = juce::MidiMessage::noteOff(midiChannel, midiNoteNumber);
        playerInstance->processMidiMessage(message);
        std::cout << "MIDI Note Off: " << midiNoteNumber << std::endl;
    }
}

void PlayerEditorComponent::createPlugin(const juce::PluginDescription& desc, juce::Point<int> pos)
{
    // Use the new plugin chain editor instead of old graph editor
    if (pluginChainEditor != nullptr && player)
    {
        // Log plugin description details for debugging
        DBG("Attempting to create plugin: " + desc.name);
        DBG("  Format: " + desc.pluginFormatName);
        
        // Add the plugin to the instrument's plugin chain
        if (auto* instrument = player->getInstrument())
        {
            instrument->addPlugin(desc);
            pluginChainEditor->refreshPluginList();
        }
    }
    else
    {
        DBG("Cannot create plugin: no plugin chain editor available");
    }
}

void PlayerEditorComponent::buttonClicked(Button* button)
{
    if (button == &saveButton)
    {
        // Update the player data from UI
        updatePlayerFromUI();
        
        // Update the player's name from the UI (important for file naming)
        auto newName = nameEditor.getText().trim();
        if (newName.isNotEmpty())
        {
            auto& metadata = player->getMetadata();
            metadata.name = newName;
        }
        
        // Transfer Player state back to the main Player object
        if (player)
        {
            // Update Player name to match the UI
            if (newName.isNotEmpty())
            {
                player->setName(newName);
            }
            
            // Save the Player's complete state and store it in the main Player
            auto completeState = player->saveCompleteState();
            
            // Store this state in the Player object so PlayerManager can save it
            player->setPlayerState(completeState);
            
            std::cout << "Player state transferred to Player object for persistence" << std::endl;
            std::cout << "Plugin count: " << player->getPluginCount() << std::endl;
            
            // Also update the capabilities from the UI (this is already done in updatePlayerFromUI())
            auto& capabilities = player->getInstrumentCapabilities();
            std::cout << "Saving instrument capabilities: type=" << (int)capabilities.type 
                      << ", instrumentType=" << capabilities.instrumentType << std::endl;
        }
        
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
}

void PlayerEditorComponent::comboBoxChanged(ComboBox* comboBox)
{
    if (comboBox == &capabilityTypeCombo || comboBox == &instrumentTypeCombo)
    {
        updateCapabilitiesUIVisibility();
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

    // Instrument Capabilities section
    capabilitiesLabel.setBounds(sidePanel.getX() + panelMargin, y, fieldWidth + labelWidth + labelFieldGap, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Instrument type
    instrumentTypeLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    instrumentTypeCombo.setBounds(instrumentTypeLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Capability type
    capabilityTypeLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    capabilityTypeCombo.setBounds(capabilityTypeLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Note range section
    noteRangeLabel.setBounds(sidePanel.getX() + panelMargin, y, fieldWidth + labelWidth + labelFieldGap, fieldHeight);
    y += fieldHeight + 4;

    // Min note
    minNoteLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    minNoteSlider.setBounds(minNoteLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + 4;

    // Max note
    maxNoteLabel.setBounds(sidePanel.getX() + panelMargin, y, labelWidth, fieldHeight);
    maxNoteSlider.setBounds(maxNoteLabel.getRight() + labelFieldGap, y, fieldWidth, fieldHeight);
    y += fieldHeight + fieldSpacing;

    // Sound mapping section
    soundMappingLabel.setBounds(sidePanel.getX() + panelMargin, y, fieldWidth + labelWidth + labelFieldGap, fieldHeight);
    y += fieldHeight + 4;

    soundMappingEditor.setBounds(sidePanel.getX() + panelMargin, y, fieldWidth + labelWidth + labelFieldGap, fieldHeight * 4);
    y += fieldHeight * 4 + 4;

    addPresetButton.setBounds(sidePanel.getX() + panelMargin, y, 100, fieldHeight);
    y += fieldHeight + fieldSpacing * 2;

    // Buttons
    const int buttonWidth = (fieldWidth + labelWidth + labelFieldGap - 2 * panelMargin) / 3;
    const int buttonHeight = 30;
    const int buttonY = sidePanel.getBottom() - panelMargin - buttonHeight;

    saveButton.setBounds(sidePanel.getX() + panelMargin, buttonY, buttonWidth, buttonHeight);
    cancelButton.setBounds(sidePanel.getX() + panelMargin + buttonWidth + panelMargin, buttonY, buttonWidth, buttonHeight);

    // --- Main area for plugin chain editor and MIDI keyboard ---
    if (pluginChainEditor && midiKeyboard)
    {
        // Split main area: MIDI keyboard at bottom, plugin editor above
        const int keyboardHeight = 80;
        auto keyboardArea = area.removeFromBottom(keyboardHeight);
        
        // Set MIDI keyboard bounds
        midiKeyboard->setBounds(keyboardArea);
        
        // Set plugin chain editor bounds
        pluginChainEditor->setBounds(area);
        
        std::cout << "Layout: PluginChainEditor=" << area.toString() << ", MidiKeyboard=" << keyboardArea.toString() << std::endl;
    }
    else
    {
        // No plugin chain editor available - just clear the area
        DBG("[PlayerEditorComponent::resized] No plugin chain editor available");
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

// getChosenType method removed - not needed in Tracktion-based approach

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


void PlayerEditorComponent::setupCapabilitiesUI()
{
    // Capabilities section label
    capabilitiesLabel.setText("Instrument Capabilities", dontSendNotification);
    capabilitiesLabel.setFont(Font(FontOptions(16.0f, Font::bold)));
    addAndMakeVisible(capabilitiesLabel);
    
    // Instrument type
    instrumentTypeLabel.setText("Instrument Type:", dontSendNotification);
    instrumentTypeLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(instrumentTypeLabel);
    
    instrumentTypeCombo.addItem("drums", 1);
    instrumentTypeCombo.addItem("piano", 2);
    instrumentTypeCombo.addItem("bass", 3);
    instrumentTypeCombo.addItem("guitar", 4);
    instrumentTypeCombo.addItem("other", 5);
    instrumentTypeCombo.setTextWhenNothingSelected("Select type");
    instrumentTypeCombo.addListener(this);
    addAndMakeVisible(instrumentTypeCombo);
    
    // Capability type
    capabilityTypeLabel.setText("Capability Type:", dontSendNotification);
    capabilityTypeLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(capabilityTypeLabel);
    
    capabilityTypeCombo.addItem("Note Range", 1);
    capabilityTypeCombo.addItem("Sound Mapping", 2);
    capabilityTypeCombo.setSelectedId(1, dontSendNotification);
    capabilityTypeCombo.addListener(this);
    addAndMakeVisible(capabilityTypeCombo);
    
    // Note range controls
    noteRangeLabel.setText("MIDI Note Range", dontSendNotification);
    noteRangeLabel.setFont(Font(FontOptions(14.0f, Font::bold)));
    addAndMakeVisible(noteRangeLabel);
    
    minNoteLabel.setText("Min Note:", dontSendNotification);
    minNoteLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(minNoteLabel);
    
    minNoteSlider.setRange(0, 127, 1);
    minNoteSlider.setValue(36); // C2
    minNoteSlider.setSliderStyle(Slider::LinearHorizontal);
    minNoteSlider.setTextBoxStyle(Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(minNoteSlider);
    
    maxNoteLabel.setText("Max Note:", dontSendNotification);
    maxNoteLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(maxNoteLabel);
    
    maxNoteSlider.setRange(0, 127, 1);
    maxNoteSlider.setValue(79); // G5
    maxNoteSlider.setSliderStyle(Slider::LinearHorizontal);
    maxNoteSlider.setTextBoxStyle(Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(maxNoteSlider);
    
    // Sound mapping controls
    soundMappingLabel.setText("Sound Mappings (format: soundName:midiNote)", dontSendNotification);
    soundMappingLabel.setFont(Font(FontOptions(14.0f, Font::bold)));
    addAndMakeVisible(soundMappingLabel);
    
    soundMappingEditor.setMultiLine(true);
    soundMappingEditor.setReturnKeyStartsNewLine(true);
    soundMappingEditor.setScrollbarsShown(true);
    soundMappingEditor.setCaretVisible(true);
    soundMappingEditor.setPopupMenuEnabled(true);
    soundMappingEditor.setText("kick:36\nsnare:38\nhihat:42");
    addAndMakeVisible(soundMappingEditor);
    
    addPresetButton.setButtonText("Add Preset");
    addPresetButton.onClick = [this] {
        PopupMenu menu;
        menu.addItem(1, "Drums (GM Standard)");
        menu.addItem(2, "Piano (C2-G5)");
        menu.addItem(3, "Bass (E1-G3)");
        menu.addItem(4, "Guitar (E2-E6)");
        
        menu.showMenuAsync(PopupMenu::Options(), [this](int result) {
            if (result > 0)
            {
                InstrumentCapabilities capabilities;
                switch (result)
                {
                    case 1: capabilities = Player::createDrumCapabilities(); break;
                    case 2: capabilities = Player::createPianoCapabilities(); break;
                    case 3: capabilities = Player::createBassCapabilities(); break;
                    case 4: capabilities = Player::createGuitarCapabilities(); break;
                }
                
                // Update UI with preset
                instrumentTypeCombo.setText(capabilities.instrumentType, dontSendNotification);
                if (capabilities.type == InstrumentCapabilityType::Mapping)
                {
                    capabilityTypeCombo.setSelectedId(2, dontSendNotification);
                    String mappingText;
                    for (const auto& mapping : capabilities.soundMappings)
                    {
                        mappingText += mapping.first + ":" + String(mapping.second) + "\n";
                    }
                    soundMappingEditor.setText(mappingText);
                }
                else
                {
                    capabilityTypeCombo.setSelectedId(1, dontSendNotification);
                    minNoteSlider.setValue(capabilities.minNote);
                    maxNoteSlider.setValue(capabilities.maxNote);
                }
                updateCapabilitiesUIVisibility();
            }
        });
    };
    addAndMakeVisible(addPresetButton);
    
    // Initially update visibility
    updateCapabilitiesUIVisibility();
}

void PlayerEditorComponent::updateCapabilitiesUIVisibility()
{
    bool isMapping = capabilityTypeCombo.getSelectedId() == 2;
    
    // Show/hide note range controls
    noteRangeLabel.setVisible(!isMapping);
    minNoteLabel.setVisible(!isMapping);
    minNoteSlider.setVisible(!isMapping);
    maxNoteLabel.setVisible(!isMapping);
    maxNoteSlider.setVisible(!isMapping);
    
    // Show/hide sound mapping controls
    soundMappingLabel.setVisible(isMapping);
    soundMappingEditor.setVisible(isMapping);
}

// updateCapabilitiesFromUI method removed - now using Player::updateCapabilitiesFromUI backend method

void PlayerEditorComponent::updateCapabilitiesInUI()
{
    if (!player) return;
    
    const auto& capabilities = player->getInstrumentCapabilities();
    
    instrumentTypeCombo.setText(capabilities.instrumentType, dontSendNotification);
    capabilityTypeCombo.setSelectedId(
        (capabilities.type == InstrumentCapabilityType::Mapping) ? 2 : 1, 
        dontSendNotification);
    
    if (capabilities.type == InstrumentCapabilityType::Mapping)
    {
        String mappingText;
        for (const auto& mapping : capabilities.soundMappings)
        {
            mappingText += mapping.first + ":" + String(mapping.second) + "\n";
        }
        soundMappingEditor.setText(mappingText, dontSendNotification);
    }
    else
    {
        minNoteSlider.setValue(capabilities.minNote, dontSendNotification);
        maxNoteSlider.setValue(capabilities.maxNote, dontSendNotification);
    }
    
    updateCapabilitiesUIVisibility();
}

void PlayerEditorComponent::setupNewPlayerSystem()
{
    std::cout << "PlayerEditorComponent: Setting up new Player system..." << std::endl;
    
    if (!audioEngine)
    {
        std::cout << "No audio engine available" << std::endl;
        return;
    }
    
    // Create a Player for this player's data
    auto& metadata = player->getMetadata();
    playerInstance = std::make_unique<Player>(
        metadata.name + "_Player", 
        metadata.name,
        audioEngine->getFormatManager(),
        audioEngine->getKnownPluginList()
    );
    
    if (!playerInstance->initialize())
    {
        std::cout << "Failed to initialize Player" << std::endl;
        return;
    }
    
    // Try to load existing state from the Player object (via PlayerManager)
    auto playerName = metadata.name;
    
    // Check if the Player has stored Player state
    if (player && player->hasPlayerState())
    {
        std::cout << "Loading Player state from Player object..." << std::endl;
        auto storedState = player->getPlayerState();
        
        if (playerInstance->loadCompleteState(storedState))
        {
            std::cout << "Player state loaded successfully from Player object!" << std::endl;
        }
        else
        {
            std::cerr << "Failed to load Player state from Player object" << std::endl;
        }
    }
    else
    {
        std::cout << "No stored Player state found in Player object" << std::endl;
        std::cout << "Starting with empty plugin configuration" << std::endl;
    }
    
    // Create the plugin chain editor
    pluginChainEditor = std::make_unique<PluginChainEditor>();
    pluginChainEditor->setPlayer(playerInstance.get());
    addAndMakeVisible(pluginChainEditor.get());
    
    // Create MIDI keyboard to replace the tone generation buttons
    midiKeyboard = std::make_unique<juce::MidiKeyboardComponent>(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);
    midiKeyboard->setName("MIDI Keyboard");
    addAndMakeVisible(midiKeyboard.get());
    
    // Connect MIDI keyboard to the Player
    keyboardState.addListener(this);
    
    std::cout << "Player system setup complete" << std::endl;
    std::cout << "Current plugin count: " << (playerInstance->getInstrument() ? playerInstance->getInstrument()->getPluginCount() : 0) << std::endl;
    
    // Debug: Show current capabilities
    if (playerInstance) {
        auto& capabilities = playerInstance->getInstrumentCapabilities();
        std::cout << "Loaded capabilities: type=" << (int)capabilities.type 
                  << ", instrumentType=" << capabilities.instrumentType
                  << ", soundMappings.size()=" << capabilities.soundMappings.size() << std::endl;
    }
}

void PlayerEditorComponent::switchToPluginChainEditor()
{
    // Old graph editor has been removed
    
    // Show the new plugin chain editor
    if (pluginChainEditor)
    {
        pluginChainEditor->setVisible(true);
    }
    
    resized(); // Update layout
}

} // namespace covertracks
