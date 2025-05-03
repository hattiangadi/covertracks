#include "MainComponent.h"
#include "PlaybackDemoAudio.h"

// Need various tracktion headers, some might be implicitly included via tracktion_engine.h
#include "tracktion_engine/tracktion_engine.h"

// Include for TemporaryFile
#include <juce_core/juce_core.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace te = tracktion;

//==============================================================================
// Implementation for the nested PluginListDialogComponent
//==============================================================================
MainComponent::PluginListDialogComponent::PluginListDialogComponent (juce::AudioPluginFormatManager& fm, juce::KnownPluginList& kl)
    : formatManager (fm), knownPluginList (kl)
{
    // Create the PluginListComponent (false for OK/Cancel as we add our own)
    pluginListComponent = std::make_unique<juce::PluginListComponent>(formatManager,
                                                                        knownPluginList,
                                                                        juce::File{},
                                                                        nullptr,
                                                                        false); // Don't show built-in OK/Cancel
    addAndMakeVisible (*pluginListComponent);

    addAndMakeVisible (okButton);
    okButton.addListener (this);

    addAndMakeVisible (cancelButton);
    cancelButton.addListener (this);
}

void MainComponent::PluginListDialogComponent::resized()
{
    auto bounds = getLocalBounds();
    auto buttonArea = bounds.removeFromBottom (40);
    okButton.setBounds (buttonArea.removeFromRight (80).reduced (5));
    cancelButton.setBounds (buttonArea.removeFromRight (80).reduced (5));
    pluginListComponent->setBounds (bounds.reduced (10));
}

void MainComponent::PluginListDialogComponent::buttonClicked (juce::Button* button)
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        if (button == &okButton)
        {
            auto& table = pluginListComponent->getTableListBox();
            int selectedRow = table.getSelectedRow();

            // Check if selection is valid and not a blacklisted item
            // (Assuming blacklisted are appended, adjust logic if needed based on TableModel)
            if (selectedRow >= 0 && selectedRow < knownPluginList.getNumTypes())
            {
                selectedPluginDesc = knownPluginList.getTypes()[selectedRow];
                dw->exitModalState (1); // OK code (non-zero)
            }
            else
            {
                // Optionally show a message if nothing valid is selected?
                // For now, just don't close the dialog on OK if invalid.
                // Consider using NativeMessageBox::showMessageBoxAsync for feedback
            }
        }
        else if (button == &cancelButton)
        {
            dw->exitModalState (0); // Cancel code (zero)
        }
    }
}

juce::PluginDescription MainComponent::PluginListDialogComponent::getSelectedPlugin() const
{
     return selectedPluginDesc;
}


//==============================================================================
MainComponent::MainComponent (tracktion::Engine& engineIn)
    : engine(engineIn),
      edit(tracktion::createEmptyEdit(engine, {})),
      knownPluginList(engine.getPluginManager().knownPluginList), // Match header order
      selectionManager(engine), // Match header order
      formatManager(),          // Match header order
      // audioFileChooser(), // Not initialized yet
      playButton("Play"),       // Match header order
      stopButton("Stop"),       // Match header order
      addButton("Add Plugin"),  // Initialize missing button & match order
      showPluginButton("Show Plugins") // Match header order & text
{
    // Initialize AudioPluginFormatManager - add default formats
    formatManager.addDefaultFormats();

    // KnownPluginList setup
    knownPluginList.addChangeListener(this);

    // Register as a listener to the transport control
    edit->getTransport().addChangeListener (this);

    addAndMakeVisible (playButton);
    playButton.onClick = [this] { edit->getTransport().play (false); };

    addAndMakeVisible (stopButton);
    stopButton.onClick = [this] { edit->getTransport().stop (false, false); };

    addAndMakeVisible (showPluginButton); // Match header order & text
    showPluginButton.onClick = [this] { showPluginList(); }; // Match header order & text

    // Make add button visible and set callback (placeholder for now)
    addAndMakeVisible(addButton);
    addButton.onClick = [this] { /* Placeholder: Maybe show file chooser or plugin list? */ };

    setSize (800, 600);
}

