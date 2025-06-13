// Source/Musician/UI/PatternEditorComponent.cpp
#include "PatternEditorComponent.h"
#include <iostream>

namespace covertracks {

PatternEditorComponent::PatternEditorComponent(Musician* musicianPtr, PlayerManager* playerMgr)
    : headerLabel("HeaderLabel", "Pattern Editor"),
      playerLabel("PlayerLabel", "Player:"),
      parseButton("Parse"),
      loadButton("Load"),
      playButton("Play"),
      musician(musicianPtr),
      playerManager(playerMgr),
      selectedPlayer(nullptr)
{
    // Set up header label
    headerLabel.setFont(Font(FontOptions().withHeight(18.0f).withStyle("bold")));
    headerLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(headerLabel);
    
    // Set up pattern editor
    patternEditor.setMultiLine(true);
    patternEditor.setReturnKeyStartsNewLine(true);
    patternEditor.setReadOnly(false);
    patternEditor.setScrollbarsShown(true);
    patternEditor.setCaretVisible(true);
    patternEditor.setPopupMenuEnabled(true);
    patternEditor.setText(""); // Start with an empty editor instead of a hardcoded pattern
    addAndMakeVisible(patternEditor);
    
    // Set up player selection
    playerLabel.setFont(Font(FontOptions().withHeight(14.0f)));
    addAndMakeVisible(playerLabel);
    
    playerSelector.addListener(this);
    addAndMakeVisible(playerSelector);
    updatePlayerSelector();
    
    // Set up buttons
    parseButton.addListener(this);
    addAndMakeVisible(parseButton);
    
    loadButton.addListener(this);
    addAndMakeVisible(loadButton);
    
    playButton.addListener(this);
    addAndMakeVisible(playButton);
    
    // Set up MIDI keyboard visualization
    keyboardState = std::make_unique<MidiKeyboardState>();
    midiKeyboard = std::make_unique<MidiKeyboardComponent>(*keyboardState, MidiKeyboardComponent::horizontalKeyboard);
    midiKeyboard->setAvailableRange(36, 96); // C2 to C7
    addAndMakeVisible(*midiKeyboard);
    
    // Parse the initial pattern
    parseCurrentPattern();
    
    // Listen for changes from the musician
    if (musician)
        musician->addChangeListener(this);
}

PatternEditorComponent::~PatternEditorComponent()
{
    // Remove listeners
    parseButton.removeListener(this);
    loadButton.removeListener(this);
    playButton.removeListener(this);
    playerSelector.removeListener(this);
    
    if (musician)
        musician->removeChangeListener(this);
    
    // Clean up MIDI keyboard
    keyboardState.reset();
    midiKeyboard.reset();
}

void PatternEditorComponent::paint(Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void PatternEditorComponent::resized()
{
    auto area = getLocalBounds();
    
    // Header at the top
    headerLabel.setBounds(area.removeFromTop(30));
    
    // Player selection
    auto playerArea = area.removeFromTop(30);
    playerLabel.setBounds(playerArea.removeFromLeft(60).reduced(5));
    playerSelector.setBounds(playerArea.reduced(5));
    
    // Buttons at the bottom
    auto buttonArea = area.removeFromBottom(40);
    int buttonWidth = buttonArea.getWidth() / 3;
    
    parseButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    loadButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    playButton.setBounds(buttonArea.reduced(5));
    
    // Split remaining area between the editor and MIDI keyboard
    auto editorArea = area.withWidth(area.getWidth() / 2);
    auto keyboardArea = area.withX(editorArea.getRight());
    
    patternEditor.setBounds(editorArea.reduced(5));
    midiKeyboard->setBounds(keyboardArea.reduced(5));
}

void PatternEditorComponent::buttonClicked(Button* button)
{
    if (button == &parseButton)
    {
        // Parse the current pattern
        parseCurrentPattern();
        listeners.call(&Listener::patternParseRequested, this);
    }
    else if (button == &loadButton)
    {
        // Load the parsed pattern into the selected player
        loadParsedPattern();
        listeners.call(&Listener::patternLoadRequested, this);
    }
    else if (button == &playButton)
    {
        // Play the loaded pattern
        playLoadedPattern();
        listeners.call(&Listener::patternPlayRequested, this);
    }
}

void PatternEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == musician)
    {
        // Musician state has changed (e.g., new pattern generated)
        // We can update the UI if needed
    }
}

void PatternEditorComponent::setPattern(const String& patternNotation)
{
    // Set the pattern text
    patternEditor.setText(patternNotation);
    
    // Parse the pattern
    parseCurrentPattern();
}

String PatternEditorComponent::getPatternNotation() const
{
    return patternEditor.getText();
}

