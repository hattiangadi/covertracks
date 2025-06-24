#pragma once

#include <JuceHeader.h>
#include "../Player/Core/Player.h"
#include "../Player/Core/Instrument.h"
#include "../Audio/AudioEngineInterface.h"

namespace covertracks {

/**
 * @brief Simple test component for verifying Instrument and Player functionality
 * 
 * This test demonstrates the new clean architecture where:
 * - Instrument owns all audio responsibilities
 * - Player handles orchestration between Musician and Instrument
 * - Simple audio generation without plugin complexity
 */
class TracktionAudioTest : public juce::Component,
                          public juce::Button::Listener
{
public:
    TracktionAudioTest();
    ~TracktionAudioTest() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    
private:
    void testBasicInstrument();
    void testPlayer();
    void testSimultaneousPlayers();
    void stopAllTests();
    void updateStatus(const juce::String& message);
    
    // UI Components
    juce::Label titleLabel;
    juce::TextButton basicInstrumentButton;
    juce::TextButton tracktionPlayerButton;
    juce::TextButton simultaneousButton;
    juce::TextButton stopButton;
    juce::Label statusLabel;
    
    // Test components using new architecture
    std::unique_ptr<Instrument> testInstrument1;
    std::unique_ptr<Instrument> testInstrument2;
    std::unique_ptr<Player> testPlayer1;
    std::unique_ptr<Player> testPlayer2;
    
    // Shared plugin format manager and known plugin list
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TracktionAudioTest)
};

} // namespace covertracks