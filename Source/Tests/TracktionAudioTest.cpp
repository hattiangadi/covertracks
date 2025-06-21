#include "TracktionAudioTest.h"

namespace covertracks {

TracktionAudioTest::TracktionAudioTest()
{
    // Set up title
    titleLabel.setText("Tracktion Engine Audio Test", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up test buttons
    sineWaveButton.setButtonText("Generate 440Hz Sine Wave (2s)");
    sineWaveButton.addListener(this);
    addAndMakeVisible(sineWaveButton);
    
    twoTonesButton.setButtonText("Generate Two Tones (440Hz + 660Hz)");
    twoTonesButton.addListener(this);
    addAndMakeVisible(twoTonesButton);
    
    stopButton.setButtonText("Stop All Audio");
    stopButton.addListener(this);
    addAndMakeVisible(stopButton);
    
    // Status label
    statusLabel.setText("Ready to test Tracktion Engine audio", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);
}

TracktionAudioTest::~TracktionAudioTest()
{
    stopAllAudio();
}

void TracktionAudioTest::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 2);
}

void TracktionAudioTest::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(20); // spacing
    
    sineWaveButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    twoTonesButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    stopButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(20);
    
    statusLabel.setBounds(area.removeFromTop(30));
}

void TracktionAudioTest::buttonClicked(juce::Button* button)
{
    if (button == &sineWaveButton)
    {
        statusLabel.setText("Generating 440Hz sine wave...", juce::dontSendNotification);
        generateSineWave(440.0f, 2.0f);
    }
    else if (button == &twoTonesButton)
    {
        statusLabel.setText("Generating two simultaneous tones...", juce::dontSendNotification);
        generateTwoTones();
    }
    else if (button == &stopButton)
    {
        statusLabel.setText("Stopping audio...", juce::dontSendNotification);
        stopAllAudio();
    }
}

void TracktionAudioTest::generateSineWave(float frequency, float duration)
{
    try 
    {
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        
        if (!currentEdit)
        {
            statusLabel.setText("ERROR: No Tracktion Edit available", juce::dontSendNotification);
            return;
        }
        
        // Stop any existing playback
        stopAllAudio();
        
        // Create a new audio track if we don't have one
        if (!testTrack)
        {
            testTrack = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
            testTrack->setName("Test Audio Track");
        }
        
        // Generate sine wave audio data
        auto sampleRate = audioEngine.getSampleRate();
        auto numSamples = static_cast<int>(sampleRate * duration);
        
        juce::AudioBuffer<float> audioBuffer(1, numSamples); // Mono
        auto* channelData = audioBuffer.getWritePointer(0);
        
        // Generate sine wave
        for (int i = 0; i < numSamples; ++i)
        {
            auto sample = std::sin(2.0 * juce::MathConstants<double>::pi * frequency * i / sampleRate);
            channelData[i] = static_cast<float>(sample * 0.5); // 50% volume
        }
        
        // Create a temporary audio file
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        auto tempFile = tempDir.getChildFile("tracktion_test_" + juce::String(juce::Random::getSystemRandom().nextInt()) + ".wav");
        
        // Write audio buffer to file
        if (auto fileStream = std::unique_ptr<juce::FileOutputStream>(tempFile.createOutputStream()))
        {
            juce::WavAudioFormat wavFormat;
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(fileStream.get(), sampleRate, 1, 16, {}, 0)))
            {
                fileStream.release(); // Writer takes ownership
                writer->writeFromAudioSampleBuffer(audioBuffer, 0, numSamples);
                writer.reset(); // Finalize file
                
                // Create audio clip on the track
                auto& transport = currentEdit->getTransport();
                transport.setPosition(0.0);
                
                // Insert the audio file as a clip
                if (auto clip = testTrack->insertWaveClip("Test Sine Wave", tempFile, 
                                                       te::TimePosition::fromSeconds(0.0), 
                                                       false))
                {
                    statusLabel.setText("Playing " + juce::String(frequency, 1) + "Hz sine wave", juce::dontSendNotification);
                    
                    // Start playback
                    transport.play(false);
                    isPlaying = true;
                    
                    // Schedule cleanup after duration
                    juce::Timer::callAfterDelay(static_cast<int>(duration * 1000 + 500), [this, tempFile]() 
                    {
                        stopAllAudio();
                        tempFile.deleteFile(); // Clean up temp file
                        statusLabel.setText("Sine wave test completed", juce::dontSendNotification);
                    });
                }
                else
                {
                    statusLabel.setText("ERROR: Failed to create audio clip", juce::dontSendNotification);
                    tempFile.deleteFile();
                }
            }
            else
            {
                statusLabel.setText("ERROR: Failed to create audio writer", juce::dontSendNotification);
            }
        }
        else
        {
            statusLabel.setText("ERROR: Failed to create temp file", juce::dontSendNotification);
        }
    }
    catch (const std::exception& e)
    {
        statusLabel.setText("ERROR: " + juce::String(e.what()), juce::dontSendNotification);
    }
}

