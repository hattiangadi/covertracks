#include "TestsComponent.h"
#include "../Tests/TestRunner.h"
#include <sstream>
#include <iostream>

namespace covertracks {

TestsComponent::TestsComponent()
    : titleLabel("titleLabel", "Unit Tests"),
      runButton("Run Tests"),
      clearButton("Clear Results"),
      audioTestLabel("audioTestLabel", "Tracktion Engine Audio Tests")
{
    // Set up title label
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("bold")));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up buttons
    runButton.addListener(this);
    addAndMakeVisible(runButton);
    
    clearButton.addListener(this);
    addAndMakeVisible(clearButton);
    
    // Set up results text area
    resultsText.setMultiLine(true);
    resultsText.setReadOnly(true);
    resultsText.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(12.0f)));
    resultsText.setText("Click 'Run Tests' to execute PatternParser unit tests...");
    addAndMakeVisible(resultsText);
    
    // Set up audio test section
    audioTestLabel.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("bold")));
    audioTestLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(audioTestLabel);
    
    // Set up audio test buttons
    sineWaveButton.setButtonText("Generate 440Hz Sine Wave (2s)");
    sineWaveButton.addListener(this);
    addAndMakeVisible(sineWaveButton);
    
    twoTonesButton.setButtonText("Generate Two Tones (440Hz + 660Hz)");
    twoTonesButton.addListener(this);
    addAndMakeVisible(twoTonesButton);
    
    stopAudioButton.setButtonText("Stop All Audio");
    stopAudioButton.addListener(this);
    addAndMakeVisible(stopAudioButton);
    
    // Audio status label
    audioStatusLabel.setText("Ready to test Tracktion Engine audio", juce::dontSendNotification);
    audioStatusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(audioStatusLabel);
    
    // Test runner is now handled by the runAllTests() function
}

TestsComponent::~TestsComponent()
{
    stopAllAudio();
    
    runButton.removeListener(this);
    clearButton.removeListener(this);
    sineWaveButton.removeListener(this);
    twoTonesButton.removeListener(this);
    stopAudioButton.removeListener(this);
}

void TestsComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw a border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void TestsComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Split the component into two sections: left for unit tests, right for audio tests
    auto leftSection = bounds.removeFromLeft(bounds.getWidth() / 2);
    bounds.removeFromLeft(10); // spacing between sections
    auto rightSection = bounds;
    
    // === LEFT SECTION: Unit Tests ===
    // Position title label at the top
    titleLabel.setBounds(leftSection.removeFromTop(30));
    
    // Add spacing
    leftSection.removeFromTop(10);
    
    // Position buttons
    auto buttonRow = leftSection.removeFromTop(30);
    runButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(10); // spacing
    clearButton.setBounds(buttonRow.removeFromLeft(100));
    
    // Add spacing
    leftSection.removeFromTop(10);
    
    // Position results text area to fill the remaining space
    resultsText.setBounds(leftSection);
    
    // === RIGHT SECTION: Audio Tests ===
    // Position audio test label at the top
    audioTestLabel.setBounds(rightSection.removeFromTop(30));
    
    // Add spacing
    rightSection.removeFromTop(10);
    
    // Position audio test buttons
    sineWaveButton.setBounds(rightSection.removeFromTop(40));
    rightSection.removeFromTop(10);
    
    twoTonesButton.setBounds(rightSection.removeFromTop(40));
    rightSection.removeFromTop(10);
    
    stopAudioButton.setBounds(rightSection.removeFromTop(40));
    rightSection.removeFromTop(20);
    
    // Position audio status label
    audioStatusLabel.setBounds(rightSection.removeFromTop(30));
}

void TestsComponent::buttonClicked(juce::Button* button)
{
    if (button == &runButton)
    {
        runTests();
    }
    else if (button == &clearButton)
    {
        resultsText.setText("Click 'Run Tests' to execute PatternParser and PatternGenerator unit tests...");
    }
    else if (button == &sineWaveButton)
    {
        audioStatusLabel.setText("Generating 440Hz sine wave...", juce::dontSendNotification);
        generateSineWave(440.0f, 2.0f);
    }
    else if (button == &twoTonesButton)
    {
        audioStatusLabel.setText("Generating two simultaneous tones...", juce::dontSendNotification);
        generateTwoTones();
    }
    else if (button == &stopAudioButton)
    {
        audioStatusLabel.setText("Stopping audio...", juce::dontSendNotification);
        stopAllAudio();
    }
}

