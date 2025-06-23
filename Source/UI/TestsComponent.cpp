#include "TestsComponent.h"
#include "../Tests/TestRunner.h"
#include "../Player/UI/TracktionPlayerEditor.h"
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
    
    pluginEditorButton.setButtonText("Plugin Chain Editor");
    pluginEditorButton.addListener(this);
    addAndMakeVisible(pluginEditorButton);
    
    // Audio status label
    audioStatusLabel.setText("Ready to test Tracktion Engine audio", juce::dontSendNotification);
    audioStatusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(audioStatusLabel);
    
    // Initialize plugin format manager
    formatManager.addDefaultFormats();
    
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
    pluginEditorButton.removeListener(this);
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
    rightSection.removeFromTop(10);
    
    pluginEditorButton.setBounds(rightSection.removeFromTop(40));
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
    else if (button == &pluginEditorButton)
    {
        showPluginEditor();
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
        audioStatusLabel.setText("Creating TracktionInstrument for " + juce::String(frequency, 1) + "Hz sine wave...", juce::dontSendNotification);
        
        // Stop any existing audio
        stopAllAudio();
        
        // Create a TracktionInstrument for testing
        testInstrument = std::make_unique<TracktionInstrument>("TestInstrument_SineWave", formatManager, knownPluginList);
        
        if (!testInstrument->registerWithAudioEngine())
        {
            audioStatusLabel.setText("ERROR: Failed to register instrument with audio engine", juce::dontSendNotification);
            return;
        }
        
        // Create a simple MIDI chart with the specified frequency
        juce::Array<juce::var> midiEventsArray;
        
        // Convert frequency to MIDI note number (A4 = 440Hz = MIDI note 69)
        int midiNote = static_cast<int>(69 + 12 * std::log2(frequency / 440.0));
        
        // Note on at time 0
        juce::DynamicObject::Ptr noteOnEvent = new juce::DynamicObject();
        noteOnEvent->setProperty("type", "noteOn");
        noteOnEvent->setProperty("time", 0.0);
        noteOnEvent->setProperty("channel", 1);
        noteOnEvent->setProperty("note", midiNote);
        noteOnEvent->setProperty("velocity", 100); // MIDI velocity 0-127
        midiEventsArray.add(juce::var(noteOnEvent.get()));
        
        // Note off at specified duration
        juce::DynamicObject::Ptr noteOffEvent = new juce::DynamicObject();
        noteOffEvent->setProperty("type", "noteOff");
        noteOffEvent->setProperty("time", duration);
        noteOffEvent->setProperty("channel", 1);
        noteOffEvent->setProperty("note", midiNote);
        noteOffEvent->setProperty("velocity", 0); // Note off velocity is typically 0
        midiEventsArray.add(juce::var(noteOffEvent.get()));
        
        juce::var chart = juce::var(midiEventsArray);
        
        std::cout << "TestsComponent: Playing " << frequency << "Hz (MIDI note " << midiNote << ") for " << duration << " seconds using TracktionInstrument" << std::endl;
        
        // Play the chart using TracktionInstrument
        if (testInstrument->playChart(chart, 120.0))
        {
            audioStatusLabel.setText("Playing " + juce::String(frequency, 1) + "Hz (MIDI note " + juce::String(midiNote) + ") using TracktionInstrument", juce::dontSendNotification);
            
            // Schedule completion status update
            juce::Timer::callAfterDelay(static_cast<int>((duration + 0.5) * 1000), [this]() {
                audioStatusLabel.setText("Sine wave test completed using new architecture", juce::dontSendNotification);
            });
        }
        else
        {
            audioStatusLabel.setText("ERROR: Failed to play chart on TracktionInstrument", juce::dontSendNotification);
        }
    }
    catch (const std::exception& e)
    {
        audioStatusLabel.setText("ERROR: " + juce::String(e.what()), juce::dontSendNotification);
        std::cerr << "TestsComponent::generateSineWave error: " << e.what() << std::endl;
    }
}

