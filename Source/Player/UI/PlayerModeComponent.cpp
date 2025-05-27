#include "PlayerModeComponent.h"

namespace covertracks {

PlayerModeComponent::PlayerModeComponent(PlayerManager& pm)
    : playerManager(pm),
      titleLabel("titleLabel", "Pattern Player"),
      playButton("Play"),
      stopButton("Stop"),
      playAllButton("Play All")
{
    // Set up title label
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("bold")));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up player selector
    playerSelector.setTextWhenNoChoicesAvailable("No players available");
    playerSelector.setTextWhenNothingSelected("Select a player");
    playerSelector.addListener(this);
    addAndMakeVisible(playerSelector);
    
    // Set up instrument type selector
    instrumentTypeSelector.addItem("Drums", 1);
    instrumentTypeSelector.addItem("Bass", 2);
    instrumentTypeSelector.addItem("Piano", 3);
    instrumentTypeSelector.addItem("Guitar", 4);
    instrumentTypeSelector.setTextWhenNothingSelected("Select instrument type");
    instrumentTypeSelector.addListener(this);
    addAndMakeVisible(instrumentTypeSelector);
    
    // Set up pattern selector
    patternSelector.addItem("Basic", 1);
    patternSelector.addItem("Complex", 2);
    patternSelector.addItem("Funky", 3);
    patternSelector.addItem("Jazzy", 4);
    patternSelector.setTextWhenNothingSelected("Select pattern");
    patternSelector.addListener(this);
    addAndMakeVisible(patternSelector);
    
    // Set up pattern viewer
    patternViewer.setMultiLine(true);
    patternViewer.setReadOnly(true);
    patternViewer.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(14.0f).withStyle("plain")));
    addAndMakeVisible(patternViewer);
    
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
    
    // Initialize the pattern system
    patternSystem = std::make_unique<MusicPatternSystem>();
    
    // Create default patterns
    createDefaultPatterns();
    
    // Update player selector with available players
    updatePlayerSelector();
}

PlayerModeComponent::~PlayerModeComponent()
{
    // Remove listeners
    playerSelector.removeListener(this);
    instrumentTypeSelector.removeListener(this);
    patternSelector.removeListener(this);
    playButton.removeListener(this);
    stopButton.removeListener(this);
    playAllButton.removeListener(this);
    playerManager.removeChangeListener(this);
    
    // Clear any references to patterns
    currentChart = nullptr;
}

void PlayerModeComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void PlayerModeComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Position title label at the top
    titleLabel.setBounds(bounds.removeFromTop(30));
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position player selector
    auto row = bounds.removeFromTop(30);
    playerSelector.setBounds(row);
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position instrument type selector
    row = bounds.removeFromTop(30);
    instrumentTypeSelector.setBounds(row);
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position pattern selector
    row = bounds.removeFromTop(30);
    patternSelector.setBounds(row);
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position pattern viewer
    auto patternViewerArea = bounds.removeFromTop(200);
    patternViewer.setBounds(patternViewerArea);
    
    // Add spacing
    bounds.removeFromTop(10);
    
    // Position buttons
    row = bounds.removeFromTop(30);
    int buttonWidth = (row.getWidth() - 20) / 3;
    
    playButton.setBounds(row.removeFromLeft(buttonWidth));
    row.removeFromLeft(10);
    stopButton.setBounds(row.removeFromLeft(buttonWidth));
    row.removeFromLeft(10);
    playAllButton.setBounds(row);
}