MainComponent::~MainComponent()
{
    // Unregister from the transport control before the edit is destroyed
    if (edit)
        edit->getTransport().removeChangeListener (this);
    // Ensure any file choosers are destroyed before the UI component is fully gone
    chooser = nullptr;
    audioFileChooser = nullptr;
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // No drawing needed for now, just clear background.
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto buttonBounds = bounds.removeFromTop (30);
    playButton.setBounds (buttonBounds.removeFromLeft (100));
    buttonBounds.removeFromLeft (10);
    stopButton.setBounds (buttonBounds.removeFromLeft (100));
    buttonBounds.removeFromLeft (10);
    showPluginButton.setBounds (buttonBounds.removeFromLeft (100)); // Match header order & text
    buttonBounds.removeFromLeft (10);
    addButton.setBounds (buttonBounds.removeFromLeft (100)); // Position add button

    juce::FlexBox fb;
    fb.items.add (juce::FlexItem (showPluginButton).withFlex (1.0f)); // Match header order & text
    fb.items.add (juce::FlexItem (stopButton).withFlex (1.0f));
    fb.performLayout (bounds.reduced (10));
}

//==============================================================================
void MainComponent::createOrLoadEdit()
{
    // edit = tracktion::createEmptyEdit(engine, {}); // Already done in constructor
    jassert(edit != nullptr);

    // Ensure at least one track exists
    if (edit->getTrackList().isEmpty())
    {
        // Example: Load from a file or create new
        // For now, just ensure a basic structure
        edit->ensureNumberOfAudioTracks (2);
    }
}

void MainComponent::createTracksAndClips (tracktion::Edit& editToUse)
{
    auto& transport = editToUse.getTransport();
    transport.setLoopRange (tracktion::TimeRange (tracktion::TimePosition::fromSeconds (0.0), tracktion::TimePosition::fromSeconds (4.0)));
    transport.looping = true;
    transport.position = tracktion::TimePosition::fromSeconds (0.0);
    transport.play (false);

    // Ensure two audio tracks exist and get references to them
    editToUse.ensureNumberOfAudioTracks (2);
    auto audioTracks = tracktion::getAudioTracks (editToUse);
    if (audioTracks.size() < 2)
    {
        jassertfalse; // Should not happen after ensureNumberOfAudioTracks
        return;
    }
    auto track1 = audioTracks[0];
    auto track2 = audioTracks[1];

    if (track1 && track2)
    {
        // Create a temporary file to hold the audio data
        juce::TemporaryFile tempFile (juce::File::createTempFile (".ogg"));
        juce::MemoryInputStream stream (PlaybackDemoAudio::guitar_loop_ogg,
                                      PlaybackDemoAudio::guitar_loop_oggSize,
                                      false);
        tempFile.getFile().replaceWithData (stream.getData(), stream.getDataSize());

        // Create AudioFile from the temporary file
        tracktion::AudioFile audioFile (engine, tempFile.getFile());

        if (audioFile.isValid())
        {
            // Add a clip to track1
            te::TimePosition start = te::TimePosition::fromSeconds (0.0); // Using te:: alias
            // Revert to getLength() and convert using fromSeconds()
            te::TimeDuration duration = te::TimeDuration::fromSeconds(audioFile.getLength());
            tracktion::TimeRange clipRange (start, duration);
            tracktion::TimeRange loopRange (start, duration);

            auto clip1 = track1->insertNewClip (tracktion::TrackItem::Type::wave,
                                                "Wave Clip",
                                                clipRange,
                                                nullptr); // selectionManager = nullptr

            // No .get() needed, clip1 is Clip*
            if (auto waveClip = dynamic_cast<tracktion::WaveAudioClip*> (clip1))
            {
                // Clip properties usually set at creation or via specific methods
            }

            // Add a clip to track2, this time looping
            start = tracktion::TimePosition::fromSeconds(4.0);
            duration = tracktion::TimeDuration::fromSeconds(2.0);
            clipRange = tracktion::TimeRange (start, start + duration);

            // Use 4-arg overload with nullptr for SelectionManager
            auto clip2 = track2->insertNewClip (tracktion::TrackItem::Type::wave,
                                                "Loop Clip",
                                                clipRange,
                                                nullptr); // selectionManager = nullptr

            // No .get() needed, clip2 is Clip*
            if (auto waveClip = dynamic_cast<tracktion::WaveAudioClip*> (clip2))
            {
                // Set loop range after creation using the specific method
                waveClip->setLoopRange (loopRange);
            }

            // Find Track 1 by iterating
            te::AudioTrack* track1 = nullptr;
            for (auto* track : editToUse.getTrackList())
            {
                if (auto* audioTrack = dynamic_cast<te::AudioTrack*> (track))
                {
                    if (audioTrack->getName() == "Track 1")
                    {
                        track1 = audioTrack;
                        break;
                    }
                }
            }

            // Set parameter on Track 1 if found
            if (track1)
            {
                if (auto* vol = dynamic_cast<te::VolumeAndPanPlugin*> (track1->pluginList.getPluginsOfType<te::VolumeAndPanPlugin>().getFirst()))
                {
                    auto gainParam = vol->getAutomatableParameterByID (te::IDs::volume.toString());
                    jassert (gainParam != nullptr); 

                    if (gainParam)
                    {
                        gainParam->setParameter (0.5f, juce::NotificationType::dontSendNotification);
                        DBG("Volume plugin gain set to 0.5");
                    }
                    else
                    {
                        DBG("Gain parameter not found");
                    }
                }
                else
                {
                    DBG("Volume plugin not found on Track 1");
                }
            }
            else
            {
                DBG("Track 1 not found");
            }
        }

        // Add a level meter plugin to track 2
        if (track2 != nullptr)
        {
            // Create built-in plugin via Edit's PluginCache
            if (auto meterPluginPtr = editToUse.getPluginCache().createNewPlugin (tracktion::LevelMeterPlugin::xmlTypeName, {}))
                // Insert into track list
                track2->pluginList.insertPlugin (meterPluginPtr, 0, nullptr);
        }
    }
}

