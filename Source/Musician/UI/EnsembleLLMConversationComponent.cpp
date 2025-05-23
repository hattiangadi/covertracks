#include "EnsembleLLMConversationComponent.h"
#include "../../UI/MainHostWindow.h"

// Import the CommandIDs namespace and getCommandManager function
using namespace covertracks;

namespace covertracks {

//==============================================================================
// MusicianStatusTable Implementation
//==============================================================================

EnsembleLLMConversationComponent::MusicianStatusTable::MusicianStatusTable()
{
    // Set up the table
    table.setModel(this);
    table.setMultipleSelectionEnabled(false);
    table.setHeaderHeight(24);
    table.getHeader().addColumn("Musician", 1, 150, 100, -1, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Instrument", 2, 150, 100, -1, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Status", 3, 150, 100, -1, juce::TableHeaderComponent::defaultFlags);
    addAndMakeVisible(table);
}

EnsembleLLMConversationComponent::MusicianStatusTable::~MusicianStatusTable()
{
    table.setModel(nullptr);
}

void EnsembleLLMConversationComponent::MusicianStatusTable::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void EnsembleLLMConversationComponent::MusicianStatusTable::resized()
{
    table.setBounds(getLocalBounds());
}

int EnsembleLLMConversationComponent::MusicianStatusTable::getNumRows()
{
    return musicianRoles.size();
}

void EnsembleLLMConversationComponent::MusicianStatusTable::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::lightblue);
    }
    else if (rowNumber % 2)
    {
        g.fillAll(juce::Colours::lightgrey.withAlpha(0.2f));
    }
}

void EnsembleLLMConversationComponent::MusicianStatusTable::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(rowIsSelected ? juce::Colours::darkblue : juce::Colours::black);
    g.setFont(14.0f);
    
    if (rowNumber < musicianRoles.size())
    {
        const auto& role = musicianRoles[rowNumber];
        juce::String text;
        
        switch (columnId)
        {
            case 1: // Musician
                text = role.name;
                break;
                
            case 2: // Instrument
                text = role.instrument;
                break;
                
            case 3: // Status
                if (rowNumber < statuses.size())
                {
                    text = statuses[rowNumber];
                }
                else
                {
                    text = "Not started";
                }
                break;
        }
        
        g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

juce::Component* EnsembleLLMConversationComponent::MusicianStatusTable::refreshComponentForCell(int /*rowNumber*/, int /*columnId*/, bool /*isRowSelected*/, juce::Component* /*existingComponentToUpdate*/)
{
    // We're not using custom components for cells in this simple implementation
    return nullptr;
}

void EnsembleLLMConversationComponent::MusicianStatusTable::updateMusicianRoles(const juce::Array<MusicianRole>& roles)
{
    musicianRoles = roles;
    statuses.clear();
    
    // Initialize statuses for all musicians
    for (int i = 0; i < musicianRoles.size(); ++i)
    {
        statuses.add("Not started");
    }
    
    table.updateContent();
    repaint();
}

void EnsembleLLMConversationComponent::MusicianStatusTable::updateMusicianStatus(int index, const juce::String& status)
{
    if (index >= 0 && index < statuses.size())
    {
        statuses.set(index, status);
        table.repaintRow(index);
    }
}

void EnsembleLLMConversationComponent::MusicianStatusTable::clearStatuses()
{
    for (int i = 0; i < statuses.size(); ++i)
    {
        statuses.set(i, "Not started");
    }
    
    table.repaint();
}

//==============================================================================
// EnsembleLLMConversationComponent Implementation
//==============================================================================

EnsembleLLMConversationComponent::EnsembleLLMConversationComponent(EnsembleManager* manager, std::shared_ptr<LLMClient> client)
    : ensembleManager(manager), llmClient(client)
{
    // Set up ensemble selector
    ensembleLabel.setText("Ensemble:", juce::dontSendNotification);
    juce::FontOptions fontOptions;
    ensembleLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(ensembleLabel);
    
    ensembleSelector.addListener(this);
    addAndMakeVisible(ensembleSelector);
    
    // Set up song selection
    songLabel.setText("Song:", juce::dontSendNotification);
    songLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(songLabel);
    
    songSelector.addListener(this);
    addAndMakeVisible(songSelector);
    
    // Set up instructions editor
    instructionsLabel.setText("Performance Instructions:", juce::dontSendNotification);
    instructionsLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(instructionsLabel);
    
    instructionsEditor.setMultiLine(true);
    instructionsEditor.setReturnKeyStartsNewLine(true);
    instructionsEditor.setReadOnly(false);
    instructionsEditor.setScrollbarsShown(true);
    instructionsEditor.setCaretVisible(true);
    instructionsEditor.setPopupMenuEnabled(true);
    instructionsEditor.setText("Create a jazz-influenced performance with a relaxed feel. The bass should provide a steady groove, the drums should use brushes for a softer sound, and the piano should add tasteful embellishments.");
    instructionsEditor.addListener(this);
    addAndMakeVisible(instructionsEditor);
    
    // Set up buttons
    generateButton.setButtonText("Generate Patterns");
    generateButton.addListener(this);
    addAndMakeVisible(generateButton);
    
    playButton.setButtonText("Play");
    playButton.addListener(this);
    playButton.setEnabled(false);
    addAndMakeVisible(playButton);
    
    stopButton.setButtonText("Stop");
    stopButton.addListener(this);
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);
    
    // Set up musician status table
    addAndMakeVisible(musicianStatusTable);
    
    // Update UI
    updateEnsembleSelector();
    updateSongSelector();
    
    // Start timer for UI updates
    startTimer(500);
}