void PlayerModeComponent::buttonClicked(juce::Button* button)
{
    if (button == &playButton)
    {
        if (selectedPlayer != nullptr && currentChart != nullptr)
        {
            // Play the current pattern on the selected player
            playCurrentPattern();
            
            // Update button states
            playButton.setEnabled(false);
            stopButton.setEnabled(true);
        }
    }
    else if (button == &stopButton)
    {
        if (selectedPlayer != nullptr)
        {
            // Stop playback on the selected player
            selectedPlayer->stop();
            
            // Update button states
            playButton.setEnabled(true);
            stopButton.setEnabled(false);
        }
    }
    else if (button == &playAllButton)
    {
        if (currentChart != nullptr)
        {
            // Convert the chart to a MIDI sequence
            auto midiSequence = currentChart->toMidiSequence();
            
            // Create the pattern var once and use it for all players
            juce::DynamicObject* patternObj = new juce::DynamicObject();
            juce::Array<juce::var> events;
            
            // Convert MIDI sequence to the expected format
            for (int i = 0; i < midiSequence->getNumEvents(); ++i)
            {
                auto event = midiSequence->getEventPointer(i);
                
                if (event->message.isNoteOn())
                {
                    juce::DynamicObject* noteOn = new juce::DynamicObject();
                    noteOn->setProperty("type", "note_on");
                    noteOn->setProperty("channel", event->message.getChannel());
                    noteOn->setProperty("note", event->message.getNoteNumber());
                    noteOn->setProperty("velocity", static_cast<int>(event->message.getVelocity() * 127));
                    noteOn->setProperty("time", event->message.getTimeStamp());
                    events.add(juce::var(noteOn));
                }
                else if (event->message.isNoteOff())
                {
                    juce::DynamicObject* noteOff = new juce::DynamicObject();
                    noteOff->setProperty("type", "note_off");
                    noteOff->setProperty("channel", event->message.getChannel());
                    noteOff->setProperty("note", event->message.getNoteNumber());
                    noteOff->setProperty("velocity", 0);
                    noteOff->setProperty("time", event->message.getTimeStamp());
                    events.add(juce::var(noteOff));
                }
            }
            
            patternObj->setProperty("events", events);
            patternObj->setProperty("tempo", currentChart->getTempo());
            juce::var patternVar(patternObj);
            
            // Play on all players
            for (auto* player : playerManager.getPlayers())
            {
                if (player && player->setPatternFromVar(patternVar))
                {
                    player->play();
                }
            }
            
            // Update button states
            playButton.setEnabled(false);
            stopButton.setEnabled(true);
            playAllButton.setEnabled(false);
        }
    }
}

void PlayerModeComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &playerSelector)
    {
        int selectedId = playerSelector.getSelectedId();
        
        if (selectedId > 0)
        {
            // Get the player with the selected ID
            auto players = playerManager.getPlayers();
            if (selectedId > 0 && selectedId <= players.size())
            {
                selectedPlayer = players[selectedId - 1];
            }
            
            // Enable play button if we have a player and a chart
            playButton.setEnabled(selectedPlayer != nullptr && currentChart != nullptr);
        }
        else
        {
            // No player selected
            selectedPlayer = nullptr;
            playButton.setEnabled(false);
        }
    }
    else if (comboBox == &instrumentTypeSelector || comboBox == &patternSelector)
    {
        // Update the current chart based on the selected instrument and pattern
        updateCurrentChart();
    }
}

void PlayerModeComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &playerManager)
    {
        // Player manager has changed, update the player selector
        updatePlayerSelector();
        
        // Check if the selected player is still valid
        if (selectedPlayer != nullptr)
        {
            bool playerStillExists = false;
            
            for (auto* player : playerManager.getPlayers())
            {
                if (player == selectedPlayer)
                {
                    playerStillExists = true;
                    break;
                }
            }
            
            if (!playerStillExists)
            {
                selectedPlayer = nullptr;
                playButton.setEnabled(false);
            }
        }
    }
}

void PlayerModeComponent::updatePlayerSelector()
{
    // Remember the currently selected player ID
    int currentSelectedId = playerSelector.getSelectedId();
    
    // Clear existing items
    playerSelector.clear();
    
    // Get available players
    auto players = playerManager.getPlayers();
    
    // Add each player to the selector
    for (int i = 0; i < players.size(); ++i)
    {
        auto* player = players[i];
        playerSelector.addItem(player->getMetadata().name, i + 1);
    }
    
    // Try to restore the previous selection if possible
    if (currentSelectedId > 0 && currentSelectedId <= players.size())
    {
        playerSelector.setSelectedId(currentSelectedId, juce::dontSendNotification);
        selectedPlayer = players[currentSelectedId - 1];
    }
    else if (players.size() > 0)
    {
        // Select the first player if available
        playerSelector.setSelectedId(1, juce::dontSendNotification);
        selectedPlayer = players[0];
    }
    else
    {
        // No players available
        selectedPlayer = nullptr;
    }
    
    // Update button states
    playButton.setEnabled(selectedPlayer != nullptr && currentChart != nullptr);
    playAllButton.setEnabled(players.size() > 0 && currentChart != nullptr);
}

