// Source/Musician/UI/PatternEditorComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/Musician.h"
#include "../Core/MusicPatternSystem.h"
#include "../Core/PatternParser.h"
#include "../../Player/Core/Player.h"
#include "../../Player/Core/PlayerManager.h"

namespace covertracks {

/**
 * @brief UI Component for editing musical patterns
 * 
 * This component provides an interface for editing and visualizing
 * musical patterns in the notation format.
 */
class PatternEditorComponent : public Component,
                              public Button::Listener,
                              public ComboBox::Listener,
                              private ChangeListener,
                              private Timer
{
public:
    /**
     * @brief Construct a new Pattern Editor Component
     * 
     * @param musician The musician to use for pattern handling
     * @param playerManager The player manager for player selection
     */
    PatternEditorComponent(Musician* musician, PlayerManager* playerManager = nullptr);
    
    /**
     * @brief Destroy the Pattern Editor Component
     */
    ~PatternEditorComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(Button* button) override;
    
    // ComboBox::Listener
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    
    // Pattern access
    void setPattern(const String& patternNotation);
    String getPatternNotation() const;
    
    // Playback position
    void setPlaybackPosition(double positionInBeats);
    void startPlaybackTracking(double tempo = 120.0); // Start continuous tracking
    void stopPlaybackTracking(); // Stop tracking
    
    // Notification functions
    void patternUpdated();
    
    // Listener interface for receiving events from the PatternEditorComponent
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void patternPlayRequested(PatternEditorComponent* editor) = 0;
        virtual void patternParseRequested(PatternEditorComponent* editor) = 0;
        virtual void patternLoadRequested(PatternEditorComponent* editor) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // ChangeListener
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    // Timer callback for playback tracking
    void timerCallback() override;
    
    // UI components
    TextEditor patternEditor;
    
    Label headerLabel;
    Label playerLabel;
    ComboBox playerSelector;
    TextButton parseButton;
    TextButton loadButton;
    TextButton playButton;
    TextButton stopButton;
    
    // Piano roll visualization
    class PianoRollComponent : public Component
    {
    public:
        PianoRollComponent();
        void setMidiEvents(const var& chart);
        void setPlaybackPosition(double positionInBeats); // Set current playback position
        void setScrollOffset(double offset); // Set scroll position
        double getScrollOffset() const; // Get current scroll position
        void paint(Graphics& g) override;
        void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
        
    private:
        struct NoteEvent
        {
            int note;
            double timestamp;
            double duration;
            String source;
        };
        
        Array<NoteEvent> events;
        double scrollOffset = 0.0; // Scroll position in bars
        double playbackPosition = -1.0; // Current playback position in beats (-1 = not playing)
        
        bool isWhiteKeyNote(int midiNote);
        String getMidiNoteName(int midiNote);
        Colour getSourceColour(const String& source);
    };
    
    std::unique_ptr<PianoRollComponent> pianoRoll;
    
    // References
    Musician* musician;
    PlayerManager* playerManager;
    Player* selectedPlayer;
    
    // State
    String currentPatternNotation;
    var currentParsedChart;
    
    // Playback tracking
    bool isTrackingPlayback = false;
    double playbackTempo = 120.0;
    double playbackStartTime = 0.0; // When playback started (in milliseconds)
    double playbackStartBeat = 0.0; // Beat position when tracking started
    
    // Helper methods
    void updatePlayerSelector();
    void parseCurrentPattern();
    void loadParsedPattern();
    void playLoadedPattern();
    void updatePianoRollVisualization();
    void stopCurrentPlayback();
    void onPlayerSelectionChanged();
    void autoScrollToPlaybackPosition(double currentBeat);
    
    // Listeners
    ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternEditorComponent)
};

} // namespace covertracks