EnsembleLLMConversationComponent::~EnsembleLLMConversationComponent()
{
    stopTimer();
    ensembleSelector.removeListener(this);
    songSelector.removeListener(this);
    instructionsEditor.removeListener(this);
}

void EnsembleLLMConversationComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &ensembleSelector)
    {
        // Update the musician status table with the selected ensemble's musicians
        updateMusicianStatusTable();
    }
    else if (comboBoxThatHasChanged == &songSelector)
    {
        int selectedId = songSelector.getSelectedId();
        if (selectedId > 0)
        {
            // Get the default songs directory
            juce::File songsDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("Covertracks")
                .getChildFile("Songs");
            
            // Find all .json files in the directory
            juce::Array<juce::File> songFiles;
            songsDir.findChildFiles(songFiles, juce::File::findFiles, false, "*.json");
            
            if (selectedId <= songFiles.size())
            {
                // Load the selected song
                try
                {
                    juce::String jsonContent = songFiles[selectedId - 1].loadFileAsString();
                    juce::var jsonVar = juce::JSON::parse(jsonContent);
                    auto song = Song::fromVar(jsonVar);
                    if (song)
                    {
                        setCurrentSong(song);
                    }
                }
                catch (const std::exception& e)
                {
                    DBG("Error loading song: " + juce::String(e.what()));
                }
            }
        }
    }
}

void EnsembleLLMConversationComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("Ensemble LLM Conversation", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
}

void EnsembleLLMConversationComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(40);
    
    // Ensemble selector
    auto selectorArea = bounds.removeFromTop(30);
    ensembleLabel.setBounds(selectorArea.removeFromLeft(100));
    ensembleSelector.setBounds(selectorArea.reduced(5, 0));
    
    auto rightPanel = bounds;
    
    // Right panel: Song selection, instructions, and musician status
    auto songArea = rightPanel.removeFromTop(30);
    songLabel.setBounds(songArea.removeFromLeft(100));
    songSelector.setBounds(songArea.reduced(5, 0));
    
    rightPanel.removeFromTop(10); // Spacing
    
    // Instructions area
    auto instructionsLabelArea = rightPanel.removeFromTop(30);
    instructionsLabel.setBounds(instructionsLabelArea);
    
    auto instructionsArea = rightPanel.removeFromTop(100);
    instructionsEditor.setBounds(instructionsArea.reduced(5));
    
    // Buttons
    auto buttonArea = rightPanel.removeFromTop(40);
    auto buttonWidth = buttonArea.getWidth() / 3;
    
    generateButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    stopButton.setBounds(buttonArea.reduced(5));
    
    // Musician status table
    musicianStatusTable.setBounds(rightPanel.reduced(5));
}

