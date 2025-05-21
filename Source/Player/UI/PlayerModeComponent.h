#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"

namespace covertracks {

/**
 * @brief Component for player mode functionality
 * 
 * This component provides UI for playing patterns on one or more players
 */
class PlayerModeComponent : public Component,
                            public Button::Listener,
                            public ComboBox::Listener
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
    
    // Load a pattern from a JSON file
    var loadPatternFromFile(const File& file);
    
private:
    // Reference to the player manager
    PlayerManager& playerManager;
    
    // UI components
    Label titleLabel;
    ComboBox playerSelector;
    TextButton playButton;
    TextButton stopButton;
    TextButton playAllButton;
    
    // Currently selected player
    Player* selectedPlayer = nullptr;
    
    // Currently loaded pattern
    var currentPattern;
    
    // Helper methods
    void updatePlayerSelector();
    void loadDefaultPattern();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerModeComponent)
};

} // namespace covertracks
