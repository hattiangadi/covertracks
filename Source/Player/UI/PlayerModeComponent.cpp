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
    instrumentTypeSelector.addItem("Other", 5);
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
    
    playAllButton.setButtonText("Play All");
    playAllButton.addListener(this);
    playAllButton.setColour(TextButton::buttonColourId, Colours::green.darker());
    playAllButton.setEnabled(true);
    addAndMakeVisible(playAllButton);
    
    patternModeToggle.setButtonText("Use Pattern-Based Playback");
    patternModeToggle.addListener(this);
    patternModeToggle.setToggleState(usePatternBasedPlayback, dontSendNotification);
    addAndMakeVisible(patternModeToggle);
    
    // Register as a listener for player manager changes
    playerManager.addChangeListener(this);
    
    // Initialize the pattern system
    patternSystem = std::make_unique<MusicPatternSystem>();
    
    // Initialize the pattern parser
    patternParser = std::make_unique<PatternParser>(*patternSystem);
    
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
    
    // Position the buttons
    row = bounds.removeFromTop(30);
    auto buttonArea = row.withWidth(row.getWidth() / 2);
    playButton.setBounds(buttonArea.removeFromLeft(100));
    stopButton.setBounds(buttonArea.removeFromLeft(100));
    playAllButton.setBounds(buttonArea.removeFromLeft(100));
    
    // Position the pattern mode toggle
    row = bounds.removeFromTop(30);
    patternModeToggle.setBounds(row);
}