void EnsembleLLMConversationComponent::buttonClicked(juce::Button* button)
{
    if (button == &generateButton)
    {
        startGeneration();
    }
    else if (button == &playButton)
    {
        playGeneratedPatterns();
    }
    else if (button == &stopButton)
    {
        stopPlayback();
    }
}

void EnsembleLLMConversationComponent::textEditorReturnKeyPressed(juce::TextEditor& /*editor*/)
{
    // Nothing to do for now
}

void EnsembleLLMConversationComponent::updateSongSelector()
{
    songSelector.clear();
    
    // Get the default songs directory
    juce::File songsDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Covertracks")
        .getChildFile("Songs");
    
    // Find all .json files in the directory
    juce::Array<juce::File> songFiles;
    songsDir.findChildFiles(songFiles, juce::File::findFiles, false, "*.json");
    
    // Add each song to the selector
    int id = 1;
    for (const auto& file : songFiles)
    {
        try
        {
            // Read the file content
            juce::String jsonContent = file.loadFileAsString();
            
            // Parse the JSON
            juce::var jsonVar = juce::JSON::parse(jsonContent);
            
            // Get the song title
            if (jsonVar.hasProperty("name"))
            {
                juce::String title = jsonVar["name"].toString();
                songSelector.addItem(title, id++);
            }
        }
        catch (const std::exception& e)
        {
            DBG("Error loading song from " + file.getFileName() + ": " + e.what());
        }
    }
    
    if (songSelector.getNumItems() > 0)
    {
        songSelector.setSelectedId(1);
    }
    else
    {
        // If no songs are found, add a placeholder message
        songSelector.addItem("No songs found", -1);
        songSelector.setSelectedId(-1);
    }
}

