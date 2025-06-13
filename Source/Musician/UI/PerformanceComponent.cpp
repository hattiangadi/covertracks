#include "PerformanceComponent.h"
#include "../Core/Song.h"
#include "../Core/MusicPatternComponents.h"
#include "../Core/PatternParser.h"
#include "../Core/MusicPatternSystem.h"
#include "../../Player/Core/PlayerManager.h"

namespace covertracks {

//==============================================================================
// MusicianStatusTable implementation
//==============================================================================

PerformanceComponent::MusicianStatusTable::MusicianStatusTable()
{
    // Set up the table
    table.setModel(this);
    table.setMultipleSelectionEnabled(false);
    table.setHeaderHeight(24);
    table.getHeader().addColumn("Role", 1, 150, 100, -1, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Instrument", 2, 150, 100, -1, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Status", 3, 200, 100, -1, juce::TableHeaderComponent::defaultFlags);
    table.getHeader().addColumn("Generate", 4, 100, 80, -1, juce::TableHeaderComponent::defaultFlags);
    addAndMakeVisible(table);
}

PerformanceComponent::MusicianStatusTable::~MusicianStatusTable()
{
    table.setModel(nullptr);
}

void PerformanceComponent::MusicianStatusTable::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PerformanceComponent::MusicianStatusTable::resized()
{
    table.setBounds(getLocalBounds());
}

int PerformanceComponent::MusicianStatusTable::getNumRows()
{
    return musicianRoles.size();
}

void PerformanceComponent::MusicianStatusTable::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::lightblue);
    }
    else if (rowNumber % 2)
    {
        g.fillAll(juce::Colours::white.withAlpha(0.2f));
    }
}

void PerformanceComponent::MusicianStatusTable::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(rowIsSelected ? juce::Colours::darkblue : juce::Colours::white);
    g.setFont(14.0f);
    
    if (rowNumber < musicianRoles.size())
    {
        juce::String text;
        
        switch (columnId)
        {
            case 1: // Role
                text = musicianRoles[rowNumber].name;
                break;
                
            case 2: // Instrument
                text = musicianRoles[rowNumber].instrument;
                break;
                
            case 3: // Status
                if (rowNumber < statuses.size())
                {
                    text = statuses[rowNumber];
                }
                break;
                
            default:
                break;
        }
        
        g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

juce::Component* PerformanceComponent::MusicianStatusTable::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, juce::Component* existingComponentToUpdate)
{
    // Only create components for the "Generate" column
    if (columnId == 4)
    {
        // Create a generate button for this row
        auto* genButton = static_cast<juce::TextButton*>(existingComponentToUpdate);
        
        if (genButton == nullptr)
        {
            genButton = new juce::TextButton("Generate");
            genButton->setComponentID("generate_" + juce::String(rowNumber));
        }
        
        return genButton;
    }
    
    return nullptr;
}

void PerformanceComponent::MusicianStatusTable::updateMusicianRoles(const juce::Array<MusicianRole>& roles)
{
    musicianRoles = roles;
    statuses.clear();
    statuses.clear();
    for (int i = 0; i < roles.size(); ++i)
    {
        statuses.add("");
    }
    
    table.updateContent();
    table.repaint();
}

void PerformanceComponent::MusicianStatusTable::updateMusicianStatus(int index, const juce::String& status)
{
    if (index >= 0 && index < statuses.size())
    {
        statuses.set(index, status);
        table.repaintRow(index);
    }
}

void PerformanceComponent::MusicianStatusTable::clearStatuses()
{
    for (int i = 0; i < statuses.size(); ++i)
    {
        statuses.set(i, "");
    }
    
    table.repaint();
}

juce::String PerformanceComponent::MusicianStatusTable::getStatusMessage(int index) const
{
    if (index >= 0 && index < statuses.size())
    {
        return statuses[index];
    }
    return "";
}

void PerformanceComponent::MusicianStatusTable::addGenerateButton(int rowNumber, juce::Button::Listener* listener)
{
    if (auto* button = dynamic_cast<juce::TextButton*>(table.getCellComponent(4, rowNumber)))
    {
        button->addListener(listener);
    }
}

//==============================================================================
// PerformanceComponent implementation
//==============================================================================

