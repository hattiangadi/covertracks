/*
  ==============================================================================
   AudioEngine.cpp
   
   Implementation of the AudioEngine class.
  ==============================================================================
*/

#include "AudioEngine.h"
#include "../UI/MainHostWindow.h"
#include <iostream>

namespace covertracks {

AudioEngine::AudioEngine(AudioDeviceManager& dm, 
                         AudioPluginFormatManager& fm,
                         KnownPluginList& kpl)
    : deviceManager(dm),
      formatManager(fm),
      knownPluginList(kpl)
{
    // Add this as a change listener to the device manager
    deviceManager.addChangeListener(this);
    
    // Check the current double precision setting from user settings
    if (auto* props = getAppProperties().getUserSettings())
        doublePrecisionProcessing = props->getBoolValue("doublePrecisionProcessing", false);
        
    std::cout << "AudioEngine initialized (double precision: " 
              << (doublePrecisionProcessing ? "enabled" : "disabled") << ")" << std::endl;
}

AudioEngine::~AudioEngine()
{
    // Notify all Instruments that the AudioEngine is being destroyed
    // This prevents them from trying to unregister when they're destroyed
    std::cout << "AudioEngine destructor: Notifying Instruments of destruction" << std::endl;
    covertracks::Instrument::notifyAudioEngineDestroyed();
    
    // Stop all audio processing
    const ScopedLock sl(processorLock);
    
    for (auto* player : audioProcessorPlayers)
    {
        deviceManager.removeAudioCallback(player);
        player->setProcessor(nullptr);
    }
    
    // Unregister as a listener
    deviceManager.removeChangeListener(this);
    
    std::cout << "AudioEngine destroyed" << std::endl;
}

bool AudioEngine::registerProcessor(AudioProcessor* processor)
{
    if (processor == nullptr)
        return false;
    
    // Use a lock to ensure thread safety when modifying the processor list
    const ScopedLock sl(processorLock);
        
    // Check if already registered
    for (auto* registeredProcessor : registeredProcessors)
    {
        if (registeredProcessor == processor)
            return true; // Already registered
    }
    
    try
    {
        // Create a new AudioProcessorPlayer
        auto* player = new AudioProcessorPlayer(doublePrecisionProcessing);
        player->setProcessor(processor);
        
        // Register with the device manager
        deviceManager.addAudioCallback(player);
        
        // Store the player
        audioProcessorPlayers.add(player);
        registeredProcessors.add(processor);
        
        std::cout << "Registered processor: " << processor->getName() << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to register processor: " << e.what() << std::endl;
        return false;
    }
}

void AudioEngine::unregisterProcessor(AudioProcessor* processor)
{
    if (processor == nullptr)
        return;
    
    // Use a lock to ensure thread safety when modifying the processor list
    const ScopedLock sl(processorLock);
        
    // Find the player for this processor
    auto* player = findPlayerForProcessor(processor);
    
    if (player != nullptr)
    {
        // Unregister from device manager
        deviceManager.removeAudioCallback(player);
        
        // Release the processor
        player->setProcessor(nullptr);
        
        // Remove from our arrays
        audioProcessorPlayers.removeObject(player);
        registeredProcessors.removeFirstMatchingValue(processor);
        
        std::cout << "Unregistered processor: " << processor->getName() << std::endl;
    }
}

bool AudioEngine::registerMidiInput(const String& midiInputDeviceName, MidiInputCallback* callbackObject)
{
    if (callbackObject == nullptr)
    {
        std::cerr << "ERROR: Attempted to register null MIDI callback" << std::endl;
        return false;
    }
        
    try
    {
        std::cout << "DEBUG: Before registering MIDI callback for '" 
                  << (midiInputDeviceName.isEmpty() ? "default" : midiInputDeviceName) 
                  << "', callback ptr: " << callbackObject << std::endl;
                  
        deviceManager.addMidiInputDeviceCallback(midiInputDeviceName, callbackObject);
        
        std::cout << "SUCCESS: Registered MIDI input callback for device: " 
                  << (midiInputDeviceName.isEmpty() ? "default" : midiInputDeviceName) 
                  << ", callback ptr: " << callbackObject << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: Failed to register MIDI input callback: " << e.what() << std::endl;
        return false;
    }
}

void AudioEngine::unregisterMidiInput(const String& midiInputDeviceName, MidiInputCallback* callbackObject)
{
    if (callbackObject == nullptr)
    {
        std::cerr << "ERROR: Attempted to unregister null MIDI callback" << std::endl;
        return;
    }
    
    try
    {
        std::cout << "DEBUG: Before unregistering MIDI callback for '" 
                  << (midiInputDeviceName.isEmpty() ? "default" : midiInputDeviceName) 
                  << "', callback ptr: " << callbackObject << std::endl;
                  
        // Get list of available MIDI inputs for debugging
        auto midiInputs = MidiInput::getAvailableDevices();
        std::cout << "DEBUG: Available MIDI inputs: " << midiInputs.size() << std::endl;
        for (auto& input : midiInputs)
        {
            std::cout << "  - " << input.name << " (ID: " << input.identifier << ")" << std::endl;
        }
        
        deviceManager.removeMidiInputDeviceCallback(midiInputDeviceName, callbackObject);
        
        std::cout << "SUCCESS: Unregistered MIDI input callback for device: " 
                  << (midiInputDeviceName.isEmpty() ? "default" : midiInputDeviceName) 
                  << ", callback ptr: " << callbackObject << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR: Failed to unregister MIDI input callback: " << e.what() << std::endl;
    }
}

std::unique_ptr<XmlElement> AudioEngine::getAudioDeviceState() const
{
    return deviceManager.createStateXml();
}

bool AudioEngine::setAudioDeviceState(const XmlElement* stateXml)
{
    if (stateXml == nullptr)
        return false;
        
    // Initialize the device manager with the saved state
    auto result = deviceManager.initialise(
        deviceManager.getAudioDeviceSetup().inputChannels.countNumberOfSetBits(),
        deviceManager.getAudioDeviceSetup().outputChannels.countNumberOfSetBits(),
        stateXml,
        true);
    // Convert result to bool - true if no error
    return (result == juce::String());
}

void AudioEngine::showAudioSettings(Component* parentComponent)
{
    auto* audioSettingsComp = new AudioDeviceSelectorComponent(
        deviceManager,
        0, 256,  // input channels
        0, 256,  // output channels
        true,    // show MIDI input
        true,    // show MIDI output
        true,    // show channels as stereo pairs
        false);  // hide advanced options
        
    audioSettingsComp->setSize(500, 450);
    
    DialogWindow::LaunchOptions o;
    o.content.setOwned(audioSettingsComp);
    o.dialogTitle = "Audio Settings";
    o.componentToCentreAround = parentComponent;
    o.dialogBackgroundColour = parentComponent->getLookAndFeel().findColour(ResizableWindow::backgroundColourId);
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = false;
    
    auto* w = o.create();
    
    // Use a lambda to handle the dialog close event
    w->enterModalState(true, ModalCallbackFunction::create(
        [this](int)
        {
            // Save audio state when dialog closes
            auto audioState = this->getAudioDeviceState();
            
            if (auto* props = getAppProperties().getUserSettings())
            {
                props->setValue("audioDeviceState", audioState.get());
                props->saveIfNeeded();
            }
            
            // Notify listeners of the change
            broadcaster.sendChangeMessage();
        }), true);
}

double AudioEngine::getSampleRate() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getCurrentSampleRate();
        
