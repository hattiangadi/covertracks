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
    // Forward declarations
    class Player;
    class GraphDocumentComponent;
}

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
     // Updated constructor that doesn't take deviceManager as a reference
     AudioEngine(AudioPluginFormatManager& formatManager,
                 KnownPluginList& knownPluginList);
     ~AudioEngine() override;
     
     // Core accessors
     AudioDeviceManager& getDeviceManager() { return *deviceManager; }
     AudioPluginFormatManager& getFormatManager() { return formatManager; }
     KnownPluginList& getKnownPluginList() { return knownPluginList; }
     
     // Player graph management
     AudioProcessorGraph::NodeID addPlayerGraph(AudioProcessorGraph& playerGraph);
     void removePlayerGraph(AudioProcessorGraph::NodeID nodeId);
     
     // Player state management
     void activatePlayer(covertracks::Player* player);
     void deactivatePlayer(covertracks::Player* player);
     
     // Connection management
     bool connectNodes(AudioProcessorGraph::NodeID sourceNodeId, 
                       int sourceChannelIndex,
                       AudioProcessorGraph::NodeID destNodeId,
                       int destChannelIndex);
    
    // Get the node ID for a processor graph in the main graph
    AudioProcessorGraph::NodeID getPlayerGraphNodeId(AudioProcessorGraph* processorGraph) const;
     
     // Resource pooling
     std::shared_ptr<AudioProcessor> getSharedProcessor(const String& processorId);
     void registerSharedProcessor(const String& processorId, std::shared_ptr<AudioProcessor> processor);
     
     // Existing methods (from current implementation)
     bool registerProcessor(AudioProcessor* processor);
     void unregisterProcessor(AudioProcessor* processor);
     bool registerMidiInput(const String& midiInputDeviceName, MidiInputCallback* callbackObject);
     void unregisterMidiInput(const String& midiInputDeviceName, MidiInputCallback* callbackObject);
     
     // Audio device management
     std::unique_ptr<XmlElement> getAudioDeviceState() const;
     bool setAudioDeviceState(const XmlElement* stateXml);
     void showAudioSettings(Component* parentComponent);
     
     // MIDI input management
     void enableMidiInput(bool enabled);
     bool isMidiInputEnabled() const { return midiEnabled; }
     void sendMidiMessage(const MidiMessage& message);
     void processMidiKeyboardState(MidiKeyboardState& keyboardState);
     
     // Audio output management
     void setMasterGain(float gain);
     
     // Audio state getters
     double getSampleRate() const;
     int getBufferSize() const;
     float getCpuUsage() const;
     int getInputLatencySamples() const;
     int getOutputLatencySamples() const;
     StringArray getAvailableAudioDeviceNames() const;
     
     // Processing configuration
     void setDoublePrecisionProcessing(bool doublePrecision);
     bool isDoublePrecisionProcessing() const;
     
     // Listener management
     void addChangeListener(ChangeListener* listener);
     void removeChangeListener(ChangeListener* listener);
     
 private:
     // ChangeListener implementation
     void changeListenerCallback(ChangeBroadcaster* source) override;
     
     // Main processor graph hosting all player graphs
     std::unique_ptr<AudioProcessorGraph> mainProcessorGraph;
     
     // Owned audio device manager
     std::unique_ptr<AudioDeviceManager> deviceManager;
     
     // References to shared resources (not owned)
     AudioPluginFormatManager& formatManager;
     KnownPluginList& knownPluginList;
     
     // Track active players and their node IDs in the main graph
     std::map<covertracks::Player*, AudioProcessorGraph::NodeID> activePlayers;
     
     // Resource pool for shared processors
     std::map<String, std::shared_ptr<AudioProcessor>> sharedProcessors;
     
     // Utility method to find a player for a processor
     AudioProcessorPlayer* findPlayerForProcessor(AudioProcessor* processor);
     
     // Processing state
     bool doublePrecisionProcessing = false;
     bool midiEnabled = false;
     
     // Resource management
     OwnedArray<AudioProcessorPlayer> audioProcessorPlayers;
     Array<AudioProcessor*> registeredProcessors;
     
     // Thread safety for processor registration/unregistration
     CriticalSection processorLock;
     
     // Change broadcaster
     ChangeBroadcaster broadcaster;
     
     JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
 };

} // namespace covertracks