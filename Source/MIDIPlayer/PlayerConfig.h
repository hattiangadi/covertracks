#pragma once
#include <JuceHeader.h>

struct PlayerConfig {
    juce::String playerId;
    juce::String instrumentType;
    juce::String pluginId;
    juce::String sampleName;
    
    // MIDI and timing parameters
    double tempo = 120.0;  // Default tempo in BPM
    int ppq = 480;        // Default pulses per quarter note
    
    // Helper methods for specific instrument configurations
    static PlayerConfig forBass(const juce::String& id = "bass") {
        PlayerConfig config;
        config.playerId = id;
        config.instrumentType = "bass";
        config.pluginId = coverband::VSTHost::SAMPLER_PLUGIN_ID;
        config.sampleName = "bass_c1.wav";
        return config;
    }
    
    static PlayerConfig forPiano(const juce::String& id = "piano") {
        PlayerConfig config;
        config.playerId = id;
        config.instrumentType = "piano";
        config.pluginId = coverband::VSTHost::SAMPLER_PLUGIN_ID;
        config.sampleName = "piano_a4.wav";
        return config;
    }
    
    static PlayerConfig forDrum(const juce::String& id = "drums") {
        PlayerConfig config;
        config.playerId = id;
        config.instrumentType = "drum";
        config.pluginId = coverband::VSTHost::SAMPLER_PLUGIN_ID;
        config.sampleName = "drum_kick.wav";
        return config;
    }
    
    static PlayerConfig forGuitar(const juce::String& id = "guitar") {
        PlayerConfig config;
        config.playerId = id;
        config.instrumentType = "guitar";
        config.pluginId = coverband::VSTHost::SAMPLER_PLUGIN_ID;
        config.sampleName = "guitar_e4.wav";
        return config;
    }
};