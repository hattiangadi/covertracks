/*
  ==============================================================================
   AudioEngine.h
   
   A centralized audio engine to manage shared audio resources
   across the application.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace covertracks {

/**
 * @brief Central audio engine for managing shared audio resources
 * 
 * This class centralizes audio device management, plugin format handling,
 * and plugin list management to avoid resource duplication and conflicts
 * between different parts of the application.
 */
class AudioEngine final : public ChangeListener
{
public:
    /**
     * @brief Construct a new AudioEngine
     * 
     * @param deviceManager Reference to the shared AudioDeviceManager
     * @param formatManager Reference to the shared AudioPluginFormatManager
     * @param knownPluginList Reference to the shared KnownPluginList
     */
    AudioEngine(AudioDeviceManager& deviceManager,
                AudioPluginFormatManager& formatManager,
                KnownPluginList& knownPluginList);
    
    /**
     * @brief Destroy the AudioEngine
     */
    ~AudioEngine() override;
    
    /**
     * @brief Get the shared AudioDeviceManager
     * 
     * @return AudioDeviceManager& Reference to the shared AudioDeviceManager
     */
    AudioDeviceManager& getDeviceManager() { return deviceManager; }
    
    /**
     * @brief Get the shared AudioPluginFormatManager
     * 
     * @return AudioPluginFormatManager& Reference to the shared AudioPluginFormatManager
     */
    AudioPluginFormatManager& getFormatManager() { return formatManager; }
    
    /**
     * @brief Get the shared KnownPluginList
     * 
     * @return KnownPluginList& Reference to the shared KnownPluginList
     */
    KnownPluginList& getKnownPluginList() { return knownPluginList; }
    
    /**
     * @brief Register an AudioProcessor to receive audio callbacks
     * 
     * @param processor The AudioProcessor to register
     * @return true if registration was successful
     * @return false if registration failed
     */
    bool registerProcessor(AudioProcessor* processor);
    
    /**
     * @brief Unregister an AudioProcessor from audio callbacks
     * 
     * @param processor The AudioProcessor to unregister
     */
    void unregisterProcessor(AudioProcessor* processor);
    
    /**
     * @brief Register for MIDI input on the specified device
     * 
     * @param midiInputDeviceName The MIDI input device name, empty for default
     * @param callbackObject The object that will receive MIDI callbacks
     * @return true if registration was successful
     * @return false if registration failed
     */
    bool registerMidiInput(const String& midiInputDeviceName, MidiInputCallback* callbackObject);
    
    /**
     * @brief Unregister from MIDI input
     * 
     * @param midiInputDeviceName The MIDI input device name
     * @param callbackObject The callback object to unregister
     */
    void unregisterMidiInput(const String& midiInputDeviceName, MidiInputCallback* callbackObject);
    
    /**
     * @brief Get the current audio device settings
     * 
     * @return std::unique_ptr<XmlElement> XML representation of current audio settings
     */
    std::unique_ptr<XmlElement> getAudioDeviceState() const;
    
    /**
     * @brief Apply audio device settings from XML
     * 
     * @param stateXml XML representation of audio settings
     * @return true if settings were applied successfully
     * @return false if settings failed to apply
     */
    bool setAudioDeviceState(const XmlElement* stateXml);
    
    /**
     * @brief Show audio settings dialog
     * 
     * @param parentComponent The parent component for the dialog
     */
    void showAudioSettings(Component* parentComponent);
    
    /**
     * @brief Get the current sample rate
     * 
     * @return double The current sample rate
     */
    double getSampleRate() const;
    
    /**
     * @brief Get the current buffer size
     * 
     * @return int The current buffer size
     */
    int getBufferSize() const;
    
    /**
     * @brief Set whether to use double precision processing
     * 
     * @param doublePrecision true for double precision, false for single precision
     */
    void setDoublePrecisionProcessing(bool doublePrecision);
    
    /**
     * @brief Check if double precision processing is enabled
     * 
     * @return true if double precision is enabled
     * @return false if single precision is used
     */
    bool isDoublePrecisionProcessing() const { return doublePrecisionProcessing; }
    
    /**
     * @brief Add a change listener to be notified of engine changes
     * 
     * @param listener The listener to add
     */
    void addChangeListener(ChangeListener* listener);
    
    /**
     * @brief Remove a change listener
     * 
     * @param listener The listener to remove
     */
    void removeChangeListener(ChangeListener* listener);
    
private:
    // ChangeListener implementation
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    // References to shared resources (not owned)
    AudioDeviceManager& deviceManager;
    AudioPluginFormatManager& formatManager;
    KnownPluginList& knownPluginList;
    
    // Additional state management
    bool doublePrecisionProcessing = false;
    
    // ChangeBroadcaster for notifying listeners of engine changes
    ChangeBroadcaster broadcaster;
    
    // Resource management
    OwnedArray<AudioProcessorPlayer> audioProcessorPlayers;
    Array<AudioProcessor*> registeredProcessors;
    
    // Thread safety for processor registration/unregistration
    CriticalSection processorLock;
    
    // Utility method to find a player for a processor
    AudioProcessorPlayer* findPlayerForProcessor(AudioProcessor* processor);
    
    // CPU and latency monitoring
    float getCpuUsage() const;
    int getInputLatencySamples() const;
    int getOutputLatencySamples() const;
    
    // Audio device enumeration
    StringArray getAvailableAudioDeviceNames() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};

} // namespace covertracks