#include "PlayerModeComponent.h"

namespace covertracks {

PlayerModeComponent::PlayerModeComponent(PlayerManager& pm)
    : playerManager(pm),
      titleLabel("titleLabel", "Player Mode"),
      playButton("Play"),
      stopButton("Stop"),
      playAllButton("Play All")
{
    // Set up title label
    titleLabel.setFont(Font(18.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up player selector
    playerSelector.setTextWhenNoChoicesAvailable("No players available");
    playerSelector.setTextWhenNothingSelected("Select a player");
    playerSelector.addListener(this);
    addAndMakeVisible(playerSelector);
    
    // Set up buttons
    playButton.addListener(this);
    playButton.setEnabled(false);
    addAndMakeVisible(playButton);
    
    stopButton.addListener(this);
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);
    
    playAllButton.addListener(this);
    playAllButton.setEnabled(false);
    addAndMakeVisible(playAllButton);
    
    // Register as a listener for player manager changes
    playerManager.addChangeListener(this);
    
    // Update player selector with available players
    updatePlayerSelector();
    
    // Load default pattern
    loadDefaultPattern();
}

PlayerModeComponent::~PlayerModeComponent()
{
    // Remove listeners
    playerSelector.removeListener(this);
    playButton.removeListener(this);
    stopButton.removeListener(this);
    playAllButton.removeListener(this);
    
    // Remove change listener from player manager
    playerManager.removeChangeListener(this);
}

void PlayerModeComponent::paint(Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void PlayerModeComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title at the top
    titleLabel.setBounds(area.removeFromTop(30));
    
    area.removeFromTop(10); // Add some spacing
    
    // Player selector
    playerSelector.setBounds(area.removeFromTop(30));
    
    area.removeFromTop(20); // Add some spacing
    
    // Buttons in a row
    auto buttonArea = area.removeFromTop(30);
    int buttonWidth = (buttonArea.getWidth() - 20) / 3; // 10px spacing between buttons
    
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(10); // Spacing
    stopButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(10); // Spacing
    playAllButton.setBounds(buttonArea);
}

void PlayerModeComponent::buttonClicked(Button* button)
{
    if (button == &playButton)
    {
        if (selectedPlayer != nullptr && !currentPattern.isVoid())
        {
            // Play the pattern on the selected player
            playerManager.playPattern(selectedPlayer, currentPattern);
            stopButton.setEnabled(true);
        }
    }
    else if (button == &stopButton)
    {
        // Stop playback
        if (selectedPlayer != nullptr)
        {
            playerManager.stopPlayback(selectedPlayer);
        }
        else
        {
            playerManager.stopPlayback(nullptr); // Stop all players
        }
        stopButton.setEnabled(false);
    }
    else if (button == &playAllButton)
    {
        if (!currentPattern.isVoid())
        {
            // Play the pattern on all active players
            playerManager.playPatternOnAllActivePlayers(currentPattern);
            stopButton.setEnabled(true);
        }
    }
}

void PlayerModeComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &playerSelector)
    {
        int selectedId = playerSelector.getSelectedId();
        
        if (selectedId > 0)
        {
            // Get the selected player
            auto players = playerManager.getPlayers();
            
            if (selectedId <= players.size())
            {
                selectedPlayer = players[selectedId - 1];
                playButton.setEnabled(true);
                playAllButton.setEnabled(true);
                
                // Prepare the player for playback
                if (!selectedPlayer->isReadyForPlayback())
                {
                    selectedPlayer->prepareForPlayback();
                }
            }
        }
        else
        {
            selectedPlayer = nullptr;
            playButton.setEnabled(false);
        }
    }
}

void PlayerModeComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    // If the change came from the PlayerManager, update our player selector
    if (source == &playerManager)
    {
        std::cout << "PlayerModeComponent received change notification from PlayerManager" << std::endl;
        
        // Remember the currently selected player ID
        int currentSelectedId = playerSelector.getSelectedId();
        
        // Update the player selector
        updatePlayerSelector();
        
        // Try to restore the previous selection if possible
        if (currentSelectedId > 0 && currentSelectedId <= playerSelector.getNumItems())
        {
            playerSelector.setSelectedId(currentSelectedId, juce::dontSendNotification);
        }
        else if (playerSelector.getNumItems() > 0)
        {
            // Select the first player if the previous selection is no longer valid
            playerSelector.setSelectedItemIndex(0, juce::sendNotification);
        }
    }
}

var PlayerModeComponent::loadPatternFromFile(const File& file)
{
    if (!file.existsAsFile())
    {
        return var();
    }
    
    // Load the JSON file
    auto json = JSON::parse(file);
    
    if (json.isVoid())
    {
        return var();
    }
    
    // Store the pattern
    currentPattern = json;
    
    // Enable play buttons if we have a player selected
    playButton.setEnabled(selectedPlayer != nullptr);
    playAllButton.setEnabled(!playerManager.getPlayers().isEmpty());
    
    return json;
}

void PlayerModeComponent::updatePlayerSelector()
{
    // Clear existing items
    playerSelector.clear();
    
    // Get all available players
    auto players = playerManager.getPlayers();
    
    // Add players to the selector
    int index = 1;
    for (auto* player : players)
    {
        playerSelector.addItem(player->getMetadata().name, index++);
    }
    
    // Enable/disable play buttons based on available players
    playAllButton.setEnabled(!players.isEmpty() && !currentPattern.isVoid());
}

void PlayerModeComponent::loadDefaultPattern()
{
    // Try to load the default pattern
    File patternFile = File::getSpecialLocation(File::currentApplicationFile)
                        .getParentDirectory()
                        .getChildFile("Resources")
                        .getChildFile("TestPatterns")
                        .getChildFile("simple_pattern.json");
    
    if (!patternFile.existsAsFile())
    {
        // Try a different location
        patternFile = File(File::getCurrentWorkingDirectory())
                        .getChildFile("Resources")
                        .getChildFile("TestPatterns")
                        .getChildFile("simple_pattern.json");
    }
    
    if (patternFile.existsAsFile())
    {
        loadPatternFromFile(patternFile);
        std::cout << "Loaded default pattern from: " << patternFile.getFullPathName() << std::endl;
    }
    else
    {
        std::cerr << "Could not find default pattern file" << std::endl;
        
        // Create a simple pattern as fallback
        DynamicObject* obj = new DynamicObject();
        obj->setProperty("command", "play_pattern");
        obj->setProperty("tempo", 120.0);
        
        Array<var> events;
        
        // C4 note
        DynamicObject* noteOn = new DynamicObject();
        noteOn->setProperty("type", "note_on");
        noteOn->setProperty("channel", 1);
        noteOn->setProperty("note", 60);
        noteOn->setProperty("velocity", 0.8);
        noteOn->setProperty("timestamp", 0.0);
        events.add(var(noteOn));
        
        DynamicObject* noteOff = new DynamicObject();
        noteOff->setProperty("type", "note_off");
        noteOff->setProperty("channel", 1);
        noteOff->setProperty("note", 60);
        noteOff->setProperty("velocity", 0.0);
        noteOff->setProperty("timestamp", 0.5);
        events.add(var(noteOff));
        
        obj->setProperty("events", events);
        currentPattern = var(obj);
        
        // Enable play buttons if we have a player selected
        playButton.setEnabled(selectedPlayer != nullptr);
        playAllButton.setEnabled(!playerManager.getPlayers().isEmpty());
    }
}

} // namespace covertracks
