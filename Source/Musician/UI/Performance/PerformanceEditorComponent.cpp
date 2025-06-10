#include "PerformanceEditorComponent.h"

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
      coordinationLabel("coordinationLabel", "Coordination"),
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
    
    // Set up player table
    playerTable = std::make_unique<PlayerTable>(this);
    addAndMakeVisible(playerTable.get());
    
    // Set up coordination section
    coordinationLabel.setFont(Font(FontOptions().withHeight(16.0f).withStyle("bold")));
    addAndMakeVisible(coordinationLabel);
    
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
    nameEditor.removeListener(this);
    descriptionEditor.removeListener(this);
    
    songSelector.removeListener(this);
    ensembleSelector.removeListener(this);
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
    
    // Player table takes most of the space
    auto tableHeight = static_cast<int>(bounds.getHeight() * 0.4f);
    playerTable->setBounds(bounds.removeFromTop(tableHeight));
    bounds.removeFromTop(10);
    
    // Coordination section
    coordinationLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    
    auto coordButtons = bounds.removeFromTop(30);
    int buttonWidth = coordButtons.getWidth() / 3;
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

void PerformanceEditorComponent::buttonClicked(Button* button)
{
    if (button == &loadAllButton)
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
}

void PerformanceEditorComponent::loadPerformance(Performance* performance)
{
    currentPerformance = performance;
    isNewPerformance = (performance == nullptr);
    
    if (currentPerformance)
    {
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
    
    if (!playerManager)
    {
        statusDisplay.setText("No player manager available");
        return;
    }
    
    auto ensemble = currentPerformance->getEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    statusDisplay.setText("Generating chart for " + role.getName() + "...");
    
    // Find the player for this role
    Player* player = role.player;
    if (!player)
    {
        // Try to find a player by name in the PlayerManager
        auto players = playerManager->getPlayers();
        for (auto* p : players)
        {
            if (p && p->getMetadata().name == role.getName())
            {
                player = p;
                break;
            }
        }
        
        if (!player)
        {
            statusDisplay.setText("No player assigned to role " + role.getName());
            return;
        }
    }
    
    // Create parameters for chart generation
    var parameters = var(new DynamicObject());
    
    // Set instruments as array
    var instrumentsArray = var(Array<var>());
    instrumentsArray.append(var(role.getInstrumentName()));
    parameters.getDynamicObject()->setProperty("instruments", instrumentsArray);
    
    // Set style/genre
    String style = currentPerformance->getGenre();
    if (style.isEmpty()) style = "default";
    parameters.getDynamicObject()->setProperty("style", style);
    
    // Set tempo
    int tempo = 120; // Default
    if (auto song = currentPerformance->getSong())
    {
        tempo = static_cast<int>(song->getTempo());
    }
    parameters.getDynamicObject()->setProperty("tempo", tempo);
    
    // Create comprehensive description for the LLM
    String description = "Generate a " + role.getInstrumentName() + " chart for the role '" + role.getName() + "'";
    
    if (auto song = currentPerformance->getSong())
    {
        description += " for the song '" + song->getTitle() + "'";
        
        String songStructure = song->getSourceStructureJson();
        if (songStructure.isNotEmpty())
        {
            description += ". Song structure: " + songStructure;
        }
    }
    
    String instructions = currentPerformance->getInstructions();
    if (instructions.isNotEmpty())
    {
        description += ". Performance instructions: " + instructions;
    }
    
    String key = currentPerformance->getKey();
    if (key.isNotEmpty())
    {
        description += ". Key: " + key;
    }
    
    String mood = currentPerformance->getMood();
    if (mood.isNotEmpty())
    {
        description += ". Mood: " + mood;
    }
    
    parameters.getDynamicObject()->setProperty("description", description);
    
    // Generate chart using Player interface (delegates to internal Musician)
    String roleName = role.getName();
    player->generateChart(parameters, [this, roleName](bool success, var chart)
    {
        if (success)
        {
            // Store the generated chart in the performance
            if (currentPerformance)
            {
                currentPerformance->setGeneratedChart(roleName, chart);
                currentPerformance->setStatusMessage(roleName, "Chart Generated");
            }
            statusDisplay.setText("Chart generated for " + roleName + " - ready to load");
        }
        else
        {
            statusDisplay.setText("Failed to generate chart for " + roleName);
            if (currentPerformance)
            {
                currentPerformance->setStatusMessage(roleName, "Generation Failed");
            }
        }
        
        // Update the player table to show new status
        if (playerTable)
            playerTable->updateContent();
    });
}

void PerformanceEditorComponent::loadChartForPlayer(int playerIndex)
{
    if (!currentPerformance || !playerManager)
    {
        statusDisplay.setText("No performance or player manager available");
        return;
    }
    
    auto ensemble = currentPerformance->getEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    statusDisplay.setText("Loading chart for " + role.getName() + "...");
    
    // Get the generated chart for this role
    var chart = currentPerformance->getGeneratedChart(role.getName());
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
    
    // Find an existing player for this role or use the one assigned to the role
    Player* player = role.player;
    if (!player)
    {
        // Try to find a player by name in the PlayerManager
        auto players = playerManager->getPlayers();
        for (auto* p : players)
        {
            if (p && p->getMetadata().name == role.getName())
            {
                player = p;
                break;
            }
        }
        
        if (!player)
        {
            statusDisplay.setText("No player assigned to role " + role.getName());
            return;
        }
    }
    
    // The chart is already in the correct format from Musician
    // Load the chart into the player
    if (player->loadChart(chart))
    {
        statusDisplay.setText("Chart loaded for " + role.getName());
        currentPerformance->setStatusMessage(role.getName(), "Chart Loaded");
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
    
    auto ensemble = currentPerformance->getEnsemble();
    if (playerIndex < 0 || playerIndex >= ensemble.size())
    {
        statusDisplay.setText("Invalid player index: " + String(playerIndex));
        return;
    }
    
    const auto role = ensemble[playerIndex];
    
    // Get the player for this role
    Player* player = role.player;
    if (!player)
    {
        // Try to find a player by name in the PlayerManager
        auto players = playerManager->getPlayers();
        for (auto* p : players)
        {
            if (p && p->getMetadata().name == role.getName())
            {
                player = p;
                break;
            }
        }
        
        if (!player)
        {
            statusDisplay.setText("No player found for " + role.getName() + " - load chart first");
            return;
        }
    }
    
    // Start playing - Player::play() returns bool
    if (player->play())
    {
        statusDisplay.setText("Playing " + role.getName());
        currentPerformance->setStatusMessage(role.getName(), "Playing");
    }
    else
    {
        statusDisplay.setText("Failed to start playing " + role.getName());
        currentPerformance->setStatusMessage(role.getName(), "Play Failed");
    }
}

void PerformanceEditorComponent::loadAllCharts()
{
    if (!currentPerformance)
    {
        statusDisplay.setText("No performance available");
        return;
    }
    
    auto ensemble = currentPerformance->getEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Loading all charts...");
    
    int loaded = 0;
    int total = ensemble.size();
    
    for (int i = 0; i < ensemble.size(); ++i)
    {
        const auto role = ensemble[i];
        
        // Check if chart is generated
        auto chart = currentPerformance->getGeneratedChart(role.getName());
        if (!chart.isVoid())
        {
            // Load the chart for this player
            loadChartForPlayer(i);
            
            // Check if it loaded successfully
            String status = currentPerformance->getStatusMessage(role.getName());
            if (status == "Chart Loaded")
            {
                loaded++;
            }
        }
    }
    
    statusDisplay.setText("Loaded " + String(loaded) + " of " + String(total) + " charts");
    
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
    
    auto ensemble = currentPerformance->getEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Starting all players...");
    
    int playing = 0;
    int total = ensemble.size();
    
    // Start synchronized playback for all players
    for (int i = 0; i < ensemble.size(); ++i)
    {
        const auto role = ensemble[i];
        
        Player* player = role.player;
        if (!player)
        {
            // Try to find a player by name in the PlayerManager
            auto players = playerManager->getPlayers();
            for (auto* p : players)
            {
                if (p && p->getMetadata().name == role.getName())
                {
                    player = p;
                    break;
                }
            }
        }
        
        if (player)
        {
            if (player->play())
            {
                playing++;
                currentPerformance->setStatusMessage(role.getName(), "Playing");
            }
            else
            {
                currentPerformance->setStatusMessage(role.getName(), "Play Failed");
            }
        }
        else
        {
            currentPerformance->setStatusMessage(role.getName(), "Not Ready");
        }
    }
    
    statusDisplay.setText("Playing " + String(playing) + " of " + String(total) + " players");
    
    // Update the player table to reflect new status
    if (playerTable)
        playerTable->updateContent();
}

void PerformanceEditorComponent::stopAllPlayers()
{
    if (!currentPerformance || !playerManager)
    {
        statusDisplay.setText("No performance or player manager available");
        return;
    }
    
    auto ensemble = currentPerformance->getEnsemble();
    if (ensemble.isEmpty())
    {
        statusDisplay.setText("No ensemble selected");
        return;
    }
    
    statusDisplay.setText("Stopping all players...");
    
    int stopped = 0;
    
    // Stop all players
    for (int i = 0; i < ensemble.size(); ++i)
    {
        const auto role = ensemble[i];
        
        Player* player = role.player;
        if (!player)
        {
            // Try to find a player by name in the PlayerManager
            auto players = playerManager->getPlayers();
            for (auto* p : players)
            {
                if (p && p->getMetadata().name == role.getName())
                {
                    player = p;
                    break;
                }
            }
        }
        
        if (player)
        {
            player->stop(); // stop() returns void
            stopped++;
            currentPerformance->setStatusMessage(role.getName(), "Stopped");
        }
    }
    
    statusDisplay.setText("Stopped " + String(stopped) + " players");
    
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
            currentPerformance->setEnsembleName(selectedEnsemble);
            // Refresh the ensemble cache since we changed ensembles
            currentPerformance->refreshEnsembleCache();
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

// PlayerTable implementation
PerformanceEditorComponent::PlayerTable::PlayerTable(PerformanceEditorComponent* parentComponent)
    : parent(parentComponent)
{
    table.setModel(this);
    table.getHeader().addColumn("Player", 1, 100);
    table.getHeader().addColumn("Instrument", 2, 100);
    table.getHeader().addColumn("Chart Status", 3, 100);
    table.getHeader().addColumn("Actions", 4, 200);
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
    
    // Get the number of players from the selected ensemble
    if (parent->currentPerformance)
    {
        auto ensemble = parent->currentPerformance->getEnsemble();
        return ensemble.size();
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
        auto ensemble = parent->currentPerformance->getEnsemble();
        if (rowNumber < ensemble.size())
        {
            const auto& role = ensemble[rowNumber];
            if (columnId == 1) text = role.getName();
            if (columnId == 2) text = role.getInstrumentName();
            if (columnId == 3) 
            {
                // Show the actual status from the performance
                String status = parent->currentPerformance->getStatusMessage(role.getName());
                if (status.isEmpty())
                {
                    // Check if chart exists
                    auto chart = parent->currentPerformance->getGeneratedChart(role.getName());
                    text = !chart.isVoid() ? "Chart Generated" : "No Chart";
                }
                else
                {
                    text = status;
                }
            }
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
        }
    }
    else
    {
        // Fallback for when no performance is loaded
        if (columnId == 2) text = "Piano"; // Default instrument
        if (columnId == 3) text = "No Chart";
    }
    
    g.drawText(text, Rectangle<int>(2, 0, width - 4, height), Justification::centredLeft, true);
}

Component* PerformanceEditorComponent::PlayerTable::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate)
{
    if (columnId == 4) // Actions column
    {
        // Create a custom component that layouts buttons properly
        class ActionButtonsComponent : public Component
        {
        public:
            ActionButtonsComponent(PlayerTable* parentTable, int row)
                : parent(parentTable), rowIndex(row)
            {
                generateBtn.setButtonText("Gen");
                generateBtn.setComponentID("generate_" + String(row));
                generateBtn.addListener(parent);
                addAndMakeVisible(generateBtn);
                
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
                int buttonWidth = bounds.getWidth() / 3;
                generateBtn.setBounds(bounds.removeFromLeft(buttonWidth).reduced(1));
                loadBtn.setBounds(bounds.removeFromLeft(buttonWidth).reduced(1));
                playBtn.setBounds(bounds.reduced(1));
            }
            
        private:
            PlayerTable* parent;
            int rowIndex;
            TextButton generateBtn;
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

} // namespace covertracks