void PlayerModeComponent::buttonClicked(juce::Button* button)
{
    if (button == &playButton)
    {
        // Play the current pattern
        if (usePatternBasedPlayback)
        {
            playPatternBasedPattern();
        }
        else
        {
            playCurrentPattern();
        }
    }
    else if (button == &stopButton)
    {
        // Stop the current player
        if (selectedPlayer != nullptr)
        {
            selectedPlayer->stop();
            
            // Update button states
            playButton.setEnabled(true);
            stopButton.setEnabled(false);
        }
    }
    else if (button == &playAllButton)
    {
        // Play all patterns for all instruments
        if (selectedPlayer != nullptr && currentChart != nullptr)
        {
            // Get the selected pattern style
            juce::String style = patternSelector.getText();
            if (style.isEmpty())
            {
                style = "Basic"; // Default to basic if none selected
            }
            
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
    else if (button == &patternModeToggle)
    {
        // Toggle between MIDI-based and pattern-based playback
        usePatternBasedPlayback = patternModeToggle.getToggleState();
        
        // Update the pattern viewer to show the current mode
        updatePatternViewer();
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
    // Create patterns for drums with different styles
    auto drumsBasic = createDrumPattern("basic");
    auto drumsComplex = createDrumPattern("complex");
    auto drumsFunky = createDrumPattern("funky");
    auto drumsJazzy = createDrumPattern("jazzy");
    
    // Register the drum patterns
    patternSystem->addChart("drums_basic", drumsBasic);
    patternSystem->addChart("drums_complex", drumsComplex);
    patternSystem->addChart("drums_funky", drumsFunky);
    patternSystem->addChart("drums_jazzy", drumsJazzy);
    
    // Create patterns for bass with different styles
    auto bassBasic = createBassPattern("basic");
    auto bassComplex = createBassPattern("complex");
    auto bassFunky = createBassPattern("funky");
    auto bassJazzy = createBassPattern("jazzy");
    
    // Register the bass patterns
    patternSystem->addChart("bass_basic", bassBasic);
    patternSystem->addChart("bass_complex", bassComplex);
    patternSystem->addChart("bass_funky", bassFunky);
    patternSystem->addChart("bass_jazzy", bassJazzy);
    
    // Create patterns for piano with different styles
    auto pianoBasic = createPianoPattern("basic");
    auto pianoComplex = createPianoPattern("complex");
    auto pianoFunky = createPianoPattern("funky");
    auto pianoJazzy = createPianoPattern("jazzy");
    
    // Register the piano patterns
    patternSystem->addChart("piano_basic", pianoBasic);
    patternSystem->addChart("piano_complex", pianoComplex);
    patternSystem->addChart("piano_funky", pianoFunky);
    patternSystem->addChart("piano_jazzy", pianoJazzy);
    
    // Create patterns for guitar with different styles
    auto guitarBasic = createGuitarPattern("basic");
    auto guitarComplex = createGuitarPattern("complex");
    auto guitarFunky = createGuitarPattern("funky");
    auto guitarJazzy = createGuitarPattern("jazzy");
    
    // Register the guitar patterns
    patternSystem->addChart("guitar_basic", guitarBasic);
    patternSystem->addChart("guitar_complex", guitarComplex);
    patternSystem->addChart("guitar_funky", guitarFunky);
    patternSystem->addChart("guitar_jazzy", guitarJazzy);
    
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

std::shared_ptr<Chart> PlayerModeComponent::createDrumPattern(const juce::String& style)
{
    // Create a drum pattern based on the style
    juce::String patternName = "Drum Pattern - " + style;
    auto chart = std::make_shared<Chart>(patternName, 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Main Beat");
    
    // Create a bar with a drum pattern based on the style
    auto bar = std::make_shared<Bar>(4, 4);
    
    // Note: Channel 9 in code = Channel 10 in MIDI (standard drum channel)
    // Common drum notes:
    // 36 = Kick drum
    // 38 = Snare drum
    // 42 = Closed hi-hat
    // 46 = Open hi-hat
    // 49 = Crash cymbal
    // 51 = Ride cymbal
    // 43 = Floor tom
    
    if (style == "basic")
    {
        // Basic rock beat
        // Kick drum on beats 1 and 3
        bar->addNote(0, 9, 36, 0.7f, 0.5f);  // Beat 1
        bar->addNote(2, 9, 36, 0.7f, 0.5f);  // Beat 3
        
        // Snare on beats 2 and 4
        bar->addNote(1, 9, 38, 0.7f, 0.5f);  // Beat 2
        bar->addNote(3, 9, 38, 0.7f, 0.5f);  // Beat 4
        
        // Hi-hat on every eighth note
        for (int i = 0; i < 8; ++i)
        {
            bar->addNote(static_cast<float>(i * 0.5f), 9, 42, 0.6f, 0.5f);
        }
    }
    else if (style == "complex")
    {
        // More complex beat with sixteenth notes
        // Kick pattern
        bar->addNote(0.0, 9, 36, 0.7f, 0.5f);  // Beat 1
        bar->addNote(1.5, 9, 36, 0.7f, 0.5f);  // Beat 2.5
        bar->addNote(2.0, 9, 36, 0.7f, 0.5f);  // Beat 3
        bar->addNote(3.5, 9, 36, 0.7f, 0.5f);  // Beat 4.5
        
        // Snare pattern
        bar->addNote(1.0, 9, 38, 0.7f, 0.5f);  // Beat 2
        bar->addNote(3.0, 9, 38, 0.7f, 0.5f);  // Beat 4
        
        // Hi-hat pattern with sixteenth notes
        for (int i = 0; i < 16; ++i)
        {
            float time = static_cast<float>(i * 0.25f);
            // Alternate between closed and open hi-hat
            int hiHatNote = (i % 4 == 2) ? 46 : 42;  // Open hi-hat on "e" of each beat
            bar->addNote(time, 9, hiHatNote, 0.6f, 0.25f);
        }
    }
    else if (style == "funky")
    {
        // Funky syncopated beat
        // Kick pattern
        bar->addNote(0.0, 9, 36, 0.7f, 0.5f);   // Beat 1
        bar->addNote(1.75, 9, 36, 0.7f, 0.5f);  // Beat 2.75
        bar->addNote(2.5, 9, 36, 0.7f, 0.5f);   // Beat 3.5
        
        // Snare pattern
        bar->addNote(1.0, 9, 38, 0.7f, 0.5f);   // Beat 2
        bar->addNote(2.75, 9, 38, 0.6f, 0.25f); // Beat 3.75 (ghost note)
        bar->addNote(3.0, 9, 38, 0.7f, 0.5f);   // Beat 4
        bar->addNote(3.5, 9, 38, 0.6f, 0.25f);  // Beat 4.5 (ghost note)
        
        // Hi-hat pattern
        for (int i = 0; i < 8; ++i)
        {
            float time = static_cast<float>(i * 0.5f);
            // Open hi-hat on offbeats
            int hiHatNote = (i % 2 == 1) ? 46 : 42;
            bar->addNote(time, 9, hiHatNote, 0.6f, 0.5f);
        }
    }
    else if (style == "jazzy")
    {
        // Jazz ride pattern with swing feel
        // Kick pattern (sparse)
        bar->addNote(0.0, 9, 36, 0.7f, 0.5f);  // Beat 1
        bar->addNote(2.66, 9, 36, 0.7f, 0.5f); // Beat 3 with swing
        
        // Snare pattern (light)
        bar->addNote(1.33, 9, 38, 0.6f, 0.5f); // Beat 2 with swing
        bar->addNote(3.33, 9, 38, 0.6f, 0.5f); // Beat 4 with swing
        
        // Ride cymbal pattern with swing feel
        bar->addNote(0.0, 9, 51, 0.7f, 0.5f);  // Beat 1
        bar->addNote(0.66, 9, 51, 0.6f, 0.5f); // Beat 1.5 with swing
        bar->addNote(1.33, 9, 51, 0.7f, 0.5f); // Beat 2 with swing
        bar->addNote(2.0, 9, 51, 0.6f, 0.5f);  // Beat 2.5 with swing
        bar->addNote(2.66, 9, 51, 0.7f, 0.5f); // Beat 3 with swing
        bar->addNote(3.33, 9, 51, 0.7f, 0.5f); // Beat 4 with swing
        
        // Hi-hat with foot on beats 2 and 4
        bar->addNote(1.33, 9, 44, 0.5f, 0.5f); // Beat 2 with swing
        bar->addNote(3.33, 9, 44, 0.5f, 0.5f); // Beat 4 with swing
    }
    else
    {
        // Default to basic if style is not recognized
        // Kick drum on beats 1 and 3
        bar->addNote(0, 9, 36, 0.7f, 0.5f);  // Beat 1
        bar->addNote(2, 9, 36, 0.7f, 0.5f);  // Beat 3
        
        // Snare on beats 2 and 4
        bar->addNote(1, 9, 38, 0.7f, 0.5f);  // Beat 2
        bar->addNote(3, 9, 38, 0.7f, 0.5f);  // Beat 4
        
        // Hi-hat on every eighth note
        for (int i = 0; i < 8; ++i)
        {
            bar->addNote(static_cast<float>(i * 0.5f), 9, 42, 0.6f, 0.5f);
        }
    }
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    return chart;
}

std::shared_ptr<Chart> PlayerModeComponent::createBassPattern(const juce::String& style)
{
    // Create a bass pattern based on the style
    juce::String patternName = "Bass Pattern - " + style;
    auto chart = std::make_shared<Chart>(patternName, 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Bass Line");
    
    // Create a bar with a bass line based on the style
    auto bar = std::make_shared<Bar>(4, 4);
    
    if (style == "basic")
    {
        // Basic walking bass line in C
        bar->addNote(0, 0, 36, 0.7f, 1.0f);    // C2 (1 beat duration)
        bar->addNote(1, 0, 38, 0.7f, 1.0f);    // D2 (1 beat duration)
        bar->addNote(2, 0, 40, 0.7f, 1.0f);    // E2 (1 beat duration)
        bar->addNote(3, 0, 43, 0.7f, 1.0f);    // G2 (1 beat duration)
    }
    else if (style == "complex")
    {
        // More complex pattern with eighth notes
        bar->addNote(0.0, 0, 36, 0.7f, 0.5f);    // C2 (half beat duration)
        bar->addNote(0.5, 0, 43, 0.7f, 0.5f);    // G2 (half beat duration)
        bar->addNote(1.0, 0, 36, 0.7f, 0.5f);    // C2 (half beat duration)
        bar->addNote(1.5, 0, 43, 0.7f, 0.5f);    // G2 (half beat duration)
        bar->addNote(2.0, 0, 38, 0.7f, 0.5f);    // D2 (half beat duration)
        bar->addNote(2.5, 0, 45, 0.7f, 0.5f);    // A2 (half beat duration)
        bar->addNote(3.0, 0, 43, 0.7f, 0.5f);    // G2 (half beat duration)
        bar->addNote(3.5, 0, 41, 0.7f, 0.5f);    // F2 (half beat duration)
    }
    else if (style == "funky")
    {
        // Funky slap bass pattern
        bar->addNote(0.0, 0, 36, 0.8f, 0.25f);   // C2 (1/4 beat duration) - slap
        bar->addNote(0.5, 0, 36, 0.6f, 0.25f);   // C2 (1/4 beat duration) - ghost note
        bar->addNote(1.0, 0, 43, 0.8f, 0.25f);   // G2 (1/4 beat duration) - pop
        bar->addNote(1.5, 0, 36, 0.6f, 0.25f);   // C2 (1/4 beat duration) - ghost note
        bar->addNote(2.0, 0, 36, 0.8f, 0.25f);   // C2 (1/4 beat duration) - slap
        bar->addNote(2.5, 0, 38, 0.6f, 0.25f);   // D2 (1/4 beat duration) - ghost note
        bar->addNote(3.0, 0, 43, 0.8f, 0.25f);   // G2 (1/4 beat duration) - pop
        bar->addNote(3.5, 0, 41, 0.8f, 0.5f);    // F2 (1/2 beat duration) - slide
    }
    else if (style == "jazzy")
    {
        // Jazzy walking bass line
        bar->addNote(0.0, 0, 36, 0.7f, 0.9f);    // C2 (slightly less than 1 beat)
        bar->addNote(1.0, 0, 40, 0.7f, 0.9f);    // E2 (slightly less than 1 beat)
        bar->addNote(2.0, 0, 43, 0.7f, 0.9f);    // G2 (slightly less than 1 beat)
        bar->addNote(3.0, 0, 41, 0.7f, 0.9f);    // F2 (slightly less than 1 beat)
    }
    else
    {
        // Default to basic if style is not recognized
        bar->addNote(0, 0, 36, 0.7f, 1.0f);    // C2 (1 beat duration)
        bar->addNote(1, 0, 38, 0.7f, 1.0f);    // D2 (1 beat duration)
        bar->addNote(2, 0, 40, 0.7f, 1.0f);    // E2 (1 beat duration)
        bar->addNote(3, 0, 43, 0.7f, 1.0f);    // G2 (1 beat duration)
    }
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    return chart;
}

std::shared_ptr<Chart> PlayerModeComponent::createPianoPattern(const juce::String& style)
{
    // Create a piano pattern based on the style
    juce::String patternName = "Piano Pattern - " + style;
    auto chart = std::make_shared<Chart>(patternName, 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Piano Melody");
    
    // Create a bar with a piano melody based on the style
    auto bar = std::make_shared<Bar>(4, 4);
    
    if (style == "basic")
    {
        // Basic C major chord arpeggio with clear pattern
        // C major triad (C-E-G) with octave
        bar->addNote(0.0, 0, 60, 0.7f, 0.9f);    // C4 (nearly 1 beat duration)
        bar->addNote(1.0, 0, 64, 0.7f, 0.9f);    // E4 (nearly 1 beat duration)
        bar->addNote(2.0, 0, 67, 0.7f, 0.9f);    // G4 (nearly 1 beat duration)
        bar->addNote(3.0, 0, 72, 0.7f, 0.9f);    // C5 (nearly 1 beat duration)
        
        // Add chord on beat 1 for emphasis
        bar->addNote(0.0, 0, 48, 0.5f, 0.9f);    // C3 (lower octave)
        bar->addNote(0.0, 0, 52, 0.5f, 0.9f);    // E3 (lower octave)
        bar->addNote(0.0, 0, 55, 0.5f, 0.9f);    // G3 (lower octave)
    }
    else if (style == "complex")
    {
        // Complex pattern with sixteenth notes and chord variations
        // First beat: C major chord broken into 16th notes
        bar->addNote(0.00, 0, 60, 0.6f, 0.24f);  // C4 (16th note)
        bar->addNote(0.25, 0, 64, 0.6f, 0.24f);  // E4 (16th note)
        bar->addNote(0.50, 0, 67, 0.6f, 0.24f);  // G4 (16th note)
        bar->addNote(0.75, 0, 72, 0.6f, 0.24f);  // C5 (16th note)
        
        // Second beat: F major chord (F-A-C) broken pattern
        bar->addNote(1.00, 0, 65, 0.6f, 0.24f);  // F4 (16th note)
        bar->addNote(1.25, 0, 69, 0.6f, 0.24f);  // A4 (16th note)
        bar->addNote(1.50, 0, 72, 0.6f, 0.24f);  // C5 (16th note)
        bar->addNote(1.75, 0, 69, 0.6f, 0.24f);  // A4 (16th note)
        
        // Third beat: G major chord (G-B-D) broken pattern
        bar->addNote(2.00, 0, 67, 0.6f, 0.24f);  // G4 (16th note)
        bar->addNote(2.25, 0, 71, 0.6f, 0.24f);  // B4 (16th note)
        bar->addNote(2.50, 0, 74, 0.6f, 0.24f);  // D5 (16th note)
        bar->addNote(2.75, 0, 71, 0.6f, 0.24f);  // B4 (16th note)
        
        // Fourth beat: C major chord descending
        bar->addNote(3.00, 0, 72, 0.6f, 0.24f);  // C5 (16th note)
        bar->addNote(3.25, 0, 67, 0.6f, 0.24f);  // G4 (16th note)
        bar->addNote(3.50, 0, 64, 0.6f, 0.24f);  // E4 (16th note)
        bar->addNote(3.75, 0, 60, 0.6f, 0.24f);  // C4 (16th note)
        
        // Add bass notes for harmonic support
        bar->addNote(0.0, 0, 48, 0.5f, 0.9f);    // C3 (bass note)
        bar->addNote(1.0, 0, 53, 0.5f, 0.9f);    // F3 (bass note)
        bar->addNote(2.0, 0, 55, 0.5f, 0.9f);    // G3 (bass note)
        bar->addNote(3.0, 0, 48, 0.5f, 0.9f);    // C3 (bass note)
    }
    else if (style == "funky")
    {
        // Funky pattern with syncopation and rhythmic interest
        // Syncopated C7 chord (C-E-G-Bb)
        bar->addNote(0.0, 0, 60, 0.7f, 0.4f);    // C4 (syncopated rhythm)
        bar->addNote(0.5, 0, 67, 0.7f, 0.4f);    // G4
        bar->addNote(1.0, 0, 70, 0.8f, 0.3f);    // Bb4 (7th, accented)
        bar->addNote(1.5, 0, 64, 0.6f, 0.4f);    // E4
        
        // Funky rhythmic pattern continues
        bar->addNote(2.0, 0, 67, 0.7f, 0.3f);    // G4
        bar->addNote(2.5, 0, 65, 0.7f, 0.3f);    // F4 (passing tone)
        bar->addNote(2.75, 0, 64, 0.7f, 0.3f);   // E4 (syncopated 16th)
        bar->addNote(3.0, 0, 62, 0.7f, 0.3f);    // D4 (passing tone)
        bar->addNote(3.25, 0, 60, 0.8f, 0.3f);   // C4 (syncopated 16th)
        bar->addNote(3.5, 0, 62, 0.6f, 0.3f);    // D4 (passing tone)
        bar->addNote(3.75, 0, 64, 0.7f, 0.3f);   // E4 (leading back to C)
        
        // Add percussive chord stabs
        bar->addNote(0.0, 0, 48, 0.6f, 0.2f);    // C3 (short stab)
        bar->addNote(0.0, 0, 52, 0.6f, 0.2f);    // E3 (short stab)
        bar->addNote(1.0, 0, 48, 0.6f, 0.2f);    // C3 (short stab)
        bar->addNote(1.0, 0, 55, 0.6f, 0.2f);    // G3 (short stab)
        bar->addNote(2.0, 0, 48, 0.6f, 0.2f);    // C3 (short stab)
        bar->addNote(2.0, 0, 58, 0.6f, 0.2f);    // Bb3 (short stab)
    }
    else if (style == "jazzy")
    {
        // Jazzy pattern with extended chords and chromatic movement
        // Cmaj9 chord (C-E-G-B-D) with jazz voicing
        bar->addNote(0.0, 0, 52, 0.6f, 0.5f);    // E3 (half beat duration)
        bar->addNote(0.0, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
        bar->addNote(0.0, 0, 67, 0.6f, 0.5f);    // G4 (half beat duration)
        bar->addNote(0.0, 0, 71, 0.6f, 0.5f);    // B4 (half beat duration - 9th)
        
        // Chromatic walk down
        bar->addNote(0.5, 0, 71, 0.5f, 0.25f);    // B4 (short duration)
        bar->addNote(0.75, 0, 70, 0.5f, 0.25f);   // Bb4 (chromatic passing tone)
        
        // A7 chord (A-C#-E-G)
        bar->addNote(1.0, 0, 57, 0.6f, 0.5f);    // A3
        bar->addNote(1.0, 0, 61, 0.6f, 0.5f);    // C#4
        bar->addNote(1.0, 0, 64, 0.6f, 0.5f);    // E4
        bar->addNote(1.0, 0, 67, 0.6f, 0.5f);    // G4 (7th)
        
        // D7 chord (D-F#-A-C)
        bar->addNote(2.0, 0, 50, 0.6f, 0.5f);    // D3
        bar->addNote(2.0, 0, 54, 0.6f, 0.5f);    // F#3
        bar->addNote(2.0, 0, 57, 0.6f, 0.5f);    // A3
        bar->addNote(2.0, 0, 60, 0.6f, 0.5f);    // C4 (7th)
        
        // Chromatic approach to G7
        bar->addNote(2.5, 0, 66, 0.5f, 0.25f);    // F#4 (approach note)
        bar->addNote(2.75, 0, 65, 0.5f, 0.25f);   // F4 (chromatic approach)
        
        // G7 chord (G-B-D-F)
        bar->addNote(3.0, 0, 55, 0.6f, 0.5f);    // G3
        bar->addNote(3.0, 0, 59, 0.6f, 0.5f);    // B3
        bar->addNote(3.0, 0, 62, 0.6f, 0.5f);    // D4
        bar->addNote(3.0, 0, 65, 0.6f, 0.5f);    // F4 (7th)
        
        // Chromatic approach back to C
        bar->addNote(3.5, 0, 59, 0.5f, 0.25f);    // B3 (approach note)
        bar->addNote(3.75, 0, 60, 0.5f, 0.25f);   // C4 (target note)
    }
    else
    {
        // Default to basic if style is not recognized
        bar->addNote(0, 0, 60, 0.6f, 1.0f);    // C4 (1 beat duration)
        bar->addNote(1, 0, 64, 0.6f, 1.0f);    // E4 (1 beat duration)
        bar->addNote(2, 0, 67, 0.6f, 1.0f);    // G4 (1 beat duration)
        bar->addNote(3, 0, 72, 0.6f, 1.0f);    // C5 (1 beat duration)
    }
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    return chart;
}

std::shared_ptr<Chart> PlayerModeComponent::createGuitarPattern(const juce::String& style)
{
    // Create a guitar pattern based on the style
    juce::String patternName = "Guitar Pattern - " + style;
    auto chart = std::make_shared<Chart>(patternName, 120.0);
    
    // Create a section
    auto section = std::make_shared<Section>("Verse");
    
    // Create a phrase
    auto phrase = std::make_shared<Phrase>("Guitar Riff");
    
    // Create a bar with a guitar pattern based on the style
    auto bar = std::make_shared<Bar>(4, 4);
    
    if (style == "basic")
    {
        // Basic C major arpeggio pattern
        bar->addNote(0.0, 0, 48, 0.6f, 0.5f);    // C3 (half beat duration)
        bar->addNote(0.5, 0, 52, 0.6f, 0.5f);    // E3 (half beat duration)
        bar->addNote(1.0, 0, 55, 0.6f, 0.5f);    // G3 (half beat duration)
        bar->addNote(1.5, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
        bar->addNote(2.0, 0, 48, 0.6f, 0.5f);    // C3 (half beat duration)
        bar->addNote(2.5, 0, 52, 0.6f, 0.5f);    // E3 (half beat duration)
        bar->addNote(3.0, 0, 55, 0.6f, 0.5f);    // G3 (half beat duration)
        bar->addNote(3.5, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
    }
    else if (style == "complex")
    {
        // More complex pattern with chord strumming
        // C major chord
        bar->addNote(0.0, 0, 48, 0.6f, 0.25f);    // C3
        bar->addNote(0.0, 0, 52, 0.6f, 0.25f);    // E3
        bar->addNote(0.0, 0, 55, 0.6f, 0.25f);    // G3
        
        // G major chord
        bar->addNote(1.0, 0, 43, 0.6f, 0.25f);    // G2
        bar->addNote(1.0, 0, 47, 0.6f, 0.25f);    // B2
        bar->addNote(1.0, 0, 50, 0.6f, 0.25f);    // D3
        
        // A minor chord
        bar->addNote(2.0, 0, 45, 0.6f, 0.25f);    // A2
        bar->addNote(2.0, 0, 48, 0.6f, 0.25f);    // C3
        bar->addNote(2.0, 0, 52, 0.6f, 0.25f);    // E3
        
        // F major chord
        bar->addNote(3.0, 0, 41, 0.6f, 0.25f);    // F2
        bar->addNote(3.0, 0, 45, 0.6f, 0.25f);    // A2
        bar->addNote(3.0, 0, 48, 0.6f, 0.25f);    // C3
        
        // Add some picking between chords
        bar->addNote(0.5, 0, 55, 0.5f, 0.25f);    // G3
        bar->addNote(1.5, 0, 50, 0.5f, 0.25f);    // D3
        bar->addNote(2.5, 0, 52, 0.5f, 0.25f);    // E3
        bar->addNote(3.5, 0, 48, 0.5f, 0.25f);    // C3
    }
    else if (style == "funky")
    {
        // Funky rhythm guitar pattern
        // Muted 16th note strums
        for (int i = 0; i < 16; i++)
        {
            // Skip certain 16ths for syncopation
            if (i == 3 || i == 7 || i == 11 || i == 15) continue;
            
            float time = static_cast<float>(i * 0.25f);
            // Use different chord voicings
            if (i < 4) // C9 chord
            {
                bar->addNote(time, 0, 48, 0.7f, 0.2f);    // C3
                bar->addNote(time, 0, 52, 0.7f, 0.2f);    // E3
                bar->addNote(time, 0, 55, 0.7f, 0.2f);    // G3
                bar->addNote(time, 0, 58, 0.7f, 0.2f);    // Bb3
            }
            else if (i < 8) // F13 chord
            {
                bar->addNote(time, 0, 41, 0.7f, 0.2f);    // F2
                bar->addNote(time, 0, 45, 0.7f, 0.2f);    // A2
                bar->addNote(time, 0, 48, 0.7f, 0.2f);    // C3
                bar->addNote(time, 0, 57, 0.7f, 0.2f);    // A3
            }
            else if (i < 12) // G13 chord
            {
                bar->addNote(time, 0, 43, 0.7f, 0.2f);    // G2
                bar->addNote(time, 0, 47, 0.7f, 0.2f);    // B2
                bar->addNote(time, 0, 50, 0.7f, 0.2f);    // D3
                bar->addNote(time, 0, 59, 0.7f, 0.2f);    // B3
            }
            else // C9 chord again
            {
                bar->addNote(time, 0, 48, 0.7f, 0.2f);    // C3
                bar->addNote(time, 0, 52, 0.7f, 0.2f);    // E3
                bar->addNote(time, 0, 55, 0.7f, 0.2f);    // G3
                bar->addNote(time, 0, 58, 0.7f, 0.2f);    // Bb3
            }
        }
    }
    else if (style == "jazzy")
    {
        // Jazz comping pattern with 7th and 9th chords
        // Cmaj7 chord with shell voicing (3rd and 7th emphasized)
        bar->addNote(0.0, 0, 48, 0.6f, 0.33f);    // C3 (root)
        bar->addNote(0.0, 0, 52, 0.7f, 0.33f);    // E3 (3rd - emphasized)
        bar->addNote(0.0, 0, 55, 0.5f, 0.33f);    // G3 (5th)
        bar->addNote(0.0, 0, 59, 0.7f, 0.33f);    // B3 (maj7 - emphasized)
        
        // Dm7 chord with shell voicing
        bar->addNote(1.0, 0, 50, 0.6f, 0.33f);    // D3 (root)
        bar->addNote(1.0, 0, 53, 0.7f, 0.33f);    // F3 (minor 3rd - emphasized)
        bar->addNote(1.0, 0, 57, 0.5f, 0.33f);    // A3 (5th)
        bar->addNote(1.0, 0, 60, 0.7f, 0.33f);    // C4 (minor 7th - emphasized)
        
        // G7 chord with shell voicing
        bar->addNote(2.0, 0, 43, 0.6f, 0.33f);    // G2 (root)
        bar->addNote(2.0, 0, 47, 0.7f, 0.33f);    // B2 (3rd - emphasized)
        bar->addNote(2.0, 0, 50, 0.5f, 0.33f);    // D3 (5th)
        bar->addNote(2.0, 0, 53, 0.7f, 0.33f);    // F3 (dominant 7th - emphasized)
        
        // Cmaj9 chord with extended voicing
        bar->addNote(3.0, 0, 48, 0.6f, 0.33f);    // C3 (root)
        bar->addNote(3.0, 0, 52, 0.6f, 0.33f);    // E3 (3rd)
        bar->addNote(3.0, 0, 55, 0.5f, 0.33f);    // G3 (5th)
        bar->addNote(3.0, 0, 59, 0.7f, 0.33f);    // B3 (maj7)
        bar->addNote(3.0, 0, 62, 0.6f, 0.33f);    // D4 (9th)
        
        // Add some jazz-style approach notes and chromatic passing tones
        bar->addNote(0.66, 0, 47, 0.5f, 0.25f);   // B2 - approach to C
        bar->addNote(1.66, 0, 49, 0.5f, 0.25f);   // C#3 - approach to D
        bar->addNote(2.66, 0, 42, 0.5f, 0.25f);   // F#2 - approach to G
    }
    else
    {
        // Default to basic if style is not recognized
        bar->addNote(0.0, 0, 48, 0.6f, 0.5f);    // C3 (half beat duration)
        bar->addNote(0.5, 0, 52, 0.6f, 0.5f);    // E3 (half beat duration)
        bar->addNote(1.0, 0, 55, 0.6f, 0.5f);    // G3 (half beat duration)
        bar->addNote(1.5, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
        bar->addNote(2.0, 0, 48, 0.6f, 0.5f);    // C3 (half beat duration)
        bar->addNote(2.5, 0, 52, 0.6f, 0.5f);    // E3 (half beat duration)
        bar->addNote(3.0, 0, 55, 0.6f, 0.5f);    // G3 (half beat duration)
        bar->addNote(3.5, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
    }
    
    // Add the bar to the phrase
    phrase->addBar(bar);
    
    // Add the phrase to the section
    section->addPhrase(phrase);
    
    // Add the section to the chart
    chart->addSection(section);
    
    return chart;
}

void PlayerModeComponent::updatePatternViewer()
{
    // Show the current playback mode
    juce::String text = juce::String("Playback Mode: ") + (usePatternBasedPlayback ? juce::String("Pattern-Based") : juce::String("MIDI-Based")) + juce::String("\n\n");
    
    if (currentChart == nullptr)
    {
        patternViewer.setText(text + juce::String("No pattern selected"));
        return;
    }
    
    // Create a text representation of the chart structure
    text += juce::String("Chart: ") + currentChart->getTitle() + juce::String("\n");
    text += juce::String("Tempo: ") + juce::String(currentChart->getTempo()) + juce::String(" BPM\n\n");
    
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
    // Default to nullptr
    currentChart = nullptr;
    
    // Try to detect instrument type from player if one is selected
    int instrumentType = instrumentTypeSelector.getSelectedId();
    bool autoDetectedInstrument = false;
    
    if (selectedPlayer != nullptr)
    {
        // Get the player's metadata to check for instrument type
        auto& metadata = selectedPlayer->getMetadata();
        juce::String category = metadata.category.toLowerCase();
        
        // Auto-detect instrument type if possible
        if (category.contains("drum"))
        {
            instrumentType = 1; // Drums
            autoDetectedInstrument = true;
        }
        else if (category.contains("bass"))
        {
            instrumentType = 2; // Bass
            autoDetectedInstrument = true;
        }
        else if (category.contains("piano") || category.contains("keyboard") || 
                 category.contains("keys") || category.contains("synth"))
        {
            instrumentType = 3; // Piano/Keys
            autoDetectedInstrument = true;
        }
        else if (category.contains("guitar"))
        {
            instrumentType = 4; // Guitar
            autoDetectedInstrument = true;
        }
        else
        {
            instrumentType = 5; // Other/Unknown
            autoDetectedInstrument = true;
        }
        
        // Update the instrument type selector if we auto-detected
        if (autoDetectedInstrument && instrumentTypeSelector.getSelectedId() != instrumentType)
        {
            instrumentTypeSelector.setSelectedId(instrumentType, juce::dontSendNotification);
        }
    }
    
    // Get the selected pattern type
    int patternType = patternSelector.getSelectedId();
    juce::String patternSuffix;
    
    switch (patternType)
    {
        case 1: patternSuffix = "_basic"; break;
        case 2: patternSuffix = "_complex"; break;
        case 3: patternSuffix = "_funky"; break;
        case 4: patternSuffix = "_jazzy"; break;
        default: patternSuffix = "_basic"; break;
    }
    
    // Select the appropriate chart based on the instrument and pattern type
    juce::String chartName;
    
    switch (instrumentType)
    {
        case 1: chartName = "drums" + patternSuffix; break;
        case 2: chartName = "bass" + patternSuffix; break;
        case 3: chartName = "piano" + patternSuffix; break;
        case 4: chartName = "guitar" + patternSuffix; break;
        case 5: // Other/Unknown - default to piano
        default: chartName = "piano" + patternSuffix; break;
    }
    
    currentChart = patternSystem->getChart(chartName);
    
    // If the chart doesn't exist, fall back to basic pattern
    if (currentChart == nullptr && patternSuffix != "_basic")
    {
        chartName = chartName.replace(patternSuffix, "_basic");
        currentChart = patternSystem->getChart(chartName);
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
    
    // Get the pattern style from the selector
    int patternType = patternSelector.getSelectedId();
    juce::String patternStyle;
    
    switch (patternType)
    {
        case 1: patternStyle = "Basic"; break;
        case 2: patternStyle = "Complex"; break;
        case 3: patternStyle = "Funky"; break;
        case 4: patternStyle = "Jazzy"; break;
        default: patternStyle = "Basic"; break;
    }
    
    std::cout << "Playing " << patternStyle << " pattern" << std::endl;
    
    // Convert the chart to a MIDI sequence
    auto midiSequence = currentChart->toMidiSequence();
    
    // Create the pattern var
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
            noteOn->setProperty("timestamp", event->message.getTimeStamp() / 960.0); // Convert from ticks to seconds
            events.add(juce::var(noteOn));
        }
        else if (event->message.isNoteOff())
        {
            juce::DynamicObject* noteOff = new juce::DynamicObject();
            noteOff->setProperty("type", "note_off");
            noteOff->setProperty("channel", event->message.getChannel());
            noteOff->setProperty("note", event->message.getNoteNumber());
            noteOff->setProperty("timestamp", event->message.getTimeStamp() / 960.0); // Convert from ticks to seconds
            events.add(juce::var(noteOff));
        }
        else if (event->message.isController())
        {
            juce::DynamicObject* controller = new juce::DynamicObject();
            controller->setProperty("type", "controller");
            controller->setProperty("channel", event->message.getChannel());
            controller->setProperty("controller", event->message.getControllerNumber());
            controller->setProperty("value", event->message.getControllerValue());
            controller->setProperty("timestamp", event->message.getTimeStamp() / 960.0); // Convert from ticks to seconds
            events.add(juce::var(controller));
        }
    }
    
    // Add an all-notes-off event at the end to ensure clean playback
    if (!events.isEmpty())
    {
        // Find the maximum timestamp
        double maxTimestamp = 0.0;
        for (int i = 0; i < events.size(); ++i)
        {
            if (auto* obj = events[i].getDynamicObject())
            {
                double timestamp = obj->getProperty("timestamp");
                maxTimestamp = std::max(maxTimestamp, timestamp);
            }
        }
        
        // Add all-notes-off controller message
        juce::DynamicObject* allNotesOff = new juce::DynamicObject();
        allNotesOff->setProperty("type", "controller");
        allNotesOff->setProperty("channel", 1);
        allNotesOff->setProperty("controller", 123); // All notes off
        allNotesOff->setProperty("value", 0);
        allNotesOff->setProperty("timestamp", maxTimestamp + 0.5); // 0.5 seconds after the last note
        events.add(juce::var(allNotesOff));
    }
    
    patternObj->setProperty("events", events);
    patternObj->setProperty("tempo", currentChart->getTempo());
    juce::var patternVar(patternObj);
    
    // Update the pattern viewer to show what we're playing
    juce::String text = juce::String("Playing ") + currentChart->getTitle() + juce::String(":\n");
    text += juce::String("Tempo: ") + juce::String(currentChart->getTempo()) + juce::String(" BPM\n");
    text += juce::String("Total events: ") + juce::String(events.size()) + juce::String("\n\n");
    
    // Show a few events for reference
    int eventsToShow = std::min(10, events.size());
    for (int i = 0; i < eventsToShow; ++i)
    {
        if (auto* obj = events[i].getDynamicObject())
        {
            juce::String type = obj->getProperty("type").toString();
            if (type == "note_on")
            {
                int note = static_cast<int>(obj->getProperty("note"));
                double timestamp = static_cast<double>(obj->getProperty("timestamp"));
                text += juce::String("Note On: ") + getMidiNoteName(note) + juce::String(" (") + juce::String(note) + juce::String(") at ") + 
                        juce::String(timestamp, 2) + juce::String("s\n");
            }
            else if (type == "note_off")
            {
                int note = static_cast<int>(obj->getProperty("note"));
                double timestamp = static_cast<double>(obj->getProperty("timestamp"));
                text += juce::String("Note Off: ") + getMidiNoteName(note) + juce::String(" (") + juce::String(note) + juce::String(") at ") + 
                        juce::String(timestamp, 2) + juce::String("s\n");
            }
        }
    }
    
    if (events.size() > eventsToShow)
    {
        text += juce::String("... and ") + juce::String(events.size() - eventsToShow) + juce::String(" more events\n");
    }
    
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

void PlayerModeComponent::playPatternBasedPattern()
{
    if (selectedPlayer == nullptr)
    {
        return;
    }
    
    // Get the selected instrument type and pattern style
    juce::String instrumentType = instrumentTypeSelector.getText();
    juce::String style = patternSelector.getText();
    
    if (instrumentType.isEmpty() || style.isEmpty())
    {
        return;
    }
    
    // Create a pattern-based chart
    auto chart = createPatternBasedChart(instrumentType, style);
    
    // Store the chart for reference
    currentChart = chart;
    
    // Convert the chart to a MIDI sequence
    auto midiSequence = chart->toMidiSequence();
    
    // Create the pattern var
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
            noteOn->setProperty("velocity", event->message.getVelocity());
            noteOn->setProperty("timestamp", event->message.getTimeStamp() / 960.0); // Convert from ticks to seconds
            events.add(juce::var(noteOn));
        }
        else if (event->message.isNoteOff())
        {
            juce::DynamicObject* noteOff = new juce::DynamicObject();
            noteOff->setProperty("type", "note_off");
            noteOff->setProperty("channel", event->message.getChannel());
            noteOff->setProperty("note", event->message.getNoteNumber());
            noteOff->setProperty("timestamp", event->message.getTimeStamp() / 960.0); // Convert from ticks to seconds
            events.add(juce::var(noteOff));
        }
        else if (event->message.isController())
        {
            juce::DynamicObject* controller = new juce::DynamicObject();
            controller->setProperty("type", "controller");
            controller->setProperty("channel", event->message.getChannel());
            controller->setProperty("controller", event->message.getControllerNumber());
            controller->setProperty("value", event->message.getControllerValue());
            controller->setProperty("timestamp", event->message.getTimeStamp() / 960.0); // Convert from ticks to seconds
            events.add(juce::var(controller));
        }
    }
    
    // Add an all-notes-off event at the end to ensure clean playback
    if (!events.isEmpty())
    {
        // Find the maximum timestamp
        double maxTimestamp = 0.0;
        for (int i = 0; i < events.size(); ++i)
        {
            if (auto* obj = events[i].getDynamicObject())
            {
                double timestamp = obj->getProperty("timestamp");
                maxTimestamp = std::max(maxTimestamp, timestamp);
            }
        }
        
        // Add all-notes-off controller message
        juce::DynamicObject* allNotesOff = new juce::DynamicObject();
        allNotesOff->setProperty("type", "controller");
        allNotesOff->setProperty("channel", 1);
        allNotesOff->setProperty("controller", 123); // All notes off
        allNotesOff->setProperty("value", 0);
        allNotesOff->setProperty("timestamp", maxTimestamp + 0.5); // 0.5 seconds after the last note
        events.add(juce::var(allNotesOff));
    }
    
    patternObj->setProperty("events", events);
    patternObj->setProperty("tempo", chart->getTempo());
    juce::var patternVar(patternObj);
    
    // Update the pattern viewer to show what we're playing
    juce::String text = juce::String("Playing Pattern-Based ") + chart->getTitle() + juce::String(":\n");
    text += juce::String("Tempo: ") + juce::String(chart->getTempo()) + juce::String(" BPM\n");
    text += juce::String("Total events: ") + juce::String(events.size()) + juce::String("\n\n");
    
    // Show a few events for reference
    int eventsToShow = std::min(10, events.size());
    for (int i = 0; i < eventsToShow; ++i)
    {
        if (auto* obj = events[i].getDynamicObject())
        {
            juce::String type = obj->getProperty("type").toString();
            if (type == "note_on")
            {
                int note = static_cast<int>(obj->getProperty("note"));
                double timestamp = static_cast<double>(obj->getProperty("timestamp"));
                text += juce::String("Note On: ") + getMidiNoteName(note) + juce::String(" (") + juce::String(note) + juce::String(") at ") + 
                        juce::String(timestamp, 2) + juce::String("s\n");
            }
            else if (type == "note_off")
            {
                int note = static_cast<int>(obj->getProperty("note"));
                double timestamp = static_cast<double>(obj->getProperty("timestamp"));
                text += juce::String("Note Off: ") + getMidiNoteName(note) + juce::String(" (") + juce::String(note) + juce::String(") at ") + 
                        juce::String(timestamp, 2) + juce::String("s\n");
            }
        }
    }
    
    if (events.size() > eventsToShow)
    {
        text += juce::String("... and ") + juce::String(events.size() - eventsToShow) + juce::String(" more events\n");
    }
    
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

std::shared_ptr<Chart> PlayerModeComponent::createPatternBasedChart(const juce::String& instrumentType, const juce::String& style)
{
    // Define pattern notations for different instruments and styles
    juce::String patternNotation;
    
    if (instrumentType == "Drums")
    {
        if (style == "Basic")
        {
            patternNotation = "4/4: [1: kick] [2: snare] [3: kick] [4: snare+closed_hat]";
        }
        else if (style == "Complex")
        {
            patternNotation = "4/4: [1: kick+closed_hat] [1.5: closed_hat] [2: snare+closed_hat] [2.5: closed_hat] [3: kick+closed_hat] [3.5: closed_hat] [4: snare+closed_hat] [4.5: closed_hat]";
        }
        else if (style == "Funky")
        {
            patternNotation = "4/4: [1: kick+closed_hat] [1.75: kick] [2: snare+closed_hat] [2.5: closed_hat] [3: kick+closed_hat] [3.5: kick] [4: snare+closed_hat] [4.5: closed_hat]";
        }
        else if (style == "Jazzy")
        {
            patternNotation = "4/4: [1: ride] [1.33: ride] [1.67: ride] [2: snare+ride] [2.33: ride] [2.67: ride] [3: ride] [3.33: ride] [3.67: ride] [4: snare+ride] [4.33: ride] [4.67: ride]";
        }
    }
    else if (instrumentType == "Bass")
    {
        if (style == "Basic")
        {
            patternNotation = "4/4: [1: C2] [2: G2] [3: C2] [4: G2]";
        }
        else if (style == "Complex")
        {
            patternNotation = "4/4: [1: C2] [1.5: E2] [2: G2] [2.5: A2] [3: C3] [3.5: A2] [4: G2] [4.5: E2]";
        }
        else if (style == "Funky")
        {
            patternNotation = "4/4: [1: C2] [1.75: C2] [2: E2] [2.5: G2] [3: C3] [3.5: B2] [4: A2] [4.5: G2]";
        }
        else if (style == "Jazzy")
        {
            patternNotation = "4/4: [1: C2] [1.33: E2] [1.67: G2] [2: Bb2] [2.33: A2] [2.67: G2] [3: F2] [3.33: E2] [3.67: D2] [4: C2] [4.33: D2] [4.67: E2]";
        }
    }
    else if (instrumentType == "Piano")
    {
        if (style == "Basic")
        {
            patternNotation = "4/4: [1: C3+E3+G3] [2: C3+F3+A3] [3: C3+E3+G3] [4: G2+B2+D3]";
        }
        else if (style == "Complex")
        {
            patternNotation = "4/4: [1: C3+E3+G3] [1.5: C3+E3+G3] [2: A2+C3+E3] [2.5: A2+C3+E3] [3: F2+A2+C3] [3.5: F2+A2+C3] [4: G2+B2+D3] [4.5: G2+B2+D3]";
        }
        else if (style == "Funky")
        {
            patternNotation = "4/4: [1: C3+E3+G3] [1.75: C3+E3+G3] [2.5: A2+C3+E3] [3: F2+A2+C3] [3.5: F2+A2+C3] [4: G2+B2+D3]";
        }
        else if (style == "Jazzy")
        {
            patternNotation = "4/4: [1: C3+E3+G3+Bb3] [1.5: A2+C3+E3+G3] [2: D3+F3+A3+C4] [2.5: G2+B2+D3+F3] [3: C3+E3+G3+Bb3] [3.5: A2+C3+E3+G3] [4: D3+F3+A3+C4] [4.5: G2+B2+D3+F3]";
        }
    }
    else if (instrumentType == "Guitar")
    {
        if (style == "Basic")
        {
            patternNotation = "4/4: [1: C3+E3+G3] [2: A2+C3+E3] [3: F2+A2+C3] [4: G2+B2+D3]";
        }
        else if (style == "Complex")
        {
            patternNotation = "4/4: [1: C3+E3+G3] [1.25: E3] [1.5: G3] [1.75: C4] [2: A2+C3+E3] [2.25: C3] [2.5: E3] [2.75: A3] [3: F2+A2+C3] [3.25: A2] [3.5: C3] [3.75: F3] [4: G2+B2+D3] [4.25: B2] [4.5: D3] [4.75: G3]";
        }
        else if (style == "Funky")
        {
            patternNotation = "4/4: [1: C3+E3+G3:v80] [1.25: C3+E3+G3:v40] [1.5: C3+E3+G3:v80] [1.75: C3+E3+G3:v40] [2: A2+C3+E3:v80] [2.25: A2+C3+E3:v40] [2.5: A2+C3+E3:v80] [2.75: A2+C3+E3:v40] [3: F2+A2+C3:v80] [3.25: F2+A2+C3:v40] [3.5: F2+A2+C3:v80] [3.75: F2+A2+C3:v40] [4: G2+B2+D3:v80] [4.25: G2+B2+D3:v40] [4.5: G2+B2+D3:v80] [4.75: G2+B2+D3:v40]";
        }
        else if (style == "Jazzy")
        {
            patternNotation = "4/4: [1: C3+E3+G3+Bb3] [1.33: E3+G3+Bb3+D4] [1.67: G3+Bb3+D4+F4] [2: A2+C3+E3+G3] [2.33: C3+E3+G3+B3] [2.67: E3+G3+B3+D4] [3: F2+A2+C3+E3] [3.33: A2+C3+E3+G3] [3.67: C3+E3+G3+Bb3] [4: G2+B2+D3+F3] [4.33: B2+D3+F3+A3] [4.67: D3+F3+A3+C4]";
        }
    }
    
    // Create a chart from the pattern notation
    auto bar = patternSystem->createBarFromNotation(patternNotation);
    
    auto phrase = std::make_shared<Phrase>();
    phrase->setName(style + " " + instrumentType + " Pattern");
    
    // Add multiple bars to create a longer pattern
    const int numBars = 4; // Create a 4-bar pattern
    
    // First bar - use the original pattern
    phrase->addBar(bar);
    
    // Create variations for the remaining bars
    for (int i = 1; i < numBars; ++i)
    {
        // For drums and bass, create variation patterns
        if (instrumentType == "Drums" || instrumentType == "Bass")
        {
            // Create variation patterns based on the bar number
            juce::String variationPattern;
            
            if (instrumentType == "Drums")
            {
                if (i == 1) // Second bar - similar to first but with a fill at the end
                {
                    if (style == "Basic")
                        variationPattern = "4/4: [1: kick] [2: snare] [3: kick] [3.5: kick] [4: snare+closed_hat] [4.5: snare]";
                    else if (style == "Complex")
                        variationPattern = "4/4: [1: kick+closed_hat] [1.5: closed_hat] [2: snare+closed_hat] [2.5: closed_hat] [3: kick+closed_hat] [3.5: kick] [4: snare+closed_hat] [4.25: snare] [4.5: snare] [4.75: snare]";
                    else if (style == "Funky")
                        variationPattern = "4/4: [1: kick+closed_hat] [1.75: kick] [2: snare+closed_hat] [2.5: closed_hat] [3: kick+closed_hat] [3.25: kick] [3.5: kick] [4: snare+closed_hat] [4.5: closed_hat]";
                    else if (style == "Jazzy")
                        variationPattern = "4/4: [1: ride] [1.33: ride] [1.67: ride] [2: snare+ride] [2.33: ride] [2.67: ride] [3: ride] [3.33: ride] [3.67: ride] [4: snare+ride] [4.33: crash] [4.67: snare]";
                }
                else if (i == 2) // Third bar - variation
                {
                    if (style == "Basic")
                        variationPattern = "4/4: [1: kick+closed_hat] [2: snare] [3: kick+closed_hat] [4: snare]";
                    else if (style == "Complex")
                        variationPattern = "4/4: [1: kick+closed_hat] [1.5: closed_hat] [2: snare+closed_hat] [2.5: closed_hat] [3: kick+closed_hat] [3.25: kick] [3.5: closed_hat] [4: snare+closed_hat] [4.5: closed_hat]";
                    else if (style == "Funky")
                        variationPattern = "4/4: [1: kick+closed_hat] [1.5: closed_hat] [2: snare+closed_hat] [2.5: closed_hat] [3: kick+closed_hat] [3.5: kick] [4: snare+closed_hat] [4.5: closed_hat]";
                    else if (style == "Jazzy")
                        variationPattern = "4/4: [1: ride] [1.33: ride] [1.67: ride] [2: snare+ride] [2.33: ride] [2.67: ride] [3: ride+kick] [3.33: ride] [3.67: ride+kick] [4: snare+ride] [4.33: ride] [4.67: ride]";
                }
                else // Fourth bar - fill or transition
                {
                    if (style == "Basic")
                        variationPattern = "4/4: [1: kick] [2: snare] [3: kick] [3.5: snare] [4: snare] [4.25: snare] [4.5: snare] [4.75: snare]";
                    else if (style == "Complex")
                        variationPattern = "4/4: [1: kick+closed_hat] [1.5: closed_hat] [2: snare+closed_hat] [2.5: closed_hat] [3: tom1] [3.25: tom1] [3.5: tom2] [3.75: tom2] [4: snare] [4.25: snare] [4.5: snare] [4.75: crash]";
                    else if (style == "Funky")
                        variationPattern = "4/4: [1: kick+closed_hat] [1.5: kick] [2: snare+closed_hat] [2.5: kick] [3: kick] [3.25: kick] [3.5: snare] [3.75: kick] [4: snare] [4.25: snare] [4.5: snare] [4.75: snare]";
                    else if (style == "Jazzy")
                        variationPattern = "4/4: [1: ride] [1.33: ride] [1.67: ride] [2: snare+ride] [2.33: ride] [2.67: ride] [3: tom1] [3.33: tom1] [3.67: tom2] [4: snare] [4.33: snare] [4.67: crash]";
                }
            }
            else if (instrumentType == "Bass")
            {
                if (i == 1) // Second bar - slight variation
                {
                    if (style == "Basic")
                        variationPattern = "4/4: [1: C2] [2: G2] [3: E2] [4: G2]";
                    else if (style == "Complex")
                        variationPattern = "4/4: [1: C2] [1.5: E2] [2: G2] [2.5: A2] [3: E2] [3.5: A2] [4: G2] [4.5: E2]";
                    else if (style == "Funky")
                        variationPattern = "4/4: [1: C2] [1.75: C2] [2: E2] [2.5: G2] [3: A2] [3.5: G2] [4: E2] [4.5: C2]";
                    else if (style == "Jazzy")
                        variationPattern = "4/4: [1: C2] [1.33: E2] [1.67: G2] [2: Bb2] [2.33: A2] [2.67: G2] [3: E2] [3.33: D2] [3.67: C2] [4: D2] [4.33: E2] [4.67: G2]";
                }
                else if (i == 2) // Third bar - different chord or pattern
                {
                    if (style == "Basic")
                        variationPattern = "4/4: [1: F2] [2: C3] [3: F2] [4: C3]";
                    else if (style == "Complex")
                        variationPattern = "4/4: [1: F2] [1.5: A2] [2: C3] [2.5: D3] [3: F2] [3.5: D3] [4: C3] [4.5: A2]";
                    else if (style == "Funky")
                        variationPattern = "4/4: [1: F2] [1.75: F2] [2: A2] [2.5: C3] [3: F3] [3.5: E3] [4: D3] [4.5: C3]";
                    else if (style == "Jazzy")
                        variationPattern = "4/4: [1: F2] [1.33: A2] [1.67: C3] [2: E3] [2.33: D3] [2.67: C3] [3: Bb2] [3.33: A2] [3.67: G2] [4: F2] [4.33: G2] [4.67: A2]";
                }
                else // Fourth bar - return to root or transition
                {
                    if (style == "Basic")
                        variationPattern = "4/4: [1: G2] [2: D3] [3: G2] [4: C2]";
                    else if (style == "Complex")
                        variationPattern = "4/4: [1: G2] [1.5: B2] [2: D3] [2.5: E3] [3: G2] [3.5: F2] [4: E2] [4.5: D2]";
                    else if (style == "Funky")
                        variationPattern = "4/4: [1: G2] [1.75: G2] [2: B2] [2.5: D3] [3: G3] [3.5: F3] [4: E3] [4.5: D3]";
                    else if (style == "Jazzy")
                        variationPattern = "4/4: [1: G2] [1.33: B2] [1.67: D3] [2: F3] [2.33: E3] [2.67: D3] [3: C3] [3.33: B2] [3.67: A2] [4: G2] [4.33: F2] [4.67: E2]";
                }
            }
            
            if (!variationPattern.isEmpty())
            {
                auto variationBar = patternSystem->createBarFromNotation(variationPattern);
                phrase->addBar(variationBar);
            }
            else
            {
                // Fallback to the original bar if no variation defined
                phrase->addBar(bar);
            }
        }
        else
        {
            // For other instruments, just repeat the original bar for now
            // In a more advanced implementation, we could create variations for piano and guitar too
            phrase->addBar(bar);
        }
    }
    
    auto section = std::make_shared<Section>();
    section->setName(style + " Section");
    section->addPhrase(phrase);
    
    auto chart = std::make_shared<Chart>();
    chart->setTitle(style + " " + instrumentType + " Pattern");
    chart->setTempo(480); // Increase tempo to compensate for timing issues (4x faster)
    chart->addSection(section);
    
    return chart;
}

} // namespace covertracks
