#include "TracktionAudioTest.h"
#include <iostream>

namespace covertracks {

TracktionAudioTest::TracktionAudioTest()
{
    // Set up title
    titleLabel.setText("New Architecture Audio Test - Instrument & Player", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("bold")));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up test buttons
    basicInstrumentButton.setButtonText("Test Basic Instrument (440Hz Sine)");
    basicInstrumentButton.addListener(this);
    addAndMakeVisible(basicInstrumentButton);
    
    tracktionPlayerButton.setButtonText("Test Player (Chart Generation + Play)");
    tracktionPlayerButton.addListener(this);
    addAndMakeVisible(tracktionPlayerButton);
    
    simultaneousButton.setButtonText("Test Simultaneous Players (440Hz + 660Hz)");
    simultaneousButton.addListener(this);
    addAndMakeVisible(simultaneousButton);
    
    stopButton.setButtonText("Stop All Tests");
    stopButton.addListener(this);
    addAndMakeVisible(stopButton);
    
    // Status label
    statusLabel.setText("Ready to test new architecture (Instrument owns audio, Player orchestrates)", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);
    
    std::cout << "TracktionAudioTest: Initialized new architecture test component" << std::endl;
}

TracktionAudioTest::~TracktionAudioTest()
{
    stopAllTests();
    std::cout << "TracktionAudioTest: Destroyed" << std::endl;
}

void TracktionAudioTest::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw border
    g.setColour(juce::Colours::lightblue);
    g.drawRect(getLocalBounds(), 3);
    
    // Draw architecture diagram
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(40); // Skip title
    area.removeFromBottom(200); // Skip buttons and status
    
    g.setColour(juce::Colours::darkgrey);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    
    g.drawText("Architecture: Player → Instrument → TracktionEngine → Audio", 
               area.removeFromTop(20), juce::Justification::centred);
    g.drawText("• Instrument: Owns Track, Plugins, Audio Generation", 
               area.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("• Player: Orchestrates Musician ↔ Instrument interaction", 
               area.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("• Clean separation: Audio ownership vs. orchestration", 
               area.removeFromTop(20), juce::Justification::centredLeft);
}

void TracktionAudioTest::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(100); // Space for architecture info
    
    basicInstrumentButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    tracktionPlayerButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    simultaneousButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    stopButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(15);
    
    statusLabel.setBounds(area.removeFromTop(40));
}

void TracktionAudioTest::buttonClicked(juce::Button* button)
{
    if (button == &basicInstrumentButton)
    {
        updateStatus("Testing basic Instrument audio generation...");
        testBasicInstrument();
    }
    else if (button == &tracktionPlayerButton)
    {
        updateStatus("Testing Player chart generation and playback...");
        testPlayer();
    }
    else if (button == &simultaneousButton)
    {
        updateStatus("Testing simultaneous Players...");
        testSimultaneousPlayers();
    }
    else if (button == &stopButton)
    {
        updateStatus("Stopping all tests...");
        stopAllTests();
    }
}

void TracktionAudioTest::testBasicInstrument()
{
    try 
    {
        std::cout << "🧪 Testing basic Instrument functionality" << std::endl;
        
        // Stop any existing tests
        stopAllTests();
        
        // Create a Instrument
        testInstrument1 = std::make_unique<Instrument>("TestInstrument1", formatManager, knownPluginList);
        
        if (!testInstrument1->registerWithAudioEngine())
        {
            updateStatus("❌ ERROR: Failed to register instrument with audio engine");
            return;
        }
        
        std::cout << "✅ Instrument registered with audio engine" << std::endl;
        
        // Create a simple MIDI chart with a 440Hz note
        juce::Array<juce::var> midiEventsArray;
        juce::var chart = juce::var(midiEventsArray);
        
        // Note on at time 0
        juce::DynamicObject::Ptr noteOnEvent = new juce::DynamicObject();
        noteOnEvent->setProperty("type", "noteOn");
        noteOnEvent->setProperty("time", 0.0);
        noteOnEvent->setProperty("channel", 1);
        noteOnEvent->setProperty("note", 69); // A4 = 440Hz
        noteOnEvent->setProperty("velocity", 0.8);
        midiEventsArray.add(juce::var(noteOnEvent.get()));
        
        // Note off at time 2.0
        juce::DynamicObject::Ptr noteOffEvent = new juce::DynamicObject();
        noteOffEvent->setProperty("type", "noteOff");
        noteOffEvent->setProperty("time", 2.0);
        noteOffEvent->setProperty("channel", 1);
        noteOffEvent->setProperty("note", 69);
        noteOffEvent->setProperty("velocity", 0.8);
        midiEventsArray.add(juce::var(noteOffEvent.get()));
        
        // Update chart with the array
        chart = juce::var(midiEventsArray);
        
        std::cout << "🎵 Playing chart with " << midiEventsArray.size() << " MIDI events" << std::endl;
        
        // Play the chart
        if (testInstrument1->playChart(chart, 120.0))
        {
            updateStatus("✅ Instrument: Playing 440Hz note (A4) for 2 seconds");
            
            // Schedule completion status update
            juce::Timer::callAfterDelay(2500, [this]() {
                updateStatus("✅ Basic Instrument test completed successfully");
            });
        }
        else
        {
            updateStatus("❌ ERROR: Failed to play chart on Instrument");
        }
    }
    catch (const std::exception& e)
    {
        updateStatus("❌ ERROR in basic instrument test: " + juce::String(e.what()));
        std::cerr << "TracktionAudioTest::testBasicInstrument error: " << e.what() << std::endl;
    }
}