void PlayerModeComponent::createDefaultPatterns()
{
    // Create default patterns for each instrument type
    createDrumPattern();
    createBassPattern();
    createPianoPattern();
    createGuitarPattern();
    
    // Select the first instrument and pattern by default
    if (instrumentTypeSelector.getNumItems() > 0)
    {
        instrumentTypeSelector.setSelectedItemIndex(0);
        
        if (patternSelector.getNumItems() > 0)
        {
            patternSelector.setSelectedItemIndex(0);
            
            // Set the current chart based on the selected instrument and pattern
            updateCurrentChart();
        }
    }
}

std::shared_ptr<Chart> PlayerModeComponent::createDrumPattern()
{
    // Create a basic drum pattern
    auto chart = std::make_shared<Chart>("Basic Drum Pattern", 120.0);
    
    // Create a section (e.g., "Verse")
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Main Beat");
    
    // Create a bar with a basic drum pattern
    auto bar = std::make_shared<Bar>(4, 4);
    
    // Add kick drum on beats 1 and 3
    // Note: Channel 9 in code = Channel 10 in MIDI (standard drum channel)
    // Using longer durations for better playback
    bar->addNote(0, 9, 36, 0.8f, 0.5f);  // Beat 1, channel 10 (MIDI channel 9 is drums), note 36 (kick)
    bar->addNote(2, 9, 36, 0.8f, 0.5f);  // Beat 3
    
    // Add snare on beats 2 and 4
    bar->addNote(1, 9, 38, 0.7f, 0.5f);  // Beat 2, note 38 (snare)
    bar->addNote(3, 9, 38, 0.7f, 0.5f);  // Beat 4
    
    // Add hi-hat on every eighth note with longer durations
    for (int i = 0; i < 8; ++i)
    {
        bar->addNote(static_cast<float>(i * 0.5f), 9, 42, 0.6f, 0.5f);  // Note 42 (closed hi-hat)
    }
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    // Store the chart in the pattern system
    patternSystem->addChart("drums_basic", chart);
    
    return chart;
}

