#include "PerformanceEditorComponent.h"
#include "../../../LLM/Core/PromptManager.h"

namespace covertracks {

PerformanceEditorComponent::PerformanceEditorComponent(PerformanceManager* pm,
                                                     std::shared_ptr<LLMClient> client,
                                                     PlayerManager* pm2)
    : performanceManager(pm),
      llmClient(client),
      playerManager(pm2),
      titleLabel("titleLabel", "Performance Editor"),
      nameEditor("nameEditor"),
      descriptionEditor("descriptionEditor"),
      songLabel("songLabel", "Song"),
      ensembleLabel("ensembleLabel", "Ensemble"),
      styleLabel("styleLabel", "Style"),
      characterLabel("characterLabel", "Character"),
      coordinationLabel("coordinationLabel", "Coordination"),
      generateAllButton("Generate All"),
      prepAllButton("Prep All"),
      loadAllButton("Load All Charts"),
      playAllButton("Play All"),
      stopAllButton("Stop All"),
      saveButton("Save"),
      backButton("Back"),
      statusDisplay("statusDisplay")
{
    // Set up title label
    titleLabel.setFont(Font(FontOptions().withHeight(20.0f).withStyle("bold")));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up name editor
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.addListener(this);
    addAndMakeVisible(nameEditor);
    
    // Set up description editor
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.addListener(this);
    addAndMakeVisible(descriptionEditor);
    
    // Set up song section
    songLabel.setFont(Font(FontOptions().withHeight(16.0f).withStyle("bold")));
    addAndMakeVisible(songLabel);
    
    songSelector.addListener(this);
    addAndMakeVisible(songSelector);
    
    // Set up ensemble section
    ensembleLabel.setFont(Font(FontOptions().withHeight(16.0f).withStyle("bold")));
    addAndMakeVisible(ensembleLabel);
    
    ensembleSelector.addListener(this);
    addAndMakeVisible(ensembleSelector);
    
    // Set up style section
    styleLabel.setFont(Font(FontOptions().withHeight(16.0f).withStyle("bold")));
    addAndMakeVisible(styleLabel);
    
    styleSelector.addItem("Jazz", 1);
    styleSelector.addItem("Pop", 2);
    styleSelector.addItem("Rock", 3);
    styleSelector.addItem("Classical", 4);
    styleSelector.setSelectedItemIndex(0);
    styleSelector.addListener(this);
    addAndMakeVisible(styleSelector);
    
    // Set up character section
    characterLabel.setFont(Font(FontOptions().withHeight(16.0f).withStyle("bold")));
    addAndMakeVisible(characterLabel);
    
    characterEditor.setMultiLine(false);
    characterEditor.setReturnKeyStartsNewLine(false);
    characterEditor.addListener(this);
    addAndMakeVisible(characterEditor);
    
    // Set up player table
    playerTable = std::make_unique<PlayerTable>(this);
    addAndMakeVisible(playerTable.get());
    
    // Set up coordination section
    coordinationLabel.setFont(Font(FontOptions().withHeight(16.0f).withStyle("bold")));
    addAndMakeVisible(coordinationLabel);
    
    generateAllButton.addListener(this);
    addAndMakeVisible(generateAllButton);
    
    prepAllButton.addListener(this);
    addAndMakeVisible(prepAllButton);
    
    loadAllButton.addListener(this);
    addAndMakeVisible(loadAllButton);
    
    playAllButton.addListener(this);
    addAndMakeVisible(playAllButton);
    
    stopAllButton.addListener(this);
    addAndMakeVisible(stopAllButton);
    
    // Set up action buttons
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    backButton.addListener(this);
    addAndMakeVisible(backButton);
    
    // Set up status display
    statusDisplay.setMultiLine(true);
    statusDisplay.setReadOnly(true);
    statusDisplay.setColour(TextEditor::backgroundColourId, Colours::black.withAlpha(0.1f));
    addAndMakeVisible(statusDisplay);
    
    // Set up pattern editor (initially hidden)
    patternEditor = std::make_unique<PatternEditorComponent>(playerManager);
    patternEditor->addListener(this);
    patternEditor->setVisible(false);
    addAndMakeVisible(*patternEditor);
    
    // Set up LLM client for all players in the PlayerManager
    if (llmClient && playerManager)
    {
        auto players = playerManager->getPlayers();
        for (auto* player : players)
        {
            if (player)
            {
                player->setLLMClient(llmClient);
            }
        }
    }
    
    // Set up song and ensemble selectors
    updateSongSelector();
    updateEnsembleSelector();
    
    // Create default new performance
    createNewPerformance();
}

PerformanceEditorComponent::~PerformanceEditorComponent()
{
    // Note: Generation state is now managed by the Performance class
    
    nameEditor.removeListener(this);
    descriptionEditor.removeListener(this);
    
    songSelector.removeListener(this);
    ensembleSelector.removeListener(this);
    styleSelector.removeListener(this);
    characterEditor.removeListener(this);
    loadAllButton.removeListener(this);
    playAllButton.removeListener(this);
    stopAllButton.removeListener(this);
    saveButton.removeListener(this);
    backButton.removeListener(this);
}

void PerformanceEditorComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void PerformanceEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    if (isInPatternEditMode)
    {
        // In pattern edit mode, pattern editor takes the entire space
        patternEditor->setBounds(bounds);
    }
    else
    {
        // Normal performance editor layout
        
        // Title at top
        titleLabel.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(10);
        
        // Name and description
        auto nameArea = bounds.removeFromTop(25);
        nameEditor.setBounds(nameArea);
        bounds.removeFromTop(5);
        
        auto descArea = bounds.removeFromTop(60);
        descriptionEditor.setBounds(descArea);
        bounds.removeFromTop(10);
        
        // Song section
        songLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        songSelector.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        // Ensemble section
        ensembleLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        ensembleSelector.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        // Style section
        styleLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        styleSelector.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        // Character section
        characterLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        characterEditor.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        // Player table takes most of the space
        auto tableHeight = static_cast<int>(bounds.getHeight() * 0.4f);
        playerTable->setBounds(bounds.removeFromTop(tableHeight));
        bounds.removeFromTop(10);
        
        // Coordination section
        coordinationLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        auto coordButtons = bounds.removeFromTop(30);
        int buttonWidth = coordButtons.getWidth() / 5;
        generateAllButton.setBounds(coordButtons.removeFromLeft(buttonWidth).reduced(2));
        prepAllButton.setBounds(coordButtons.removeFromLeft(buttonWidth).reduced(2));
        loadAllButton.setBounds(coordButtons.removeFromLeft(buttonWidth).reduced(2));
        playAllButton.setBounds(coordButtons.removeFromLeft(buttonWidth).reduced(2));
        stopAllButton.setBounds(coordButtons.reduced(2));
        bounds.removeFromTop(10);
        
        // Status display
        auto statusHeight = static_cast<int>(bounds.getHeight() * 0.3f);
        statusDisplay.setBounds(bounds.removeFromTop(statusHeight));
        bounds.removeFromTop(5);
        
        // Action buttons at bottom
        auto actionButtons = bounds.removeFromTop(30);
        saveButton.setBounds(actionButtons.removeFromLeft(actionButtons.getWidth() / 2).reduced(2));
        backButton.setBounds(actionButtons.reduced(2));
    }
}

