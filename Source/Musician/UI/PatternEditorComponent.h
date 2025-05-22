// Source/Musician/UI/PatternEditorComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/Musician.h"
#include "../Core/MusicPatternSystem.h"
#include "../Core/PatternParser.h"

namespace covertracks {

/**
 * @brief UI Component for editing musical patterns
 * 
 * This component provides an interface for editing and visualizing
 * musical patterns in the notation format.
 */
class PatternEditorComponent : public Component,
                              public Button::Listener,
                              private ChangeListener
{
public:
    /**
     * @brief Construct a new Pattern Editor Component
     * 
     * @param musician The musician to use for pattern handling
     */
    PatternEditorComponent(Musician* musician);
    
    /**
     * @brief Destroy the Pattern Editor Component
     */
    ~PatternEditorComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(Button* button) override;
    
    // Pattern access
    void setPattern(const String& patternNotation);
    String getPatternNotation() const;
    
    // Notification functions
    void patternUpdated();
    
    // Listener interface for receiving events from the PatternEditorComponent
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void patternPlayRequested(PatternEditorComponent* editor) = 0;
        virtual void patternEditRequested(PatternEditorComponent* editor) = 0;
        virtual void patternGenerateRequested(PatternEditorComponent* editor) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // ChangeListener
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    // UI components
    TextEditor patternEditor;
    
    Label headerLabel;
    TextButton playButton;
    TextButton editButton;
    TextButton parseButton;
    TextButton generateButton;
    
    // Pattern visualization
    Component visualizationPanel;
    
    // References
    Musician* musician;
    
    // State
    String currentPatternNotation;
    std::shared_ptr<Song> currentSong;
    
    // Helper methods
    void updateVisualization();
    void parseCurrentPattern();
    
    // Listeners
    ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternEditorComponent)
};

} // namespace covertracks