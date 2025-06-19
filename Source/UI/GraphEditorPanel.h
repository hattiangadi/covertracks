/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Plugins/PluginGraph.h"

namespace covertracks {
class MainHostWindow;
class PlayerEditorComponent; // Forward declaration

//==============================================================================
/**
    A panel that displays and edits a PluginGraph.
*/
class GraphEditorPanel final : public Component,
                               public ChangeListener,
                               private Timer
{
public:
    //==============================================================================
    GraphEditorPanel (PluginGraph& graph);
    ~GraphEditorPanel() override;

    void createNewPlugin (const PluginDescriptionAndPreference&, Point<int> position);

    void paint (Graphics&) override;
    void resized() override;

    void mouseDown (const MouseEvent&) override;
    void mouseUp   (const MouseEvent&) override;
    void mouseDrag (const MouseEvent&) override;

    void changeListenerCallback (ChangeBroadcaster*) override;

    void setOwnerPlayerEditorComponent(PlayerEditorComponent* owner);

    //==============================================================================
    void updateComponents();

    //==============================================================================
    void showPopupMenu (Point<int> position, PlayerEditorComponent* playerEditorComponent = nullptr);

    //==============================================================================
    void beginConnectorDrag (AudioProcessorGraph::NodeAndChannel source,
                             AudioProcessorGraph::NodeAndChannel dest,
                             const MouseEvent&);
    void dragConnector (const MouseEvent&);
    void endDraggingConnector (const MouseEvent&);

    //==============================================================================
    PluginGraph& graph;

private:
    struct PluginComponent;
    struct ConnectorComponent;
    struct PinComponent;

    OwnedArray<PluginComponent> nodes;
    OwnedArray<ConnectorComponent> connectors;
    std::unique_ptr<ConnectorComponent> draggingConnector;
    std::unique_ptr<PopupMenu> menu;

    PluginComponent* getComponentForPlugin (AudioProcessorGraph::NodeID) const;
    ConnectorComponent* getComponentForConnection (const AudioProcessorGraph::Connection&) const;
    PinComponent* findPinAt (Point<float>) const;

    //==============================================================================
    Point<int> originalTouchPos;

    void timerCallback() override;

    PlayerEditorComponent* ownerPlayerEditorComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphEditorPanel)
};


//==============================================================================
/**
    A panel that embeds a GraphEditorPanel with a midi keyboard at the bottom.

    It also manages the graph itself, and plays it.
*/
class GraphDocumentComponent final : public Component,
                                     public DragAndDropTarget,
                                     public DragAndDropContainer,
                                     public ChangeListener,
                                     public MidiInputCallback,
                                     public Timer
{
    // Allow PlayerEditorComponent to access private members
    friend class PlayerEditorComponent;
public:
    // Modified constructor
    GraphDocumentComponent(AudioPluginFormatManager& fm,
                          KnownPluginList& kpl,
                          AudioEngine& engine);
    
    ~GraphDocumentComponent() override;
    
    // Plugin creation
    void createNewPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos);
    bool closeAnyOpenPluginWindows();
    
    // UI updates
    void updateComponents();
    
    // Resized override
    void resized() override;
    
    // ChangeListener implementation
    void changeListenerCallback(ChangeBroadcaster*) override;
    
    // Note: MidiKeyboardStateListener implementation has been moved to the KeyboardStateListener class
    
    // MidiInputCallback implementation
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;
    
    // Timer implementation
    void timerCallback() override;
    
    // Double precision setting
    void setDoublePrecision(bool doublePrecision);
    
    // Drag and drop
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDropped(const SourceDetails&) override;
    
    // Side panel management
    void showSidePanel(bool isSettingsPanel);
    void hideLastSidePanel();
    
    // Graph release
    void releaseGraph();
    
    // Graph pointer
    std::unique_ptr<PluginGraph> graph;
    
    // UI components
    std::unique_ptr<GraphEditorPanel> graphPanel;
    std::unique_ptr<MidiKeyboardComponent> keyboardComp;
    
    // AudioEngine reference (not owned)
    AudioEngine& audioEngine;
    
    // Edit mode audio processor wrapper for connecting to AudioEngine
    class EditModeAudioProcessor;
    std::unique_ptr<EditModeAudioProcessor> editModeProcessor;
    
private:
    // References to shared resources (not owned)
    KnownPluginList& pluginList;
    
    // Keyboard state for MIDI input
    MidiKeyboardState keyState;
    
    // MIDI message collector for handling MIDI messages with proper timing
    MidiMessageCollector midiCollector;
    
    // Custom MIDI keyboard state listener that forwards to both MidiMessageCollector and AudioEngine
    class KeyboardStateListener : public MidiKeyboardStateListener
    {
    public:
        KeyboardStateListener(MidiMessageCollector& collector, AudioEngine& engine) 
            : midiCollector(collector), audioEngine(engine) {}
        
        void handleNoteOn(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity) override
        {
            MidiMessage message = MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
            message.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            
            // Add to collector for timing
            midiCollector.addMessageToQueue(message);
            
            // Send directly to AudioEngine for immediate routing to all processors
            audioEngine.sendMidiMessage(message);
            
            std::cout << "KeyboardStateListener: Sent note on to AudioEngine - Channel: " << midiChannel 
                      << ", Note: " << midiNoteNumber 
                      << ", Velocity: " << velocity << std::endl;
        }
        
        void handleNoteOff(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity) override
        {
            MidiMessage message = MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);
            message.setTimeStamp(Time::getMillisecondCounterHiRes() * 0.001);
            
            // Add to collector for timing
            midiCollector.addMessageToQueue(message);
            
            // Send directly to AudioEngine for immediate routing to all processors
            audioEngine.sendMidiMessage(message);
            
            std::cout << "KeyboardStateListener: Sent note off to AudioEngine - Channel: " << midiChannel 
                      << ", Note: " << midiNoteNumber 
                      << ", Velocity: " << velocity << std::endl;
        }
        
    private:
        MidiMessageCollector& midiCollector;
        AudioEngine& audioEngine;
    };
    
    // The keyboard state listener instance
    std::unique_ptr<KeyboardStateListener> keyboardStateListener;
    
    // Track edit mode connection state
    bool editModeConnected = false;
    
    // Access to keyboard state for MIDI routing
    MidiKeyboardState& getMidiKeyboardState() { return keyState; }
    
    // Access to MIDI message collector for processors
    MidiMessageCollector& getMidiMessageCollector() { return midiCollector; }
    
    // Edit mode audio connection methods
    void connectEditModeToAudioEngine();
    void disconnectEditModeFromAudioEngine();
    
    // Status bar
    struct TooltipBar;
    std::unique_ptr<TooltipBar> statusBar;
    
    // Mobile UI components
    class TitleBarComponent;
    std::unique_ptr<TitleBarComponent> titleBarComponent;
    
    // Plugin list
    struct PluginListBoxModel;
    std::unique_ptr<PluginListBoxModel> pluginListBoxModel;
    ListBox pluginListBox;
    
    // Side panels
    SidePanel mobileSettingsSidePanel { "Settings", 300, true };
    SidePanel pluginListSidePanel    { "Plugins", 250, false };
    SidePanel* lastOpenedSidePanel = nullptr;
    
    // Utility methods
    void checkAvailableWidth();
    void updateMidiOutput();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphDocumentComponent)
};
} // namespace covertracks