void PerformanceEditorComponent::buttonClicked(Button* button)
{
    if (button == &generateAllButton)
    {
        generateAllCharts();
    }
    else if (button == &prepAllButton)
    {
        prepAllCharts();
    }
    else if (button == &loadAllButton)
    {
        loadAllCharts();
    }
    else if (button == &playAllButton)
    {
        playAllPlayers();
    }
    else if (button == &stopAllButton)
    {
        stopAllPlayers();
    }
    else if (button == &saveButton)
    {
        saveCurrentPerformance();
    }
    else if (button == &backButton)
    {
        if (backCallback)
            backCallback();
    }
}

void PerformanceEditorComponent::textEditorReturnKeyPressed(TextEditor& /*editor*/)
{
    // Handle return key - update performance if needed
    updateTitle();
}

void PerformanceEditorComponent::textEditorEscapeKeyPressed(TextEditor& /*editor*/)
{
    // Handle escape key - revert changes if needed
}

void PerformanceEditorComponent::textEditorFocusLost(TextEditor& /*editor*/)
{
    // Handle focus lost - save changes
    updateTitle();
}

void PerformanceEditorComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &songSelector)
    {
        songSelectionChanged();
    }
    else if (comboBoxThatHasChanged == &ensembleSelector)
    {
        ensembleSelectionChanged();
    }
    else if (comboBoxThatHasChanged == &styleSelector)
    {
        // Style selection changed - no immediate action needed
        // Style will be used during pattern generation
    }
}

void PerformanceEditorComponent::loadPerformance(Performance* performance)
{
    currentPerformance = performance;
    isNewPerformance = (performance == nullptr);
    
    if (currentPerformance)
    {
        // Clean up any stale status messages from older format versions
        currentPerformance->cleanupStaleStatusMessages();
        
        // Load existing performance
        nameEditor.setText(currentPerformance->getName());
        descriptionEditor.setText(currentPerformance->getDescription());
        
        // Set song selection if available
        if (auto song = currentPerformance->getSong())
        {
            // Find and select the song in the combo box
            for (int i = 0; i < songSelector.getNumItems(); ++i)
            {
                if (songSelector.getItemText(i) == song->getTitle())
                {
                    songSelector.setSelectedItemIndex(i, dontSendNotification);
                    break;
                }
            }
        }
        
        // Set ensemble selection
        String ensembleName = currentPerformance->getEnsembleName();
        for (int i = 0; i < ensembleSelector.getNumItems(); ++i)
        {
            if (ensembleSelector.getItemText(i) == ensembleName)
            {
                ensembleSelector.setSelectedItemIndex(i, dontSendNotification);
                break;
            }
        }
        
        // Load ensemble with PlayerManager to get proper player assignments
        refreshPerformanceEnsembleCache();
    }
    else
    {
        createNewPerformance();
    }
    
    updateUI();
}

void PerformanceEditorComponent::setBackCallback(std::function<void()> callback)
{
    backCallback = callback;
}

// Helper methods - stub implementations
void PerformanceEditorComponent::createNewPerformance()
{
    currentPerformance = nullptr;  // Will be created when saved
    isNewPerformance = true;
    
    nameEditor.setText("New Performance");
    descriptionEditor.setText("");
    updateTitle();
}

void PerformanceEditorComponent::updateUI()
{
    updateTitle();
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::updateTitle()
{
    String title = isNewPerformance ? "New Performance" : "Edit Performance";
    titleLabel.setText(title, dontSendNotification);
}

void PerformanceEditorComponent::saveCurrentPerformance()
{
    if (!performanceManager)
    {
        statusDisplay.setText("No performance manager available");
        return;
    }
    
    std::shared_ptr<Performance> performanceToSave;
    
    // For new performances, create the Performance object now
    if (isNewPerformance && !currentPerformance)
    {
        performanceToSave = std::make_shared<Performance>(nameEditor.getText());
        currentPerformance = performanceToSave.get(); // Keep raw pointer for UI
        
        // Apply pending data
        if (pendingEnsembleName.isNotEmpty())
        {
            currentPerformance->setEnsembleName(pendingEnsembleName);
        }
        if (pendingSong)
        {
            currentPerformance->setSong(pendingSong);
        }
    }
    else if (currentPerformance)
    {
        // For existing performances, we need to find the shared_ptr
        // For now, create a new shared_ptr (this is not ideal but works for the fix)
        performanceToSave = std::shared_ptr<Performance>(currentPerformance, [](Performance*){});
    }
    
    if (!performanceToSave)
    {
        statusDisplay.setText("No performance to save");
        return;
    }
    
    // Update performance with current form data
    performanceToSave->setName(nameEditor.getText());
    performanceToSave->setDescription(descriptionEditor.getText());
    
    // Save using PerformanceManager (returns void)
    try 
    {
        performanceManager->savePerformance(performanceToSave);
        isNewPerformance = false; // No longer new after saving
        statusDisplay.setText("Performance saved successfully");
        updateTitle();
    }
    catch (...)
    {
        statusDisplay.setText("Failed to save performance");
    }
}

void PerformanceEditorComponent::generateChartForPlayer(int playerIndex)
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    String roleName = role.getName();
    
    // Ensure player is initialized before generation
    if (role.player)
    {
        if (llmClient)
        {
            role.player->setLLMClient(llmClient);
        }
        
        if (!role.player->initialize())
        {
            statusDisplay.setText("Failed to initialize player " + roleName);
            return;
        }
    }
    
    // Set performance style and character from UI
    String style = styleSelector.getText();
    String character = characterEditor.getText();
    currentPerformance->setStyle(style);
    currentPerformance->setCharacter(character);
    
    // Use the clean backend method
    bool started = currentPerformance->generateChartForRole(
        roleName,
        role,
        llmClient.get(),
        "", // Use performance style
        "", // Use performance character  
        [this, roleName](bool success, const String& message)
        {
            // UI callback - just update display
            statusDisplay.setText(message);
            
            if (success)
            {
                statusDisplay.setText("LLM pattern generated for " + roleName + " - ready to prep");
            }
            else
            {
                statusDisplay.setText("Failed to generate pattern for " + roleName + ": " + message);
            }
            
            // Update the player table to show new status
            if (playerTable)
                playerTable->updateContent();
        }
    );
    
    if (!started)
    {
        statusDisplay.setText("Could not start generation for " + roleName);
    }
}