void TracktionAudioTest::testPlayer()
{
    try 
    {
        std::cout << "🧪 Testing Player orchestration functionality" << std::endl;
        
        // Stop any existing tests
        stopAllTests();
        
        // Create a Player (which internally creates a Instrument)
        testPlayer1 = std::make_unique<Player>("TestPlayer1", "Test Player 1", formatManager, knownPluginList);
        
        if (!testPlayer1->initialize())
        {
            updateStatus("❌ ERROR: Failed to initialize Player");
            return;
        }
        
        std::cout << "✅ Player initialized and registered" << std::endl;
        
        // Create a simple chord chart (C major chord)
        juce::Array<juce::var> midiEventsArray;
        
        // C major chord: C, E, G (notes 60, 64, 67)
        std::vector<int> chordNotes = {60, 64, 67}; // C4, E4, G4
        
        for (int note : chordNotes)
        {
            // Note on at time 0
            juce::DynamicObject::Ptr noteOnEvent = new juce::DynamicObject();
            noteOnEvent->setProperty("type", "noteOn");
            noteOnEvent->setProperty("time", 0.0);
            noteOnEvent->setProperty("channel", 1);
            noteOnEvent->setProperty("note", note);
            noteOnEvent->setProperty("velocity", 0.6);
            midiEventsArray.add(juce::var(noteOnEvent.get()));
            
            // Note off at time 3.0
            juce::DynamicObject::Ptr noteOffEvent = new juce::DynamicObject();
            noteOffEvent->setProperty("type", "noteOff");
            noteOffEvent->setProperty("time", 3.0);
            noteOffEvent->setProperty("channel", 1);
            noteOffEvent->setProperty("note", note);
            noteOffEvent->setProperty("velocity", 0.6);
            midiEventsArray.add(juce::var(noteOffEvent.get()));
        }
        
        juce::var chart = juce::var(midiEventsArray);
        
        std::cout << "🎵 Playing C major chord (3 notes) for 3 seconds" << std::endl;
        
        // Use Player to play the chart (demonstrates orchestration)
        if (testPlayer1->playChart(chart, 120.0))
        {
            updateStatus("✅ Player: Playing C major chord (C-E-G) for 3 seconds");
            
            // Schedule completion status update
            juce::Timer::callAfterDelay(3500, [this]() {
                updateStatus("✅ Player orchestration test completed successfully");
            });
        }
        else
        {
            updateStatus("❌ ERROR: Player failed to play chart");
        }
    }
    catch (const std::exception& e)
    {
        updateStatus("❌ ERROR in Player test: " + juce::String(e.what()));
        std::cerr << "TracktionAudioTest::testPlayer error: " << e.what() << std::endl;
    }
}