PerformanceComponent::PerformanceComponent(EnsembleManager* manager, PlayerManager* playerMgr)
    : ensembleManager(manager), playerManager(playerMgr)
{
    // Set up performance selector
    performanceLabel.setText("Performance:", juce::dontSendNotification);
    juce::FontOptions fontOptions;
    performanceLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(performanceLabel);
    
    performanceSelector.addListener(this);
    addAndMakeVisible(performanceSelector);
    
    newPerformanceButton.setButtonText("New");
    newPerformanceButton.addListener(this);
    addAndMakeVisible(newPerformanceButton);
    
    savePerformanceButton.setButtonText("Save");
    savePerformanceButton.addListener(this);
    addAndMakeVisible(savePerformanceButton);
    
    // Set up ensemble selector
    ensembleLabel.setText("Ensemble:", juce::dontSendNotification);
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
    
    // Set up key selector
    keyLabel.setText("Key:", juce::dontSendNotification);
    keyLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(keyLabel);
    
    keySelector.addItem("C Major", 1);
    keySelector.addItem("G Major", 2);
    keySelector.addItem("D Major", 3);
    keySelector.addItem("A Major", 4);
    keySelector.addItem("E Major", 5);
    keySelector.addItem("B Major", 6);
    keySelector.addItem("F# Major", 7);
    keySelector.addItem("C# Major", 8);
    keySelector.addItem("F Major", 9);
    keySelector.addItem("Bb Major", 10);
    keySelector.addItem("Eb Major", 11);
    keySelector.addItem("Ab Major", 12);
    keySelector.addItem("Db Major", 13);
    keySelector.addItem("Gb Major", 14);
    keySelector.addItem("Cb Major", 15);
    keySelector.addItem("A Minor", 16);
    keySelector.addItem("E Minor", 17);
    keySelector.addItem("B Minor", 18);
    keySelector.addItem("F# Minor", 19);
    keySelector.addItem("C# Minor", 20);
    keySelector.addItem("G# Minor", 21);
    keySelector.addItem("D# Minor", 22);
    keySelector.addItem("A# Minor", 23);
    keySelector.addItem("D Minor", 24);
    keySelector.addItem("G Minor", 25);
    keySelector.addItem("C Minor", 26);
    keySelector.addItem("F Minor", 27);
    keySelector.addItem("Bb Minor", 28);
    keySelector.addItem("Eb Minor", 29);
    keySelector.addItem("Ab Minor", 30);
    keySelector.setSelectedId(1); // Default to C Major
    keySelector.addListener(this);
    addAndMakeVisible(keySelector);
    
    // Set up genre selector
    genreLabel.setText("Genre:", juce::dontSendNotification);
    genreLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(genreLabel);
    
    genreSelector.addItem("Pop", 1);
    genreSelector.addItem("Rock", 2);
    genreSelector.addItem("Jazz", 3);
    genreSelector.addItem("Classical", 4);
    genreSelector.addItem("Electronic", 5);
    genreSelector.addItem("Hip Hop", 6);
    genreSelector.addItem("R&B", 7);
    genreSelector.addItem("Country", 8);
    genreSelector.addItem("Folk", 9);
    genreSelector.addItem("Blues", 10);
    genreSelector.addItem("Ambient", 11);
    genreSelector.addItem("Funk", 12);
    genreSelector.setSelectedId(1); // Default to Pop
    genreSelector.addListener(this);
    addAndMakeVisible(genreSelector);
    
    // Set up mood selector
    moodLabel.setText("Mood:", juce::dontSendNotification);
    moodLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(moodLabel);
    
    moodSelector.addItem("Happy", 1);
    moodSelector.addItem("Sad", 2);
    moodSelector.addItem("Energetic", 3);
    moodSelector.addItem("Calm", 4);
    moodSelector.addItem("Aggressive", 5);
    moodSelector.addItem("Mysterious", 6);
    moodSelector.addItem("Romantic", 7);
    moodSelector.addItem("Nostalgic", 8);
    moodSelector.addItem("Dreamy", 9);
    moodSelector.addItem("Tense", 10);
    moodSelector.setSelectedId(1); // Default to Happy
    moodSelector.addListener(this);
    addAndMakeVisible(moodSelector);
    
    // Set up description editor
    descriptionLabel.setText("Description:", juce::dontSendNotification);
    descriptionLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(descriptionLabel);
    
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.setReadOnly(false);
    descriptionEditor.setScrollbarsShown(true);
    descriptionEditor.setCaretVisible(true);
    descriptionEditor.setPopupMenuEnabled(true);
    descriptionEditor.setText("Enter performance description here...");
    descriptionEditor.addListener(this);
    addAndMakeVisible(descriptionEditor);
    
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
    instructionsEditor.setText("Enter performance instructions here...");
    
    instructionsEditor.addListener(this);
    addAndMakeVisible(instructionsEditor);
    
    // Set up buttons
    generateButton.setButtonText("Generate All");
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
    
    // Set up status label
    statusLabel.setText("Ready", juce::dontSendNotification);
    juce::FontOptions fontOpts;
    statusLabel.setFont(fontOpts.withHeight(14.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(statusLabel);
    
    // Load performances
    loadedPerformances = performanceManager->loadPerformancesFromDefaultLocation();
    
    // Update UI
    updatePerformanceSelector();
    updateEnsembleSelector();
    updateSongSelector();
    
    // Start timer for UI updates
    startTimer(500);
}

PerformanceComponent::~PerformanceComponent()
{
    stopTimer();
    ensembleSelector.removeListener(this);
    songSelector.removeListener(this);
    performanceSelector.removeListener(this);
    instructionsEditor.removeListener(this);
}

void PerformanceComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("Performance", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
}

void PerformanceComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(40);
    
    // Performance selector area
    auto performanceArea = bounds.removeFromTop(30);
    performanceLabel.setBounds(performanceArea.removeFromLeft(100));
    auto performanceButtonsArea = performanceArea.removeFromRight(150);
    savePerformanceButton.setBounds(performanceButtonsArea.removeFromRight(70).reduced(5, 0));
    newPerformanceButton.setBounds(performanceButtonsArea.reduced(5, 0));
    performanceSelector.setBounds(performanceArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Ensemble selector
    auto selectorArea = bounds.removeFromTop(30);
    ensembleLabel.setBounds(selectorArea.removeFromLeft(100));
    ensembleSelector.setBounds(selectorArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Song selector
    auto songArea = bounds.removeFromTop(30);
    songLabel.setBounds(songArea.removeFromLeft(100));
    songSelector.setBounds(songArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Performance attributes (key, genre, mood)
    // Key selector
    auto keyArea = bounds.removeFromTop(30);
    keyLabel.setBounds(keyArea.removeFromLeft(100));
    keySelector.setBounds(keyArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Genre selector
    auto genreArea = bounds.removeFromTop(30);
    genreLabel.setBounds(genreArea.removeFromLeft(100));
    genreSelector.setBounds(genreArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Mood selector
    auto moodArea = bounds.removeFromTop(30);
    moodLabel.setBounds(moodArea.removeFromLeft(100));
    moodSelector.setBounds(moodArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Description area
    auto descriptionLabelArea = bounds.removeFromTop(30);
    descriptionLabel.setBounds(descriptionLabelArea);
    
    auto descriptionArea = bounds.removeFromTop(80);
    descriptionEditor.setBounds(descriptionArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Instructions area
    auto instructionsLabelArea = bounds.removeFromTop(30);
    instructionsLabel.setBounds(instructionsLabelArea);
    
    auto instructionsArea = bounds.removeFromTop(100);
    instructionsEditor.setBounds(instructionsArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Buttons area
    auto buttonsArea = bounds.removeFromTop(30);
    auto buttonsWidth = buttonsArea.getWidth() / 3;
    
    generateButton.setBounds(buttonsArea.removeFromLeft(buttonsWidth).reduced(5, 0));
    playButton.setBounds(buttonsArea.removeFromLeft(buttonsWidth).reduced(5, 0));
    stopButton.setBounds(buttonsArea.reduced(5, 0));
    
    // Status label area
    auto statusArea = bounds.removeFromTop(25);
    statusLabel.setBounds(statusArea.reduced(5, 0));
    
    bounds.removeFromTop(10); // Spacing
    
    // Musician status table
    musicianStatusTable.setBounds(bounds);
}

void PerformanceComponent::buttonClicked(juce::Button* button)
{
    DBG("DEBUG: buttonClicked() called - MEMORY CHECK POINT BUTTON");
    
    if (button == &generateButton)
    {
        DBG("DEBUG: Generate button clicked - MEMORY CHECK POINT GENERATE_BUTTON");
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
    else if (button == &newPerformanceButton)
    {
        createNewPerformance();
    }
    else if (button == &savePerformanceButton)
    {
        saveCurrentPerformance();
    }
    else
    {
        // Check if this is a generate button for a specific musician
        juce::String buttonId = button->getComponentID();
        if (buttonId.startsWith("generate_"))
        {
            int rowNumber = buttonId.substring(9).getIntValue();
            if (rowNumber >= 0 && rowNumber < musicianStatusTable.getMusicianRoles().size())
            {
                // Generate pattern for this specific musician
                startGenerationForRole(musicianStatusTable.getMusicianRoles()[rowNumber]);
            }
        }
    }
}

void PerformanceComponent::textEditorReturnKeyPressed(juce::TextEditor& /*editor*/)
{
    // Nothing to do for now
}

void PerformanceComponent::textEditorFocusLost(juce::TextEditor& editor)
{
    // Save performance when description or instructions are changed
    if (&editor == &descriptionEditor || &editor == &instructionsEditor)
    {
        if (currentPerformance)
        {
            saveCurrentPerformance();
        }
    }
}

void PerformanceComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
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
                        
                        // The setCurrentSong method will update the performance
                    }
                }
                catch (const std::exception& e)
                {
                    DBG("Error loading song: " + juce::String(e.what()));
                }
            }
        }
    }
    else if (comboBoxThatHasChanged == &performanceSelector)
    {
        int selectedId = performanceSelector.getSelectedId();
        if (selectedId > 0 && selectedId <= loadedPerformances.size())
        {
            // Load the selected performance
            loadPerformance(loadedPerformances[selectedId - 1]);
        }
    }
    else if (comboBoxThatHasChanged == &keySelector)
    {
        if (currentPerformance)
        {
            // Update the key in the performance
            juce::String key = keySelector.getText();
            currentPerformance->setKey(key);
            saveCurrentPerformance();
        }
    }
    else if (comboBoxThatHasChanged == &genreSelector)
    {
        if (currentPerformance)
        {
            // Update the genre in the performance
            juce::String genre = genreSelector.getText();
            currentPerformance->setGenre(genre);
            saveCurrentPerformance();
        }
    }
    else if (comboBoxThatHasChanged == &moodSelector)
    {
        if (currentPerformance)
        {
            // Update the mood in the performance
            juce::String mood = moodSelector.getText();
            currentPerformance->setMood(mood);
            saveCurrentPerformance();
        }
    }
}

void covertracks::PerformanceComponent::timerCallback()
{
    // Update UI based on current state
    if (ensembleManager)
    {
        // Update play/stop buttons based on playback state
        playButton.setEnabled(!isGenerating && currentSong != nullptr && !ensembleManager->isPlaying());
        stopButton.setEnabled(ensembleManager->isPlaying());
    }
}

void covertracks::PerformanceComponent::songSelectionChanged(SongListComponent* /*source*/, std::shared_ptr<Song> song)
{
    // If a song is selected in the SongListComponent, update our current song
    if (song)
    {
        setCurrentSong(song);
        
        // The setCurrentSong method will update the performance
    }
}

void covertracks::PerformanceComponent::songListChanged(SongListComponent* /*source*/)
{
    // When the song list changes, update our song selector
    updateSongSelector();
}

void covertracks::PerformanceComponent::setCurrentSong(std::shared_ptr<Song> song)
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
    
    // If we have a current performance, update it with the new song
    if (currentPerformance && currentSong)
    {
        currentPerformance->setSong(currentSong);
    }
}

std::shared_ptr<Song> covertracks::PerformanceComponent::getCurrentSong() const
{
    return currentSong;
}

std::shared_ptr<Chart> covertracks::PerformanceComponent::getCurrentChart() const
{
    if (currentPerformance)
    {
        return currentPerformance->getChart();
    }
    return nullptr;
}

void covertracks::PerformanceComponent::registerWithSongList(SongListComponent* songList)
{
    if (songList)
    {
        // Store the reference to the song list
        registeredSongList = songList;
        
        // Register as a listener
        songList->addListener(this);
        
        // Update our song selector immediately
        updateSongSelector();
    }
}

void covertracks::PerformanceComponent::updatePerformanceSelector()
{
    performanceSelector.clear();
    
    // Add each performance to the selector
    int id = 1;
    for (const auto& performance : loadedPerformances)
    {
        if (performance)
        {
            performanceSelector.addItem(performance->getName(), id++);
        }
    }
    
    if (performanceSelector.getNumItems() > 0)
    {
        performanceSelector.setSelectedId(1);
    }
    else
    {
        // If no performances are found, add a placeholder message
        performanceSelector.addItem("No performances found", -1);
        performanceSelector.setSelectedId(-1);
    }
}

void covertracks::PerformanceComponent::updateEnsembleSelector()
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

void covertracks::PerformanceComponent::updateSongSelector()
{
    songSelector.clear();
    
    // Check if we have a registered SongListComponent
    if (registeredSongList != nullptr)
    {
        // Get songs from the SongListComponent
        const juce::Array<std::shared_ptr<Song>>& songs = registeredSongList->getSongs();
        
        // Add each song to the selector
        int id = 1;
        for (const auto& song : songs)
        {
            if (song != nullptr)
            {
                // Add to the selector
                songSelector.addItem(song->getTitle(), id++);
            }
        }
        
        if (songSelector.getNumItems() > 0)
        {
            songSelector.setSelectedId(1);
            
            // Set the current song
            if (songs.size() > 0)
            {
                setCurrentSong(songs[0]);
            }
        }
        else
        {
            // If no songs are found, add a placeholder message
            songSelector.addItem("No songs found", -1);
            songSelector.setSelectedId(-1);
        }
        
        return;
    }
    
    // Fallback to scanning the filesystem if no SongListComponent is registered
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
            if (jsonVar.hasProperty("title"))
            {
                juce::String title = jsonVar["title"].toString();
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

void PerformanceComponent::updateMusicianStatusTable()
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
                    
                    // Update the current performance with the ensemble name
                    if (currentPerformance)
                    {
                        currentPerformance->setEnsembleName(ensemble->getName());
                    }
                }
            }
        }
        
        // Update the musician status table with the roles
        musicianStatusTable.updateMusicianRoles(roles);
        
        // Add listeners to the generate buttons
        for (int i = 0; i < roles.size(); ++i)
        {
            musicianStatusTable.addGenerateButton(i, this);
        }
    }
}

