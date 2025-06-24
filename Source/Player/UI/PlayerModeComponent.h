#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
#include "../Core/TracktionPlayer.h"
#include "../../Musician/Core/MusicPatternSystem.h"
#include "../../Musician/Core/PatternParser.h"
#include "../../Musician/Core/MusicPatternComponents.h"

namespace covertracks {

/**
 * @brief Component for player mode functionality
 * 
 * This component provides UI for playing patterns on one or more players
 */
class PlayerModeComponent : public Component,
                            private Button::Listener,
                            private ComboBox::Listener,
                            private ChangeListener
{
public:
    PlayerModeComponent(PlayerManager& playerManager);
    ~PlayerModeComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // Button::Listener implementation
    void buttonClicked(Button* button) override;
    
    // ComboBox::Listener implementation
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    
    // ChangeListener implementation
    void changeListenerCallback(ChangeBroadcaster* source) override;
    

    
private:
    // Reference to the player manager
    PlayerManager& playerManager;
    
    // UI components
    Label titleLabel;
    ComboBox playerSelector;
    ComboBox instrumentTypeSelector;
    ComboBox patternSelector;
    TextButton convertButton;
    TextButton loadButton;
    TextButton playButton;
    TextButton stopButton;
    
    // Pattern visualization
    TextEditor patternViewer;
    
    // Currently selected player and pattern
    Player* selectedPlayer = nullptr;
    std::unique_ptr<TracktionPlayer> tracktionPlayer = nullptr;  // NEW: Actual audio-capable player
    std::shared_ptr<Chart> currentChart = nullptr;
    var currentMidiChart;  // MIDI_EVENT_CHART format
    
    // Pattern system for managing hierarchical patterns
    std::unique_ptr<MusicPatternSystem> patternSystem;
    
    // Pattern parser for text-based patterns
    std::unique_ptr<PatternParser> patternParser;
    
    // Helper methods
    void createDefaultPatterns();
    std::shared_ptr<Chart> createTestPattern(const juce::String& testType, const juce::String& instrument);
    
    void updatePatternViewer();
    void convertToMidiChart();      // Chart → MIDI_EVENT_CHART  
    void loadMidiChart();           // MIDI_EVENT_CHART → Player
    void playLoadedChart();         // Player::play() (OLD WORKFLOW)
    void playPatternDirectly();     // NEW: Direct pattern → PlayerManager playback
    

    
    // Helper methods
    void updatePlayerSelector();
    void updateCurrentChart();
    void updateButtonStates();
    void setupTracktionPlayerForSelected();  // NEW: Create TracktionPlayer from selected Player
    juce::String getMidiNoteName(int midiNoteNumber);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerModeComponent)
};

} // namespace covertracks
