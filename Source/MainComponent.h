#pragma once

#include <JuceHeader.h>
#include <memory>
#include "tracktion_engine/tracktion_engine.h"
#include "PlaybackDemoAudio.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
namespace te = tracktion;

class MainComponent  : public juce::Component,
                       public juce::ChangeListener,
                       public juce::Button::Listener
{
public:
    //==============================================================================
    MainComponent (te::Engine& engineIn);
    ~MainComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    //==============================================================================
    // ValueTree::Listener methods (Remove 'override' as we don't inherit)
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) {}
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) {}
    void valueTreeParentChanged (juce::ValueTree&) {}
    void valueTreeRedirected (juce::ValueTree&) {}

    //==============================================================================
    void buttonClicked (juce::Button* button) override;

private:
    //==============================================================================
    te::Engine& engine;
    std::unique_ptr<te::Edit> edit;
    juce::KnownPluginList& knownPluginList;
    te::SelectionManager selectionManager { engine };

    // Audio File Browsing
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::FileChooser> audioFileChooser;

    juce::TextButton playButton       { "Play" },
                     stopButton       { "Stop" },
                     addButton      { "Add Plugin" },
                     showPluginButton { "Show Plugin" };

    std::unique_ptr<juce::FileChooser> chooser;          // For plugin selection

    void createOrLoadEdit();
    void updatePlayButtonText(); 
    void createTracksAndClips (te::Edit& editToUse);
    void browseForAudioFile();
    void showPluginList();
    void insertPluginFromDesc (const juce::PluginDescription& desc);

    class PluginListDialogComponent  : public juce::Component, public juce::Button::Listener
    {
    public:
        PluginListDialogComponent (juce::AudioPluginFormatManager& fm, juce::KnownPluginList& kl);
        ~PluginListDialogComponent() override = default;

        void resized() override;
        void buttonClicked (juce::Button* button) override;
        juce::PluginDescription getSelectedPlugin() const;

    private:
        juce::AudioPluginFormatManager& formatManager;
        juce::KnownPluginList& knownPluginList;
        std::unique_ptr<juce::PluginListComponent> pluginListComponent;
        juce::TextButton okButton { "OK" }, cancelButton { "Cancel" };
        juce::PluginDescription selectedPluginDesc; // Store the result here
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