void EnsembleLLMConversationComponent::setCurrentSong(std::shared_ptr<Song> song)
{
    currentSong = song;
    
    // Update the UI
    if (currentSong)
    {
        // Try to select the song in the dropdown if it exists
        for (int i = 0; i < songSelector.getNumItems(); ++i)
        {
            if (songSelector.getItemText(i) == currentSong->getTitle())
            {
                songSelector.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }
}

std::shared_ptr<Song> EnsembleLLMConversationComponent::getCurrentSong() const
{
    return currentSong;
}

void EnsembleLLMConversationComponent::timerCallback()
{
    // Update UI based on current state
    if (ensembleManager)
    {
        // Update play/stop buttons based on playback state
        playButton.setEnabled(!isGenerating && currentSong != nullptr && !ensembleManager->isPlaying());
        stopButton.setEnabled(ensembleManager->isPlaying());
        
        // Periodically refresh the song selector (every 2 seconds)
        static int refreshCounter = 0;
        if (++refreshCounter >= 4) // 500ms * 4 = 2 seconds
        {
            refreshCounter = 0;
            updateSongSelector();
        }
    }
}

void EnsembleLLMConversationComponent::updateEnsembleSelector()
{
    ensembleSelector.clear();
    
    if (ensembleManager)
    {
        // Get all ensemble files from the EnsembleManager
        juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
        
        // Add each ensemble to the selector
        int id = 1;
        for (const auto& file : ensembleFiles)
        {
            // Load the ensemble to get its name
            auto ensemble = EnsembleManager::loadFromFile(file, ensembleManager->getPlayerManager());
            if (ensemble)
            {
                ensembleSelector.addItem(ensemble->getName(), id++);
            }
        }
        
        if (ensembleSelector.getNumItems() > 0)
        {
            ensembleSelector.setSelectedId(1);
        }
        else
        {
            // If no ensembles are found, add a placeholder message
            ensembleSelector.addItem("No ensembles found - create one in the Ensembles tab", -1);
            ensembleSelector.setSelectedId(-1);
        }
        
        // Update the musician status table with the selected ensemble's musicians
        updateMusicianStatusTable();
    }
}

void EnsembleLLMConversationComponent::updateMusicianStatusTable()
{
    if (ensembleManager)
    {
        juce::Array<MusicianRole> roles;
        int selectedId = ensembleSelector.getSelectedId();
        
        if (selectedId > 0)
        {
            // Get the selected ensemble file
            juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
            if (selectedId <= ensembleFiles.size())
            {
                // Load the ensemble to get its musician roles
                auto ensemble = EnsembleManager::loadFromFile(ensembleFiles[selectedId - 1], 
                                                           ensembleManager->getPlayerManager());
                if (ensemble)
                {
                    // Get the musician roles from the ensemble
                    roles = ensemble->getMusicianRoles();
                }
            }
        }
        
        // Update the musician status table with the roles
        musicianStatusTable.updateMusicianRoles(roles);
    }
}

void EnsembleLLMConversationComponent::startGeneration()
{
    if (!ensembleManager || !currentSong || isGenerating)
    {
        return;
    }
    
    isGenerating = true;
    generateButton.setEnabled(false);
    playButton.setEnabled(false);
    
    currentInstructions = instructionsEditor.getText();
    
    // Clear previous statuses
    musicianStatusTable.clearStatuses();
    
    // Get the musician roles for the selected ensemble
    juce::Array<MusicianRole> roles;
    
    int selectedId = ensembleSelector.getSelectedId();
    
    if (selectedId > 0)
    {
        // Get the selected ensemble file
        juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
        if (selectedId <= ensembleFiles.size())
        {
            // Load the ensemble to get its musician roles
            auto ensemble = EnsembleManager::loadFromFile(ensembleFiles[selectedId - 1], 
                                                       ensembleManager->getPlayerManager());
            if (ensemble)
            {
                // Get the musician roles from the ensemble
                roles = ensemble->getMusicianRoles();
            }
        }
    }
    
    // In a real implementation, we would send LLM requests for each musician
    // For now, we'll just simulate the process with a timer
    
    // Update statuses to "In progress"
    for (int i = 0; i < roles.size(); ++i)
    {
        musicianStatusTable.updateMusicianStatus(i, "In progress");
    }
    
    // Simulate completion after a delay
    juce::Timer::callAfterDelay(2000, [this, roles]()
    {
        for (int i = 0; i < roles.size(); ++i)
        {
            musicianStatusTable.updateMusicianStatus(i, "Completed");
        }
        
        isGenerating = false;
        generateButton.setEnabled(true);
        playButton.setEnabled(true);
    });
}

void EnsembleLLMConversationComponent::cancelGeneration()
{
    if (!isGenerating)
    {
        return;
    }
    
    isGenerating = false;
    generateButton.setEnabled(true);
    
    // Update statuses to "Cancelled"
    int numRows = musicianStatusTable.getNumRows();
    for (int i = 0; i < numRows; ++i)
    {
        musicianStatusTable.updateMusicianStatus(i, "Cancelled");
    }
}

void EnsembleLLMConversationComponent::playGeneratedPatterns()
{
    if (!ensembleManager)
    {
        return;
    }
    
    ensembleManager->startPlaying();
}

void EnsembleLLMConversationComponent::stopPlayback()
{
    if (!ensembleManager)
    {
        return;
    }
    
    ensembleManager->stopPlaying();
}

juce::String EnsembleLLMConversationComponent::generateMusicianPrompt(const MusicianRole& role, 
                                                                   std::shared_ptr<Song> song,
                                                                   const juce::String& performanceInstructions)
{
    // Create a prompt for the musician to generate a pattern
    juce::String prompt;
    
    prompt += "You are " + role.name + " playing " + role.instrument + " in an ensemble.\n\n";
    
    prompt += "Song Title: " + song->getTitle() + "\n";
    prompt += "Tempo: " + juce::String(song->getTempo()) + " BPM\n\n";
    
    prompt += "Performance Instructions: " + performanceInstructions + "\n\n";
    
    prompt += "Please create a musical pattern for your instrument that fits this song and follows the performance instructions.\n";
    prompt += "Use the following notation format to describe your pattern:\n";
    prompt += "[SONG:\n  <SECTION_NAME: {NEW: [TIME_SIGNATURE: NOTE1 NOTE2 NOTE3 ...]}>\n  ...\n]\n\n";
    
    return prompt;
}

} // namespace covertracks