void TestsComponent::runTests()
{
    resultsText.setText("Running tests...\n");
    
    // Capture console output for display
    std::ostringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());
    
    // Run all tests (this will print to our captured output)
    covertracks::runAllTests();
    
    // Restore console output
    std::cout.rdbuf(old_cout);
    
    // Display the captured output
    resultsText.setText(output.str());
}

void TestsComponent::generateSineWave(float frequency, float duration)
{
    try 
    {
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        
        if (!currentEdit)
        {
            audioStatusLabel.setText("ERROR: No Tracktion Edit available", juce::dontSendNotification);
            return;
        }
        
        // Stop any existing playback
        stopAllAudio();
        
        // Create an audio track (Tracktion tracks can handle both audio and MIDI)
        auto audioTrack = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
        if (!audioTrack)
        {
            audioStatusLabel.setText("ERROR: Failed to create audio track", juce::dontSendNotification);
            return;
        }
        
        audioTrack->setName("Test MIDI Track");
        testTrack = audioTrack.get();
        
        // Add a built-in synth plugin to the track for MIDI playback
        try {
            // Create a simple built-in synth (FourOscPlugin)
            juce::PluginDescription synthDesc;
            synthDesc.name = "4-Oscillator Synth";
            synthDesc.fileOrIdentifier = "1_trkbuiltin";
            synthDesc.pluginFormatName = te::PluginManager::builtInPluginFormatName;
            synthDesc.category = te::FourOscPlugin::xmlTypeName;
            synthDesc.isInstrument = true;
            
            // Create the plugin using Tracktion's plugin cache
            if (auto synthPlugin = currentEdit->getPluginCache().createNewPlugin(te::FourOscPlugin::xmlTypeName, synthDesc))
            {
                // Add the plugin to the track
                audioTrack->pluginList.insertPlugin(synthPlugin, 0, nullptr);
                audioStatusLabel.setText("Added built-in synth: " + synthDesc.name, juce::dontSendNotification);
            }
            else
            {
                audioStatusLabel.setText("Failed to create built-in synth", juce::dontSendNotification);
            }
        }
        catch (const std::exception& e)
        {
            audioStatusLabel.setText("Error creating synth: " + juce::String(e.what()), juce::dontSendNotification);
        }
        
        // Create a simple MIDI clip with a sustained note using proper TimeRange
        te::TimePosition start = te::TimePosition::fromSeconds(0.0);
        te::TimePosition end = te::TimePosition::fromSeconds(duration);
        te::TimeRange timeRange(start, end);
        
        auto midiClip = audioTrack->insertMIDIClip("Test Note", timeRange, nullptr);
        if (midiClip)
        {
            // Add a MIDI note - convert frequency to MIDI note number (approximately)
            int midiNote = static_cast<int>(69 + 12 * std::log2(frequency / 440.0)); // A4 = 440Hz = MIDI note 69
            
            auto& sequence = midiClip->getSequence();
            // Convert time values to proper Tracktion beat positions
            te::BeatPosition startBeat = te::BeatPosition::fromBeats(0.0);
            te::BeatDuration noteDuration = te::BeatDuration::fromBeats(duration * 2.0); // Rough conversion
            sequence.addNote(midiNote, startBeat, noteDuration, 100, 0, nullptr);
            
            audioStatusLabel.setText("Created MIDI note " + juce::String(midiNote) + " (" + juce::String(frequency, 1) + "Hz)", juce::dontSendNotification);
        }
        
        // Start playback
        auto& transport = currentEdit->getTransport();
        transport.setPosition(te::TimePosition::fromSeconds(0.0));
        transport.play(false);
        isAudioPlaying = true;
        
        // Schedule stop after duration
        juce::Timer::callAfterDelay(static_cast<int>(duration * 1000), [this]() 
        {
            stopAllAudio();
            audioStatusLabel.setText("MIDI test completed", juce::dontSendNotification);
        });
    }
    catch (const std::exception& e)
    {
        audioStatusLabel.setText("ERROR: " + juce::String(e.what()), juce::dontSendNotification);
    }
}

