#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
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
    TextButton playButton;
    TextButton stopButton;
    TextButton playAllButton;
    ToggleButton patternModeToggle;
    
    // Pattern visualization
    TextEditor patternViewer;
    
    // Currently selected player and pattern
    Player* selectedPlayer = nullptr;
    std::shared_ptr<Chart> currentChart = nullptr;
    
    // Pattern system for managing hierarchical patterns
    std::unique_ptr<MusicPatternSystem> patternSystem;
    
    // Pattern-based playback
    bool usePatternBasedPlayback = false;
    std::unique_ptr<PatternParser> patternParser;
    
    // Helper methods
    void createDefaultPatterns();
    std::shared_ptr<Chart> createDrumPattern(const juce::String& style);
    std::shared_ptr<Chart> createBassPattern(const juce::String& style);
    std::shared_ptr<Chart> createPianoPattern(const juce::String& style);
    std::shared_ptr<Chart> createGuitarPattern(const juce::String& style);
    
    void updatePatternViewer();
    void playCurrentPattern();
    void playPatternBasedPattern();
    std::shared_ptr<Chart> createPatternBasedChart(const juce::String& instrumentType, const juce::String& style);
    

    
    // Helper methods
    void updatePlayerSelector();
    void updateCurrentChart();
    juce::String getMidiNoteName(int midiNoteNumber);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerModeComponent)
};

} // namespace covertracks