void MainComponent::insertPluginFromDesc (const juce::PluginDescription& desc)
{
    if (!edit)
        return;

    te::AudioTrack* track1 = nullptr;
    for (auto* track : edit->getTrackList())
    {
        if (auto* audioTrack = dynamic_cast<te::AudioTrack*> (track))
        {
            if (audioTrack->getName() == "Track 1")
            {
                track1 = audioTrack;
                break;
            }
        }
    }

    if (!track1)
    {
        DBG("Track 1 not found for plugin insertion");
        return;
    }

    // Use createNewPlugin with format name and description - use EDIT's cache
    auto pluginInstance = edit->getPluginCache().createNewPlugin (desc.pluginFormatName, desc);

    if (pluginInstance)
    {
        track1->pluginList.insertPlugin (pluginInstance, -1, nullptr);
        DBG("Plugin inserted: " + desc.name);
    }
    else
    {
        DBG("Failed to create plugin instance: " + desc.name);
    }
}

void MainComponent::browseForAudioFile()
{ 
    // Initialize the member FileChooser
    audioFileChooser = std::make_unique<juce::FileChooser> ("Please select an audio file...",
                                                          juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                          "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
 
    int flags = juce::FileBrowserComponent::openMode |
                juce::FileBrowserComponent::canSelectFiles;
    audioFileChooser->launchAsync (flags, [&engine = this->engine, &edit = this->edit, this] (const juce::FileChooser& fc)
                           {
                               if (fc.getResults().isEmpty())
                               {
                                   DBG("File chooser cancelled or no file selected");
                                   return;
                               }

                               const juce::File file (fc.getResult());
                               DBG ("File chosen: " + file.getFullPathName());

                               if (!edit)
                               {
                                   DBG("Main edit is null");
                                   return;
                               }

                               auto* editToUse = edit.get(); // Use the main edit

                               // Find Track 1 by iterating
                               te::AudioTrack* track1 = nullptr;
                               for (auto* track : editToUse->getTrackList())
                               {
                                   if (auto* audioTrack = dynamic_cast<te::AudioTrack*> (track))
                                   {
                                       if (audioTrack->getName() == "Track 1")
                                       {
                                           track1 = audioTrack;
                                           break;
                                       }
                                   }
                               }

                               if (!track1)
                               {
                                   DBG("Track 1 not found");
                                   return;
                               }

                               // Check if the track already has clips and remove them
                               auto clipsToRemove = track1->getClips();
                               for (auto* clip : clipsToRemove)
                                   clip->removeFromParent(); // Use removeFromParent for track items

                               // Re-add clip creation logic
                               const tracktion::AudioFile audioFile (engine, file);
                               if (audioFile.isValid())
                               {
                                   auto clipTimeRange = tracktion::TimeRange (tracktion::TimePosition(),
                                                                          tracktion::TimeDuration::fromSeconds (audioFile.getLength())); // Start at 0

                                   tracktion::ClipPosition clipPos (clipTimeRange); // Construct from TimeRange directly

                                   // Try insertWaveClip again with the correct ClipPosition type
                                   juce::ReferenceCountedObjectPtr<te::WaveAudioClip> waveClip =
                                       track1->insertWaveClip (file.getFileNameWithoutExtension(),
                                                               file,
                                                               clipPos, // Use ClipPosition object
                                                               false); // moveExisting = false

                                   if (waveClip)
                                   {
                                       juce::Logger::writeToLog("Wave clip inserted: " + file.getFullPathName());
                                   }
                                   else
                                   {
                                       juce::Logger::writeToLog("Failed to insert wave clip: " + file.getFullPathName());
                                   }
                               }
                               else
                               {
                                    juce::Logger::writeToLog("Invalid audio file: " + file.getFullPathName());
                               }
                           });
}

void MainComponent::updatePlayButtonText()
{
    if (edit->getTransport().isPlaying())
    {
        playButton.setButtonText ("Stop");
    }
    else
    {
        playButton.setButtonText ("Play");
    }
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // Check if the change came from the transport control
    if (source == &edit->getTransport())
    {
        updatePlayButtonText();
    }
    // Potentially handle other change sources here in the future

    if (edit)
    {
        // Use fully qualified type name and dynamic_cast
        te::AudioTrack* track = nullptr;
        for (auto* t : edit->getTrackList())
        {
            if (auto* audioTrack = dynamic_cast<te::AudioTrack*> (t))
            {
                track = audioTrack; // Found the first audio track
                break;
            }
        }

        if (track) // Check if an audio track was found
        {
            // Do something with track if needed...
        }

        // Update button state based on transport
        // playButton.setToggleState (edit->getTransport().isPlaying(), juce::dontSendNotification); // Keep this commented? Redundant with updatePlayButtonText
    }
}

// Method to show the Plugin List Component modally
void MainComponent::showPluginList()
{
    // 1. Create the content component for the dialog
    auto dialogContent = std::make_unique<PluginListDialogComponent>(formatManager, knownPluginList);
    dialogContent->setSize(500, 400); // Set a default size for the content

    // 2. Create the DialogWindow
    juce::DialogWindow::LaunchOptions launchOptions;
    launchOptions.content.setOwned (dialogContent.release()); // Dialog takes ownership
    launchOptions.componentToCentreAround = this;
    launchOptions.dialogTitle = "Add Plugin";
    launchOptions.resizable = true;
    launchOptions.useNativeTitleBar = true;
    launchOptions.escapeKeyTriggersCloseButton = true;
    launchOptions.dialogBackgroundColour = juce::Colours::grey;

    // No callback needed here, result is checked after modal state

    // 3. Create the DialogWindow using LaunchOptions
    std::unique_ptr<juce::DialogWindow> dialogWindow (launchOptions.create());
    if (!dialogWindow)
    {
        jassertfalse; // Failed to create window
        return;
    }

    // Keep a pointer to the content component before running modal
    // as the DialogWindow owns it and we need it after it closes.
    PluginListDialogComponent* listComp = dynamic_cast<PluginListDialogComponent*>(launchOptions.content.get());
    if (!listComp)
    {
        jassertfalse; // Content component type mismatch or null
        return;
    }

    // 4. Launch the modal dialog using enterModalState on the window
    int modalResult = dialogWindow->runModalLoop(); // Use runModalLoop for standard modal behaviour

    // 5. Check the result after the dialog closes
    if (modalResult > 0) // Typically means 'OK' or a positive action
    {
        // DialogWindow is closed but listComp pointer should still be valid
        // as the DialogWindow object still exists until it goes out of scope.
        const juce::PluginDescription* selectedPlugin = listComp->getSelectedPlugin();

        if (selectedPlugin != nullptr)
        {
            // We have the selected plugin description!
            juce::Logger::writeToLog("Selected plugin: " + selectedPlugin->name);
            juce::Logger::writeToLog("Plugin UID: " + juce::String(selectedPlugin->uniqueId)); // Convert ID to String

            // Example: Insert plugin into the first audio track
            auto* track = tracktion::getAudioTracks (*edit)[0];
            if (track != nullptr)
            {
                // Note: This might trigger plugin scanning if not already done
                // Pass the PluginDescription object itself
                auto* newPlugin = edit->getPluginCache().createNewPlugin (selectedPlugin->pluginFormatName,
                                                                            *selectedPlugin);

                if (newPlugin != nullptr)
                {
                    track->pluginList.insertPlugin (newPlugin, 0, nullptr);
                    juce::Logger::writeToLog ("Plugin inserted successfully!");
                }
                else
                {
                    juce::Logger::writeToLog ("Failed to create plugin instance.");
                }
            }
            else
            {
                juce::Logger::writeToLog ("No audio track found to insert plugin.");
            }
        }
        else
        {
            juce::Logger::writeToLog("No plugin selected.");
        }
    }
    else
    {
        // Dialog was cancelled (modalResult == 0) or closed otherwise
        juce::Logger::writeToLog("Plugin selection cancelled.");
    }

    // dialogWindow unique_ptr goes out of scope here, deleting the window and its content component.
}


//==============================================================================
void MainComponent::buttonClicked (juce::Button* button)
{
    // Currently handled by lambdas, but required by inheritance
}