void PerformanceEditorComponent::loadChartForPlayer(int playerIndex)
{
    if (!currentPerformance || !playerManager)
    {
        statusDisplay.setText("No performance or player manager available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    statusDisplay.setText("Loading chart for " + role.getName() + "...");
    
    // Debug: Let's see what role names are actually stored
    auto storedRoleNames = currentPerformance->getGeneratedChartRoleNames();
    DBG("DEBUG: All stored charts in performance (" << storedRoleNames.size() << " total):");
    for (int i = 0; i < storedRoleNames.size(); ++i) {
        DBG("DEBUG:   - '" << storedRoleNames[i] << "'");
    }
    
    // Get the MIDI chart for this role (created by prep step)
    var chart = currentPerformance->getMIDIChart(role.getName());
    
    // Fallback to old generated chart if no MIDI chart exists
    if (chart.isVoid())
    {
        chart = currentPerformance->getGeneratedChart(role.getName());
    }
    
    // Debug: Let's see what's actually stored
    DBG("DEBUG: Looking for chart for role: " << role.getName());
    DBG("DEBUG: Chart isVoid: " << (chart.isVoid() ? "true" : "false"));
    DBG("DEBUG: Chart isObject: " << (chart.isObject() ? "true" : "false"));
    if (!chart.isVoid()) {
        DBG("DEBUG: Chart type: " << chart.toString());
    }
    
    if (chart.isVoid())
    {
        statusDisplay.setText("No chart generated for " + role.getName() + " - generate first");
        return;
    }
    
    // Debug: Check if chart has proper structure
    if (!chart.isObject())
    {
        statusDisplay.setText("Invalid chart format for " + role.getName());
        return;
    }
    
    // Use the player assigned to this role in the ensemble
    Player* player = role.player;
    if (!player)
    {
        statusDisplay.setText("No player assigned to role " + role.getName() + " (" + role.getInstrumentName() + ")");
        return;
    }
    
    // Apply the role's level setting to the player
    player->setLevel(role.level);
    
    // Create Player for proper Load→Play separation
    auto playerInstance = createPlayerFromPlayer(player);
    if (!playerInstance)
    {
        statusDisplay.setText("Failed to create Player for " + role.getName());
        currentPerformance->setStatusMessage(role.getName(), "Player creation failed");
        return;
    }
    
    // Initialize Player
    if (!playerInstance->initialize())
    {
        statusDisplay.setText("Failed to initialize Player for " + role.getName());
        currentPerformance->setStatusMessage(role.getName(), "Initialization failed");
        return;
    }
    
    // Extract MIDI events from chart object (chart has structure: {success, tempo, events})
    juce::var midiEvents;
    if (chart.isObject() && chart.hasProperty("events"))
    {
        midiEvents = chart["events"];
        DBG("Extracted " << (midiEvents.isArray() ? midiEvents.getArray()->size() : 0) << " MIDI events for loading");
    }
    else
    {
        // Fallback: assume chart is already the events array
        midiEvents = chart;
        DBG("Using chart directly as MIDI events array for loading");
    }
    
    // Load chart for later synchronized playback (pre-converts MIDI for instant start)
    if (player->loadChart(midiEvents, 120.0))
    {
        statusDisplay.setText("Chart loaded for " + role.getName() + " (level: " + String(role.level, 1) + "x)");
        currentPerformance->setStatusMessage(role.getName(), "Chart Loaded (Ready for Sync)");
        
        // Store Player for synchronized playback
        // TODO: Add member variable to store loaded Players per role
    }
    else
    {
        statusDisplay.setText("Failed to load chart for " + role.getName());
        currentPerformance->setStatusMessage(role.getName(), "Load Failed");
    }
}

void PerformanceEditorComponent::playPlayer(int playerIndex)
{
    if (!currentPerformance || !playerManager)
    {
        statusDisplay.setText("No performance or player manager available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    statusDisplay.setText("Playing " + role.getName() + " with unified system...");
    
    juce::StringArray singleRole;
    singleRole.add(role.getName());
    
    // Use unified Performance playWithEnsemble system for individual role
    if (currentPerformance->playWithEnsemble(playerManager, ensemble, singleRole))
    {
        statusDisplay.setText("Playing " + role.getName() + " with unified system");
    }
    else
    {
        statusDisplay.setText("Failed to start unified playback for " + role.getName());
    }
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::prepChartForPlayer(int playerIndex)
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    String roleName = role.getName();
    
    statusDisplay.setText("Prepping pattern for " + roleName + "...");
    
    // Use the clean backend method
    bool success = currentPerformance->prepareChartForRole(roleName, role);
    
    if (success)
    {
        statusDisplay.setText("Successfully prepped pattern for " + roleName);
    }
    else
    {
        statusDisplay.setText("Failed to prep pattern for " + roleName);
    }
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::editPatternForPlayer(int playerIndex)
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    
    // Get existing LLM pattern if available, otherwise start with empty pattern
    String llmPattern = currentPerformance->getLLMPattern(role.getName());
    if (llmPattern.isEmpty())
    {
        // Create a basic template pattern for the role
        llmPattern = "{\n";
        llmPattern += "  \"title\": \"" + role.getName() + " Pattern\",\n";
        llmPattern += "  \"tempo\": 120,\n";
        llmPattern += "  \"patterns\": {\n";
        llmPattern += "    \"main_pattern\": [\n";
        llmPattern += "      [1.0, \"C4\", 0.8, 0.5]\n";
        llmPattern += "    ]\n";
        llmPattern += "  },\n";
        llmPattern += "  \"sections\": {\n";
        llmPattern += "    \"main\": {\n";
        llmPattern += "      \"bars\": 4,\n";
        llmPattern += "      \"basePattern\": \"main_pattern\"\n";
        llmPattern += "    }\n";
        llmPattern += "  },\n";
        llmPattern += "  \"arrangement\": [\"main\"]\n";
        llmPattern += "}";
        
        statusDisplay.setText("Creating new pattern for " + role.getName());
    }
    else
    {
        statusDisplay.setText("Editing existing pattern for " + role.getName());
    }
    
    // Switch to pattern editor mode
    enterPatternEditMode(role.getName(), role.player, llmPattern);
}

void PerformanceEditorComponent::generateAllCharts()
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Generating all LLM patterns...");
    
    int generated = 0;
    int total = ensemble.size();
    
    // Use backend method for each role
    for (const auto& role : ensemble)
    {
        String roleName = role.getName();
        
        bool started = currentPerformance->generateChartForRole(
            roleName,
            role,
            llmClient.get(),
            "", // Use performance style
            "", // Use performance character
            [this, roleName](bool success, const String& message)
            {
                // UI callback - just update display
                if (success)
                {
                    statusDisplay.setText("LLM pattern generated for " + roleName + " - ready to prep");
                }
                else
                {
                    statusDisplay.setText("Failed to generate pattern for " + roleName + ": " + message);
                }
                
                // Update the player table
                if (playerTable)
                    playerTable->updateContent();
            }
        );
        
        if (started)
        {
            generated++;
        }
    }
    
    statusDisplay.setText("Started generation for " + String(generated) + " of " + String(total) + " players");
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::prepAllCharts()
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Prepping all LLM patterns to MIDI charts...");
    
    int prepped = 0;
    int total = ensemble.size();
    
    // Use backend method for each role
    for (const auto& role : ensemble)
    {
        String roleName = role.getName();
        
        // Use the backend preparation method
        if (currentPerformance->prepareChartForRole(roleName, role, 0.0))
        {
            prepped++;
        }
    }
    
    statusDisplay.setText("Prepped " + String(prepped) + " of " + String(total) + " patterns");
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::loadAllCharts()
{
    if (!currentPerformance || !playerManager)
    {
        statusDisplay.setText("No performance or player manager available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Loading all charts with unified system...");
    
    // Use unified Performance loadChartsWithEnsemble system (Load→Play separation)
    int loaded = currentPerformance->loadChartsWithEnsemble(playerManager, ensemble);
    
    statusDisplay.setText("Loaded " + String(loaded) + " charts with unified system");
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::playAllPlayers()
{
    if (!currentPerformance || !playerManager)
    {
        statusDisplay.setText("No performance or player manager available");
        return;
    }
    
    auto ensemble = getPerformanceEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Starting unified ensemble playback...");
    
    // Use unified Performance playWithEnsemble system (plays all roles by default)
    if (currentPerformance->playWithEnsemble(playerManager, ensemble))
    {
        statusDisplay.setText("Unified ensemble playback started!");
    }
    else
    {
        statusDisplay.setText("Failed to start unified playback");
    }
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::stopAllPlayers()
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    statusDisplay.setText("Stopping all playback...");
    
    // Use unified Performance stop system
    currentPerformance->stopAll();
    
    // Also stop individual Players (legacy support)
    if (!activePlayers.empty())
    {
        for (auto& pair : activePlayers)
        {
            pair.second->stop();
        }
        activePlayers.clear();
    }
    
    // Stop EnsembleManager if active (legacy support)
    if (ensembleManager)
    {
        ensembleManager->stopPlayback();
    }
    
    statusDisplay.setText("All playback stopped!");
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::updateSongSelector()
{
    songSelector.clear();
    songSelector.addItem("No Song Selected", 1);
    
    // Load songs using PerformanceManager
    if (performanceManager)
    {
        // Get songs directory and load all song files
        File songsDir = File::getSpecialLocation(File::userApplicationDataDirectory)
            .getChildFile("Covertracks")
            .getChildFile("Songs");
        
        Array<File> songFiles;
        songsDir.findChildFiles(songFiles, File::findFiles, false, "*.json");
        
        int itemIndex = 2; // Start after "No Song Selected"
        for (const auto& file : songFiles)
        {
            try
            {
                String jsonContent = file.loadFileAsString();
                var jsonVar = JSON::parse(jsonContent);
                
                if (jsonVar.hasProperty("title"))
                {
                    String songTitle = jsonVar["title"].toString();
                    songSelector.addItem(songTitle, itemIndex++);
                }
            }
            catch (...)
            {
                // Skip files that can't be parsed
            }
        }
    }
    
    songSelector.setSelectedItemIndex(0); // Default to "No Song Selected"
}

void PerformanceEditorComponent::songSelectionChanged()
{
    String selectedSong = songSelector.getText();
    if (selectedSong == "No Song Selected")
    {
        statusDisplay.setText("No song selected");
        if (currentPerformance)
            currentPerformance->setSong(nullptr);
    }
    else
    {
        statusDisplay.setText("Selected song: " + selectedSong);
        
        // Load the selected song and set it on the performance (or pending data)
        if (performanceManager)
        {
            // Find the song by name and load it
            File songsDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                .getChildFile("Covertracks")
                .getChildFile("Songs");
            
            Array<File> songFiles;
            songsDir.findChildFiles(songFiles, File::findFiles, false, "*.json");
            
            for (const auto& file : songFiles)
            {
                try
                {
                    String jsonContent = file.loadFileAsString();
                    var jsonVar = JSON::parse(jsonContent);
                    
                    if (jsonVar.hasProperty("title") && jsonVar["title"].toString() == selectedSong)
                    {
                        auto song = Song::fromVar(jsonVar);
                        if (song)
                        {
                            if (currentPerformance)
                            {
                                currentPerformance->setSong(song);
                            }
                            else
                            {
                                // Store in pending data for new performances
                                pendingSong = song;
                            }
                            break;
                        }
                    }
                }
                catch (...)
                {
                    // Skip files that can't be parsed
                }
            }
        }
    }
}

void PerformanceEditorComponent::updateEnsembleSelector()
{
    ensembleSelector.clear();
    ensembleSelector.addItem("No Ensemble Selected", 1);
    
    // Load real ensembles from disk
    juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    int itemIndex = 2; // Start after "No Ensemble Selected"
    for (const auto& file : ensembleFiles)
    {
        // Load the ensemble to get its name
        auto ensemble = EnsembleManager::loadFromFile(file, nullptr); // We don't need PlayerManager just to get the name
        if (ensemble)
        {
            String ensembleName = ensemble->getName();
            ensembleSelector.addItem(ensembleName, itemIndex++);
        }
    }
    
    ensembleSelector.setSelectedItemIndex(0); // Default to "No Ensemble Selected"
}

void PerformanceEditorComponent::ensembleSelectionChanged()
{
    String selectedEnsemble = ensembleSelector.getText();
    if (selectedEnsemble == "No Ensemble Selected")
    {
        statusDisplay.setText("No ensemble selected");
        // Clear the ensemble manager when no ensemble is selected
        ensembleManager.reset();
        if (currentPerformance)
        {
            currentPerformance->setEnsembleName("");
        }
        else
        {
            // For new performances, store in pending data
            pendingEnsembleName = "";
        }
    }
    else
    {
        statusDisplay.setText("Selected ensemble: " + selectedEnsemble);
        if (currentPerformance)
        {
            // Handle pattern preservation when switching ensembles
            handleEnsembleSwitch(selectedEnsemble);
        }
        else
        {
            // For new performances, store in pending data
            pendingEnsembleName = selectedEnsemble;
        }
    }
    
    // Update the player table to show the new ensemble
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::handleEnsembleSwitch(const String& newEnsembleName)
{
    if (!currentPerformance)
        return;
        
    statusDisplay.setText("Switching to ensemble: " + newEnsembleName);
    
    // Step 1: Preserve existing LLM patterns by role name
    std::map<String, String> preservedPatterns;
    auto currentEnsemble = getPerformanceEnsemble();
    for (const auto& role : currentEnsemble)
    {
        String pattern = currentPerformance->getLLMPattern(role.getName());
        if (pattern.isNotEmpty())
        {
            preservedPatterns[role.getName()] = pattern;
            DBG("Preserving pattern for role: " << role.getName());
        }
    }
    
    // Step 2: Clear MIDI charts since they're player-specific
    for (const auto& role : currentEnsemble)
    {
        currentPerformance->setMIDIChart(role.getName(), juce::var());
        currentPerformance->setStatusMessage(role.getName(), "");
        DBG("Cleared MIDI chart for role: " << role.getName());
    }
    
    // Step 3: Set the new ensemble name and refresh cache
    currentPerformance->setEnsembleName(newEnsembleName);
    refreshPerformanceEnsembleCache();
    
    // Step 4: Restore patterns for matching role names in new ensemble
    auto newEnsemble = getPerformanceEnsemble();
    int restoredCount = 0;
    for (const auto& role : newEnsemble)
    {
        auto it = preservedPatterns.find(role.getName());
        if (it != preservedPatterns.end())
        {
            currentPerformance->setLLMPattern(role.getName(), it->second);
            currentPerformance->setStatusMessage(role.getName(), "Pattern Preserved");
            restoredCount++;
            DBG("Restored pattern for role: " << role.getName());
        }
        else
        {
            currentPerformance->setStatusMessage(role.getName(), "New Role - No Pattern");
            DBG("New role (no pattern to restore): " << role.getName());
        }
    }
    
    // Step 5: Update status message
    if (restoredCount > 0)
    {
        statusDisplay.setText("Ensemble switched. Preserved " + String(restoredCount) + " patterns, cleared MIDI charts.");
    }
    else
    {
        statusDisplay.setText("Ensemble switched. No patterns to preserve, cleared MIDI charts.");
    }
    
    DBG("Ensemble switch complete. Preserved " << restoredCount << " patterns from " << preservedPatterns.size() << " total.");
}

// PlayerTable implementation
PerformanceEditorComponent::PlayerTable::PlayerTable(PerformanceEditorComponent* parentComponent)
    : parent(parentComponent)
{
    table.setModel(this);
    table.getHeader().addColumn("Player", 1, 100);
    table.getHeader().addColumn("Instrument", 2, 100);
    table.getHeader().addColumn("Chart Status", 3, 100);
    table.getHeader().addColumn("Level", 4, 80);
    table.getHeader().addColumn("Actions", 5, 200);
    addAndMakeVisible(table);
}

PerformanceEditorComponent::PlayerTable::~PlayerTable()
{
    table.setModel(nullptr);
    clearButtons();
}

void PerformanceEditorComponent::PlayerTable::resized()
{
    table.setBounds(getLocalBounds());
}

int PerformanceEditorComponent::PlayerTable::getNumRows()
{
    if (!parent) return 0;
    
    // Use the cached ensemble roles that have proper player assignments
    if (parent->currentPerformance)
    {
        // Use the cached roles from PerformanceEditorComponent which have PlayerManager assignments
        return parent->cachedEnsembleRoles.size();
    }
    else if (parent->isNewPerformance && parent->pendingEnsembleName.isNotEmpty())
    {
        // For new performances, load ensemble from pending name
        return getEnsembleRolesFromName(parent->pendingEnsembleName).size();
    }
    
    return 0; // No players initially
}

void PerformanceEditorComponent::PlayerTable::paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(getLookAndFeel().findColour(TextEditor::highlightColourId));
    else
        g.fillAll(getLookAndFeel().findColour(ListBox::backgroundColourId));
}

void PerformanceEditorComponent::PlayerTable::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setColour(getLookAndFeel().findColour(ListBox::textColourId));
    g.setFont(Font(FontOptions().withHeight(14.0f)));
    
    String text = "Player " + String(rowNumber + 1);
    
    // Get actual ensemble data if available
    if (parent && parent->currentPerformance)
    {
        // Use the cached ensemble roles that have proper player assignments
        const auto& ensemble = parent->cachedEnsembleRoles;
        if (rowNumber < ensemble.size())
        {
            const auto& role = ensemble[rowNumber];
            if (columnId == 1) text = role.getName();
            if (columnId == 2) text = role.getInstrumentName();
            if (columnId == 3) 
            {
                // Check if chart actually exists first
                auto chart = parent->currentPerformance->getGeneratedChart(role.getName());
                if (!chart.isVoid())
                {
                    // Chart exists, show the stored status or default to "Chart Generated"
                    String status = parent->currentPerformance->getStatusMessage(role.getName());
                    text = status.isEmpty() ? "Chart Generated" : status;
                }
                else
                {
                    // No chart exists, always show "No Chart" regardless of stored status
                    text = "No Chart";
                }
            }
            if (columnId == 4) text = String(role.level, 1) + "x"; // Show level as "1.0x", "1.5x", etc.
        }
    }
    else if (parent && parent->isNewPerformance && parent->pendingEnsembleName.isNotEmpty())
    {
        // For new performances, get ensemble from pending name
        auto ensemble = getEnsembleRolesFromName(parent->pendingEnsembleName);
        if (rowNumber < ensemble.size())
        {
            const auto& role = ensemble[rowNumber];
            if (columnId == 1) text = role.getName();
            if (columnId == 2) text = role.getInstrumentName();
            if (columnId == 3) text = "No Chart"; // New performance has no charts yet
            if (columnId == 4) text = String(role.level, 1) + "x"; // Show level
        }
    }
    else
    {
        // Fallback for when no performance is loaded
        if (columnId == 2) text = "Piano"; // Default instrument
        if (columnId == 3) text = "No Chart";
        if (columnId == 4) text = "1.0x"; // Default level
    }
    
    g.drawText(text, Rectangle<int>(2, 0, width - 4, height), Justification::centredLeft, true);
}

Component* PerformanceEditorComponent::PlayerTable::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate)
{
    if (columnId == 4) // Level column
    {
        // Create a level slider component
        class LevelSliderComponent : public Component, public Slider::Listener
        {
        public:
            LevelSliderComponent(PlayerTable* parentTable, int row)
                : parent(parentTable), rowIndex(row)
            {
                levelSlider.setRange(0.0, 2.0, 0.1);
                levelSlider.setValue(1.0);
                levelSlider.setSliderStyle(Slider::LinearHorizontal);
                levelSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
                levelSlider.addListener(this);
                addAndMakeVisible(levelSlider);
                
                // Load current level from ensemble
                updateSliderFromEnsemble();
            }
            
            void resized() override
            {
                levelSlider.setBounds(getLocalBounds().reduced(2));
            }
            
            void sliderValueChanged(Slider* slider) override
            {
                if (slider == &levelSlider && parent && parent->parent)
                {
                    // Update the ensemble role level
                    auto ensemble = parent->parent->getPerformanceEnsemble();
                    if (rowIndex >= 0 && rowIndex < ensemble.size())
                    {
                        // Note: We need to update the cached ensemble in PerformanceEditorComponent
                        // This is a bit tricky since the ensemble is cached. We'll need to
                        // add a method to update individual role levels.
                        parent->parent->updateRoleLevel(rowIndex, static_cast<float>(levelSlider.getValue()));
                    }
                }
            }
            
            void updateSliderFromEnsemble()
            {
                if (parent && parent->parent)
                {
                    auto ensemble = parent->parent->getPerformanceEnsemble();
                    if (rowIndex >= 0 && rowIndex < ensemble.size())
                    {
                        const auto& role = ensemble[rowIndex];
                        levelSlider.setValue(role.level, dontSendNotification);
                    }
                }
            }
            
        private:
            PlayerTable* parent;
            int rowIndex;
            Slider levelSlider;
        };
        
        auto* levelComponent = dynamic_cast<LevelSliderComponent*>(existingComponentToUpdate);
        if (levelComponent == nullptr)
        {
            levelComponent = new LevelSliderComponent(this, rowNumber);
        }
        else
        {
            levelComponent->updateSliderFromEnsemble();
        }
        return levelComponent;
    }
    else if (columnId == 5) // Actions column
    {
        // Create a custom component that layouts buttons properly
        class ActionButtonsComponent : public Component
        {
        public:
            ActionButtonsComponent(PlayerTable* parentTable, int row)
                : parent(parentTable)
            {
                generateBtn.setButtonText("Gen");
                generateBtn.setComponentID("generate_" + String(row));
                generateBtn.addListener(parent);
                addAndMakeVisible(generateBtn);
                
                prepBtn.setButtonText("Prep");
                prepBtn.setComponentID("prep_" + String(row));
                prepBtn.addListener(parent);
                addAndMakeVisible(prepBtn);
                
                editBtn.setButtonText("Edit");
                editBtn.setComponentID("edit_" + String(row));
                editBtn.addListener(parent);
                addAndMakeVisible(editBtn);
                
                loadBtn.setButtonText("Load");
                loadBtn.setComponentID("load_" + String(row));
                loadBtn.addListener(parent);
                addAndMakeVisible(loadBtn);
                
                playBtn.setButtonText("Play");
                playBtn.setComponentID("play_" + String(row));
                playBtn.addListener(parent);
                addAndMakeVisible(playBtn);
            }
            
            void resized() override
            {
                auto bounds = getLocalBounds();
                int buttonWidth = bounds.getWidth() / 5;
                generateBtn.setBounds(bounds.removeFromLeft(buttonWidth).reduced(1));
                prepBtn.setBounds(bounds.removeFromLeft(buttonWidth).reduced(1));
                editBtn.setBounds(bounds.removeFromLeft(buttonWidth).reduced(1));
                loadBtn.setBounds(bounds.removeFromLeft(buttonWidth).reduced(1));
                playBtn.setBounds(bounds.reduced(1));
            }
            
        private:
            PlayerTable* parent;
            TextButton generateBtn;
            TextButton prepBtn;
            TextButton editBtn;
            TextButton loadBtn;
            TextButton playBtn;
        };
        
        if (existingComponentToUpdate == nullptr)
        {
            return new ActionButtonsComponent(this, rowNumber);
        }
        return existingComponentToUpdate;
    }
    
    return nullptr;
}

void PerformanceEditorComponent::PlayerTable::buttonClicked(Button* button)
{
    String buttonId = button->getComponentID();
    
    if (buttonId.startsWith("generate_"))
    {
        int playerIndex = buttonId.substring(9).getIntValue();
        if (parent)
            parent->generateChartForPlayer(playerIndex);
    }
    else if (buttonId.startsWith("prep_"))
    {
        int playerIndex = buttonId.substring(5).getIntValue();
        if (parent)
            parent->prepChartForPlayer(playerIndex);
    }
    else if (buttonId.startsWith("edit_"))
    {
        int playerIndex = buttonId.substring(5).getIntValue();
        if (parent)
            parent->editPatternForPlayer(playerIndex);
    }
    else if (buttonId.startsWith("load_"))
    {
        int playerIndex = buttonId.substring(5).getIntValue();
        if (parent)
            parent->loadChartForPlayer(playerIndex);
    }
    else if (buttonId.startsWith("play_"))
    {
        int playerIndex = buttonId.substring(5).getIntValue();
        if (parent)
            parent->playPlayer(playerIndex);
    }
}

void PerformanceEditorComponent::PlayerTable::updateContent()
{
    table.updateContent();
}

void PerformanceEditorComponent::PlayerTable::clearButtons()
{
    generateButtons.clear();
    loadButtons.clear();
    playButtons.clear();
}

Array<MusicianRole> PerformanceEditorComponent::PlayerTable::getEnsembleRolesFromName(const String& ensembleName)
{
    Array<MusicianRole> roles;
    
    if (ensembleName.isEmpty())
        return roles;
    
    // Load the ensemble from disk (similar to Performance::refreshEnsembleCache)
    Array<File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    for (const auto& file : ensembleFiles)
    {
        auto ensemble = EnsembleManager::loadFromFile(file, nullptr);
        if (ensemble && ensemble->getName() == ensembleName)
        {
            auto ensembleRoles = ensemble->getMusicianRoles();
            for (auto role : ensembleRoles)
            {
                role.player = nullptr; // Clear player pointer
                roles.add(role);
            }
            break;
        }
    }
    
    return roles;
}

void PerformanceEditorComponent::refreshPerformanceEnsembleCache()
{
    // Cache ensemble roles with proper player assignments using PlayerManager
    cachedEnsembleRoles.clear();
    
    if (!currentPerformance || !playerManager)
        return;
    
    String ensembleName = currentPerformance->getEnsembleName();
    if (ensembleName.isEmpty())
        return;
    
    // Load the ensemble with PlayerManager to get proper player assignments
    Array<File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    for (const auto& file : ensembleFiles)
    {
        auto tempEnsemble = EnsembleManager::loadFromFile(file, playerManager); // Pass PlayerManager!
        if (tempEnsemble && tempEnsemble->getName() == ensembleName)
        {
            // Found the right ensemble - store it as our main ensemble manager
            ensembleManager = std::move(tempEnsemble);
            cachedEnsembleRoles = ensembleManager->getMusicianRoles();
            DBG("PerformanceEditorComponent: Loaded ensemble '" << ensembleName << "' with " << cachedEnsembleRoles.size() << " roles");
            
            // Debug player assignments
            for (const auto& role : cachedEnsembleRoles)
            {
                DBG("Role: " << role.getName() << " (" << role.getInstrumentName() << ") - Player: " << (role.player ? role.player->getMetadata().name : "null"));
            }
            break;
        }
        // tempEnsemble will be destroyed here if it's not the right one
    }
}

juce::Array<MusicianRole> PerformanceEditorComponent::getPerformanceEnsemble() const
{
    return cachedEnsembleRoles;
}

void PerformanceEditorComponent::enterPatternEditMode(const String& roleName, Player* player, const String& llmPattern)
{
    if (!patternEditor || !player)
        return;
    
    // Store edit state
    isInPatternEditMode = true;
    editingRoleName = roleName;
    editingPlayer = player;
    
    // Set up pattern editor with the LLM pattern and player
    patternEditor->setPattern(llmPattern);
    patternEditor->setSelectedPlayer(player);
    
    // Set performance context (song name and role)
    String songName = "";
    if (currentPerformance && currentPerformance->getSong())
    {
        songName = currentPerformance->getSong()->getTitle();
    }
    patternEditor->setPerformanceContext(songName, roleName);
    
    patternEditor->setPerformanceEditMode(true);
    
    // Hide all performance editor UI components
    titleLabel.setVisible(false);
    nameEditor.setVisible(false);
    descriptionEditor.setVisible(false);
    songLabel.setVisible(false);
    songSelector.setVisible(false);
    ensembleLabel.setVisible(false);
    ensembleSelector.setVisible(false);
    styleLabel.setVisible(false);
    styleSelector.setVisible(false);
    characterLabel.setVisible(false);
    characterEditor.setVisible(false);
    playerTable->setVisible(false);
    coordinationLabel.setVisible(false);
    generateAllButton.setVisible(false);
    prepAllButton.setVisible(false);
    loadAllButton.setVisible(false);
    playAllButton.setVisible(false);
    stopAllButton.setVisible(false);
    statusDisplay.setVisible(false);
    saveButton.setVisible(false);
    backButton.setVisible(false);
    
    // Show pattern editor
    patternEditor->setVisible(true);
    
    // Update layout
    resized();
    
    // Update status
    if (currentPerformance)
    {
        currentPerformance->setStatusMessage(roleName, "Editing pattern...");
    }
}

void PerformanceEditorComponent::exitPatternEditMode(bool saveChanges)
{
    if (!isInPatternEditMode || !patternEditor)
        return;
    
    if (saveChanges && currentPerformance && !editingRoleName.isEmpty())
    {
        // Get the edited pattern from the pattern editor
        String editedPattern = patternEditor->getPatternNotation();
        
        // Validate that we have some content
        if (editedPattern.trim().isEmpty())
        {
            // Don't exit edit mode if pattern is empty - just show error
            // Note: we can't show statusDisplay since it's hidden in edit mode
            return;
        }
        
        // Save it back to the performance as an LLM pattern
        currentPerformance->setLLMPattern(editingRoleName, editedPattern);
        currentPerformance->setStatusMessage(editingRoleName, "Pattern saved (manual)");
        
        // Clear any existing MIDI chart since the pattern was modified
        currentPerformance->setMIDIChart(editingRoleName, var());
        
        DBG("DEBUG: Saved manually edited pattern for " << editingRoleName << " (" << editedPattern.length() << " chars)");
    }
    else if (currentPerformance && !editingRoleName.isEmpty())
    {
        currentPerformance->setStatusMessage(editingRoleName, "Edit cancelled");
    }
    
    // Reset edit state
    isInPatternEditMode = false;
    editingRoleName = "";
    editingPlayer = nullptr;
    
    // Hide pattern editor
    patternEditor->setPerformanceEditMode(false);
    patternEditor->setVisible(false);
    
    // Show all performance editor UI components
    titleLabel.setVisible(true);
    nameEditor.setVisible(true);
    descriptionEditor.setVisible(true);
    songLabel.setVisible(true);
    songSelector.setVisible(true);
    ensembleLabel.setVisible(true);
    ensembleSelector.setVisible(true);
    styleLabel.setVisible(true);
    styleSelector.setVisible(true);
    characterLabel.setVisible(true);
    characterEditor.setVisible(true);
    playerTable->setVisible(true);
    coordinationLabel.setVisible(true);
    generateAllButton.setVisible(true);
    prepAllButton.setVisible(true);
    loadAllButton.setVisible(true);
    playAllButton.setVisible(true);
    stopAllButton.setVisible(true);
    statusDisplay.setVisible(true);
    saveButton.setVisible(true);
    backButton.setVisible(true);
    
    // Update layout
    resized();
    
    // Update status and player table
    if (saveChanges && currentPerformance && !editingRoleName.isEmpty())
    {
        statusDisplay.setText("Pattern saved for " + editingRoleName + " - use Prep to generate MIDI chart");
    }
    else if (currentPerformance && !editingRoleName.isEmpty())
    {
        statusDisplay.setText("Pattern editing cancelled for " + editingRoleName);
    }
    
    // Update the player table
    if (playerTable)
        playerTable->updateContent();
}

// PatternEditorComponent::Listener implementations
void PerformanceEditorComponent::patternPlayRequested(PatternEditorComponent* /*editor*/)
{
    // Not used in performance mode - play is handled by the main interface
}

void PerformanceEditorComponent::patternParseRequested(PatternEditorComponent* /*editor*/)
{
    // Not used in performance mode - parsing is handled by prep step
}

void PerformanceEditorComponent::patternLoadRequested(PatternEditorComponent* /*editor*/)
{
    // Not used in performance mode - loading is handled by main interface
}

void PerformanceEditorComponent::performancePatternSaveRequested(PatternEditorComponent* /*editor*/)
{
    // Save the edited pattern and exit edit mode
    exitPatternEditMode(true);
}

void PerformanceEditorComponent::performancePatternCancelRequested(PatternEditorComponent* /*editor*/)
{
    // Cancel editing and exit edit mode
    exitPatternEditMode(false);
}

void PerformanceEditorComponent::updateRoleLevel(int roleIndex, float level)
{
    // Update the cached ensemble roles
    if (roleIndex >= 0 && roleIndex < cachedEnsembleRoles.size())
    {
        cachedEnsembleRoles.getReference(roleIndex).level = level;
        
        // Also update the actual performance ensemble if it exists
        if (currentPerformance)
        {
            // Get the current ensemble from the performance
            auto ensemble = currentPerformance->getEnsemble();
            if (roleIndex < ensemble.size())
            {
                String roleName = ensemble[roleIndex].getName();
                
                // We need to update the performance's ensemble
                // This requires updating the saved ensemble file
                String ensembleName = currentPerformance->getEnsembleName();
                if (ensembleName.isNotEmpty())
                {
                    // Find and update the ensemble file
                    Array<File> ensembleFiles = EnsembleManager::getEnsembleFiles();
                    for (const auto& file : ensembleFiles)
                    {
                        // Use existing ensemble manager if it matches, otherwise load temporarily
                        bool shouldUseExisting = (ensembleManager && ensembleManager->getName() == ensembleName);
                        auto* workingEnsemble = shouldUseExisting ? ensembleManager.get() : nullptr;
                        std::unique_ptr<EnsembleManager> tempEnsemble;
                        
                        if (!shouldUseExisting)
                        {
                            tempEnsemble = EnsembleManager::loadFromFile(file, playerManager);
                            workingEnsemble = tempEnsemble.get();
                        }
                        
                        if (workingEnsemble && workingEnsemble->getName() == ensembleName)
                        {
                            // Update the role level in the ensemble
                            auto& roles = workingEnsemble->getMusicianRoles();
                            for (auto& role : roles)
                            {
                                if (role.getName() == roleName)
                                {
                                    // Use a non-const reference to modify the role
                                    auto& mutableRole = const_cast<MusicianRole&>(role);
                                    mutableRole.level = level;
                                    break;
                                }
                            }
                            
                            // Save the updated ensemble
                            workingEnsemble->saveToFile(file);
                            
                            // Refresh our cache
                            refreshPerformanceEnsembleCache();
                            
                            statusDisplay.setText("Updated level for " + roleName + " to " + String(level, 1) + "x");
                            break;
                        }
                    }
                }
            }
        }
    }
}

void PerformanceEditorComponent::requestJsonContinuation(const String& roleName, const String& incompleteJson)
{
    if (!llmClient)
    {
        statusDisplay.setText("No LLM client available for continuation");
        return;
    }
    
    // Create a continuation prompt
    String continuationPrompt = "Continue this JSON exactly where it left off. Do not repeat any part, just continue from where it stopped. Complete the JSON to make it valid:\n\n";
    continuationPrompt += incompleteJson;
    continuationPrompt += "\n\nContinue from exactly where this JSON stopped. Make sure the final result is valid JSON that ends with }";
    
    // Update status
    currentPerformance->setStatusMessage(roleName, "Requesting continuation...");
    if (playerTable)
        playerTable->updateContent();
    
    llmClient->sendMessage(continuationPrompt, [this, roleName, incompleteJson](bool success, const String& continuationResponse)
    {
        MessageManager::callAsync([this, roleName, incompleteJson, success, continuationResponse]()
        {
            if (success)
            {
                // Combine the incomplete JSON with the continuation
                String continuationPart = continuationResponse.trim();
                
                // Remove any leading { or opening that might be repeated
                if (continuationPart.startsWith("{"))
                {
                    continuationPart = continuationPart.substring(1);
                }
                
                String completeJson = incompleteJson + continuationPart;
                
                // Validate the completed JSON
                if (completeJson.startsWith("{") && completeJson.endsWith("}"))
                {
                    currentPerformance->setLLMPattern(roleName, completeJson);
                    currentPerformance->setStatusMessage(roleName, "LLM pattern ready (continued)");
                    statusDisplay.setText("Completed JSON for " + roleName + " with continuation");
                    DBG("DEBUG: Successfully completed JSON for " << roleName << " with continuation");
                }
                else
                {
                    statusDisplay.setText("Continuation failed for " + roleName + " - JSON still invalid");
                    currentPerformance->setStatusMessage(roleName, "Continuation failed");
                    DBG("WARNING: Continuation still resulted in invalid JSON for " << roleName);
                }
            }
            else
            {
                statusDisplay.setText("Failed to get continuation for " + roleName);
                currentPerformance->setStatusMessage(roleName, "Continuation failed");
            }
            
            // Update the player table
            if (playerTable)
                playerTable->updateContent();
        });
    });
}

// =========================================================================
// Player Creation (NEW: For Synchronized Ensemble Playback)
// =========================================================================

std::unique_ptr<Player> PerformanceEditorComponent::createPlayerFromPlayer(Player* player)
{
    if (!player)
        return nullptr;
        
    if (!playerManager)
        return nullptr;
    
    try 
    {
        // Create Player with same ID and metadata as original Player
        auto newPlayer = std::make_unique<Player>(
            player->getId(),
            player->getMetadata().name,
            playerManager->getFormatManager(),
            playerManager->getKnownPluginList()
        );
        
        // Initialize the Player for audio processing
        if (!newPlayer->initialize())
        {
            std::cerr << "[PerformanceEditor] Failed to initialize Player for: " << player->getMetadata().name << std::endl;
            return nullptr;
        }
        
        // Load player state if available (plugins, configurations, etc.)
        if (player->hasPlayerState())
        {
            auto savedState = player->getPlayerState();
            if (!savedState.isVoid())
            {
                if (newPlayer->loadCompleteState(savedState))
                {
                    std::cout << "[PerformanceEditor] ✅ Loaded saved state for Player: " << player->getMetadata().name << std::endl;
                }
                else
                {
                    std::cerr << "[PerformanceEditor] ⚠️ Failed to load saved state for: " << player->getMetadata().name << std::endl;
                }
            }
        }
        else
        {
            std::cout << "[PerformanceEditor] Creating new Player without saved state: " << player->getMetadata().name << std::endl;
            
            // Add default instrument based on player capabilities
            const auto& capabilities = player->getInstrumentCapabilities();
            if (capabilities.instrumentType == "drums")
            {
                // Add default synth for drums
                newPlayer->getInstrument()->addDefaultSynth();
            }
            else if (capabilities.instrumentType == "piano" || capabilities.instrumentType == "bass" || capabilities.instrumentType == "guitar")
            {
                // Add default synth for melodic instruments
                newPlayer->getInstrument()->addDefaultSynth();
            }
        }
        
        std::cout << "[PerformanceEditor] ✅ Created Player for synchronized playback: " << player->getMetadata().name << std::endl;
        return newPlayer;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[PerformanceEditor] Exception creating Player for " << player->getMetadata().name << ": " << e.what() << std::endl;
        return nullptr;
    }
}

} // namespace covertracks
