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
      stopButton("Stop"),
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
    
    stopButton.addListener(this);
    addAndMakeVisible(stopButton);
    
    // Set up piano roll visualization
    pianoRoll = std::make_unique<PianoRollComponent>();
    addAndMakeVisible(*pianoRoll);
    
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
    stopButton.removeListener(this);
    playerSelector.removeListener(this);
    
    if (musician)
        musician->removeChangeListener(this);
    
    // Clean up piano roll
    pianoRoll.reset();
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
    int buttonWidth = buttonArea.getWidth() / 4;
    
    parseButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    loadButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    stopButton.setBounds(buttonArea.reduced(5));
    
    // Split remaining area between the editor and piano roll
    auto editorArea = area.withWidth(area.getWidth() / 2);
    auto pianoRollArea = area.withX(editorArea.getRight());
    
    patternEditor.setBounds(editorArea.reduced(5));
    pianoRoll->setBounds(pianoRollArea.reduced(5));
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
    else if (button == &stopButton)
    {
        // Stop the current playback
        stopCurrentPlayback();
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
            
            // Update piano roll visualization
            updatePianoRollVisualization();
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

void PatternEditorComponent::stopCurrentPlayback()
{
    if (selectedPlayer)
    {
        selectedPlayer->stop();
        std::cout << "Stopped playback on " << selectedPlayer->getMetadata().name << std::endl;
    }
    else
    {
        std::cout << "No player selected to stop" << std::endl;
    }
}

void PatternEditorComponent::updatePianoRollVisualization()
{
    if (pianoRoll)
    {
        pianoRoll->setMidiEvents(currentParsedChart);
        pianoRoll->repaint();
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

// Piano Roll Visualization Component Implementation
PatternEditorComponent::PianoRollComponent::PianoRollComponent() = default;

void PatternEditorComponent::PianoRollComponent::setMidiEvents(const var& chart)
{
    events.clear();
    
    if (chart.hasProperty("events") && chart["events"].isArray())
    {
        var eventsArray = chart["events"];
        
        for (int i = 0; i < eventsArray.size(); ++i)
        {
            var event = eventsArray[i];
            
            if (event.hasProperty("type") && event.hasProperty("note") && event.hasProperty("timestamp"))
            {
                String type = event["type"].toString();
                int note = static_cast<int>(static_cast<double>(event["note"]));
                double timestamp = static_cast<double>(event["timestamp"]);
                double duration = event.hasProperty("duration") ? static_cast<double>(event["duration"]) : 0.25;
                
                if (type == "note_on")
                {
                    events.add({note, timestamp, duration});
                }
            }
        }
    }
    
    repaint();
}

void PatternEditorComponent::PianoRollComponent::paint(Graphics& g)
{
    g.fillAll(Colours::white);
    
    // Draw background grid
    g.setColour(Colours::lightgrey);
    
    auto bounds = getLocalBounds();
    int width = bounds.getWidth();
    int height = bounds.getHeight();
    
    // Time grid - vertical lines every beat
    double beatsToShow = 8.0; // Show 8 beats
    double pixelsPerBeat = width / beatsToShow;
    
    for (double beat = 0; beat <= beatsToShow; beat += 0.25) // Quarter note grid
    {
        int x = static_cast<int>(beat * pixelsPerBeat);
        bool isDownbeat = (static_cast<int>(beat * 4) % 4 == 0);
        g.setColour(isDownbeat ? Colours::grey : Colours::lightgrey);
        g.drawVerticalLine(x, 0.0f, static_cast<float>(height));
    }
    
    // Note grid - horizontal lines for piano keys
    int minNote = 36; // C2
    int maxNote = 96; // C7
    int noteRange = maxNote - minNote;
    double pixelsPerNote = static_cast<double>(height) / noteRange;
    
    for (int note = minNote; note <= maxNote; ++note)
    {
        int y = static_cast<int>((maxNote - note) * pixelsPerNote);
        bool isWhiteKey = isWhiteKeyNote(note);
        g.setColour(isWhiteKey ? Colours::lightgrey : Colours::darkgrey.withAlpha(0.3f));
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(width));
    }
    
    // Draw notes
    g.setColour(Colours::blue.withAlpha(0.7f));
    
    for (const auto& event : events)
    {
        int note = event.note;
        double timestamp = event.timestamp;
        double duration = event.duration;
        
        if (note >= minNote && note <= maxNote)
        {
            int x = static_cast<int>(timestamp * pixelsPerBeat);
            int y = static_cast<int>((maxNote - note) * pixelsPerNote);
            int noteWidth = static_cast<int>(duration * pixelsPerBeat);
            int noteHeight = static_cast<int>(pixelsPerNote * 0.8); // 80% of note space
            
            Rectangle<int> noteRect(x, y, noteWidth, noteHeight);
            g.fillRect(noteRect);
            
            // Draw note border
            g.setColour(Colours::darkblue);
            g.drawRect(noteRect, 1);
            g.setColour(Colours::blue.withAlpha(0.7f));
        }
    }
    
    // Draw time labels
    g.setColour(Colours::black);
    g.setFont(Font(FontOptions().withHeight(12.0f)));
    
    for (double beat = 0; beat <= beatsToShow; beat += 1.0)
    {
        int x = static_cast<int>(beat * pixelsPerBeat);
        g.drawText(String(beat, 1), x + 2, 2, 50, 20, Justification::centredLeft);
    }
    
    // Draw note labels on the left
    for (int note = minNote; note <= maxNote; note += 12) // Every octave
    {
        int y = static_cast<int>((maxNote - note) * pixelsPerNote);
        String noteName = getMidiNoteName(note);
        g.drawText(noteName, 2, y - 10, 30, 20, Justification::centredLeft);
    }
}

bool PatternEditorComponent::PianoRollComponent::isWhiteKeyNote(int midiNote)
{
    int noteInOctave = midiNote % 12;
    return (noteInOctave == 0 || noteInOctave == 2 || noteInOctave == 4 || 
            noteInOctave == 5 || noteInOctave == 7 || noteInOctave == 9 || noteInOctave == 11);
}

String PatternEditorComponent::PianoRollComponent::getMidiNoteName(int midiNote)
{
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteInOctave = midiNote % 12;
    return String(noteNames[noteInOctave]) + String(octave);
}

} // namespace covertracks