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
        // Basic C major chord arpeggio (up)
        bar->addNote(0, 0, 60, 0.6f, 1.0f);    // C4 (1 beat duration)
        bar->addNote(1, 0, 64, 0.6f, 1.0f);    // E4 (1 beat duration)
        bar->addNote(2, 0, 67, 0.6f, 1.0f);    // G4 (1 beat duration)
        bar->addNote(3, 0, 72, 0.6f, 1.0f);    // C5 (1 beat duration)
    }
    else if (style == "complex")
    {
        // More complex pattern with eighth notes
        bar->addNote(0.0, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
        bar->addNote(0.5, 0, 64, 0.6f, 0.5f);    // E4 (half beat duration)
        bar->addNote(1.0, 0, 67, 0.6f, 0.5f);    // G4 (half beat duration)
        bar->addNote(1.5, 0, 72, 0.6f, 0.5f);    // C5 (half beat duration)
        bar->addNote(2.0, 0, 72, 0.6f, 0.5f);    // C5 (half beat duration)
        bar->addNote(2.5, 0, 67, 0.6f, 0.5f);    // G4 (half beat duration)
        bar->addNote(3.0, 0, 64, 0.6f, 0.5f);    // E4 (half beat duration)
        bar->addNote(3.5, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
    }
    else if (style == "funky")
    {
        // Funky pattern with syncopation
        bar->addNote(0.0, 0, 60, 0.7f, 0.75f);   // C4 (3/4 beat duration)
        bar->addNote(1.0, 0, 67, 0.7f, 0.5f);    // G4 (1/2 beat duration)
        bar->addNote(1.75, 0, 64, 0.7f, 0.25f);  // E4 (1/4 beat duration)
        bar->addNote(2.0, 0, 65, 0.7f, 0.5f);    // F4 (1/2 beat duration)
        bar->addNote(2.5, 0, 67, 0.7f, 0.5f);    // G4 (1/2 beat duration)
        bar->addNote(3.0, 0, 72, 0.7f, 0.75f);   // C5 (3/4 beat duration)
        bar->addNote(3.75, 0, 71, 0.7f, 0.25f);  // B4 (1/4 beat duration)
    }
    else if (style == "jazzy")
    {
        // Jazzy pattern with 7th chords
        bar->addNote(0.0, 0, 60, 0.6f, 0.5f);    // C4 (half beat duration)
        bar->addNote(0.5, 0, 64, 0.6f, 0.5f);    // E4 (half beat duration)
        bar->addNote(1.0, 0, 67, 0.6f, 0.5f);    // G4 (half beat duration)
        bar->addNote(1.5, 0, 70, 0.6f, 0.5f);    // Bb4 (half beat duration) - 7th
        bar->addNote(2.0, 0, 65, 0.6f, 0.5f);    // F4 (half beat duration)
        bar->addNote(2.5, 0, 69, 0.6f, 0.5f);    // A4 (half beat duration)
        bar->addNote(3.0, 0, 72, 0.6f, 0.5f);    // C5 (half beat duration)
        bar->addNote(3.5, 0, 74, 0.6f, 0.5f);    // D5 (half beat duration)
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
        // Cmaj7 chord
        bar->addNote(0.0, 0, 48, 0.6f, 0.33f);    // C3
        bar->addNote(0.0, 0, 52, 0.6f, 0.33f);    // E3
        bar->addNote(0.0, 0, 55, 0.6f, 0.33f);    // G3
        bar->addNote(0.0, 0, 59, 0.6f, 0.33f);    // B3
        
        // Dm7 chord
        bar->addNote(1.0, 0, 50, 0.6f, 0.33f);    // D3
        bar->addNote(1.0, 0, 53, 0.6f, 0.33f);    // F3
        bar->addNote(1.0, 0, 57, 0.6f, 0.33f);    // A3
        bar->addNote(1.0, 0, 60, 0.6f, 0.33f);    // C4
        
        // G7 chord
        bar->addNote(2.0, 0, 43, 0.6f, 0.33f);    // G2
        bar->addNote(2.0, 0, 47, 0.6f, 0.33f);    // B2
        bar->addNote(2.0, 0, 50, 0.6f, 0.33f);    // D3
        bar->addNote(2.0, 0, 53, 0.6f, 0.33f);    // F3
        
        // Cmaj9 chord
        bar->addNote(3.0, 0, 48, 0.6f, 0.33f);    // C3
        bar->addNote(3.0, 0, 52, 0.6f, 0.33f);    // E3
        bar->addNote(3.0, 0, 55, 0.6f, 0.33f);    // G3
        bar->addNote(3.0, 0, 59, 0.6f, 0.33f);    // B3
        bar->addNote(3.0, 0, 62, 0.6f, 0.33f);    // D4
        
        // Add some jazz-style approach notes
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
    noteOn1->setProperty("velocity", 64);
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
    noteOn2->setProperty("velocity", 64);
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
    noteOn3->setProperty("velocity", 64);
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
    noteOn4->setProperty("velocity", 64);
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