    return 44100.0; // Default sample rate
}

int AudioEngine::getBufferSize() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getCurrentBufferSizeSamples();
        
    return 512; // Default buffer size
}

void AudioEngine::setDoublePrecisionProcessing(bool doublePrecision)
{
    if (doublePrecision == doublePrecisionProcessing)
        return; // No change
    
    // Use a lock to ensure thread safety when modifying processor connections
    const ScopedLock sl(processorLock);
        
    doublePrecisionProcessing = doublePrecision;
    
    // Update all players
    for (auto* player : audioProcessorPlayers)
    {
        // Store the processor temporarily
        auto* processor = player->getCurrentProcessor();
        
        // Unregister from device manager
        deviceManager.removeAudioCallback(player);
        
        // Release the processor from the player
        player->setProcessor(nullptr);
        
        // Set the double precision flag
        player->setDoublePrecisionProcessing(doublePrecisionProcessing);
        
        // Re-assign the processor
        player->setProcessor(processor);
        
        // Re-register with device manager
        deviceManager.addAudioCallback(player);
    }
    
    // Save the setting
    if (auto* props = getAppProperties().getUserSettings())
    {
        props->setValue("doublePrecisionProcessing", doublePrecision);
        props->saveIfNeeded();
    }
    
    std::cout << "Double precision processing " 
              << (doublePrecisionProcessing ? "enabled" : "disabled") << std::endl;
              
    // Notify listeners of the change
    broadcaster.sendChangeMessage();
}

void AudioEngine::addChangeListener(ChangeListener* listener)
{
    broadcaster.addChangeListener(listener);
}

void AudioEngine::removeChangeListener(ChangeListener* listener)
{
    broadcaster.removeChangeListener(listener);
}

void AudioEngine::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == &deviceManager)
    {
        // Device manager changed, notify our listeners
        broadcaster.sendChangeMessage();
        
        // Output some debug info
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            std::cout << "Audio device changed: " << device->getName() 
                      << " (" << device->getCurrentSampleRate() << " Hz, " 
                      << device->getCurrentBufferSizeSamples() << " samples)" << std::endl;
        }
        else
        {
            std::cout << "Audio device removed" << std::endl;
        }
    }
}

AudioProcessorPlayer* AudioEngine::findPlayerForProcessor(AudioProcessor* processor)
{
    // This method is called from within locked methods, so no need for additional locking
    for (int i = 0; i < registeredProcessors.size(); ++i)
    {
        if (registeredProcessors[i] == processor)
            return audioProcessorPlayers[i];
    }
    
    return nullptr;
}

float AudioEngine::getCpuUsage() const
{
    return static_cast<float>(deviceManager.getCpuUsage());
}

int AudioEngine::getInputLatencySamples() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getInputLatencyInSamples();
    return 0;
}

int AudioEngine::getOutputLatencySamples() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getOutputLatencyInSamples();
    return 0;
}

StringArray AudioEngine::getAvailableAudioDeviceNames() const
{
    StringArray deviceNames;
    // Get available device types without copying the OwnedArray
    for (int i = 0; i < deviceManager.getAvailableDeviceTypes().size(); ++i)
    {
        auto* type = deviceManager.getAvailableDeviceTypes()[i];
        auto names = type->getDeviceNames();
        deviceNames.addArray(names);
    }
    
    return deviceNames;
}

} // namespace covertracks