void TestsComponent::generateTwoTones()
{
    try 
    {
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        
        if (!currentEdit)
        {
            audioStatusLabel.setText("ERROR: No Tracktion Edit available", juce::dontSendNotification);
            return;
        }
        
        // Stop any existing playback
        stopAllAudio();
        
        // Create two audio tracks for two different tones
        auto track1 = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
        auto track2 = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
        
        if (!track1 || !track2)
        {
            audioStatusLabel.setText("ERROR: Failed to create audio tracks", juce::dontSendNotification);
            return;
        }
        
        track1->setName("Test Track 1 (440Hz)");
        track2->setName("Test Track 2 (660Hz)");
        
        // Add built-in synth plugins to both tracks
        try {
            juce::PluginDescription synthDesc;
            synthDesc.name = "4-Oscillator Synth";
            synthDesc.fileOrIdentifier = "1_trkbuiltin";
            synthDesc.pluginFormatName = te::PluginManager::builtInPluginFormatName;
            synthDesc.category = te::FourOscPlugin::xmlTypeName;
            synthDesc.isInstrument = true;
            
            // Add synth to both tracks
            if (auto synth1 = currentEdit->getPluginCache().createNewPlugin(te::FourOscPlugin::xmlTypeName, synthDesc))
            {
                track1->pluginList.insertPlugin(synth1, 0, nullptr);
            }
            
            if (auto synth2 = currentEdit->getPluginCache().createNewPlugin(te::FourOscPlugin::xmlTypeName, synthDesc))
            {
                track2->pluginList.insertPlugin(synth2, 0, nullptr);
            }
        }
        catch (const std::exception& e)
        {
            audioStatusLabel.setText("Error creating synths: " + juce::String(e.what()), juce::dontSendNotification);
            return;
        }
        
        // Create MIDI clips with different notes
        const float duration = 3.0f;
        
        // Create proper time range
        te::TimePosition start = te::TimePosition::fromSeconds(0.0);
        te::TimePosition end = te::TimePosition::fromSeconds(duration);
        te::TimeRange timeRange(start, end);
        
        // Track 1: 440Hz (A4 = MIDI note 69)
        auto clip1 = track1->insertMIDIClip("440Hz Note", timeRange, nullptr);
        if (clip1)
        {
            auto& seq1 = clip1->getSequence();
            te::BeatPosition startBeat = te::BeatPosition::fromBeats(0.0);
            te::BeatDuration noteDuration = te::BeatDuration::fromBeats(duration * 2.0);
            seq1.addNote(69, startBeat, noteDuration, 80, 0, nullptr); // A4 = 440Hz
        }
        
        // Track 2: 660Hz (approximately E5 = MIDI note 76)
        auto clip2 = track2->insertMIDIClip("660Hz Note", timeRange, nullptr); 
        if (clip2)
        {
            auto& seq2 = clip2->getSequence();
            int midiNote = static_cast<int>(69 + 12 * std::log2(660.0 / 440.0)); // Convert 660Hz to MIDI note
            te::BeatPosition startBeat = te::BeatPosition::fromBeats(0.0);
            te::BeatDuration noteDuration = te::BeatDuration::fromBeats(duration * 2.0);
            seq2.addNote(midiNote, startBeat, noteDuration, 80, 0, nullptr);
        }
        
        // Start playback
        auto& transport = currentEdit->getTransport();
        transport.setPosition(te::TimePosition::fromSeconds(0.0));
        transport.play(false);
        isAudioPlaying = true;
        
        audioStatusLabel.setText("Playing two tones: 440Hz + 660Hz", juce::dontSendNotification);
        
        // Schedule stop after duration
        juce::Timer::callAfterDelay(3000, [this]() 
        {
            stopAllAudio();
            audioStatusLabel.setText("Two tones test completed", juce::dontSendNotification);
        });
    }
    catch (const std::exception& e)
    {
        audioStatusLabel.setText("ERROR: " + juce::String(e.what()), juce::dontSendNotification);
    }
}

void TestsComponent::stopAllAudio()
{
    try 
    {
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        
        if (currentEdit && isAudioPlaying)
        {
            auto& transport = currentEdit->getTransport();
            transport.stop(false, false);
            isAudioPlaying = false;
            
            // Clear all test tracks
            for (auto track : currentEdit->getTrackList())
            {
                if (track->getName() == "Test Audio Track")
                {
                    currentEdit->deleteTrack(track);
                    break;
                }
            }
            
            testTrack = nullptr;
            audioStatusLabel.setText("Audio stopped", juce::dontSendNotification);
        }
    }
    catch (const std::exception& e)
    {
        audioStatusLabel.setText("ERROR stopping audio: " + juce::String(e.what()), juce::dontSendNotification);
    }
}

} // namespace covertracks