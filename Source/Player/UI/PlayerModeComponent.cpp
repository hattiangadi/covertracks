#include "PlayerModeComponent.h"

namespace covertracks {

PlayerModeComponent::PlayerModeComponent(PlayerManager& pm)
    : playerManager(pm),
      titleLabel("titleLabel", "Pattern Player"),
      convertButton("Convert to MIDI"),
      loadButton("Load Chart"),
      playButton("Play"),
      stopButton("Stop")
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
    // Integration test pattern with all features (auto-selects based on instrument)
    patternSelector.addItem("Integration", 9);  // Complete chart for selected instrument
    patternSelector.setTextWhenNothingSelected("Select pattern");
    patternSelector.addListener(this);
    addAndMakeVisible(patternSelector);
    
    // Set up pattern viewer
    patternViewer.setMultiLine(true);
    patternViewer.setReadOnly(true);
    patternViewer.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(14.0f).withStyle("plain")));
    addAndMakeVisible(patternViewer);
    
    // Set up buttons
    convertButton.addListener(this);
    convertButton.setEnabled(false);
    addAndMakeVisible(convertButton);
    
    loadButton.addListener(this);
    loadButton.setEnabled(false);
    addAndMakeVisible(loadButton);
    
    playButton.addListener(this);
    playButton.setEnabled(false);
    addAndMakeVisible(playButton);
    
    stopButton.addListener(this);
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);
    
    // Register as a listener for player manager changes
    playerManager.addChangeListener(this);
    
    // Initialize the pattern system without default instruments
    patternSystem = std::make_unique<MusicPatternSystem>(false);
    
    DBG("[PlayerModeComponent] Created pattern system without default instruments");
    
    // Create default drum capabilities 
    InstrumentCapabilities drumCapabilities;
    drumCapabilities.type = InstrumentCapabilityType::Mapping;
    drumCapabilities.instrumentType = "drums";
    drumCapabilities.soundMappings = {
        {"kick", 36}, {"snare", 38}, {"hihat", 42}, {"openHihat", 46},
        {"crash", 49}, {"ride", 51}, {"rimshot", 37}
    };
    
    // Update the pattern system with the Player's drum sound mappings
    InstrumentDefinition drums("drums");
    
    // First, create a mapping between test pattern names and Player sound names
    std::map<String, String> drumNameMappings = {
        {"bassDrum", "kick"},           // bassDrum maps to kick (36)
        {"snare", "snare"},             // snare maps to snare (38)
        {"ride", "ride"},        // ride maps to ride (51)
        {"closedHiHat", "hihat"},      // closedHiHat maps to hihat (42)
        {"openHiHat", "openHihat"},    // openHiHat maps to openHihat (46)
        {"snareRimHit", "rimshot"},    // snareRimHit maps to rimshot (37)
        {"mediumCrashCymbal", "crash"}, // mediumCrashCymbal maps to crash (49)
        {"crash", "crash"}  // crash maps to crash (49)
    };
    
    // Add drum primitives from the Player's sound mappings
    for (const auto& mapping : drumCapabilities.soundMappings)
    {
        Primitive primitive;
        primitive.name = mapping.first;
        primitive.midiNote = mapping.second;
        primitive.description = mapping.first + " drum sound";
        drums.addPrimitive(mapping.first, primitive);
        
        // Add debug output for this mapping
        DBG("Adding Player drum mapping: " + mapping.first + " = " + String(mapping.second));
    }
    
    // Now add all the test pattern name mappings directly with explicit MIDI notes
    // This ensures we don't rely on the lookup which might be failing
    Primitive bassDrum;
    bassDrum.name = "bassDrum";
    bassDrum.midiNote = 36; // kick drum MIDI note
    bassDrum.description = "Bass Drum (kick)";
    drums.addPrimitive("bassDrum", bassDrum);
    DBG("Added explicit bassDrum mapping with MIDI note 36");
    
    Primitive ride;
    ride.name = "ride";
    ride.midiNote = 51; // ride cymbal MIDI note
    ride.description = "Ride Cymbal";
    drums.addPrimitive("ride", ride);
    DBG("Added explicit ride mapping with MIDI note 51");
    
    // Add other explicit mappings for common drum names used in patterns
    Primitive closedHiHat;
    closedHiHat.name = "closedHiHat";
    closedHiHat.midiNote = 42; // closed hi-hat MIDI note
    closedHiHat.description = "Closed Hi-Hat";
    drums.addPrimitive("closedHiHat", closedHiHat);
    
    Primitive openHiHat;
    openHiHat.name = "openHiHat";
    openHiHat.midiNote = 46; // open hi-hat MIDI note
    openHiHat.description = "Open Hi-Hat";
    drums.addPrimitive("openHiHat", openHiHat);
    
    Primitive snareRimHit;
    snareRimHit.name = "snareRimHit";
    snareRimHit.midiNote = 37; // rimshot MIDI note
    snareRimHit.description = "Snare Rim Hit";
    drums.addPrimitive("snareRimHit", snareRimHit);
    
    Primitive crashCymbal;
    crashCymbal.name = "crashCymbal";
    crashCymbal.midiNote = 49; // crash cymbal MIDI note
    crashCymbal.description = "Crash Cymbal";
    drums.addPrimitive("crashCymbal", crashCymbal);
    drums.addPrimitive("mediumCrashCymbal", crashCymbal);
    drums.addPrimitive("crash", crashCymbal);
    
    // Add additional drum mappings that aren't in the Player's capabilities
    // These will use the specific mappings you provided
    std::map<String, int> additionalDrumMappings = {
        {"bassDrumAlternative", 37},
        {"floorTom", 43},
        {"floorTomFlam", 57},
        {"halfOpenHiHat", 44},
        {"hiHatCloseFootPedal", 59},
        {"hiHatOpenFootPedalClose", 60},
        {"mediumTomFlam", 58},
        {"midTom", 45},
        {"quickSnareFlam", 55},
        {"quickSnareFlamAlternative", 56},
        {"rideBell", 48},
        {"rimClick", 40},
        {"rimClickAlternative", 41},
        {"slowSnareFlam", 53},
        {"slowSnareFlamAlternative", 54},
        {"stoppedMediumCrashCymbal", 52},
        {"stoppedSmallCrashCymbal", 50}
    };
    
    for (const auto& mapping : additionalDrumMappings)
    {
        Primitive primitive;
        primitive.name = mapping.first;
        primitive.midiNote = mapping.second;
        primitive.description = mapping.first + " drum sound";
        drums.addPrimitive(mapping.first, primitive);
    }
    
    // Add the drum kit with all mappings to the pattern system
    patternSystem->addInstrumentDefinition("drums", drums);
    
    // Debug output to verify our mappings
    DBG("[PlayerModeComponent] Verifying drum mappings:");
    auto& drumDef = patternSystem->getInstrumentDefinition("drums");
    auto allPrimitives = drumDef.getPrimitives();
    for (const auto& primitivePair : allPrimitives)
    {
        DBG("  " + primitivePair.first + " = " + String(primitivePair.second.midiNote));
    }
    
    // Add a piano instrument definition with basic primitives
    InstrumentDefinition piano("piano");
    
    Primitive c3;
    c3.name = "C3";
    c3.midiNote = 48;
    c3.description = "C3 note";
    piano.addPrimitive("C3", c3);
    
    Primitive e3;
    e3.name = "E3";
    e3.midiNote = 52;
    e3.description = "E3 note";
    piano.addPrimitive("E3", e3);
    
    Primitive g3;
    g3.name = "G3";
    g3.midiNote = 55;
    g3.description = "G3 note";
    piano.addPrimitive("G3", g3);
    
    patternSystem->addInstrumentDefinition("piano", piano);
    
    // CRITICAL: Make sure to recreate the pattern parser after updating ALL instrument definitions
    // so it has access to the updated primitives
    patternParser = std::make_unique<PatternParser>(*patternSystem);
    DBG("[PlayerModeComponent] Recreated PatternParser with updated instrument definitions");
    
    // Register the drum capabilities with the PatternParser
    patternParser->registerInstrumentCapabilities("drums", drumCapabilities);
    DBG("[PlayerModeComponent] Registered drum capabilities with PatternParser");
    
    // Debug output to verify the drum capabilities
    DBG("[PlayerModeComponent] Drum capabilities:");
    for (const auto& mapping : drumCapabilities.soundMappings)
    {
        DBG("  " + mapping.first + " = " + String(mapping.second));
    }
    
    // First update player selector with available players
    updatePlayerSelector();
    
    // Now create default patterns after the pattern system has been fully initialized
    createDefaultPatterns();
}