void TracktionAudioTest::testSimultaneousPlayers()
{
    try 
    {
        std::cout << "🧪 Testing simultaneous Players" << std::endl;
        
        // Stop any existing tests
        stopAllTests();
        
        // Create two Players
        testPlayer1 = std::make_unique<Player>("TestPlayer1", "Player 1 (440Hz)", formatManager, knownPluginList);
        testPlayer2 = std::make_unique<Player>("TestPlayer2", "Player 2 (660Hz)", formatManager, knownPluginList);
        
        if (!testPlayer1->initialize() || !testPlayer2->initialize())
        {
            updateStatus("❌ ERROR: Failed to initialize Players");
            return;
        }
        
        std::cout << "✅ Two Players initialized" << std::endl;
        
        // Create chart for first player (440Hz = A4)
        juce::Array<juce::var> events1Array;
        
        juce::DynamicObject::Ptr noteOn1 = new juce::DynamicObject();
        noteOn1->setProperty("type", "noteOn");
        noteOn1->setProperty("time", 0.0);
        noteOn1->setProperty("channel", 1);
        noteOn1->setProperty("note", 69); // A4 = 440Hz
        noteOn1->setProperty("velocity", 0.5);
        events1Array.add(juce::var(noteOn1.get()));
        
        juce::DynamicObject::Ptr noteOff1 = new juce::DynamicObject();
        noteOff1->setProperty("type", "noteOff");
        noteOff1->setProperty("time", 4.0);
        noteOff1->setProperty("channel", 1);
        noteOff1->setProperty("note", 69);
        noteOff1->setProperty("velocity", 0.5);
        events1Array.add(juce::var(noteOff1.get()));
        
        // Create chart for second player (660Hz = approximately E5)
        juce::Array<juce::var> events2Array;
        
        juce::DynamicObject::Ptr noteOn2 = new juce::DynamicObject();
        noteOn2->setProperty("type", "noteOn");
        noteOn2->setProperty("time", 0.0);
        noteOn2->setProperty("channel", 1);
        noteOn2->setProperty("note", 76); // E5 = ~659Hz (close to 660Hz)
        noteOn2->setProperty("velocity", 0.5);
        events2Array.add(juce::var(noteOn2.get()));
        
        juce::DynamicObject::Ptr noteOff2 = new juce::DynamicObject();
        noteOff2->setProperty("type", "noteOff");
        noteOff2->setProperty("time", 4.0);
        noteOff2->setProperty("channel", 1);
        noteOff2->setProperty("note", 76);
        noteOff2->setProperty("velocity", 0.5);
        events2Array.add(juce::var(noteOff2.get()));
        
        std::cout << "🎵 Starting simultaneous playback on both players" << std::endl;
        
        // Create the final charts
        juce::var chart1 = juce::var(events1Array);
        juce::var chart2 = juce::var(events2Array);
        
        // Start both players simultaneously
        bool success1 = testPlayer1->playChart(chart1, 120.0);
        bool success2 = testPlayer2->playChart(chart2, 120.0);
        
        if (success1 && success2)
        {
            updateStatus("✅ Simultaneous Players: Playing 440Hz + 660Hz harmony for 4 seconds");
            
            // Schedule completion status update
            juce::Timer::callAfterDelay(4500, [this]() {
                updateStatus("✅ Simultaneous Players test completed - Clean architecture works!");
            });
        }
        else
        {
            updateStatus("❌ ERROR: Failed to start simultaneous playback");
        }
    }
    catch (const std::exception& e)
    {
        updateStatus("❌ ERROR in simultaneous players test: " + juce::String(e.what()));
        std::cerr << "TracktionAudioTest::testSimultaneousPlayers error: " << e.what() << std::endl;
    }
}

void TracktionAudioTest::stopAllTests()
{
    try 
    {
        std::cout << "🛑 Stopping all TracktionAudioTest instances" << std::endl;
        
        // Stop and clean up all test components
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
        
        if (testInstrument1)
        {
            testInstrument1->stopPlayback();
            testInstrument1.reset();
        }
        
        if (testInstrument2)
        {
            testInstrument2->stopPlayback();
            testInstrument2.reset();
        }
        
        updateStatus("🛑 All tests stopped - Ready for new tests");
        std::cout << "✅ All test components cleaned up" << std::endl;
    }
    catch (const std::exception& e)
    {
        updateStatus("❌ ERROR stopping tests: " + juce::String(e.what()));
        std::cerr << "TracktionAudioTest::stopAllTests error: " << e.what() << std::endl;
    }
}

void TracktionAudioTest::updateStatus(const juce::String& message)
{
    statusLabel.setText(message, juce::dontSendNotification);
    std::cout << "TracktionAudioTest: " << message.toStdString() << std::endl;
}

} // namespace covertracks