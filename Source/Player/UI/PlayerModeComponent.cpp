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
    // Progressive test cases for hierarchical pattern system
    patternSelector.addItem("T1_Notes", 1);       // Basic note patterns
    patternSelector.addItem("T2_MultiBars", 2);   // Multi-bar patterns  
    patternSelector.addItem("T3_Chords", 3);      // Chord definitions
    patternSelector.addItem("T4_Layers", 4);      // Rhythmic layers
    patternSelector.addItem("T5_LayersChords", 5); // Layers + chords
    patternSelector.addItem("T6_Sections", 6);    // Phrase/section structure
    patternSelector.addItem("T7_References", 7);  // Pattern references
    patternSelector.addItem("T8_Exceptions", 8);  // Exceptions/modifications
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
    
}

void PlayerModeComponent::buttonClicked(juce::Button* button)
{
    if (button == &playButton)
    {
        // Play the current pattern
        playCurrentPattern();
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
    // Create progressive test cases for hierarchical pattern system
    // Each test case progressively builds complexity
    
    // T1: Basic note patterns for each instrument
    auto drumsT1 = createTestPattern("T1_Notes", "drums");
    auto bassT1 = createTestPattern("T1_Notes", "bass");
    auto pianoT1 = createTestPattern("T1_Notes", "piano");
    auto guitarT1 = createTestPattern("T1_Notes", "guitar");
    
    patternSystem->addChart("drums_T1_Notes", drumsT1);
    patternSystem->addChart("bass_T1_Notes", bassT1);
    patternSystem->addChart("piano_T1_Notes", pianoT1);
    patternSystem->addChart("guitar_T1_Notes", guitarT1);
    
    // T2: Multi-bar patterns
    auto drumsT2 = createTestPattern("T2_MultiBars", "drums");
    auto bassT2 = createTestPattern("T2_MultiBars", "bass");
    auto pianoT2 = createTestPattern("T2_MultiBars", "piano");
    auto guitarT2 = createTestPattern("T2_MultiBars", "guitar");
    
    patternSystem->addChart("drums_T2_MultiBars", drumsT2);
    patternSystem->addChart("bass_T2_MultiBars", bassT2);
    patternSystem->addChart("piano_T2_MultiBars", pianoT2);
    patternSystem->addChart("guitar_T2_MultiBars", guitarT2);
    
    // T3: Chord definitions and usage
    auto drumsT3 = createTestPattern("T3_Chords", "drums");
    auto bassT3 = createTestPattern("T3_Chords", "bass");
    auto pianoT3 = createTestPattern("T3_Chords", "piano");
    auto guitarT3 = createTestPattern("T3_Chords", "guitar");
    
    patternSystem->addChart("drums_T3_Chords", drumsT3);
    patternSystem->addChart("bass_T3_Chords", bassT3);
    patternSystem->addChart("piano_T3_Chords", pianoT3);
    patternSystem->addChart("guitar_T3_Chords", guitarT3);
    
    // T4: Rhythmic layers
    auto drumsT4 = createTestPattern("T4_Layers", "drums");
    auto bassT4 = createTestPattern("T4_Layers", "bass");
    auto pianoT4 = createTestPattern("T4_Layers", "piano");
    auto guitarT4 = createTestPattern("T4_Layers", "guitar");
    
    patternSystem->addChart("drums_T4_Layers", drumsT4);
    patternSystem->addChart("bass_T4_Layers", bassT4);
    patternSystem->addChart("piano_T4_Layers", pianoT4);
    patternSystem->addChart("guitar_T4_Layers", guitarT4);
    
    // T5: Layers + chords combined
    auto drumsT5 = createTestPattern("T5_LayersChords", "drums");
    auto bassT5 = createTestPattern("T5_LayersChords", "bass");
    auto pianoT5 = createTestPattern("T5_LayersChords", "piano");
    auto guitarT5 = createTestPattern("T5_LayersChords", "guitar");
    
    patternSystem->addChart("drums_T5_LayersChords", drumsT5);
    patternSystem->addChart("bass_T5_LayersChords", bassT5);
    patternSystem->addChart("piano_T5_LayersChords", pianoT5);
    patternSystem->addChart("guitar_T5_LayersChords", guitarT5);
    
    // T6: Phrase/section structure
    auto drumsT6 = createTestPattern("T6_Sections", "drums");
    auto bassT6 = createTestPattern("T6_Sections", "bass");
    auto pianoT6 = createTestPattern("T6_Sections", "piano");
    auto guitarT6 = createTestPattern("T6_Sections", "guitar");
    
    patternSystem->addChart("drums_T6_Sections", drumsT6);
    patternSystem->addChart("bass_T6_Sections", bassT6);
    patternSystem->addChart("piano_T6_Sections", pianoT6);
    patternSystem->addChart("guitar_T6_Sections", guitarT6);
    
    // T7: Pattern references and copying
    auto drumsT7 = createTestPattern("T7_References", "drums");
    auto bassT7 = createTestPattern("T7_References", "bass");
    auto pianoT7 = createTestPattern("T7_References", "piano");
    auto guitarT7 = createTestPattern("T7_References", "guitar");
    
    patternSystem->addChart("drums_T7_References", drumsT7);
    patternSystem->addChart("bass_T7_References", bassT7);
    patternSystem->addChart("piano_T7_References", pianoT7);
    patternSystem->addChart("guitar_T7_References", guitarT7);
    
    // T8: Exceptions and modifications
    auto drumsT8 = createTestPattern("T8_Exceptions", "drums");
    auto bassT8 = createTestPattern("T8_Exceptions", "bass");
    auto pianoT8 = createTestPattern("T8_Exceptions", "piano");
    auto guitarT8 = createTestPattern("T8_Exceptions", "guitar");
    
    patternSystem->addChart("drums_T8_Exceptions", drumsT8);
    patternSystem->addChart("bass_T8_Exceptions", bassT8);
    patternSystem->addChart("piano_T8_Exceptions", pianoT8);
    patternSystem->addChart("guitar_T8_Exceptions", guitarT8);
    
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

std::shared_ptr<Chart> PlayerModeComponent::createTestPattern(const juce::String& testType, const juce::String& instrument)
{
    String jsonPattern = "";
    String patternName = testType + " - " + instrument;
    
    if (testType == "T1_Notes")
    {
        // Test 1: Basic note patterns
        if (instrument == "drums")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Drums",
                "tempo": 120,
                "primitives": {
                    "drums": {"kick": 36, "snare": 38, "hihat": 42}
                },
                "sections": {
                    "main": [
                        {"beat": 1.0, "primitive": "kick"},
                        {"beat": 2.0, "primitive": "snare"},
                        {"beat": 3.0, "primitive": "kick"},
                        {"beat": 4.0, "primitive": "snare"}
                    ]
                }
            })";
        }
        else if (instrument == "bass")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Bass",
                "tempo": 120,
                "primitives": {
                    "bass": {"C2": 36, "D2": 38, "E2": 40, "G2": 43}
                },
                "sections": {
                    "main": [
                        {"beat": 1.0, "primitive": "C2"},
                        {"beat": 2.0, "primitive": "D2"},
                        {"beat": 3.0, "primitive": "E2"},
                        {"beat": 4.0, "primitive": "G2"}
                    ]
                }
            })";
        }
        else if (instrument == "piano")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Piano",
                "tempo": 120,
                "primitives": {
                    "piano": {"C3": 48, "E3": 52, "G3": 55, "C4": 60}
                },
                "sections": {
                    "main": [
                        {"beat": 1.0, "primitive": "C3"},
                        {"beat": 2.0, "primitive": "E3"},
                        {"beat": 3.0, "primitive": "G3"},
                        {"beat": 4.0, "primitive": "C4"}
                    ]
                }
            })";
        }
        else if (instrument == "guitar")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Guitar",
                "tempo": 120,
                "primitives": {
                    "guitar": {"E3": 52, "A3": 57, "D4": 62, "G4": 67}
                },
                "sections": {
                    "main": [
                        {"beat": 1.0, "primitive": "E3"},
                        {"beat": 2.0, "primitive": "A3"},
                        {"beat": 3.0, "primitive": "D4"},
                        {"beat": 4.0, "primitive": "G4"}
                    ]
                }
            })";
        }
    }
    else if (testType == "T2_MultiBars")
    {
        // Test 2: Multi-bar patterns spanning several bars
        if (instrument == "drums")
        {
            jsonPattern = R"({
                "title": "T2_MultiBars_Drums",
                "tempo": 120,
                "primitives": {
                    "drums": {"kick": 36, "snare": 38, "hihat": 42, "tom": 47}
                },
                "sections": {
                    "main": {
                        "bars": 4,
                        "barPatterns": [
                            [
                                {"beat": 1.0, "primitive": "kick"},
                                {"beat": 2.0, "primitive": "snare"},
                                {"beat": 3.0, "primitive": "kick"},
                                {"beat": 4.0, "primitive": "snare"}
                            ],
                            [
                                {"beat": 1.0, "primitive": "kick"},
                                {"beat": 1.5, "primitive": "kick"},
                                {"beat": 2.0, "primitive": "snare"},
                                {"beat": 4.0, "primitive": "snare"}
                            ],
                            [
                                {"beat": 1.0, "primitive": "kick"},
                                {"beat": 2.0, "primitive": "snare"},
                                {"beat": 3.0, "primitive": "kick"},
                                {"beat": 4.0, "primitive": "snare"}
                            ],
                            [
                                {"beat": 1.0, "primitive": "kick"},
                                {"beat": 2.0, "primitive": "tom"},
                                {"beat": 2.5, "primitive": "tom"},
                                {"beat": 3.0, "primitive": "tom"},
                                {"beat": 4.0, "primitive": "snare"}
                            ]
                        ]
                    }
                }
            })";
        }
        else
        {
            jsonPattern = R"({
                "title": "T2_MultiBars_Piano",
                "tempo": 120,
                "primitives": {
                    "piano": {"C4": 60, "E4": 64, "G4": 67, "C5": 72, "D4": 62, "F4": 65, "A4": 69, "D5": 74}
                },
                "sections": {
                    "main": {
                        "bars": 3,
                        "barPatterns": [
                            [
                                {"beat": 1.0, "primitive": "C4"},
                                {"beat": 2.0, "primitive": "E4"},
                                {"beat": 3.0, "primitive": "G4"},
                                {"beat": 4.0, "primitive": "C5"}
                            ],
                            [
                                {"beat": 1.0, "primitive": "D4"},
                                {"beat": 2.0, "primitive": "F4"},
                                {"beat": 3.0, "primitive": "A4"},
                                {"beat": 4.0, "primitive": "D5"}
                            ],
                            [
                                {"beat": 1.0, "primitive": "C4"},
                                {"beat": 1.5, "primitive": "E4"},
                                {"beat": 2.0, "primitive": "G4"},
                                {"beat": 3.0, "primitive": "C5"},
                                {"beat": 4.0, "primitive": "G4"}
                            ]
                        ]
                    }
                }
            })";
        }
    }
    else if (testType == "T3_Chords")
    {
        // Test 3: Chord definitions and usage
        jsonPattern = R"({
            "title": "T3_Chords",
            "tempo": 120,
            "primitives": {
                "piano": {"C3": 48, "E3": 52, "G3": 55, "F3": 53, "A3": 57, "C4": 60}
            },
            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"]
            },
            "sections": {
                "main": [
                    {"beat": 1.0, "chord": "Cmaj"},
                    {"beat": 3.0, "chord": "Fmaj"}
                ]
            }
        })";
    }
    else if (testType == "T4_Layers")
    {
        // Test 4: Single instrument with multiple rhythm layers
        if (instrument == "drums")
        {
            jsonPattern = R"({
                "title": "T4_Layers_Drums",
                "tempo": 120,
                "primitives": {
                    "drums": {"kick": 36, "snare": 38, "hihat": 42}
                },
                "sections": {
                    "main": {
                        "layers": {
                            "kickPattern": [
                                {"beat": 1.0, "primitive": "kick"},
                                {"beat": 3.0, "primitive": "kick"}
                            ],
                            "snarePattern": [
                                {"beat": 2.0, "primitive": "snare"},
                                {"beat": 4.0, "primitive": "snare"}
                            ],
                            "hihatPattern": [
                                {"beat": 1.0, "primitive": "hihat"},
                                {"beat": 1.5, "primitive": "hihat"},
                                {"beat": 2.0, "primitive": "hihat"},
                                {"beat": 2.5, "primitive": "hihat"},
                                {"beat": 3.0, "primitive": "hihat"},
                                {"beat": 3.5, "primitive": "hihat"},
                                {"beat": 4.0, "primitive": "hihat"},
                                {"beat": 4.5, "primitive": "hihat"}
                            ]
                        }
                    }
                }
            })";
        }
        else
        {
            jsonPattern = R"({
                "title": "T4_Layers_Other",
                "tempo": 120,
                "primitives": {
                    "piano": {"C4": 60, "E4": 64, "G4": 67, "C5": 72},
                    "bass": {"C2": 36, "G2": 43}
                },
                "sections": {
                    "main": {
                        "layers": {
                            "piano": [
                                {"beat": 1.0, "primitive": "C4"},
                                {"beat": 1.5, "primitive": "E4"},
                                {"beat": 2.0, "primitive": "G4"},
                                {"beat": 2.5, "primitive": "C5"}
                            ],
                            "bass": [
                                {"beat": 1.0, "primitive": "C2"},
                                {"beat": 3.0, "primitive": "G2"}
                            ]
                        }
                    }
                }
            })";
        }
    }
    else if (testType == "T5_LayersChords")
    {
        // Test 5: Layers + chords combined
        jsonPattern = R"({
            "title": "T5_LayersChords_Piano",
            "tempo": 120,
            "primitives": {
                "piano": {
                    "C3": 48, "E3": 52, "G3": 55, "C4": 60,
                    "F3": 53, "A3": 57, "D4": 62, "G4": 67,
                    "B3": 59
                }
            },
            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"],
                "Gmaj": ["G3", "B3", "D4"]
            },
            "sections": {
                "main": {
                    "layers": {
                        "leftHand": [
                            {"beat": 1.0, "chord": "Cmaj"},
                            {"beat": 2.0, "chord": "Cmaj"},
                            {"beat": 3.0, "chord": "Fmaj"},
                            {"beat": 4.0, "chord": "Gmaj"}
                        ],
                        "rightHandStructure": [
                            {"beat": 1.5, "primitive": "C4"},
                            {"beat": 2.5, "primitive": "D4"},
                            {"beat": 3.5, "primitive": "G4"}
                        ]
                    }
                }
            }
        })";
    }
    else if (testType == "T6_Sections")
    {
        // Test 6: Multi-section songs
        jsonPattern = R"({
            "title": "T6_Sections_Song",
            "tempo": 120,
            "primitives": {
                "piano": {"C4": 60, "E4": 64, "G4": 67, "B4": 71, "D5": 74, "G5": 79}
            },
            "sections": {
                "verse": [
                    {"beat": 1.0, "primitive": "C4"},
                    {"beat": 2.0, "primitive": "E4"},
                    {"beat": 3.0, "primitive": "G4"},
                    {"beat": 4.0, "primitive": "C4"}
                ],
                "chorus": [
                    {"beat": 1.0, "primitive": "G4"},
                    {"beat": 2.0, "primitive": "B4"},
                    {"beat": 3.0, "primitive": "D5"},
                    {"beat": 4.0, "primitive": "G5"}
                ],
                "bridge": [
                    {"beat": 1.0, "primitive": "B4"},
                    {"beat": 2.0, "primitive": "D5"},
                    {"beat": 3.0, "primitive": "G5"},
                    {"beat": 4.0, "primitive": "B4"}
                ]
            },
            "arrangement": ["verse", "chorus", "verse", "chorus", "bridge", "chorus"]
        })";
    }
    else if (testType == "T7_References")
    {
        // Test 7: Pattern references
        jsonPattern = R"({
            "title": "T7_References",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42}
            },
            "patterns": {
                "basicBeat": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 2.0, "primitive": "snare"},
                    {"beat": 3.0, "primitive": "kick"},
                    {"beat": 4.0, "primitive": "snare"}
                ]
            },
            "sections": {
                "main": {
                    "layers": {
                        "drums": {"pattern": "basicBeat"}
                    }
                }
            }
        })";
    }
    else if (testType == "T8_Exceptions")
    {
        // Test 8: Pattern variations - base pattern with bar-specific overrides
        jsonPattern = R"({
            "title": "T8_Exceptions_Drums",
            "tempo": 120,
            "primitives": {
                "drums": {"kick": 36, "snare": 38, "hihat": 42, "crash": 49, "tom": 47}
            },
            "patterns": {
                "basicBeat": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 2.0, "primitive": "snare"},
                    {"beat": 3.0, "primitive": "kick"},
                    {"beat": 4.0, "primitive": "snare"}
                ],
                "fillEnding": [
                    {"beat": 1.0, "primitive": "kick"},
                    {"beat": 1.5, "primitive": "tom"},
                    {"beat": 2.0, "primitive": "tom"},
                    {"beat": 2.5, "primitive": "tom"},
                    {"beat": 3.0, "primitive": "snare"},
                    {"beat": 4.0, "primitive": "crash"}
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "basePattern": "basicBeat",
                    "variations": {
                        "bar4": "fillEnding"
                    }
                }
            }
        })";
    }
    else
    {
        // Default fallback pattern
        jsonPattern = R"({
            "title": "Default",
            "tempo": 120,
            "primitives": {
                "piano": {"C4": 60, "E4": 64, "G4": 67, "C5": 72}
            },
            "sections": {
                "main": [
                    {"beat": 1.0, "primitive": "C4"},
                    {"beat": 2.0, "primitive": "E4"},
                    {"beat": 3.0, "primitive": "G4"},
                    {"beat": 4.0, "primitive": "C5"}
                ]
            }
        })";
    }
    
    // Parse the JSON pattern using the PatternParser
    std::cout << "JSON pattern for " << patternName << ":\n" << jsonPattern << std::endl;
    
    // Parse the JSON pattern using the PatternParser
    auto chart = patternParser->parseChart(jsonPattern);
    
    // Log parsing success/failure
    if (chart == nullptr || chart->getNumSections() == 0)
    {
        std::cout << "JSON pattern parsing failed for: " << patternName << std::endl;
        // Return empty chart - we're no longer using fallback patterns
        return std::make_shared<Chart>(patternName, 120.0);
    }
    else
    {
        std::cout << "JSON pattern parsing succeeded: " << patternName << std::endl;
    }
    
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
    juce::String text = juce::String("Chart: ") + currentChart->getTitle() + juce::String("\n");
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
        case 1: patternSuffix = "_T1_Notes"; break;
        case 2: patternSuffix = "_T2_MultiBars"; break;
        case 3: patternSuffix = "_T3_Chords"; break;
        case 4: patternSuffix = "_T4_Layers"; break;
        case 5: patternSuffix = "_T5_LayersChords"; break;
        case 6: patternSuffix = "_T6_Sections"; break;
        case 7: patternSuffix = "_T7_References"; break;
        case 8: patternSuffix = "_T8_Exceptions"; break;
        default: patternSuffix = "_T1_Notes"; break;
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
    
    // If the chart doesn't exist, fall back to T1_Notes pattern
    if (currentChart == nullptr && patternSuffix != "_T1_Notes")
    {
        chartName = chartName.replace(patternSuffix, "_T1_Notes");
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



} // namespace covertracks