void TracktionAudioTest::generateTwoTones()
{
    try 
    {
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        
        if (!currentEdit)
        {
            statusLabel.setText("ERROR: No Tracktion Edit available", juce::dontSendNotification);
            return;
        }
        
        // Stop any existing playback
        stopAllAudio();
        
        // Create tracks for each tone
        auto track1 = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
        auto track2 = currentEdit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*currentEdit), nullptr);
        
        track1->setName("Tone 1 (440Hz)");
        track2->setName("Tone 2 (660Hz)");
        
        // Generate audio data for both tones
        auto sampleRate = audioEngine.getSampleRate();
        auto duration = 3.0f; // 3 seconds
        auto numSamples = static_cast<int>(sampleRate * duration);
        
        // Generate first tone (440Hz)
        juce::AudioBuffer<float> buffer1(1, numSamples);
        auto* data1 = buffer1.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            auto sample = std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * i / sampleRate);
            data1[i] = static_cast<float>(sample * 0.3); // 30% volume
        }
        
        // Generate second tone (660Hz)
        juce::AudioBuffer<float> buffer2(1, numSamples);
        auto* data2 = buffer2.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            auto sample = std::sin(2.0 * juce::MathConstants<double>::pi * 660.0 * i / sampleRate);
            data2[i] = static_cast<float>(sample * 0.3); // 30% volume
        }
        
        // Create temp files for both tones
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        auto tempFile1 = tempDir.getChildFile("tracktion_tone1_" + juce::String(juce::Random::getSystemRandom().nextInt()) + ".wav");
        auto tempFile2 = tempDir.getChildFile("tracktion_tone2_" + juce::String(juce::Random::getSystemRandom().nextInt()) + ".wav");
        
        // Write first tone
        if (auto fileStream1 = std::unique_ptr<juce::FileOutputStream>(tempFile1.createOutputStream()))
        {
            juce::WavAudioFormat wavFormat;
            if (auto writer1 = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(fileStream1.get(), sampleRate, 1, 16, {}, 0)))
            {
                fileStream1.release();
                writer1->writeFromAudioSampleBuffer(buffer1, 0, numSamples);
                writer1.reset();
                
                // Write second tone
                if (auto fileStream2 = std::unique_ptr<juce::FileOutputStream>(tempFile2.createOutputStream()))
                {
                    if (auto writer2 = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(fileStream2.get(), sampleRate, 1, 16, {}, 0)))
                    {
                        fileStream2.release();
                        writer2->writeFromAudioSampleBuffer(buffer2, 0, numSamples);
                        writer2.reset();
                        
                        // Create clips on both tracks
                        auto clip1 = track1->insertWaveClip("440Hz Tone", tempFile1, te::TimePosition::fromSeconds(0.0), false);
                        auto clip2 = track2->insertWaveClip("660Hz Tone", tempFile2, te::TimePosition::fromSeconds(0.0), false);
                        
                        if (clip1 && clip2)
                        {
                            statusLabel.setText("Playing two simultaneous tones (440Hz + 660Hz)", juce::dontSendNotification);
                            
                            // Start playback
                            auto& transport = currentEdit->getTransport();
                            transport.setPosition(0.0);
                            transport.play(false);
                            isPlaying = true;
                            
                            // Schedule cleanup
                            juce::Timer::callAfterDelay(static_cast<int>(duration * 1000 + 500), [this, tempFile1, tempFile2]() 
                            {
                                stopAllAudio();
                                tempFile1.deleteFile();
                                tempFile2.deleteFile();
                                statusLabel.setText("Two tone test completed", juce::dontSendNotification);
                            });
                        }
                        else
                        {
                            statusLabel.setText("ERROR: Failed to create audio clips", juce::dontSendNotification);
                            tempFile1.deleteFile();
                            tempFile2.deleteFile();
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        statusLabel.setText("ERROR: " + juce::String(e.what()), juce::dontSendNotification);
    }
}

void TracktionAudioTest::stopAllAudio()
{
    try 
    {
        auto& audioEngine = getAudioEngine();
        auto* currentEdit = audioEngine.getCurrentEdit();
        
        if (currentEdit && isPlaying)
        {
            auto& transport = currentEdit->getTransport();
            transport.stop(false, false);
            isPlaying = false;
            
            // Clear all tracks
            while (currentEdit->getNumTracks() > 0)
            {
                currentEdit->deleteTrack(currentEdit->getTrack(0));
            }
            
            testTrack.reset();
            statusLabel.setText("Audio stopped", juce::dontSendNotification);
        }
    }
    catch (const std::exception& e)
    {
        statusLabel.setText("ERROR stopping audio: " + juce::String(e.what()), juce::dontSendNotification);
    }
}

} // namespace covertracks