void PerformanceComponent::startGeneration()
{
    DBG("DEBUG: startGeneration() called - MEMORY CHECK POINT 1");
    
    if (!ensembleManager || !currentSong || isGenerating)
    {
        return;
    }
    
    DBG("DEBUG: startGeneration() - before setting isGenerating - MEMORY CHECK POINT 2");
    isGenerating = true;
    generateButton.setEnabled(false);
    playButton.setEnabled(false);
    
    currentInstructions = instructionsEditor.getText();
    
    // Update the current performance with the instructions
    if (currentPerformance)
    {
        currentPerformance->setInstructions(currentInstructions);
        currentPerformance->clearGeneratedCharts();
    }
    
    // Clear previous statuses
    musicianStatusTable.clearStatuses();
    
    // Use the existing roles from musicianStatusTable to avoid loading ensemble again
    const juce::Array<MusicianRole>& roles = musicianStatusTable.getMusicianRoles();
    
    // Generate patterns for each musician
    DBG("DEBUG: About to start musician loop - MEMORY CHECK POINT 3");
    for (int i = 0; i < roles.size(); ++i)
    {
        const auto& role = roles[i];
        DBG("DEBUG: Processing musician " << i << " (" << role.getName() << ") - MEMORY CHECK POINT 4");
        
        // Update status
        musicianStatusTable.updateMusicianStatus(i, "Generating...");
        
        // Get the player for this role
        Player* player = role.player;
        if (!player)
        {
            DBG("DEBUG: No player found for role " << role.getName());
            musicianStatusTable.updateMusicianStatus(i, "No Player");
            continue;
        }
        
        // Create generation parameters for the Player - let Player/Musician/PatternGenerator handle prompts
        DynamicObject::Ptr params = new DynamicObject();
        params->setProperty("style", genreSelector.getText());
        params->setProperty("tempo", currentSong ? static_cast<int>(currentSong->getTempo()) : 120);
        
        // Let Player/Musician/PatternGenerator handle prompt creation - just pass raw info
        String description = descriptionEditor.getText();
        if (description.isEmpty()) {
            description = currentInstructions;
        } else {
            description += "\n" + currentInstructions;
        }
        if (currentSong) {
            description = "Song: " + currentSong->getTitle() + "\n" + description;
        }
        
        // Add key and mood information
        String key = (currentPerformance && currentPerformance->getKey().isNotEmpty()) 
                    ? currentPerformance->getKey() : keySelector.getText();
        if (key.isNotEmpty()) {
            description += "\nKey: " + key;
        }
        
        String mood = (currentPerformance && currentPerformance->getMood().isNotEmpty()) 
                     ? currentPerformance->getMood() : moodSelector.getText();
        if (mood.isNotEmpty()) {
            description += "\nMood: " + mood;
        }
        
        // Add role-specific information
        description += "\nMusician Role: " + role.name + " (" + role.instrument + ")";
        
        params->setProperty("description", description);
        
        // Set up instruments array for this role
        Array<var> instruments;
        instruments.add(role.getInstrumentName());
        params->setProperty("instruments", instruments);
        
        var parameters(params.get());
        
        // Use Player->generateChart instead of direct LLM call
        juce::String roleName = role.getName();
        DBG("DEBUG: About to call player->generateChart for " << roleName << " via proper architecture");
        player->generateChart(parameters, [this, i, roleName](bool success, const var& midiEventChart)
        {
            DBG("DEBUG: Player callback received for " << roleName << " - success: " << (success ? "true" : "false"));
            
            // Process the response on the message thread
            juce::MessageManager::callAsync([this, i, roleName, success, midiEventChart]()
            {
                std::shared_ptr<juce::MidiMessageSequence> pattern = nullptr;
                
                if (success && midiEventChart.isObject())
                {
                    // Extract MIDI events from the Player's response
                    if (midiEventChart.hasProperty("events"))
                    {
                        pattern = std::make_shared<juce::MidiMessageSequence>();
                        var eventsVar = midiEventChart["events"];
                        
                        if (eventsVar.isArray())
                        {
                            for (const auto& eventVar : *eventsVar.getArray())
                            {
                                if (eventVar.isObject())
                                {
                                    String type = eventVar.getProperty("type", "").toString();
                                    int channel = eventVar.getProperty("channel", 1);
                                    int note = eventVar.getProperty("note", 60);
                                    double timestamp = eventVar.getProperty("timestamp", 0.0);
                                    
                                    if (type == "note_on")
                                    {
                                        int velocity = eventVar.getProperty("velocity", 100);
                                        juce::MidiMessage msg = juce::MidiMessage::noteOn(channel, note, (juce::uint8)velocity);
                                        msg.setTimeStamp(timestamp);
                                        pattern->addEvent(msg);
                                    }
                                    else if (type == "note_off")
                                    {
                                        juce::MidiMessage msg = juce::MidiMessage::noteOff(channel, note);
                                        msg.setTimeStamp(timestamp);
                                        pattern->addEvent(msg);
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (pattern && pattern->getNumEvents() > 0)
                {
                    // Update status
                    musicianStatusTable.updateMusicianStatus(i, "Generated");
                    
                    // Store the pattern in the ensemble manager
                    ensembleManager->setMIDIPatternForRole(roleName, pattern);
                    
                    // Store the pattern in the current performance
                    if (currentPerformance)
                    {
                        // Convert MidiMessageSequence to MIDI_EVENT_CHART format for storage
                        DynamicObject* chartObj = new DynamicObject();
                        Array<var> events;
                        
                        for (int eventIndex = 0; eventIndex < pattern->getNumEvents(); ++eventIndex)
                        {
                            auto event = pattern->getEventPointer(eventIndex);
                            if (event->message.isNoteOn())
                            {
                                DynamicObject* noteOn = new DynamicObject();
                                noteOn->setProperty("type", "note_on");
                                noteOn->setProperty("channel", event->message.getChannel());
                                noteOn->setProperty("note", event->message.getNoteNumber());
                                noteOn->setProperty("velocity", (int)(event->message.getVelocity() * 127));
                                noteOn->setProperty("timestamp", event->message.getTimeStamp());
                                events.add(var(noteOn));
                            }
                            else if (event->message.isNoteOff())
                            {
                                DynamicObject* noteOff = new DynamicObject();
                                noteOff->setProperty("type", "note_off");
                                noteOff->setProperty("channel", event->message.getChannel());
                                noteOff->setProperty("note", event->message.getNoteNumber());
                                noteOff->setProperty("timestamp", event->message.getTimeStamp());
                                events.add(var(noteOff));
                            }
                            else if (event->message.isController())
                            {
                                DynamicObject* controller = new DynamicObject();
                                controller->setProperty("type", "controller");
                                controller->setProperty("channel", event->message.getChannel());
                                controller->setProperty("controller", event->message.getControllerNumber());
                                controller->setProperty("value", event->message.getControllerValue());
                                controller->setProperty("timestamp", event->message.getTimeStamp());
                                events.add(var(controller));
                            }
                        }
                        
                        chartObj->setProperty("events", events);
                        chartObj->setProperty("tempo", currentSong ? currentSong->getTempo() : 120.0);
                        chartObj->setProperty("success", true);
                        
                        // Store the MIDI_EVENT_CHART in the performance
                        currentPerformance->setGeneratedChart(roleName, var(chartObj));
                        currentPerformance->setStatusMessage(roleName, "Generated");
                        
                        // Save the performance
                        saveCurrentPerformance();
                    }
                }
                else
                {
                    // Update status with error
                    musicianStatusTable.updateMusicianStatus(i, "Error: Could not parse response");
                    
                    if (currentPerformance)
                    {
                        currentPerformance->setStatusMessage(roleName, "Error: Could not parse response");
                    }
                }
                
                // Check if all musicians are done
                bool allDone = true;
                for (int j = 0; j < musicianStatusTable.getMusicianRoles().size(); ++j)
                {
                    juce::String status = musicianStatusTable.getStatusMessage(j);
                    if (status == "Generating..." || status.isEmpty())
                    {
                        allDone = false;
                        break;
                    }
                }
                
                if (allDone)
                {
                    // All musicians are done, enable the play button
                    isGenerating = false;
                    generateButton.setEnabled(true);
                    playButton.setEnabled(true);
                }
            });
        });
    }
}

void PerformanceComponent::startGenerationForRole(const MusicianRole& role)
{
    if (!ensembleManager || !currentSong || isGenerating)
    {
        return;
    }
    
    currentInstructions = instructionsEditor.getText();
    
    // Update the current performance with the instructions
    if (currentPerformance)
    {
        currentPerformance->setInstructions(currentInstructions);
    }
    
    // Find the index of the role in the table
    int roleIndex = -1;
    for (int i = 0; i < musicianStatusTable.getMusicianRoles().size(); ++i)
    {
        if (musicianStatusTable.getMusicianRoles()[i].name == role.name)
        {
            roleIndex = i;
            break;
        }
    }
    
    if (roleIndex >= 0)
    {
        // Update status
        musicianStatusTable.updateMusicianStatus(roleIndex, "Generating...");
        
        // Get the player for this role
        Player* player = role.player;
        if (!player)
        {
            DBG("DEBUG: No player found for role " << role.getName());
            musicianStatusTable.updateMusicianStatus(roleIndex, "No Player");
            return;
        }
        
        // Create generation parameters for the Player - let Player/Musician/PatternGenerator handle prompts
        DynamicObject::Ptr params = new DynamicObject();
        params->setProperty("style", genreSelector.getText());
        params->setProperty("tempo", currentSong ? static_cast<int>(currentSong->getTempo()) : 120);
        
        // Let Player/Musician/PatternGenerator handle prompt creation - just pass raw info
        String description = descriptionEditor.getText();
        if (description.isEmpty()) {
            description = currentInstructions;
        } else {
            description += "\n" + currentInstructions;
        }
        if (currentSong) {
            description = "Song: " + currentSong->getTitle() + "\n" + description;
        }
        
        // Add key and mood information
        String key = (currentPerformance && currentPerformance->getKey().isNotEmpty()) 
                    ? currentPerformance->getKey() : keySelector.getText();
        if (key.isNotEmpty()) {
            description += "\nKey: " + key;
        }
        
        String mood = (currentPerformance && currentPerformance->getMood().isNotEmpty()) 
                     ? currentPerformance->getMood() : moodSelector.getText();
        if (mood.isNotEmpty()) {
            description += "\nMood: " + mood;
        }
        
        // Add role-specific information
        description += "\nMusician Role: " + role.name + " (" + role.instrument + ")";
        
        params->setProperty("description", description);
        
        // Set up instruments array for this role
        Array<var> instruments;
        instruments.add(role.getInstrumentName());
        params->setProperty("instruments", instruments);
        
        var parameters(params.get());
        
        // Use Player->generateChart instead of direct LLM call
        juce::String roleName = role.getName();
        DBG("DEBUG: About to call player->generateChart for " << roleName << " via proper architecture");
        player->generateChart(parameters, [this, roleIndex, roleName](bool success, const var& midiEventChart)
        {
            DBG("DEBUG: Player callback received for single role " << roleName << " - success: " << (success ? "true" : "false"));
            
            // Process the response on the message thread
            juce::MessageManager::callAsync([this, roleIndex, roleName, success, midiEventChart]()
            {
                std::shared_ptr<juce::MidiMessageSequence> pattern = nullptr;
                
                if (success && midiEventChart.isObject())
                {
                    // Extract MIDI events from the Player's response
                    if (midiEventChart.hasProperty("events"))
                    {
                        pattern = std::make_shared<juce::MidiMessageSequence>();
                        var eventsVar = midiEventChart["events"];
                        
                        if (eventsVar.isArray())
                        {
                            for (const auto& eventVar : *eventsVar.getArray())
                            {
                                if (eventVar.isObject())
                                {
                                    String type = eventVar.getProperty("type", "").toString();
                                    int channel = eventVar.getProperty("channel", 1);
                                    int note = eventVar.getProperty("note", 60);
                                    double timestamp = eventVar.getProperty("timestamp", 0.0);
                                    
                                    if (type == "note_on")
                                    {
                                        int velocity = eventVar.getProperty("velocity", 100);
                                        juce::MidiMessage msg = juce::MidiMessage::noteOn(channel, note, (juce::uint8)velocity);
                                        msg.setTimeStamp(timestamp);
                                        pattern->addEvent(msg);
                                    }
                                    else if (type == "note_off")
                                    {
                                        juce::MidiMessage msg = juce::MidiMessage::noteOff(channel, note);
                                        msg.setTimeStamp(timestamp);
                                        pattern->addEvent(msg);
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (pattern && pattern->getNumEvents() > 0)
                {
                    // Update status
                    musicianStatusTable.updateMusicianStatus(roleIndex, "Generated");
                    
                    // Store the pattern in the ensemble manager
                    ensembleManager->setMIDIPatternForRole(roleName, pattern);
                    
                    // Store the pattern in the current performance
                    if (currentPerformance)
                    {
                        // Convert MidiMessageSequence to MIDI_EVENT_CHART format for storage
                        DynamicObject* chartObj = new DynamicObject();
                        Array<var> events;
                        
                        for (int eventIndex = 0; eventIndex < pattern->getNumEvents(); ++eventIndex)
                        {
                            auto event = pattern->getEventPointer(eventIndex);
                            if (event->message.isNoteOn())
                            {
                                DynamicObject* noteOn = new DynamicObject();
                                noteOn->setProperty("type", "note_on");
                                noteOn->setProperty("channel", event->message.getChannel());
                                noteOn->setProperty("note", event->message.getNoteNumber());
                                noteOn->setProperty("velocity", (int)(event->message.getVelocity() * 127));
                                noteOn->setProperty("timestamp", event->message.getTimeStamp());
                                events.add(var(noteOn));
                            }
                            else if (event->message.isNoteOff())
                            {
                                DynamicObject* noteOff = new DynamicObject();
                                noteOff->setProperty("type", "note_off");
                                noteOff->setProperty("channel", event->message.getChannel());
                                noteOff->setProperty("note", event->message.getNoteNumber());
                                noteOff->setProperty("timestamp", event->message.getTimeStamp());
                                events.add(var(noteOff));
                            }
                            else if (event->message.isController())
                            {
                                DynamicObject* controller = new DynamicObject();
                                controller->setProperty("type", "controller");
                                controller->setProperty("channel", event->message.getChannel());
                                controller->setProperty("controller", event->message.getControllerNumber());
                                controller->setProperty("value", event->message.getControllerValue());
                                controller->setProperty("timestamp", event->message.getTimeStamp());
                                events.add(var(controller));
                            }
                        }
                        
                        chartObj->setProperty("events", events);
                        chartObj->setProperty("tempo", currentSong ? currentSong->getTempo() : 120.0);
                        chartObj->setProperty("success", true);
                        
                        // Store the MIDI_EVENT_CHART in the performance
                        currentPerformance->setGeneratedChart(roleName, var(chartObj));
                        currentPerformance->setStatusMessage(roleName, "Generated");
                        
                        // Save the performance
                        saveCurrentPerformance();
                    }
                    
                    // Enable the play button
                    playButton.setEnabled(true);
                }
                else
                {
                    // Update status with error
                    musicianStatusTable.updateMusicianStatus(roleIndex, "Error: Could not parse response");
                    
                    if (currentPerformance)
                    {
                        currentPerformance->setStatusMessage(roleName, "Error: Could not parse response");
                    }
                }
            });
        });
    }
}

void PerformanceComponent::cancelGeneration()
{
    // Not implemented yet
}

void PerformanceComponent::playGeneratedPatterns()
{
    if (ensembleManager)
    {
        bool success = ensembleManager->playPatterns();
        if (success)
        {
            // Update UI to reflect playing state
            playButton.setEnabled(false);
            stopButton.setEnabled(true);
            
            // Update status message
            statusLabel.setText("Playing patterns...", juce::dontSendNotification);
            
            // Notify listeners
            listeners.call(&Listener::performancePlaybackStarted, this);
        }
        else
        {
            statusLabel.setText("Failed to start playback. No patterns available.", juce::dontSendNotification);
        }
    }
}

void PerformanceComponent::stopPlayback()
{
    if (ensembleManager)
    {
        ensembleManager->stopPlayback();
        
        // Update UI to reflect stopped state
        playButton.setEnabled(true);
        stopButton.setEnabled(false);
        
        // Update status message
        statusLabel.setText("Playback stopped", juce::dontSendNotification);
        
        // Notify listeners
        listeners.call(&Listener::performancePlaybackStopped, this);
    }
}

bool PerformanceComponent::isPlaying() const
{
    if (ensembleManager)
    {
        return ensembleManager->isPlaying();
    }
    return false;
}

void PerformanceComponent::saveCurrentPerformance()
{
    if (!currentPerformance)
    {
        // Create a new performance if none exists
        createNewPerformance();
    }
    
    // Update the performance with current values
    if (currentSong)
    {
        currentPerformance->setSong(currentSong);
    }
    
    // Get the selected ensemble name
    int selectedEnsembleId = ensembleSelector.getSelectedId();
    if (selectedEnsembleId > 0)
    {
        juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
        if (selectedEnsembleId <= ensembleFiles.size())
        {
            auto ensemble = EnsembleManager::loadFromFile(ensembleFiles[selectedEnsembleId - 1], 
                                                       ensembleManager->getPlayerManager());
            if (ensemble)
            {
                currentPerformance->setEnsembleName(ensemble->getName());
            }
        }
    }
    
    currentPerformance->setDescription(descriptionEditor.getText());
    currentPerformance->setInstructions(instructionsEditor.getText());
    
    // Save the performance
    performanceManager->savePerformance(currentPerformance);
    
    // Reload performances and update the selector
    loadedPerformances = performanceManager->loadPerformancesFromDefaultLocation();
    updatePerformanceSelector();
    
    // Select the current performance in the selector
    for (int i = 0; i < loadedPerformances.size(); ++i)
    {
        if (loadedPerformances[i]->getUniqueID() == currentPerformance->getUniqueID())
        {
            performanceSelector.setSelectedId(i + 1);
            break;
        }
    }
    
    // Notify listeners
    listeners.call(&Listener::performanceSaved, this, currentPerformance);
}

void PerformanceComponent::createNewPerformance()
{
    // Create a new performance
    currentPerformance = std::make_shared<Performance>("New Performance");
    
    // Set the current song if available
    if (currentSong)
    {
        currentPerformance->setSong(currentSong);
    }
    
    // Set the current ensemble name if available
    int selectedEnsembleId = ensembleSelector.getSelectedId();
    if (selectedEnsembleId > 0)
    {
        juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
        if (selectedEnsembleId <= ensembleFiles.size())
        {
            auto ensemble = EnsembleManager::loadFromFile(ensembleFiles[selectedEnsembleId - 1], 
                                                       ensembleManager->getPlayerManager());
            if (ensemble)
            {
                currentPerformance->setEnsembleName(ensemble->getName());
            }
        }
    }
    
    // Set the current instructions
    currentPerformance->setDescription(descriptionEditor.getText());
    currentPerformance->setInstructions(instructionsEditor.getText());
    
    // Set the performance attributes
    currentPerformance->setKey(keySelector.getText());
    currentPerformance->setGenre(genreSelector.getText());
    currentPerformance->setMood(moodSelector.getText());
    
    // Save the performance
    performanceManager->savePerformance(currentPerformance);
    
    // Reload performances and update the selector
    loadedPerformances = performanceManager->loadPerformancesFromDefaultLocation();
    updatePerformanceSelector();
    
    // Select the current performance in the selector
    for (int i = 0; i < loadedPerformances.size(); ++i)
    {
        if (loadedPerformances[i]->getUniqueID() == currentPerformance->getUniqueID())
        {
            performanceSelector.setSelectedId(i + 1);
            break;
        }
    }
}

void PerformanceComponent::loadPerformance(std::shared_ptr<Performance> performance)
{
    if (!performance)
    {
        return;
    }
    
    currentPerformance = performance;
    
    // Load the song if available
    if (performance->getSong())
    {
        setCurrentSong(performance->getSong());
    }
    
    // Select the ensemble if available
    juce::String ensembleName = performance->getEnsembleName();
    if (ensembleName.isNotEmpty())
    {
        for (int i = 0; i < ensembleSelector.getNumItems(); ++i)
        {
            if (ensembleSelector.getItemText(i) == ensembleName)
            {
                ensembleSelector.setSelectedItemIndex(i);
                break;
            }
        }
    }
    
    // Set the description
    descriptionEditor.setText(performance->getDescription());
    
    // Set the instructions
    instructionsEditor.setText(performance->getInstructions());
    
    // Set the key selector if available
    juce::String key = performance->getKey();
    if (key.isNotEmpty())
    {
        // Find the matching item in the key selector
        for (int i = 0; i < keySelector.getNumItems(); ++i)
        {
            if (keySelector.getItemText(i) == key)
            {
                keySelector.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }
    else
    {
        keySelector.setSelectedId(1, juce::dontSendNotification); // Default to C Major
    }
    
    // Set the genre selector if available
    juce::String genre = performance->getGenre();
    if (genre.isNotEmpty())
    {
        // Find the matching item in the genre selector
        for (int i = 0; i < genreSelector.getNumItems(); ++i)
        {
            if (genreSelector.getItemText(i) == genre)
            {
                genreSelector.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }
    else
    {
        genreSelector.setSelectedId(1, juce::dontSendNotification); // Default to Pop
    }
    
    // Set the mood selector if available
    juce::String mood = performance->getMood();
    if (mood.isNotEmpty())
    {
        // Find the matching item in the mood selector
        for (int i = 0; i < moodSelector.getNumItems(); ++i)
        {
            if (moodSelector.getItemText(i) == mood)
            {
                moodSelector.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }
    else
    {
        moodSelector.setSelectedId(1, juce::dontSendNotification); // Default to Happy
    }
    
    // Update the musician status table
    updateMusicianStatusTable();
    
    // Update the status messages for each musician
    for (int i = 0; i < musicianStatusTable.getMusicianRoles().size(); ++i)
    {
        const auto& role = musicianStatusTable.getMusicianRoles()[i];
        juce::String status = performance->getStatusMessage(role.name);
        if (status.isNotEmpty())
        {
            musicianStatusTable.updateMusicianStatus(i, status);
        }
    }
    
    // Load the generated patterns into the ensemble manager
    for (int i = 0; i < musicianStatusTable.getMusicianRoles().size(); ++i)
    {
        const auto& role = musicianStatusTable.getMusicianRoles()[i];
        auto chart = performance->getGeneratedChart(role.name);
        if (!chart.isVoid())
        {
            // For now, skip setting MIDI pattern since we need conversion
            // TODO: Convert chart var back to MidiMessageSequence if needed
        }
    }
    
    // Enable the play button if there are generated patterns
    bool hasGeneratedPatterns = false;
    for (int i = 0; i < musicianStatusTable.getMusicianRoles().size(); ++i)
    {
        const auto& role = musicianStatusTable.getMusicianRoles()[i];
        if (!performance->getGeneratedChart(role.name).isVoid())
        {
            hasGeneratedPatterns = true;
            break;
        }
    }
    
    playButton.setEnabled(hasGeneratedPatterns && !ensembleManager->isPlaying());
}



int PerformanceComponent::getMidiNoteFromName(const juce::String& name)
{
    // Parse note name in scientific pitch notation (e.g., C4, A#3, Db5)
    juce::String noteName = name.trim().toUpperCase();
    
    if (noteName.isEmpty())
        return 60; // Middle C as default
    
    // Extract the octave number (last character)
    int octave = noteName.getLastCharacter() - '0';
    juce::String note = noteName.dropLastCharacters(1);
    
    // Base values for each note in octave 0
    int baseNote;
    if (note == "C") baseNote = 0;
    else if (note == "C#" || note == "DB") baseNote = 1;
    else if (note == "D") baseNote = 2;
    else if (note == "D#" || note == "EB") baseNote = 3;
    else if (note == "E") baseNote = 4;
    else if (note == "F") baseNote = 5;
    else if (note == "F#" || note == "GB") baseNote = 6;
    else if (note == "G") baseNote = 7;
    else if (note == "G#" || note == "AB") baseNote = 8;
    else if (note == "A") baseNote = 9;
    else if (note == "A#" || note == "BB") baseNote = 10;
    else if (note == "B") baseNote = 11;
    else return 60; // Default to middle C if invalid note name
    
    // Calculate MIDI note number: 12 * octave + baseNote + 12 (to start from C0 = 12)
    return 12 * octave + baseNote + 12;
}

} // namespace covertracks