PlayerModeComponent::~PlayerModeComponent()
{
    // Remove listeners
    playerSelector.removeListener(this);
    instrumentTypeSelector.removeListener(this);
    patternSelector.removeListener(this);
    convertButton.removeListener(this);
    loadButton.removeListener(this);
    playButton.removeListener(this);
    stopButton.removeListener(this);
    playerManager.removeChangeListener(this);
    
    // Clear any references to patterns and Player
    currentPatternJson = "";
    selectedPlayer = nullptr;
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
    auto buttonArea = row;
    int buttonWidth = buttonArea.getWidth() / 4;
    convertButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    loadButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    stopButton.setBounds(buttonArea.reduced(2));
    
}

void PlayerModeComponent::buttonClicked(juce::Button* button)
{
    if (button == &convertButton)
    {
        // Convert Chart to MIDI_EVENT_CHART format
        convertToMidiChart();
    }
    else if (button == &loadButton)
    {
        // Load the MIDI_EVENT_CHART into the player
        loadMidiChart();
    }
    else if (button == &playButton)
    {
        std::cout << "🔘 Play button clicked!" << std::endl;
        // Play the loaded chart
        playLoadedChart();
    }
    else if (button == &stopButton)
    {
        // Stop the current Player
        if (selectedPlayer != nullptr)
        {
            selectedPlayer->stop();
            
            // Update button states
            playButton.setEnabled(true);   // Can still play the pattern again
            stopButton.setEnabled(false);
            
            std::cout << "⏹️ Stopped Player playback" << std::endl;
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
            
            // Update selected Player
            updateSelectedPlayer();
            
            // Update button states
            updateButtonStates();
        }
        else
        {
            // No player selected
            selectedPlayer = nullptr;
            updateButtonStates();
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
                updateButtonStates();
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
    
    // Update selected Player
    updateSelectedPlayer();
    
    // Update button states
    updateButtonStates();
}

void PlayerModeComponent::createDefaultPatterns()
{
    // We no longer need to pre-create patterns since we'll generate them on-demand
    // as JSON strings and pass them directly to the Player (like LLM workflow)
    
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

juce::String PlayerModeComponent::createTestPattern(const juce::String& testType, const juce::String& instrument)
{
    String jsonPattern = "";
    
    if (testType == "T1_Notes")
    {
        // Test 1: Basic note patterns
        if (instrument == "drums")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Drums",
                "tempo": 120,
                "sections": {
                    "main": [
                        [1.0, "kick", 0.8, 1.0],
                        [2.0, "snare", 0.9, 1.0],
                        [3.0, "kick", 0.8, 1.0],
                        [4.0, "snare", 0.9, 1.0]
                    ]
                }
            })";
        }
        else if (instrument == "bass")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Bass",
                "tempo": 120,
                "sections": {
                    "main": [
                        [1.0, "C2", 0.7, 1.0],
                        [2.0, "D2", 0.7, 1.0],
                        [3.0, "E2", 0.7, 1.0],
                        [4.0, "G2", 0.7, 1.0]
                    ]
                }
            })";
        }
        else if (instrument == "piano")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Piano",
                "tempo": 120,
                "sections": {
                    "main": [
                        [1.0, "C3", 0.6, 1.0],
                        [2.0, "E3", 0.6, 1.0],
                        [3.0, "G3", 0.6, 1.0],
                        [4.0, "C4", 0.6, 1.0]
                    ]
                }
            })";
        }
        else if (instrument == "guitar")
        {
            jsonPattern = R"({
                "title": "T1_Notes_Guitar",
                "tempo": 120,
                "sections": {
                    "main": [
                        [1.0, "E3", 0.6, 1.0],
                        [2.0, "A3", 0.6, 1.0],
                        [3.0, "D4", 0.6, 1.0],
                        [4.0, "G4", 0.6, 1.0]
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
                "sections": {
                    "main": {
                        "bars": 4,
                        "barPatterns": [
                            [
                                [1.0, "kick", 0.8, 1.0],
                                [2.0, "snare", 1.0, 1.0],
                                [3.0, "kick", 0.8, 1.0],
                                [4.0, "snare", 1.0, 1.0]
                            ],
                            [
                                [1.0, "kick", 0.8, 1.0],
                                [1.5, "kick", 0.7, 0.5],
                                [2.0, "snare", 1.0, 1.0],
                                [4.0, "snare", 1.0, 1.0]
                            ],
                            [
                                [1.0, "kick", 0.8, 1.0],
                                [2.0, "snare", 1.0, 1.0],
                                [3.0, "kick", 0.8, 1.0],
                                [4.0, "snare", 1.0, 1.0]
                            ],
                            [
                                [1.0, "kick", 0.8, 1.0],
                                [2.0, "ride", 0.7, 0.5],
                                [2.5, "ride", 0.7, 0.5],
                                [3.0, "ride", 0.7, 0.5],
                                [4.0, "snare", 1.0, 1.0]
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
                "sections": {
                    "main": {
                        "bars": 3,
                        "barPatterns": [
                            [
                                [1.0, "C4", 0.6, 1.0],
                                [2.0, "E4", 0.6, 1.0],
                                [3.0, "G4", 0.6, 1.0],
                                [4.0, "C5", 0.6, 1.0]
                            ],
                            [
                                [1.0, "D4", 0.6, 1.0],
                                [2.0, "F4", 0.6, 1.0],
                                [3.0, "A4", 0.6, 1.0],
                                [4.0, "D5", 0.6, 1.0]
                            ],
                            [
                                [1.0, "C4", 0.6, 1.0],
                                [1.5, "E4", 0.5, 0.5],
                                [2.0, "G4", 0.6, 1.0],
                                [3.0, "C5", 0.6, 1.0],
                                [4.0, "G4", 0.6, 1.0]
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
            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"]
            },
            "sections": {
                "main": [
                    [1.0, {"chord": "Cmaj"}, 0.6, 2.0],
                    [3.0, {"chord": "Fmaj"}, 0.6, 2.0]
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

                "sections": {
                    "main": {
                        "layers": {
                            "kickPattern": [
                                [1.0, "kick", 0.8, 1.0],
                                [3.0, "kick", 0.8, 1.0]
                            ],
                            "snarePattern": [
                                [2.0, "snare", 0.8, 1.0],
                                [4.0, "snare", 0.8, 1.0]
                            ],
                            "hihatPattern": [
                                [1.0, "hihat", 0.8, 1.0],
                                [1.5, "hihat", 0.8, 1.0],
                                [2.0, "hihat", 0.8, 1.0],
                                [2.5, "hihat", 0.8, 1.0],
                                [3.0, "hihat", 0.8, 1.0],
                                [3.5, "hihat", 0.8, 1.0],
                                [4.0, "hihat", 0.8, 1.0],
                                [4.5, "hihat", 0.8, 0.4]
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

                "sections": {
                    "main": {
                        "layers": {
                            "piano": [
                                [1.0, "C4", 0.8, 1.0],
                                [1.5, "E4", 0.8, 1.0],
                                [2.0, "G4", 0.8, 1.0],
                                [2.5, "C5", 0.8, 1.0]
                            ],
                            "bass": [
                                [1.0, "C2", 0.8, 1.0],
                                [3.0, "G2", 0.8, 1.0]
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

            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"],
                "Gmaj": ["G3", "B3", "D4"]
            },
            "sections": {
                "main": {
                    "layers": {
                        "leftHand": [
                            [1.0, {"chord": "Cmaj"}, 0.6, 1.0],
                            [2.0, {"chord": "Cmaj"}, 0.6, 1.0],
                            [3.0, {"chord": "Fmaj"}, 0.6, 1.0],
                            [4.0, {"chord": "Gmaj"}, 0.6, 1.0]
                        ],
                        "rightHandStructure": [
                            [1.5, "C4", 0.6, 0.5],
                            [2.5, "D4", 0.6, 0.5],
                            [3.5, "G4", 0.6, 0.5]
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
            "sections": {
                "verse": [
                    [1.0, "C4", 0.8, 1.0],
                    [2.0, "E4", 0.8, 1.0],
                    [3.0, "G4", 0.8, 1.0],
                    [4.0, "C4", 0.8, 1.0]
                ],
                "chorus": [
                    [1.0, "G4", 0.8, 1.0],
                    [2.0, "B4", 0.8, 1.0],
                    [3.0, "D5", 0.8, 1.0],
                    [4.0, "G5", 0.8, 1.0]
                ],
                "bridge": [
                    [1.0, "B4", 0.8, 1.0],
                    [2.0, "D5", 0.8, 1.0],
                    [3.0, "G5", 0.8, 1.0],
                    [4.0, "B4", 0.8, 1.0]
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
            "patterns": {
                "basicBeat": [
                    [1.0, "kick", 0.8, 1.0],
                    [2.0, "snare", 0.8, 1.0],
                    [3.0, "kick", 0.8, 1.0],
                    [4.0, "snare", 0.8, 1.0]
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

            "patterns": {
                "basicBeat": [
                    [1.0, "kick", 0.8, 1.0],
                    [2.0, "snare", 0.8, 1.0],
                    [3.0, "kick", 0.8, 1.0],
                    [4.0, "snare", 0.8, 1.0]
                ],
                "fillEnding": [
                    [1.0, "kick", 0.8, 1.0],
                    [1.5, "tom2", 0.8, 1.0],
                    [2.0, "tom2", 0.8, 1.0],
                    [2.5, "tom2", 0.8, 1.0],
                    [3.0, "snare", 0.8, 1.0],
                    [4.0, "crash", 0.8, 1.0]
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
    else if (testType == "Integration")
    {
        // Complete integration chart - select based on instrument type
        if (instrument == "piano")
        {
            jsonPattern = R"({
                "title": "Integration_Piano_Chart",
            "tempo": 120,

            "chords": {
                "Cmaj": ["C3", "E3", "G3"],
                "Fmaj": ["F3", "A3", "C4"],
                "Gmaj": ["G3", "B3", "D4"],
                "Am": ["A3", "C4", "E4"]
            },
            "patterns": {
                "leftHandBass": [
                    [1.0, "C2", 0.8, 1.0],
                    [1.5, "G2", 0.8, 1.0],
                    [2.0, "C3", 0.8, 1.0],
                    [2.5, "G2", 0.8, 1.0]
                ],
                "rightHandMelody": [
                    [1.0, "C4", 0.8, 1.0],
                    [1.5, "E4", 0.8, 1.0],
                    [2.0, "G4", 0.8, 1.0],
                    [3.0, "C5", 0.8, 1.0]
                ],
                "arpeggioPattern": [
                    [1.0, "C3", 0.8, 1.0],
                    [1.25, "E3", 0.8, 1.0],
                    [1.5, "G3", 0.8, 1.0],
                    [1.75, "C4", 0.8, 1.0],
                    [2.0, "E4", 0.8, 1.0],
                    [2.25, "G4", 0.8, 1.0],
                    [2.5, "C5", 0.8, 1.0],
                    [2.75, "G4", 0.8, 1.0]
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "basePattern": "leftHandBass",
                    "variations": {
                        "bar4": "arpeggioPattern"
                    },
                    "layers": {
                        "leftHandChords": [
                            [1.0, {"chord": "Cmaj"}],
                            [3.0, {"chord": "Fmaj"}]
                        ],
                        "rightHandMelody": {"pattern": "rightHandMelody"},
                        "bassLine": [
                            [1.0, "C2", 0.8, 1.0],
                            [3.0, "F2", 0.8, 1.0]
                        ]
                    }
                },
                "chorus": {
                    "bars": 4,
                    "barPatterns": [
                        [
                            [1.0, "G3", 0.8, 1.0],
                            [1.5, "B3", 0.8, 1.0],
                            [2.0, "D4", 0.8, 1.0],
                            [3.0, "G4", 0.8, 1.0],
                            [4.0, "B4", 0.8, 1.0]
                        ],
                        [
                            [1.0, "A3", 0.8, 1.0],
                            [2.0, "C4", 0.8, 1.0],
                            [3.0, "E4", 0.8, 1.0],
                            [4.0, "A4", 0.8, 1.0]
                        ],
                        [
                            [1.0, "F3", 0.8, 1.0],
                            [2.0, "A3", 0.8, 1.0],
                            [3.0, "C4", 0.8, 1.0],
                            [4.0, "F4", 0.8, 1.0]
                        ],
                        [
                            [1.0, "C3", 0.8, 1.0],
                            [2.0, "E3", 0.8, 1.0],
                            [3.0, "G3", 0.8, 1.0],
                            [4.0, "C4", 0.8, 1.0]
                        ]
                    ],
                    "layers": {
                        "harmonyChords": [
                            [1.0, {"chord": "Gmaj"}],
                            [2.0, {"chord": "Am"}],
                            [3.0, {"chord": "Fmaj"}],
                            [4.0, {"chord": "Cmaj"}]
                        ],
                        "melodyLine": {"pattern": "arpeggioPattern"}
                    }
                },
                "bridge": {
                    "layers": {
                        "leftHandArpeggios": [
                            [1.0, "A2", 0.8, 1.0],
                            [1.5, "C3", 0.8, 1.0],
                            [2.0, "E3", 0.8, 1.0],
                            [2.5, "A3", 0.8, 1.0],
                            [3.0, "G2", 0.8, 1.0],
                            [3.5, "B2", 0.8, 1.0],
                            [4.0, "D3", 0.6, 1.0]
                        ],
                        "rightHandHarmony": [
                            [1.0, {"chord": "Am"}],
                            [3.0, {"chord": "Gmaj"}]
                        ]
                    }
                }
            },
            "arrangement": ["verse", "chorus", "verse", "chorus", "bridge", "chorus"]
        })";
        }
        else if (instrument == "drums")
        {
            jsonPattern = R"({
                "title": "Integration_Drum_Chart",
            "tempo": 120,

            "patterns": {
                "kickPattern": [
                    [1.0, "kick", 0.8, 1.0],
                    [3.0, "kick", 0.8, 1.0]
                ],
                "snareBackbeat": [
                    [2.0, "snare", 0.8, 1.0],
                    [4.0, "snare", 0.8, 1.0]
                ],
                "hihatGroove": [
                    [1.0, "hihat", 0.8, 1.0],
                    [1.5, "hihat", 0.8, 1.0],
                    [2.0, "hihat", 0.8, 1.0],
                    [2.5, "hihat", 0.8, 1.0],
                    [3.0, "hihat", 0.8, 1.0],
                    [3.5, "hihat", 0.8, 1.0],
                    [4.0, "hihat", 0.8, 1.0],
                    [4.5, "hihat", 0.8, 0.4]
                ],
                "tomFill": [
                    [1.0, "tom2", 0.8, 1.0],
                    [1.5, "tom2", 0.8, 1.0],
                    [2.0, "tom3", 0.8, 1.0],
                    [2.5, "tom3", 0.8, 1.0],
                    [3.0, "tom3", 0.8, 1.0],
                    [3.5, "tom3", 0.8, 1.0],
                    [4.0, "crash", 0.9, 1.0]
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "basePattern": "kickPattern",
                    "variations": {
                        "bar4": "tomFill"
                    },
                    "layers": {
                        "kickLayer": {"pattern": "kickPattern"},
                        "snareLayer": {"pattern": "snareBackbeat"},
                        "hihatLayer": {"pattern": "hihatGroove"}
{{ ... }}
                    }
                },
                "chorus": {
                    "bars": 4,
                    "barPatterns": [
                        [
                            [1.0, "kick", 0.8, 1.0],
                            [1.5, "kick", 0.8, 1.0],
                            [2.0, "snare", 0.8, 1.0],
                            [3.0, "kick", 0.8, 1.0],
                            [4.0, "snare", 0.8, 1.0]
                        ],
                        [
                            [1.0, "kick", 0.8, 1.0],
                            [2.0, "snare", 0.8, 1.0],
                            [2.5, "rimClick", 0.8, 1.0],
                            [3.0, "kick", 0.8, 1.0],
                            [4.0, "snare", 0.8, 1.0]
                        ],
                        [
                            [1.0, "kick", 0.8, 1.0],
                            [1.5, "ride", 0.8, 1.0],
                            [2.0, "snare", 0.8, 1.0],
                            [2.5, "ride", 0.8, 1.0],
                            [3.0, "kick", 0.8, 1.0],
                            [3.5, "ride", 0.8, 1.0],
                            [4.0, "snare", 0.8, 1.0]
                        ],
                        [
                            [1.0, "kick", 0.8, 1.0],
                            [2.0, "crash", 0.8, 1.0],
                            [2.5, "crash", 0.8, 1.0],
                            [3.0, "kick", 0.8, 1.0],
                            [4.0, "crash", 0.8, 1.0]
                        ]
                    ]
                },
                "breakdown": {
                    "layers": {
                        "kickOnly": [
                            [1.0, "kick", 0.8, 1.0],
                            [4.0, "kick", 0.8, 1.0]
                        ],
                        "ghostSnares": [
                            [2.25, "snare", 0.5, 0.25],
                            [3.75, "snare", 0.5, 0.25]
                        ],
                        "openHats": [
                            [2.0, "openHihat", 0.7, 1.0],
                            [4.0, "openHihat", 0.7, 1.0]
                        ]
                    }
                }
            },
            "arrangement": ["verse", "chorus", "verse", "chorus", "breakdown", "chorus"]
        })";
        }
        else if (instrument == "guitar")
        {
            jsonPattern = R"({
                "title": "Integration_Guitar_Chart",
            "tempo": 120,

            "chords": {
                "Cmaj": ["C3", "E4", "G3"],
                "Fmaj": ["F2", "A2", "C3"],
                "Gmaj": ["G2", "B3", "D3"],
                "Am": ["A2", "C3", "E4"]
            },
            "patterns": {
                "strummingPattern": [
                    [1.0, "G3", 0.8, 1.0],
                    [1.5, "B3", 0.8, 1.0],
                    [2.0, "E4", 0.8, 1.0],
                    [2.5, "B3", 0.8, 1.0],
                    [3.0, "G3", 0.8, 1.0],
                    [3.5, "B3", 0.8, 1.0],
                    [4.0, "E4", 0.8, 1.0]
                ],
                "bassLine": [
                    [1.0, "E2", 0.8, 1.0],
                    [1.5, "G2", 0.8, 1.0],
                    [2.0, "A2", 0.8, 1.0],
                    [3.0, "G2", 0.8, 1.0],
                    [4.0, "E2", 0.8, 1.0]
                ],
                "fingerpicking": [
                    [1.0, "E2", 0.8, 1.0],
                    [1.25, "G3", 0.8, 1.0],
                    [1.5, "B3", 0.8, 1.0],
                    [1.75, "E4", 0.8, 1.0],
                    [2.0, "B3", 0.8, 1.0],
                    [2.25, "G3", 0.8, 1.0],
                    [2.5, "E2", 0.8, 1.0],
                    [3.0, "A2", 0.8, 1.0]
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "basePattern": "strummingPattern",
                    "variations": {
                        "bar4": "fingerpicking"
                    },
                    "layers": {
                        "chordProg": [
                            [1.0, {"chord": "Cmaj"}],
                            [3.0, {"chord": "Fmaj"}]
                        ],
                        "bassLayer": {"pattern": "bassLine"}
                    }
                },
                "chorus": {
                    "bars": 4,
                    "barPatterns": [
                        [
                            [1.0, "G2", 0.8, 1.0],
                            [1.5, "D3", 0.8, 1.0],
                            [2.0, "G3", 0.8, 1.0],
                            [3.0, "B3", 0.8, 1.0],
                            [4.0, "D4", 0.8, 1.0]
                        ],
                        [
                            [1.0, "A2", 0.8, 1.0],
                            [2.0, "C3", 0.8, 1.0],
                            [3.0, "E4", 0.8, 1.0],
                            [4.0, "A3", 0.8, 1.0]
                        ],
                        [
                            [1.0, "F2", 0.8, 1.0],
                            [2.0, "A2", 0.8, 1.0],
                            [3.0, "C3", 0.8, 1.0],
                            [4.0, "F3", 0.8, 1.0]
                        ],
                        [
                            [1.0, "G2", 0.8, 1.0],
                            [2.0, "B3", 0.8, 1.0],
                            [3.0, "D3", 0.8, 1.0],
                            [4.0, "G3", 0.8, 1.0]
                        ]
                    ],
                    "layers": {
                        "harmonyChords": [
                            [1.0, {"chord": "Gmaj"}],
                            [2.0, {"chord": "Am"}],
                            [3.0, {"chord": "Fmaj"}],
                            [4.0, {"chord": "Gmaj"}]
                        ]
                    }
                },
                "bridge": {
                    "layers": {
                        "arpeggios": {"pattern": "fingerpicking"},
                        "sustainedChords": [
                            [1.0, {"chord": "Am"}],
                            [3.0, {"chord": "Fmaj"}]
                        ]
                    }
                }
            },
            "arrangement": ["verse", "chorus", "verse", "chorus", "bridge", "chorus"]
        })";
        }
        else if (instrument == "bass")
        {
            jsonPattern = R"({
                "title": "Integration_Bass_Chart",
            "tempo": 120,
            "patterns": {
                "walkingBass": [
                    [1.0, "C2", 0.8, 1.0],
                    [1.5, "D2", 0.8, 1.0],
                    [2.0, "E2", 0.8, 1.0],
                    [2.5, "F2", 0.8, 1.0],
                    [3.0, "G2", 0.8, 1.0],
                    [3.5, "A2", 0.8, 1.0],
                    [4.0, "B2", 0.8, 1.0],
                    [4.5, "C3", 0.8, 1.0]
                ],
                "rootNotes": [
                    [1.0, "C2", 0.8, 1.0],
                    [3.0, "F2", 0.8, 1.0]
                ],
                "rhythmicPattern": [
                    [1.0, "C2", 0.8, 1.0],
                    [1.75, "C2", 0.8, 1.0],
                    [2.5, "E2", 0.8, 1.0],
                    [3.0, "G2", 0.8, 1.0],
                    [4.0, "C2", 0.8, 1.0],
                    [4.5, "G2", 0.8, 1.0]
                ],
                "slapBass": [
                    [1.0, "E2", 0.8, 1.0],
                    [1.25, "E2", 0.8, 1.0],
                    [1.5, "G2", 0.8, 1.0],
                    [2.0, "A2", 0.8, 1.0],
                    [2.5, "G2", 0.8, 1.0],
                    [3.0, "E2", 0.8, 1.0],
                    [4.0, "C2", 0.8, 1.0]
                ]
            },
            "sections": {
                "verse": {
                    "bars": 4,
                    "basePattern": "rootNotes",
                    "variations": {
                        "bar4": "walkingBass"
                    },
                    "layers": {
                        "fundamentals": {"pattern": "rootNotes"},
                        "rhythmLayer": [
                            [2.0, "C2", 0.8, 1.0],
                            [4.0, "F2", 0.8, 1.0]
                        ]
                    }
                },
                "chorus": {
                    "bars": 4,
                    "barPatterns": [
                        [
                            [1.0, "G1", 0.8, 1.0],
                            [1.5, "G2", 0.8, 1.0],
                            [2.0, "D2", 0.8, 1.0],
                            [3.0, "G2", 0.8, 1.0],
                            [4.0, "B2", 0.8, 1.0]
                        ],
                        [
                            [1.0, "A1", 0.8, 1.0],
                            [2.0, "C2", 0.8, 1.0],
                            [3.0, "E2", 0.8, 1.0],
                            [4.0, "A2", 0.8, 1.0]
                        ],
                        [
                            [1.0, "F1", 0.8, 1.0],
                            [2.0, "A2", 0.8, 1.0],
                            [3.0, "C2", 0.8, 1.0],
                            [4.0, "F2", 0.8, 1.0]
                        ],
                        [
                            [1.0, "C2", 0.8, 1.0],
                            [1.5, "E2", 0.8, 1.0],
                            [2.0, "G2", 0.8, 1.0],
                            [3.0, "C3", 0.8, 1.0],
                            [4.0, "G2", 0.8, 1.0]
                        ]
                    ]
                },
                "breakdown": {
                    "layers": {
                        "slapGroove": {"pattern": "slapBass"},
                        "rhythmicAccents": {"pattern": "rhythmicPattern"}
                    }
                }
            },
            "arrangement": ["verse", "chorus", "verse", "chorus", "breakdown", "chorus"]
        })";
        }
        else
        {
            // Default integration pattern for other instruments
            jsonPattern = R"({
                "title": "Integration_Default_Chart",
                "tempo": 120,
                "sections": {
                    "main": [
                        [1.0, "C4", 0.8, 1.0],
                        [2.0, "E4", 0.8, 1.0],
                        [3.0, "G4", 0.8, 1.0],
                        [4.0, "C5", 0.8, 1.0]
                    ]
                }
            })";
        }
    }
    else
    {
        // Default fallback pattern
        jsonPattern = R"({
            "title": "Default",
            "tempo": 120,
            "sections": {
                "main": [
                    [1.0, "C4", 0.8, 1.0],
                    [2.0, "E4", 0.8, 1.0],
                    [3.0, "G4", 0.8, 1.0],
                    [4.0, "C5", 0.8, 1.0]
                ]
            }
        })";
    }
    
    return jsonPattern;
}

void PlayerModeComponent::updatePatternViewer()
{
    if (currentPatternJson.isEmpty())
    {
        patternViewer.setText("No pattern selected");
        return;
    }
    
    // Show the JSON pattern (like LLM output)
    juce::String text = "Pattern JSON (like LLM output):\n\n";
    text += currentPatternJson;
    
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
    // Clear current pattern
    currentPatternJson = "";
    
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
    juce::String testType;
    
    switch (patternType)
    {
        case 1: testType = "T1_Notes"; break;
        case 2: testType = "T2_MultiBars"; break;
        case 3: testType = "T3_Chords"; break;
        case 4: testType = "T4_Layers"; break;
        case 5: testType = "T5_LayersChords"; break;
        case 6: testType = "T6_Sections"; break;
        case 7: testType = "T7_References"; break;
        case 8: testType = "T8_Exceptions"; break;
        case 9: testType = "Integration"; break;
        default: testType = "T1_Notes"; break;
    }
    
    // Get the instrument name
    juce::String instrumentName;
    switch (instrumentType)
    {
        case 1: instrumentName = "drums"; break;
        case 2: instrumentName = "bass"; break;
        case 3: instrumentName = "piano"; break;
        case 4: instrumentName = "guitar"; break;
        case 5: // Other/Unknown - default to piano
        default: instrumentName = "piano"; break;
    }
    
    // Generate the JSON pattern on-demand
    currentPatternJson = createTestPattern(testType, instrumentName);
    
    // Update the pattern viewer
    updatePatternViewer();
    
    // Update button states for the new workflow
    updateButtonStates();
}

void PlayerModeComponent::updateButtonStates()
{
    bool hasPattern = !currentPatternJson.isEmpty();
    bool hasMidiChart = !currentMidiChart.isVoid();
    bool hasPlayer = (selectedPlayer != nullptr);
    
    // OLD WORKFLOW: Convert → Load → Play (for compatibility/testing)
    convertButton.setEnabled(hasPattern && hasPlayer);
    loadButton.setEnabled(hasMidiChart && hasPlayer);
    
    // Play button is only enabled when chart is loaded
    playButton.setEnabled(hasMidiChart && hasPlayer && selectedPlayer->isInitialized());
    stopButton.setEnabled(hasPlayer && selectedPlayer && selectedPlayer->isPlaying());
}

void PlayerModeComponent::convertToMidiChart()
{
    if (currentPatternJson.isEmpty() || selectedPlayer == nullptr)
    {
        std::cerr << "PlayerMode::convertToMidiChart - No pattern or player selected" << std::endl;
        return;
    }
    
    std::cout << "Step 1: Preparing pattern..." << std::endl;
    
    // Step 1: Prepare the pattern
    if (!selectedPlayer->preparePattern(currentPatternJson))
    {
        std::cerr << "Failed to prepare pattern" << std::endl;
        patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to prepare pattern!");
        return;
    }
    
    std::cout << "Step 2: Converting to MIDI..." << std::endl;
    
    // Step 2: Convert to MIDI (handled internally by Player)
    if (!selectedPlayer->convertToMidi())
    {
        std::cerr << "Failed to convert to MIDI" << std::endl;
        patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to convert to MIDI!");
        return;
    }
    
    // Mark that we have a MIDI chart ready
    currentMidiChart = juce::var(true); // Just a flag to enable the load button
    
    // Update the pattern viewer to show conversion results
    patternViewer.setText("Pattern successfully converted to MIDI!\n\nClick 'Load' to prepare for playback.");
    
    std::cout << "Conversion successful - ready to load" << std::endl;
    
    // Update button states
    updateButtonStates();
}

void PlayerModeComponent::loadMidiChart()
{
    if (currentMidiChart.isVoid() || selectedPlayer == nullptr)
    {
        std::cerr << "PlayerMode::loadMidiChart - No MIDI chart or player available" << std::endl;
        return;
    }
    
    std::cout << "Step 3: Loading MIDI into instrument..." << std::endl;
    
    // Step 3: Load the prepared MIDI (handled internally by Player)
    if (!selectedPlayer->loadPreparedMidi())
    {
        std::cerr << "Failed to load MIDI" << std::endl;
        patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to load MIDI!");
        return;
    }
    
    // Update pattern viewer
    patternViewer.setText(patternViewer.getText() + "\n\nMIDI loaded successfully! Click 'Play' to start.");
    
    std::cout << "MIDI loaded - ready to play" << std::endl;
    
    // Update button states
    updateButtonStates();
}

void PlayerModeComponent::playLoadedChart()
{
    std::cout << "🎵 PlayerModeComponent::playLoadedChart() called!" << std::endl;
    
    if (selectedPlayer == nullptr)
    {
        std::cerr << "❌ PlayerMode::playLoadedChart - No player available" << std::endl;
        return;
    }
    
    // Ensure player is initialized before playback
    if (!selectedPlayer->isInitialized())
    {
        std::cout << "Initializing player..." << std::endl;
        if (!selectedPlayer->initialize())
        {
            std::cerr << "Failed to initialize player" << std::endl;
            patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to initialize player!");
            return;
        }
    }
    
    std::cout << "✅ Starting playback on player..." << std::endl;
    
    // Simply play the loaded chart
    if (selectedPlayer->play())
    {
        // Update button states
        playButton.setEnabled(false);
        stopButton.setEnabled(true);
        
        std::cout << "Started playback on player " << selectedPlayer->getMetadata().name << std::endl;
        
        // Update pattern viewer
        patternViewer.setText(patternViewer.getText() + "\n\nPlayback started!");
    }
    else
    {
        std::cerr << "Failed to start playback - no chart loaded?" << std::endl;
        patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to start playback!");
    }
}

// Removed playPatternDirectly method - using state machine workflow

void PlayerModeComponent::updateSelectedPlayer()
{
    // Check if we have a selected player
    if (selectedPlayer == nullptr)
    {
        std::cout << "No player selected" << std::endl;
        return;
    }
    
    std::cout << "Using Player: " << selectedPlayer->getName() << std::endl;
}



} // namespace covertracks