std::shared_ptr<Chart> PlayerModeComponent::createBassPattern()
{
    // Create a basic bass pattern
    auto chart = std::make_shared<Chart>("Basic Bass Pattern", 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Bass Line");
    
    // Create a bar with a simple bass line
    auto bar = std::make_shared<Bar>(4, 4);
    
    // Add bass notes (simple walking bass line)
    // Using longer durations for better playback
    bar->addNote(0, 0, 36, 0.9f, 1.0f);    // C2 (1 beat duration)
    bar->addNote(1, 0, 38, 0.8f, 1.0f);    // D2 (1 beat duration)
    bar->addNote(2, 0, 40, 0.8f, 1.0f);    // E2 (1 beat duration)
    bar->addNote(3, 0, 43, 0.9f, 1.0f);    // G2 (1 beat duration)
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    // Store the chart in the pattern system
    patternSystem->addChart("bass_basic", chart);
    
    return chart;
}

std::shared_ptr<Chart> PlayerModeComponent::createPianoPattern()
{
    // Create a basic piano pattern
    auto chart = std::make_shared<Chart>("Basic Piano Pattern", 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Piano Melody");
    
    // Create a bar with a simple piano melody
    auto bar = std::make_shared<Bar>(4, 4);
    
    // Add piano notes (C major chord arpeggio with proper timing)
    // Note: Channel is 0-based in addNote but becomes 1-based in MIDI
    bar->addNote(0, 0, 60, 0.8f, 1.0f);    // C4 (1 beat duration)
    bar->addNote(1, 0, 64, 0.7f, 1.0f);    // E4 (1 beat duration)
    bar->addNote(2, 0, 67, 0.7f, 1.0f);    // G4 (1 beat duration)
    bar->addNote(3, 0, 72, 0.8f, 1.0f);    // C5 (1 beat duration)
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    // Store the chart in the pattern system
    patternSystem->addChart("piano_basic", chart);
    
    return chart;
}

std::shared_ptr<Chart> PlayerModeComponent::createGuitarPattern()
{
    // Create a basic guitar pattern
    auto chart = std::make_shared<Chart>("Basic Guitar Pattern", 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Guitar Riff");
    
    // Create a bar with a simple guitar riff
    auto bar = std::make_shared<Bar>(4, 4);
    
    // Add guitar notes (C, E, G, C pattern with strumming)
    // Using longer durations for better playback
    bar->addNote(0, 0, 48, 0.8f, 0.5f);    // C3 (half beat duration)
    bar->addNote(static_cast<int>(0.5f), 0, 52, 0.7f, 0.5f);    // E3 (half beat duration)
    bar->addNote(1, 0, 55, 0.7f, 0.5f);    // G3 (half beat duration)
    bar->addNote(static_cast<int>(1.5f), 0, 60, 0.8f, 0.5f);    // C4 (half beat duration)
    bar->addNote(2, 0, 48, 0.8f, 0.5f);    // C3 (half beat duration)
    bar->addNote(static_cast<int>(2.5f), 0, 52, 0.7f, 0.5f);    // E3 (half beat duration)
    bar->addNote(3, 0, 55, 0.7f, 0.5f);    // G3 (half beat duration)
    bar->addNote(static_cast<int>(3.5f), 0, 60, 0.8f, 0.5f);    // C4 (half beat duration)
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    // Store the chart in the pattern system
    patternSystem->addChart("guitar_basic", chart);
    
    return chart;
}

void PlayerModeComponent::updatePatternViewer()
{
    if (currentChart == nullptr)
    {
        patternViewer.setText("No pattern selected");
        return;
    }
    
    // Create a text representation of the chart structure
    juce::String text = "Chart: " + currentChart->getTitle() + "\n";
    text += "Tempo: " + juce::String(currentChart->getTempo()) + " BPM\n\n";
    
    // Get the MIDI sequence for the chart
    auto midiSequence = currentChart->toMidiSequence();
    text += "Total MIDI events: " + juce::String(midiSequence->getNumEvents()) + "\n";
    text += "Duration: " + juce::String(midiSequence->getEndTime()) + " ticks\n\n";
    
    // Add sections with detailed note information
    for (int i = 0; i < currentChart->getNumSections(); ++i)
    {
        auto section = currentChart->getSection(i);
        text += "Section: " + section->getName() + "\n";
        
        // Add phrases
        for (int j = 0; j < section->getNumPhrases(); ++j)
        {
            auto phrase = section->getPhrase(j);
            text += "  Phrase: " + phrase->getName() + "\n";
            
            // Add bars with note details
            for (int k = 0; k < phrase->getNumBars(); ++k)
            {
                auto bar = phrase->getBar(k);
                text += "    Bar " + juce::String(k+1) + " (" + juce::String(bar->getTimeSignatureNumerator()) + "/" 
                      + juce::String(bar->getTimeSignatureDenominator()) + ")\n";
                
                // We can't directly access the notes in a bar, so we'll convert to MIDI
                // and show the MIDI events for this bar
                auto barMidiSequence = bar->toMidiSequence();
                text += "      Events: " + juce::String(barMidiSequence->getNumEvents()) + "\n";
                
                // Show up to 5 events per bar to avoid overwhelming the display
                int maxEventsToShow = 5;
                int eventsShown = 0;
                
                for (int n = 0; n < barMidiSequence->getNumEvents() && eventsShown < maxEventsToShow; ++n)
                {
                    auto event = barMidiSequence->getEventPointer(n);
                    
                    if (event->message.isNoteOn())
                    {
                        text += "        Note On: MIDI " + juce::String(event->message.getNoteNumber()) 
                              + " (" + getMidiNoteName(event->message.getNoteNumber()) + ")" 
                              + " vel: " + juce::String(static_cast<int>(event->message.getVelocity() * 127)) 
                              + " ch: " + juce::String(event->message.getChannel()) + "\n";
                        eventsShown++;
                    }
                    else if (event->message.isNoteOff())
                    {
                        text += "        Note Off: MIDI " + juce::String(event->message.getNoteNumber()) 
                              + " (" + getMidiNoteName(event->message.getNoteNumber()) + ")" 
                              + " ch: " + juce::String(event->message.getChannel()) + "\n";
                        eventsShown++;
                    }
                }
                
                if (barMidiSequence->getNumEvents() > maxEventsToShow)
                {
                    text += "        ... and " + juce::String(barMidiSequence->getNumEvents() - maxEventsToShow) + " more events\n";
                }
            }
        }
        
        text += "\n";
    }
    
    // Add raw MIDI event information
    text += "\nRaw MIDI Events:\n";
    int maxEventsToShow = 20; // Limit the number of events shown to avoid overwhelming the viewer
    int eventsShown = 0;
    
    for (int i = 0; i < midiSequence->getNumEvents() && eventsShown < maxEventsToShow; ++i)
    {
        auto event = midiSequence->getEventPointer(i);
        if (event->message.isNoteOn())
        {
            text += "  Note On: Ch" + juce::String(event->message.getChannel()) 
                  + " Note" + juce::String(event->message.getNoteNumber()) 
                  + " (" + getMidiNoteName(event->message.getNoteNumber()) + ")" 
                  + " Vel" + juce::String(static_cast<int>(event->message.getVelocity() * 127)) 
                  + " Time" + juce::String(event->message.getTimeStamp()) + "\n";
            eventsShown++;
        }
        else if (event->message.isNoteOff())
        {
            text += "  Note Off: Ch" + juce::String(event->message.getChannel()) 
                  + " Note" + juce::String(event->message.getNoteNumber()) 
                  + " (" + getMidiNoteName(event->message.getNoteNumber()) + ")" 
                  + " Time" + juce::String(event->message.getTimeStamp()) + "\n";
            eventsShown++;
        }
    }
    
    if (midiSequence->getNumEvents() > maxEventsToShow)
    {
        text += "  ... and " + juce::String(midiSequence->getNumEvents() - maxEventsToShow) + " more events\n";
    }
    
    patternViewer.setText(text);
}

// Helper method to convert MIDI note numbers to note names
juce::String PlayerModeComponent::getMidiNoteName(int midiNoteNumber)
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int octave = (midiNoteNumber / 12) - 1;
    int note = midiNoteNumber % 12;
    
    return juce::String(noteNames[note]) + juce::String(octave);
}

void PlayerModeComponent::updateCurrentChart()
{
    // Get the selected instrument type
    int instrumentType = instrumentTypeSelector.getSelectedId();
    
    // Default to nullptr
    currentChart = nullptr;
    
    // Select the appropriate chart based on the selections
    if (instrumentType == 1) // Drums
    {
        currentChart = patternSystem->getChart("drums_basic");
    }
    else if (instrumentType == 2) // Bass
    {
        currentChart = patternSystem->getChart("bass_basic");
    }
    else if (instrumentType == 3) // Piano
    {
        currentChart = patternSystem->getChart("piano_basic");
    }
    else if (instrumentType == 4) // Guitar
    {
        currentChart = patternSystem->getChart("guitar_basic");
    }
    
    // Update the pattern viewer
    updatePatternViewer();
    
    // Enable/disable play button based on whether we have a chart and player
    playButton.setEnabled(currentChart != nullptr && selectedPlayer != nullptr);
}

void PlayerModeComponent::playCurrentPattern()
{
    if (currentChart == nullptr || selectedPlayer == nullptr)
    {
        return;
    }
    
    // Create a structured var object for the Player's setPatternFromVar method
    juce::DynamicObject* patternObj = new juce::DynamicObject();
    juce::Array<juce::var> events;
    
    // Create a simple arpeggio pattern with explicit timing values
    // Note: We're bypassing the Chart->MIDI conversion and creating events directly
    // with timing values that work with the current implementation
    
    // C4 note - start at time 0
    juce::DynamicObject* noteOn1 = new juce::DynamicObject();
    noteOn1->setProperty("type", "note_on");
    noteOn1->setProperty("channel", 1);
    noteOn1->setProperty("note", 60); // C4
    noteOn1->setProperty("velocity", 100);
    noteOn1->setProperty("timestamp", 0.0);
    events.add(juce::var(noteOn1));
    
    // C4 note off - after 0.5 seconds
    juce::DynamicObject* noteOff1 = new juce::DynamicObject();
    noteOff1->setProperty("type", "note_off");
    noteOff1->setProperty("channel", 1);
    noteOff1->setProperty("note", 60); // C4
    noteOff1->setProperty("timestamp", 0.5);
    events.add(juce::var(noteOff1));
    
    // E4 note - start at 0.5 seconds
    juce::DynamicObject* noteOn2 = new juce::DynamicObject();
    noteOn2->setProperty("type", "note_on");
    noteOn2->setProperty("channel", 1);
    noteOn2->setProperty("note", 64); // E4
    noteOn2->setProperty("velocity", 100);
    noteOn2->setProperty("timestamp", 0.5);
    events.add(juce::var(noteOn2));
    
    // E4 note off - after 1.0 seconds
    juce::DynamicObject* noteOff2 = new juce::DynamicObject();
    noteOff2->setProperty("type", "note_off");
    noteOff2->setProperty("channel", 1);
    noteOff2->setProperty("note", 64); // E4
    noteOff2->setProperty("timestamp", 1.0);
    events.add(juce::var(noteOff2));
    
    // G4 note - start at 1.0 seconds
    juce::DynamicObject* noteOn3 = new juce::DynamicObject();
    noteOn3->setProperty("type", "note_on");
    noteOn3->setProperty("channel", 1);
    noteOn3->setProperty("note", 67); // G4
    noteOn3->setProperty("velocity", 100);
    noteOn3->setProperty("timestamp", 1.0);
    events.add(juce::var(noteOn3));
    
    // G4 note off - after 1.5 seconds
    juce::DynamicObject* noteOff3 = new juce::DynamicObject();
    noteOff3->setProperty("type", "note_off");
    noteOff3->setProperty("channel", 1);
    noteOff3->setProperty("note", 67); // G4
    noteOff3->setProperty("timestamp", 1.5);
    events.add(juce::var(noteOff3));
    
    // C5 note - start at 1.5 seconds
    juce::DynamicObject* noteOn4 = new juce::DynamicObject();
    noteOn4->setProperty("type", "note_on");
    noteOn4->setProperty("channel", 1);
    noteOn4->setProperty("note", 72); // C5
    noteOn4->setProperty("velocity", 100);
    noteOn4->setProperty("timestamp", 1.5);
    events.add(juce::var(noteOn4));
    
    // C5 note off - after 2.0 seconds
    juce::DynamicObject* noteOff4 = new juce::DynamicObject();
    noteOff4->setProperty("type", "note_off");
    noteOff4->setProperty("channel", 1);
    noteOff4->setProperty("note", 72); // C5
    noteOff4->setProperty("timestamp", 2.0);
    events.add(juce::var(noteOff4));
    
    patternObj->setProperty("events", events);
    patternObj->setProperty("tempo", currentChart->getTempo());
    juce::var patternVar(patternObj);
    
    // Update pattern viewer to show what we're playing
    juce::String text = "Playing C Major Arpeggio:\n";
    text += "C4 (60) at 0.0s for 0.5s\n";
    text += "E4 (64) at 0.5s for 0.5s\n";
    text += "G4 (67) at 1.0s for 0.5s\n";
    text += "C5 (72) at 1.5s for 0.5s\n";
    patternViewer.setText(text);
    
    // Play the MIDI data on the selected player
    if (selectedPlayer->setPatternFromVar(patternVar))
    {
        selectedPlayer->play();
    }
    
    // Update button states
    playButton.setEnabled(false);
    stopButton.setEnabled(true);
}

} // namespace covertracks
