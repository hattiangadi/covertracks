#include "PerformanceEditorComponent.h"

namespace covertracks {

PerformanceEditorComponent::PerformanceEditorComponent(PerformanceManager* pm,
                                                     std::shared_ptr<LLMClient> client)
    : performanceManager(pm),
      llmClient(client),
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
    nameEditor.setText("New Performance");
    descriptionEditor.setText("");
    isNewPerformance = true;
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
    statusDisplay.setText("Save functionality not yet implemented");
}

void PerformanceEditorComponent::generateChartForPlayer(int playerIndex)
{
    statusDisplay.setText("Generate chart for player " + String(playerIndex) + " - not yet implemented");
}

void PerformanceEditorComponent::loadChartForPlayer(int playerIndex)
{
    statusDisplay.setText("Load chart for player " + String(playerIndex) + " - not yet implemented");
}

void PerformanceEditorComponent::playPlayer(int playerIndex)
{
    statusDisplay.setText("Play player " + String(playerIndex) + " - not yet implemented");
}

void PerformanceEditorComponent::loadAllCharts()
{
    statusDisplay.setText("Load all charts - not yet implemented");
}

void PerformanceEditorComponent::playAllPlayers()
{
    statusDisplay.setText("Play all players - not yet implemented");
}

void PerformanceEditorComponent::stopAllPlayers()
{
    statusDisplay.setText("Stop all players - not yet implemented");
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
        
        // Load the selected song and set it on the performance
        if (currentPerformance && performanceManager)
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
                            currentPerformance->setSong(song);
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
            currentPerformance->setEnsembleName("");
    }
    else
    {
        statusDisplay.setText("Selected ensemble: " + selectedEnsemble);
        if (currentPerformance)
            currentPerformance->setEnsembleName(selectedEnsemble);
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
    // Get the number of players from the selected ensemble
    if (parent && parent->currentPerformance)
    {
        auto ensemble = parent->currentPerformance->getEnsemble();
        return ensemble.size();
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
            if (columnId == 3) text = "No Chart";
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

Component* PerformanceEditorComponent::PlayerTable::refreshComponentForCell(int /*rowNumber*/, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate)
{
    if (columnId == 4) // Actions column
    {
        // Return button components for actions
        if (existingComponentToUpdate == nullptr)
        {
            auto* button = new TextButton("Generate");
            button->addListener(this);
            return button;
        }
        return existingComponentToUpdate;
    }
    
    return nullptr;
}

void PerformanceEditorComponent::PlayerTable::buttonClicked(Button* /*button*/)
{
    // Handle button clicks in table
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

} // namespace covertracks