void PatternEditorComponent::patternUpdated()
{
    // Parse the current pattern
    parseCurrentPattern();
}

void PatternEditorComponent::parseCurrentPattern()
{
    currentPatternNotation = patternEditor.getText();
    
    if (!selectedPlayer)
    {
        std::cout << "No player selected for parsing. Please select a player first." << std::endl;
        return;
    }
    
    if (!currentPatternNotation.isEmpty())
    {
        // Convert pattern string to chart format using Player's conversion
        // This uses the same code path as Performance Mode via the Player's internal Musician
        var result = selectedPlayer->convertPatternStringToChart(currentPatternNotation, 120.0);
        
        if (result.hasProperty("success") && static_cast<bool>(result["success"]))
        {
            // Store the parsed chart
            currentParsedChart = result;
            std::cout << "Pattern parsed successfully. Events: " << result["events"].size() << std::endl;
            
            // Update MIDI visualization
            updateMidiVisualization();
        }
        else
        {
            std::cout << "Failed to parse pattern" << std::endl;
            currentParsedChart = var();
        }
    }
}

void PatternEditorComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &playerSelector)
    {
        onPlayerSelectionChanged();
    }
}

void PatternEditorComponent::updatePlayerSelector()
{
    playerSelector.clear();
    playerSelector.addItem("No Player Selected", 1);
    
    if (playerManager)
    {
        auto players = playerManager->getPlayers();
        int itemIndex = 2;
        for (auto* player : players)
        {
            if (player)
            {
                playerSelector.addItem(player->getMetadata().name, itemIndex++);
            }
        }
    }
    
    playerSelector.setSelectedItemIndex(0);
    selectedPlayer = nullptr;
}

void PatternEditorComponent::onPlayerSelectionChanged()
{
    int selectedIndex = playerSelector.getSelectedItemIndex();
    selectedPlayer = nullptr;
    
    if (selectedIndex > 0 && playerManager)
    {
        auto players = playerManager->getPlayers();
        int playerIndex = selectedIndex - 1; // Adjust for "No Player Selected" option
        
        if (playerIndex >= 0 && playerIndex < players.size())
        {
            selectedPlayer = players[playerIndex];
            std::cout << "Selected player: " << selectedPlayer->getMetadata().name << std::endl;
        }
    }
}

void PatternEditorComponent::loadParsedPattern()
{
    if (!selectedPlayer)
    {
        std::cout << "No player selected for loading pattern" << std::endl;
        return;
    }
    
    if (currentParsedChart.isVoid() || !currentParsedChart.hasProperty("events"))
    {
        std::cout << "No parsed pattern available. Parse first." << std::endl;
        return;
    }
    
    // Load the parsed chart into the selected player
    bool loaded = selectedPlayer->loadChart(currentParsedChart);
    
    if (loaded)
    {
        std::cout << "Pattern loaded successfully into " << selectedPlayer->getMetadata().name << std::endl;
    }
    else
    {
        std::cout << "Failed to load pattern into " << selectedPlayer->getMetadata().name << std::endl;
    }
}

void PatternEditorComponent::playLoadedPattern()
{
    if (!selectedPlayer)
    {
        std::cout << "No player selected for playback" << std::endl;
        return;
    }
    
    // Start playback on the selected player
    bool started = selectedPlayer->play();
    
    if (started)
    {
        std::cout << "Started playback on " << selectedPlayer->getMetadata().name << std::endl;
    }
    else
    {
        std::cout << "Failed to start playback on " << selectedPlayer->getMetadata().name << std::endl;
    }
}

void PatternEditorComponent::updateMidiVisualization()
{
    if (!keyboardState || currentParsedChart.isVoid())
        return;
    
    // Clear current state
    keyboardState->allNotesOff(1);
    
    // Extract events and show note activity on the keyboard
    if (currentParsedChart.hasProperty("events"))
    {
        var events = currentParsedChart["events"];
        
        if (events.isArray())
        {
            // Show which notes are used in the pattern (briefly highlight them)
            for (int i = 0; i < events.size(); ++i)
            {
                var event = events[i];
                
                if (event.hasProperty("type") && event.hasProperty("note"))
                {
                    String type = event["type"].toString();
                    int note = static_cast<int>(static_cast<double>(event["note"]));
                    
                    if (type == "note_on" && note >= 0 && note <= 127)
                    {
                        // Briefly show the note as active
                        keyboardState->noteOn(1, note, 0.5f);
                        
                        // Schedule note off after a short delay
                        Timer::callAfterDelay(200, [this, note]()
                        {
                            if (keyboardState)
                                keyboardState->noteOff(1, note, 0.0f);
                        });
                    }
                }
            }
        }
    }
}

void PatternEditorComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PatternEditorComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace covertracks