void TestsComponent::generateTwoTones()
{
    try 
    {
        audioStatusLabel.setText("Creating two TracktionPlayers for simultaneous tones...", juce::dontSendNotification);
        
        // Stop any existing audio
        stopAllAudio();
        
        // Create two TracktionPlayers for the two tones
        testPlayer1 = std::make_unique<TracktionPlayer>("TestPlayer1_440Hz", "Player 1 (440Hz)", formatManager, knownPluginList);
        testPlayer2 = std::make_unique<TracktionPlayer>("TestPlayer2_660Hz", "Player 2 (660Hz)", formatManager, knownPluginList);
        
        if (!testPlayer1->initialize() || !testPlayer2->initialize())
        {
            audioStatusLabel.setText("ERROR: Failed to initialize TracktionPlayers", juce::dontSendNotification);
            return;
        }
        
        std::cout << "TestsComponent: Two TracktionPlayers initialized for simultaneous playback" << std::endl;
        
        const float duration = 3.0f;
        
        // Create chart for first player (440Hz = A4 = MIDI note 69)
        juce::Array<juce::var> events1Array;
        
        juce::DynamicObject::Ptr noteOn1 = new juce::DynamicObject();
        noteOn1->setProperty("type", "noteOn");
        noteOn1->setProperty("time", 0.0);
        noteOn1->setProperty("channel", 1);
        noteOn1->setProperty("note", 69); // A4 = 440Hz
        noteOn1->setProperty("velocity", 80); // MIDI velocity 0-127
        events1Array.add(juce::var(noteOn1.get()));
        
        juce::DynamicObject::Ptr noteOff1 = new juce::DynamicObject();
        noteOff1->setProperty("type", "noteOff");
        noteOff1->setProperty("time", duration);
        noteOff1->setProperty("channel", 1);
        noteOff1->setProperty("note", 69);
        noteOff1->setProperty("velocity", 0); // Note off velocity is typically 0
        events1Array.add(juce::var(noteOff1.get()));
        
        // Create chart for second player (660Hz = approximately E5 = MIDI note 76)
        juce::Array<juce::var> events2Array;
        
        int midiNote2 = static_cast<int>(69 + 12 * std::log2(660.0 / 440.0)); // Convert 660Hz to MIDI note
        
        juce::DynamicObject::Ptr noteOn2 = new juce::DynamicObject();
        noteOn2->setProperty("type", "noteOn");
        noteOn2->setProperty("time", 0.0);
        noteOn2->setProperty("channel", 1);
        noteOn2->setProperty("note", midiNote2);
        noteOn2->setProperty("velocity", 80); // MIDI velocity 0-127
        events2Array.add(juce::var(noteOn2.get()));
        
        juce::DynamicObject::Ptr noteOff2 = new juce::DynamicObject();
        noteOff2->setProperty("type", "noteOff");
        noteOff2->setProperty("time", duration);
        noteOff2->setProperty("channel", 1);
        noteOff2->setProperty("note", midiNote2);
        noteOff2->setProperty("velocity", 0); // Note off velocity is typically 0
        events2Array.add(juce::var(noteOff2.get()));
        
        juce::var chart1 = juce::var(events1Array);
        juce::var chart2 = juce::var(events2Array);
        
        std::cout << "🎵 TestsComponent: Playing simultaneous tones - 440Hz (MIDI " << 69 << ") + 660Hz (MIDI " << midiNote2 << ")" << std::endl;
        
        // Start both players simultaneously using TracktionPlayer
        bool success1 = testPlayer1->playChart(chart1, 120.0);
        bool success2 = testPlayer2->playChart(chart2, 120.0);
        
        if (success1 && success2)
        {
            audioStatusLabel.setText("Playing simultaneous tones: 440Hz + 660Hz using TracktionPlayers", juce::dontSendNotification);
            
            // Schedule completion status update
            juce::Timer::callAfterDelay(static_cast<int>((duration + 0.5) * 1000), [this]() {
                audioStatusLabel.setText("Two tones test completed using new architecture", juce::dontSendNotification);
            });
        }
        else
        {
            audioStatusLabel.setText("ERROR: Failed to start simultaneous playback with TracktionPlayers", juce::dontSendNotification);
        }
    }
    catch (const std::exception& e)
    {
        audioStatusLabel.setText("ERROR: " + juce::String(e.what()), juce::dontSendNotification);
        std::cerr << "TestsComponent::generateTwoTones error: " << e.what() << std::endl;
    }
}

void TestsComponent::stopAllAudio()
{
    try 
    {
        std::cout << "TestsComponent: Stopping all audio using new architecture" << std::endl;
        
        // Stop and clean up all TracktionPlayers
        if (testPlayer1)
        {
            testPlayer1->stop();
            testPlayer1.reset();
        }
        
        if (testPlayer2)
        {
            testPlayer2->stop();
            testPlayer2.reset();
        }
        
        // Stop and clean up TracktionInstrument
        if (testInstrument)
        {
            testInstrument->stopPlayback();
            testInstrument.reset();
        }
        
        audioStatusLabel.setText("All audio stopped using new architecture", juce::dontSendNotification);
        std::cout << "TestsComponent: All test components cleaned up" << std::endl;
    }
    catch (const std::exception& e)
    {
        audioStatusLabel.setText("ERROR stopping audio: " + juce::String(e.what()), juce::dontSendNotification);
        std::cerr << "TestsComponent::stopAllAudio error: " << e.what() << std::endl;
    }
}

void TestsComponent::showPluginEditor()
{
    std::cout << "TestsComponent: Creating test player for plugin editor..." << std::endl;
    
    // Create a test TracktionPlayer for demonstration
    auto testPlayer = std::make_unique<TracktionPlayer>("TestPlayer_PluginDemo", "Plugin Demo Player", formatManager, knownPluginList);
    
    if (!testPlayer->initialize())
    {
        audioStatusLabel.setText("ERROR: Failed to initialize test player for plugin editor", juce::dontSendNotification);
        std::cout << "ERROR: Failed to initialize test player" << std::endl;
        return;
    }
    
    std::cout << "TestsComponent: Test player initialized successfully" << std::endl;
    
    // Create and show the plugin editor window
    auto* editorWindow = new juce::DocumentWindow("Plugin Chain Editor - " + testPlayer->getName(),
                                                 juce::Colours::lightgrey,
                                                 juce::DocumentWindow::allButtons);
    
    std::cout << "TestsComponent: Creating TracktionPlayerEditor..." << std::endl;
    auto* editor = new TracktionPlayerEditor();
    editor->setPlayer(testPlayer.release()); // Transfer ownership to editor
    editor->setSize(600, 500);
    
    std::cout << "TestsComponent: TracktionPlayerEditor created, setting up window..." << std::endl;
    
    editorWindow->setContentOwned(editor, true);
    editorWindow->setResizable(true, false);
    editorWindow->setUsingNativeTitleBar(true);
    editorWindow->setVisible(true);
    
    // Note: In a real application, you'd want to manage the window lifecycle properly
    // For this demo, the window will be self-managing
    
    audioStatusLabel.setText("Plugin Chain Editor opened", juce::dontSendNotification);
    std::cout << "TestsComponent: Plugin Chain Editor window opened successfully" << std::endl;
}

} // namespace covertracks
