#include "Instrument.h"
#include <iostream>

namespace covertracks {

Instrument::Instrument(const String& name, 
                     AudioPluginFormatManager& fm,
                     KnownPluginList& kpl,
                     AudioEngine* engine)
    : instrumentName(name), 
      resourceState(ResourceState::Unloaded),
      audioEngine(engine),
      formatManager(fm),
      knownPluginList(kpl)
{
    std::cout << "Creating Instrument: " << name << std::endl;
    
    // Initialize the plugin graph with the shared format manager and plugin list
    pluginGraph = std::make_unique<PluginGraph>(formatManager, knownPluginList);
    
    // We'll create the processor graph when needed
    
    // Start the timer at 20ms interval (50Hz) for MIDI playback tracking
    startTimer(20);
}

void Instrument::prepareForEditing()
{
    // Load all resources needed for editing
    if (resourceState == ResourceState::Unloaded)
    {
        // Create the processor graph if it doesn't exist
        if (processorGraph == nullptr)
        {
            processorGraph = std::make_unique<AudioProcessorGraph>();
            std::cout << "Created processor graph for " << instrumentName << std::endl;
        }
        
        // Create the plugin graph if it doesn't exist
        if (pluginGraph == nullptr)
        {
            pluginGraph = std::make_unique<PluginGraph>(formatManager, knownPluginList);
            std::cout << "Created plugin graph for " << instrumentName << std::endl;
        }
        
        resourceState = ResourceState::EditReady;
        std::cout << "Instrument " << instrumentName << " prepared for editing" << std::endl;
    }
    
    // Ensure the processor graph is properly configured
    if (processorGraph != nullptr)
    {
        // Check if we have audio output nodes in the graph
        bool hasAudioOutputNode = false;
        for (auto* node : processorGraph->getNodes())
        {
            auto* processor = node->getProcessor();
            if (processor != nullptr && processor->getName().contains("Audio Output"))
            {
                hasAudioOutputNode = true;
                std::cout << "Found audio output node in graph: " << processor->getName() << std::endl;
                break;
            }
        }
        
        if (!hasAudioOutputNode)
        {
            std::cout << "No audio output node found in edit mode, but will be added during playback" << std::endl;
        }
    }
}

bool Instrument::prepareForPlayback()
{
    if (processorGraph == nullptr)
    {
        std::cerr << "Cannot prepare for playback: processor graph is null" << std::endl;
        return false;
    }
    
    // First, ensure we have the full graph state loaded from the PluginGraph
    if (pluginGraph != nullptr)
    {
        // Get the current graph state
        std::unique_ptr<XmlElement> state = pluginGraph->createXml();
        if (state != nullptr)
        {
            // Count nodes and connections in the XML
            int nodeCount = 0;
            int connectionCount = 0;
            
            // Count the processor nodes
            for (auto* child : state->getChildWithTagNameIterator("PROCESSOR"))
            {
                nodeCount++;
                (void)child; // Prevent unused variable warning
            }
            
            // Count the connections
            auto* connectionsElement = state->getChildByName("CONNECTIONS");
            if (connectionsElement != nullptr)
            {
                for (auto* conn : connectionsElement->getChildIterator())
                {
                    connectionCount++;
                    (void)conn; // Prevent unused variable warning
                }
            }
            
            std::cout << "Loading full graph state from PluginGraph before playback" << std::endl;
            std::cout << "Graph XML contains " << nodeCount << " nodes and " << connectionCount << " connections" << std::endl;
            
            // Print the current processor graph state before loading
            if (processorGraph != nullptr)
            {
                std::cout << "Before loading, processor graph has " << processorGraph->getNumNodes() 
                          << " nodes and " << processorGraph->getConnections().size() << " connections" << std::endl;
            }
            
            // Load the state into the processor graph
            loadPluginGraph(*state);
            
            // Print the processor graph state after loading
            if (processorGraph != nullptr)
            {
                std::cout << "After loading, processor graph has " << processorGraph->getNumNodes() 
                          << " nodes and " << processorGraph->getConnections().size() << " connections" << std::endl;
                
                // List all nodes in the graph
                std::cout << "Nodes in processor graph:" << std::endl;
                for (auto* node : processorGraph->getNodes())
                {
                    auto* processor = node->getProcessor();
                    std::cout << "  - Node ID: " << node->nodeID.uid 
                              << ", Name: " << processor->getName() 
                              << ", Inputs: " << processor->getTotalNumInputChannels() 
                              << ", Outputs: " << processor->getTotalNumOutputChannels() 
                              << ", MIDI: " << (processor->acceptsMidi() ? "yes" : "no") 
                              << std::endl;
                }
            }
        }
    }
    
    // Get the current audio device settings
    if (audioEngine != nullptr)
    {
        auto& deviceManager = audioEngine->getDeviceManager();
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        
        if (currentDevice != nullptr)
        {
            double currentSampleRate = currentDevice->getCurrentSampleRate();
            int blockSize = currentDevice->getCurrentBufferSizeSamples();
            
            // Update the member variable
            sampleRate = currentSampleRate;
            
            // First, check if we have audio output nodes in the graph
            bool hasAudioOutputNode = false;
            for (auto* node : processorGraph->getNodes())
            {
                auto* processor = node->getProcessor();
                if (processor != nullptr && processor->getName().contains("Audio Output"))
                {
                    hasAudioOutputNode = true;
                    std::cout << "Found audio output node in graph: " << processor->getName() << std::endl;
                    break;
                }
            }
            
            // If no audio output node found, create one
            if (!hasAudioOutputNode)
            {
                std::cout << "No audio output node found, creating one..." << std::endl;
                
                // Create an audio output node
                auto audioOutputNode = processorGraph->addNode(std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
                    AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
                
                if (audioOutputNode != nullptr)
                {
                    std::cout << "Created audio output node with ID: " << audioOutputNode->nodeID.uid << std::endl;
                }
                else
                {
                    std::cerr << "Failed to create audio output node" << std::endl;
                }
            }
            
            // Check if we have a MIDI input node in the graph
            bool hasMidiInputNode = false;
            for (auto* node : processorGraph->getNodes())
            {
                auto* processor = node->getProcessor();
                if (processor != nullptr && processor->getName().contains("MIDI Input"))
                {
                    hasMidiInputNode = true;
                    std::cout << "Found MIDI input node in graph: " << processor->getName() << std::endl;
                    break;
                }
            }
            
            // If no MIDI input node found, create one
            AudioProcessorGraph::Node::Ptr midiInputNode = nullptr;
            if (!hasMidiInputNode)
            {
                std::cout << "No MIDI input node found, creating one..." << std::endl;
                
                // Create a MIDI input node
                midiInputNode = processorGraph->addNode(std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
                    AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
                
                if (midiInputNode != nullptr)
                {
                    std::cout << "Created MIDI input node with ID: " << midiInputNode->nodeID.uid << std::endl;
                }
                else
                {
                    std::cerr << "Failed to create MIDI input node" << std::endl;
                }
            }
            
            // Now connect all nodes properly
            std::cout << "Setting up graph connections for " << instrumentName << "..." << std::endl;
            
            // First, find the audio output node
            AudioProcessorGraph::Node::Ptr audioOutputNode = nullptr;
            for (auto* node : processorGraph->getNodes())
            {
                auto* processor = node->getProcessor();
                if (processor != nullptr && 
                    dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor) != nullptr &&
                    processor->getName().contains("Audio Output"))
                {
                    audioOutputNode = node;
                    break;
                }
            }
            
            // If we didn't find an existing MIDI input node, use the one we created
            if (!hasMidiInputNode && midiInputNode == nullptr)
            {
                std::cerr << "No MIDI input node available!" << std::endl;
            }
            
            // Find all instrument/synth nodes that can process MIDI
            for (auto* node : processorGraph->getNodes())
            {
                auto* processor = node->getProcessor();
                
                // Skip IO nodes
                if (dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor) != nullptr)
                    continue;
                    
                std::cout << "Found processor node: " << processor->getName() 
                          << ", accepts MIDI: " << (processor->acceptsMidi() ? "yes" : "no")
                          << ", produces MIDI: " << (processor->producesMidi() ? "yes" : "no")
                          << ", audio inputs: " << processor->getTotalNumInputChannels()
                          << ", audio outputs: " << processor->getTotalNumOutputChannels() << std::endl;
                
                // Connect MIDI input to this node if it accepts MIDI
                if (processor->acceptsMidi() && midiInputNode != nullptr)
                {
                    // Connect MIDI input to this processor
                    if (processorGraph->addConnection({ { midiInputNode->nodeID, AudioProcessorGraph::midiChannelIndex }, 
                                                       { node->nodeID, AudioProcessorGraph::midiChannelIndex } }))
                    {
                        std::cout << "Connected MIDI input to node " << node->nodeID.uid << std::endl;
                    }
                    else
                    {
                        std::cerr << "Failed to connect MIDI input to node " << node->nodeID.uid << std::endl;
                    }
                }
                
                // Connect this node to audio output if it produces audio
                if (processor->getTotalNumOutputChannels() > 0 && audioOutputNode != nullptr)
                {
                    // Connect audio outputs to the audio output node
                    int numChannels = std::min(2, processor->getTotalNumOutputChannels());
                    for (int channel = 0; channel < numChannels; ++channel)
                    {
                        if (processorGraph->addConnection({ { node->nodeID, channel }, 
                                                           { audioOutputNode->nodeID, channel } }))
                        {
                            std::cout << "Connected node " << node->nodeID.uid 
                                      << " channel " << channel 
                                      << " to audio output node" << std::endl;
                        }
                        else
                        {
                            std::cerr << "Failed to connect node " << node->nodeID.uid 
                                      << " channel " << channel 
                                      << " to audio output node" << std::endl;
                        }
                    }
                }
            }
            
            // Print the final graph structure
            std::cout << "Final graph structure for " << instrumentName << ":" << std::endl;
            std::cout << "Total nodes: " << processorGraph->getNumNodes() << std::endl;
            
            int connectionCount = 0;
            for (auto& connection : processorGraph->getConnections())
            {
                std::cout << "Connection " << ++connectionCount << ": Node " 
                          << connection.source.nodeID.uid << " channel " << connection.source.channelIndex
                          << " -> Node " << connection.destination.nodeID.uid 
                          << " channel " << connection.destination.channelIndex << std::endl;
            }
            
            // Prepare the processor graph for playback
            processorGraph->setPlayConfigDetails(2, 2, currentSampleRate, blockSize);
            processorGraph->prepareToPlay(currentSampleRate, blockSize);
            
            std::cout << "Prepared graph for playback: sample rate=" << currentSampleRate 
                      << ", block size=" << blockSize << std::endl;
                      
            // Ensure the processor is registered with the AudioEngine
            audioEngine->registerProcessor(processorGraph.get());
            
            // Set the master gain to ensure audio output is active
            audioEngine->setMasterGain(0.8f);
        }
        else
        {
            std::cerr << "No audio device available for playback" << std::endl;
            return false;
        }
    }
    else
    {
        std::cerr << "No audio engine available for playback" << std::endl;
        return false;
    }
    
    return true;
}

void Instrument::releaseResources()
{
    // Release all heavy resources
    if (resourceState != ResourceState::Unloaded)
    {
        // Disconnect from AudioEngine
        disconnectFromAudioEngine();
        
        // Release processor graph resources
        if (processorGraph)
            processorGraph->releaseResources();
        
        resourceState = ResourceState::Unloaded;
        std::cout << "Instrument " << instrumentName << " resources released" << std::endl;
    }
}

bool Instrument::connectToAudioEngine(AudioEngine* engine)
{
    if (engine == nullptr)
        return false;
    
    // Store the engine reference
    audioEngine = engine;
    
    // Register the processor graph
    if (processorGraph && !engine->registerProcessor(processorGraph.get()))
    {
        std::cerr << "Failed to register processor with AudioEngine" << std::endl;
        return false;
    }
    
    // Register for MIDI input
    String midiInputId = instrumentName + "_MidiInput";
    registeredMidiInputs.add(midiInputId);
    
    if (!engine->registerMidiInput(midiInputId, this))
    {
        std::cerr << "Failed to register MIDI input with AudioEngine" << std::endl;
        return false;
    }
    
    return true;
}

void Instrument::disconnectFromAudioEngine()
{
    if (audioEngine)
    {
        // Unregister processor graph
        if (processorGraph)
            audioEngine->unregisterProcessor(processorGraph.get());
        
        // Unregister MIDI inputs
        for (const auto& midiInputId : registeredMidiInputs)
            audioEngine->unregisterMidiInput(midiInputId, this);
        
        registeredMidiInputs.clear();
        
        // Clear engine reference
        audioEngine = nullptr;
    }
}
// Static method to notify all Instruments that the AudioEngine is being destroyed
void Instrument::notifyAudioEngineDestroyed()
{
    std::cout << "[Instrument] Static notification: AudioEngine is being destroyed" << std::endl;
}

Instrument::~Instrument()
{
    // Stop any ongoing playback
    stopPlayback();
    
    // Make sure the AudioProcessorPlayer is properly cleaned up
    if (audioProcessorPlayer != nullptr)
    {
        // First, remove the processor from the player to prevent any further processing
        audioProcessorPlayer->setProcessor(nullptr);
        
        // Unregister from the AudioEngine if we're connected
        if (audioEngine != nullptr)
        {
            auto& deviceManager = audioEngine->getDeviceManager();
            deviceManager.removeAudioCallback(audioProcessorPlayer.get());
        }
        
        // Reset the AudioProcessorPlayer to ensure it's properly cleaned up
        audioProcessorPlayer.reset();
        
        std::cout << "Cleaned up AudioProcessorPlayer in Instrument destructor" << std::endl;
    }
    
    // Release all resources
    releaseResources();
    
    // Disconnect from AudioEngine
    disconnectFromAudioEngine();
    
    std::cout << "[Instrument] Destroyed: " << instrumentName << std::endl;
}

// The initialize method has been replaced by prepareForEditing and prepareForPlayback methods

bool Instrument::loadPluginGraph(const XmlElement& graphState)
{
    if (!pluginGraph)
    {
        std::cerr << "Cannot load plugin graph - no plugin graph exists" << std::endl;
        return false;
    }
    
    // Make sure we have a processor graph
    if (!processorGraph)
    {
        processorGraph = std::make_unique<AudioProcessorGraph>();
    }
    
    // Clear the processor graph first
    processorGraph->clear();
    std::cout << "Cleared processor graph before loading new state" << std::endl;
    
    // First attempt to restore the graph state
    std::cout << "Restoring graph state from XML..." << std::endl;
    
    // Count nodes in the XML
    int nodeCount = 0;
    int connectionCount = 0;
    
    // Count the processor nodes
    for (auto* child : graphState.getChildWithTagNameIterator("PROCESSOR"))
    {
        nodeCount++;
        (void)child; // Prevent unused variable warning
    }
    
    // Count the connections
    auto* connectionsElement = graphState.getChildByName("CONNECTIONS");
    if (connectionsElement != nullptr)
    {
        for (auto* conn : connectionsElement->getChildIterator())
        {
            connectionCount++;
            (void)conn; // Prevent unused variable warning
        }
    }
    
    std::cout << "XML contains " << nodeCount << " nodes and " << connectionCount << " connections" << std::endl;
    
    // Restore from XML
    pluginGraph->restoreFromXml(graphState);
    
    // Print the graph state after restoration
    auto& graphRef = pluginGraph->graph;
    std::cout << "After restoration, graph has " << graphRef.getNumNodes() << " nodes and " 
              << graphRef.getConnections().size() << " connections" << std::endl;
    
    // Check for audio output nodes with 0 input channels
    bool outputNodesFixed = false;
    
    // Get all nodes in the graph
    auto& graph = pluginGraph->graph;
    
    // Check if there are any non-IO processor nodes (actual instruments/effects)
    bool hasInstrumentNode = false;
    for (auto* node : graph.getNodes())
    {
        auto* processor = node->getProcessor();
        auto* ioProcessor = dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor);
        
        // If this is not an IO processor, it's an instrument or effect
        if (ioProcessor == nullptr)
        {
            hasInstrumentNode = true;
            std::cout << "Found instrument/effect node: " << processor->getName() << std::endl;
        }
        
        // Check if this is an audio output node
        if (ioProcessor != nullptr && ioProcessor->getType() == AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode)
        {
            std::cout << "Found audio output node (ID: " << node->nodeID.uid << "), current inputs: " 
                      << processor->getTotalNumInputChannels() << std::endl;
            
            // If the audio output node has 0 input channels, we need to fix it
            if (processor->getTotalNumInputChannels() == 0)
            {
                // Configure the node for stereo input - try multiple approaches
                std::cout << "Fixing audio output node (ID: " << node->nodeID.uid << ") with 0 input channels" << std::endl;
                bool fixed = false;
                
                // Approach 1: Set buses layout
                AudioProcessor::BusesLayout layout = processor->getBusesLayout();
                if (layout.inputBuses.size() > 0)
                {
                    layout.inputBuses[0] = AudioChannelSet::stereo();
                    if (processor->setBusesLayout(layout))
                    {
                        std::cout << "Approach 1 (setBusesLayout): success" << std::endl;
                        fixed = true;
                    }
                    else
                    {
                        std::cout << "Approach 1 (setBusesLayout): failed" << std::endl;
                    }
                }
                
                // Approach 2: Enable bus and set layout
                if (!fixed || processor->getTotalNumInputChannels() == 0)
                {
                    if (processor->enableAllBuses())
                    {
                        std::cout << "Approach 2 (enable bus): success" << std::endl;
                    }
                    else
                    {
                        std::cout << "Approach 2 (enable bus): failed" << std::endl;
                    }
                    
                    if (processor->setChannelLayoutOfBus(true, 0, AudioChannelSet::stereo()))
                    {
                        std::cout << "Approach 2 (setCurrentLayout): success" << std::endl;
                        fixed = true;
                    }
                    else
                    {
                        std::cout << "Approach 2 (setCurrentLayout): failed" << std::endl;
                    }
                }
                
                if (fixed)
                {
                    outputNodesFixed = true;
                    std::cout << "Successfully fixed audio output node to have " 
                              << processor->getTotalNumInputChannels() << " input channels" << std::endl;
                }
                else
                {
                    std::cerr << "Failed to fix audio output node!" << std::endl;
                }
            }
        }
    }
    
    // Check if there's an instrument node in the graph
    if (!hasInstrumentNode)
    {
        std::cout << "WARNING: No instrument node found in graph. Audio output may not work!" << std::endl;
        // We won't add a default synth, as the user may be testing with specific plugins
    }
    
    // If we fixed any output nodes or added a synth, we need to restore connections again
    if (outputNodesFixed)
    {
        std::cout << "Re-restoring connections after fixing audio output nodes" << std::endl;
        pluginGraph->restoreFromXml(graphState);
    }
    
    // Now we need to transfer the nodes and connections from the PluginGraph to the processorGraph
    std::cout << "Transferring nodes and connections from PluginGraph to processorGraph..." << std::endl;
    
    // Map of PluginGraph node IDs to processorGraph node IDs
    std::map<AudioProcessorGraph::NodeID, AudioProcessorGraph::NodeID> nodeIdMap;
    
    // First, create all the nodes in the processorGraph
    for (auto* node : pluginGraph->graph.getNodes())
    {
        auto* processor = node->getProcessor();
        auto* ioProcessor = dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor);
        
        // Create a copy of the processor for the processorGraph
        AudioProcessorGraph::Node::Ptr newNode;
        
        if (ioProcessor != nullptr)
        {
            // For IO processors, we need to create a new IO processor of the same type
            if (ioProcessor->getType() == AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode)
            {
                newNode = processorGraph->addNode(std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
                    AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
                std::cout << "Created audio output node in processorGraph with ID: " << newNode->nodeID.uid << std::endl;
            }
            else if (ioProcessor->getType() == AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode)
            {
                newNode = processorGraph->addNode(std::make_unique<AudioProcessorGraph::AudioGraphIOProcessor>(
                    AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
                std::cout << "Created MIDI input node in processorGraph with ID: " << newNode->nodeID.uid << std::endl;
            }
        }
        else
        {
            // For instrument/effect processors, we need to clone them
            // Since we can't clone processors directly, we'll use the plugin state to recreate them
            MemoryBlock stateData;
            processor->getStateInformation(stateData);
            
            // Unfortunately, we can't directly get the plugin description from an AudioProcessor
            // We'll need to use the plugin's name and try to find a matching plugin
            String processorName = processor->getName();
            
            std::cout << "Recreating instrument node: " << processorName << " in processorGraph" << std::endl;
            
            // Create a new instance of the plugin
            if (audioEngine != nullptr)
            {
                auto& engineFormatManager = audioEngine->getFormatManager();
                auto& enginePluginList = audioEngine->getKnownPluginList();
                String errorMessage;
                bool pluginCreated = false;
                
                // First, try to find the plugin in the KnownPluginList by name
                for (int i = 0; i < enginePluginList.getNumTypes(); ++i)
                {
                    // Get a copy of the plugin description
                    PluginDescription pluginDesc = enginePluginList.getTypes()[i];
                    if (pluginDesc.name == processorName)
                    {
                        std::cout << "Found matching plugin in KnownPluginList: " << pluginDesc.name << std::endl;
                        
                        double currentSampleRate = audioEngine->getSampleRate();
                        int currentBlockSize = audioEngine->getBufferSize();
                        
                        // Create the plugin instance
                        std::unique_ptr<AudioPluginInstance> newPlugin = 
                            engineFormatManager.createPluginInstance(pluginDesc, currentSampleRate, 
                                                                   currentBlockSize, errorMessage);
                        
                        if (newPlugin != nullptr)
                        {
                            // Set the plugin state
                            newPlugin->setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
                            
                            // Add the plugin to the processorGraph
                            newNode = processorGraph->addNode(std::move(newPlugin));
                            std::cout << "Created external plugin node in processorGraph with ID: " << newNode->nodeID.uid << std::endl;
                            pluginCreated = true;
                            break;
                        }
                        else
                        {
                            std::cerr << "Failed to create external plugin instance: " << errorMessage << std::endl;
                        }
                    }
                }
                
                // If we couldn't find the plugin in the KnownPluginList, try internal plugins
                if (!pluginCreated)
                {
                    std::cout << "Plugin not found in KnownPluginList, trying internal plugins..." << std::endl;
                    
                    // Check if it's an internal plugin
                    AudioPluginFormat* internalFormat = nullptr;
                    for (int i = 0; i < engineFormatManager.getNumFormats(); ++i)
                    {
                        auto* f = engineFormatManager.getFormat(i);
                        if (f->getName() == "Internal")
                        {
                            internalFormat = f;
                            break;
                        }
                    }
                    
                    if (internalFormat != nullptr)
                    {
                        // Try to create an internal plugin with the same name
                        PluginDescription internalDesc;
                        internalDesc.name = processorName;
                        internalDesc.pluginFormatName = "Internal";
                        
                        double currentSampleRate = audioEngine->getSampleRate();
                        int currentBlockSize = audioEngine->getBufferSize();
                        
                        std::unique_ptr<AudioPluginInstance> newPlugin(
                            internalFormat->createInstanceFromDescription(internalDesc, currentSampleRate, currentBlockSize, errorMessage));
                        
                        if (newPlugin != nullptr)
                        {
                            // Set the plugin state
                            newPlugin->setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));
                            
                            // Add the plugin to the processorGraph
                            newNode = processorGraph->addNode(std::move(newPlugin));
                            std::cout << "Created internal plugin node in processorGraph with ID: " << newNode->nodeID.uid << std::endl;
                            pluginCreated = true;
                        }
                        else
                        {
                            std::cerr << "Failed to create internal plugin instance: " << errorMessage << std::endl;
                        }
                    }
                    else
                    {
                        std::cerr << "Could not find Internal plugin format" << std::endl;
                    }
                }
                
                if (!pluginCreated)
                {
                    std::cerr << "CRITICAL ERROR: Could not recreate plugin: " << processorName << std::endl;
                    std::cerr << "This will result in no audio output for this instrument." << std::endl;
                }
            }
        }
        
        if (newNode != nullptr)
        {
            // Store the mapping between the original node ID and the new node ID
            nodeIdMap[node->nodeID] = newNode->nodeID;
        }
    }
    
    // Now create all the connections in the processorGraph
    for (auto& connection : pluginGraph->graph.getConnections())
    {
        // Map the source and destination node IDs
        auto sourceIt = nodeIdMap.find(connection.source.nodeID);
        auto destIt = nodeIdMap.find(connection.destination.nodeID);
        
        if (sourceIt != nodeIdMap.end() && destIt != nodeIdMap.end())
        {
            // Create the connection in the processorGraph
            if (processorGraph->addConnection({ { sourceIt->second, connection.source.channelIndex },
                                               { destIt->second, connection.destination.channelIndex } }))
            {
                std::cout << "Created connection in processorGraph: Node " << sourceIt->second.uid 
                          << " channel " << connection.source.channelIndex
                          << " -> Node " << destIt->second.uid 
                          << " channel " << connection.destination.channelIndex << std::endl;
            }
            else
            {
                std::cerr << "Failed to create connection in processorGraph" << std::endl;
            }
        }
    }
    
    // Print the final state of the processorGraph
    std::cout << "Final processorGraph state: " << processorGraph->getNumNodes() << " nodes and "
              << processorGraph->getConnections().size() << " connections" << std::endl;
    
    return true;
}

std::unique_ptr<XmlElement> Instrument::getPluginGraphState()
{
    if (!pluginGraph)
    {
        std::cerr << "Cannot get plugin graph state - no plugin graph exists" << std::endl;
        return nullptr;
    }
    
    return pluginGraph->createXml();
}


// The detailed implementation is already defined above

bool Instrument::playMIDISequence(const MidiMessageSequence& sequence, double newTempo)
{
    // Stop any current playback
    stopPlayback();
    
    // Set the new tempo if provided
    if (newTempo > 0)
    {
        tempo = newTempo;
    }
    
    // Make sure we're prepared for playback
    if (resourceState != ResourceState::PlaybackReady)
    {
        std::cout << "Preparing instrument for playback before playing sequence" << std::endl;
        if (!prepareForPlayback())
        {
            std::cerr << "Failed to prepare for playback" << std::endl;
            return false;
        }
    }
    
    // Ensure we have a valid processor graph
    if (processorGraph == nullptr)
    {
        std::cerr << "Cannot play sequence: processor graph is null" << std::endl;
        return false;
    }
    
    // Make sure the processor is registered with the AudioEngine
    if (audioEngine == nullptr)
    {
        std::cerr << "No audio engine available for playback" << std::endl;
        return false;
    }
    
    // Set master gain to a very low level to avoid distortion
    audioEngine->setMasterGain(0.3f);
    
    // Use the provided MIDI sequence directly
    currentSequence = std::make_unique<MidiMessageSequence>(sequence);
    
    if (currentSequence == nullptr || currentSequence->getNumEvents() == 0)
    {
        std::cerr << "Failed to use MIDI sequence (empty or null)" << std::endl;
        return false;
    }
    
    // Calculate the total duration of the sequence
    totalDuration = 0.0;
    
    for (int i = 0; i < currentSequence->getNumEvents(); ++i)
    {
        auto event = currentSequence->getEventPointer(i);
        if (event->message.isNoteOff())
        {
            totalDuration = std::max(totalDuration, event->message.getTimeStamp());
        }
    }
    
    // Sort the MIDI sequence by timestamp to ensure proper ordering
    currentSequence->sort();
    
    // Get the current sample rate from the audio device
    double currentSampleRate = 44100.0; // Default fallback
    int bufferSize = 512; // Default buffer size
    auto& deviceManager = audioEngine->getDeviceManager();
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    if (currentDevice != nullptr)
    {
        currentSampleRate = currentDevice->getCurrentSampleRate();
        bufferSize = currentDevice->getCurrentBufferSizeSamples();
    }
    
    // IMPORTANT CHANGE: Bypass AudioProcessorPlayer completely
    // Instead, we'll directly process MIDI events through the processor graph
    
    // If we have an AudioProcessorPlayer, remove it from the audio callbacks
    if (audioProcessorPlayer != nullptr)
    {
        deviceManager.removeAudioCallback(audioProcessorPlayer.get());
        audioProcessorPlayer = nullptr; // We won't be using it
    }
    
    // Make sure the processor graph is prepared for playback
    processorGraph->setPlayConfigDetails(2, 2, currentSampleRate, bufferSize);
    processorGraph->prepareToPlay(currentSampleRate, bufferSize);
    
    // Create a temporary buffer for audio processing
    AudioBuffer<float> tempBuffer(2, bufferSize);
    
    // Process all MIDI events directly
    std::cout << "Processing MIDI events directly through processor graph" << std::endl;
    std::cout << "Total events: " << currentSequence->getNumEvents() << std::endl;
    
    // Create a map to store note-on/note-off pairs to ensure proper note durations
    std::map<int, MidiMessage> activeNotes; // note number -> note-off message
    
    // First pass: Reduce velocities of all note-on events to minimize distortion
    // and ensure all events have proper timestamps
    std::cout << "Preparing MIDI events for playback:" << std::endl;
    for (int i = 0; i < currentSequence->getNumEvents(); ++i)
    {
        auto event = currentSequence->getEventPointer(i);
        double originalTimestamp = event->message.getTimeStamp();
        
        if (event->message.isNoteOn())
        {
            // Reduce velocity to 50% of original for better audio quality
            uint8 newVelocity = static_cast<uint8>(event->message.getVelocity() * 0.5f);
            
            // Create a fresh note-on message
            event->message = MidiMessage::noteOn(
                event->message.getChannel(),
                event->message.getNoteNumber(),
                newVelocity);
                
            // Preserve the original timestamp
            event->message.setTimeStamp(originalTimestamp);
            
            std::cout << "Prepared note on: channel=" << event->message.getChannel() 
                      << ", note=" << event->message.getNoteNumber() 
                      << ", velocity=" << static_cast<int>(newVelocity) 
                      << ", timestamp=" << originalTimestamp << std::endl;
        }
        else if (event->message.isNoteOff())
        {
            std::cout << "Prepared note off: channel=" << event->message.getChannel() 
                      << ", note=" << event->message.getNoteNumber() 
                      << ", timestamp=" << originalTimestamp << std::endl;
        }
    }
    
    // Register the processor with the AudioEngine to ensure it's in the audio chain
    audioEngine->registerProcessor(processorGraph.get());
    
    // Reset playback state
    nextEventIndex = 0;
    currentTime = 0.0;
    playing = true;
    paused = false;
    
    // Start a timer to process MIDI events from the sequence
    // We'll use a slightly longer interval to reduce CPU usage
    startTimer(5); // Check every 5ms for responsive playback
    
    std::cout << "Started MIDI sequence playback on " << instrumentName 
              << " (tempo: " << tempo << " BPM, duration: " << totalDuration << " ticks)" << std::endl;
    
    return true;
}

bool Instrument::playMIDIPattern(const var& pattern, double newTempo)
{
    // Stop any current playback
    stopPlayback();
    
    // Set the new tempo if provided
    if (newTempo > 0)
    {
        tempo = newTempo;
    }
    
    // Make sure we're prepared for playback
    if (resourceState != ResourceState::PlaybackReady)
    {
        std::cout << "Preparing instrument for playback before playing pattern" << std::endl;
        if (!prepareForPlayback())
        {
            std::cerr << "Failed to prepare for playback" << std::endl;
            return false;
        }
    }
    
    // Ensure we have a valid processor graph
    if (processorGraph == nullptr)
    {
        std::cerr << "Cannot play pattern: processor graph is null" << std::endl;
        return false;
    }
    
    // Make sure the processor is registered with the AudioEngine
    if (audioEngine == nullptr)
    {
        std::cerr << "No audio engine available for playback" << std::endl;
        return false;
    }
    
    // Set master gain to a very low level to avoid distortion
    audioEngine->setMasterGain(0.3f);
    
    // Convert the pattern to a MIDI sequence
    currentSequence = convertJSONToMIDISequence(pattern);
    
    if (currentSequence == nullptr || currentSequence->getNumEvents() == 0)
    {
        std::cerr << "Failed to create MIDI sequence from pattern" << std::endl;
        return false;
    }
    
    // Calculate the total duration of the pattern
    totalDuration = 0.0;
    
    for (int i = 0; i < currentSequence->getNumEvents(); ++i)
    {
        auto event = currentSequence->getEventPointer(i);
        if (event->message.isNoteOff())
        {
            totalDuration = std::max(totalDuration, event->message.getTimeStamp());
        }
    }
    
    // Sort the MIDI sequence by timestamp to ensure proper ordering
    currentSequence->sort();
    
    // Get the current sample rate from the audio device
    double currentSampleRate = 44100.0; // Default fallback
    int bufferSize = 512; // Default buffer size
    auto& deviceManager = audioEngine->getDeviceManager();
    auto* currentDevice = deviceManager.getCurrentAudioDevice();
    if (currentDevice != nullptr)
    {
        currentSampleRate = currentDevice->getCurrentSampleRate();
        bufferSize = currentDevice->getCurrentBufferSizeSamples();
    }
    
    // IMPORTANT CHANGE: Bypass AudioProcessorPlayer completely
    // Instead, we'll directly process MIDI events through the processor graph
    
    // If we have an AudioProcessorPlayer, remove it from the audio callbacks
    if (audioProcessorPlayer != nullptr)
    {
        deviceManager.removeAudioCallback(audioProcessorPlayer.get());
        audioProcessorPlayer = nullptr; // We won't be using it
    }
    
    // Make sure the processor graph is prepared for playback
    processorGraph->setPlayConfigDetails(2, 2, currentSampleRate, bufferSize);
    processorGraph->prepareToPlay(currentSampleRate, bufferSize);
    
    // Create a temporary buffer for audio processing
    AudioBuffer<float> tempBuffer(2, bufferSize);
    
    // Process all MIDI events directly
    std::cout << "Processing MIDI events directly through processor graph" << std::endl;
    std::cout << "Total events: " << currentSequence->getNumEvents() << std::endl;
    
    // Create a map to store note-on/note-off pairs to ensure proper note durations
    std::map<int, MidiMessage> activeNotes; // note number -> note-off message
    
    // First pass: Reduce velocities of all note-on events to minimize distortion
    // and ensure all events have proper timestamps
    std::cout << "Preparing MIDI events for playback:" << std::endl;
    for (int i = 0; i < currentSequence->getNumEvents(); ++i)
    {
        auto event = currentSequence->getEventPointer(i);
        double originalTimestamp = event->message.getTimeStamp();
        
        if (event->message.isNoteOn())
        {
            // Reduce velocity to 50% of original for better audio quality
            uint8 newVelocity = static_cast<uint8>(event->message.getVelocity() * 0.5f);
            
            // Create a fresh note-on message
            event->message = MidiMessage::noteOn(
                event->message.getChannel(),
                event->message.getNoteNumber(),
                newVelocity);
                
            // Preserve the original timestamp
            event->message.setTimeStamp(originalTimestamp);
            
            std::cout << "  Prepared note on: channel=" << (int)event->message.getChannel()
                      << ", note=" << event->message.getNoteNumber()
                      << ", velocity=" << (int)newVelocity
                      << ", timestamp=" << originalTimestamp << std::endl;
        }
        else if (event->message.isNoteOff())
        {
            // Create a fresh note-off message
            event->message = MidiMessage::noteOff(
                event->message.getChannel(),
                event->message.getNoteNumber());
                
            // Preserve the original timestamp
            event->message.setTimeStamp(originalTimestamp);
            
            std::cout << "  Prepared note off: channel=" << (int)event->message.getChannel()
                      << ", note=" << event->message.getNoteNumber()
                      << ", timestamp=" << originalTimestamp << std::endl;
        }
    }
    
    // Register the processor with the AudioEngine to ensure it's in the audio chain
    audioEngine->registerProcessor(processorGraph.get());
    
    // Reset playback state
    nextEventIndex = 0;
    currentTime = 0.0;
    playing = true;
    paused = false;
    
    // Start a timer to process MIDI events from the pattern
    // We'll use a slightly longer interval to reduce CPU usage
    startTimer(5); // Check every 5ms for responsive playback
    
    std::cout << "Started MIDI pattern playback on " << instrumentName 
              << " (tempo: " << tempo << " BPM, duration: " << totalDuration << " ticks)" << std::endl;
    
    return true;
}

void Instrument::stopPlayback()
{
    // Only stop if we're actually playing
    if (!playing)
        return;
    
    std::cout << "Stopping playback for player: " << instrumentName << std::endl;
    
    // Stop the timer first to prevent any further callbacks
    stopTimer();
    
    // Reset playback state
    playing = false;
    paused = false;
    nextEventIndex = 0;
    currentTime = 0.0;
    currentSequence.reset();
    
    // Clean up the AudioProcessorPlayer if it exists
    if (audioProcessorPlayer != nullptr)
    {
        // Unregister from the AudioEngine if we're connected
        if (audioEngine != nullptr)
        {
            auto& deviceManager = audioEngine->getDeviceManager();
            
            // First, remove the processor from the player to prevent any further processing
            audioProcessorPlayer->setProcessor(nullptr);
            
            // Then, remove the audio callback
            deviceManager.removeAudioCallback(audioProcessorPlayer.get());
        }
        else
        {
            // Still remove the processor even if we don't have an audio engine
            audioProcessorPlayer->setProcessor(nullptr);
        }
        
        // Reset the AudioProcessorPlayer to ensure it's properly cleaned up
        audioProcessorPlayer.reset();
        
        std::cout << "Unregistered and released AudioProcessorPlayer" << std::endl;
    }
    
    // Notify that playback has stopped
    if (onPatternFinished)
    {
        onPatternFinished();
    }
    
    std::cout << "Stopped MIDI pattern playback on " << instrumentName << std::endl;
}

void Instrument::pausePlayback()
{
    if (playing && !paused)
    {
        // Send all notes off
        MidiBuffer allNotesOff;
        
        for (int channel = 1; channel <= 16; ++channel)
        {
            allNotesOff.addEvent(MidiMessage::allNotesOff(channel), 0);
            allNotesOff.addEvent(MidiMessage::allControllersOff(channel), 0);
        }
        
        // Process the all notes off messages
        AudioBuffer<float> tempBuffer(2, 512);
        MidiBuffer midiBuffer(allNotesOff);
        if (processorGraph)
            processorGraph->processBlock(tempBuffer, midiBuffer);
        
        std::cout << "Paused MIDI pattern playback on " << instrumentName << std::endl;
    }
    
    paused = true;
}

void Instrument::resumePlayback()
{
    if (playing && paused)
    {
        paused = false;
        std::cout << "Resumed MIDI pattern playback on " << instrumentName << std::endl;
    }
}

bool Instrument::isPlaying() const
{
    return playing && !paused;
}

bool Instrument::isPaused() const
{
    return playing && paused;
}


void Instrument::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message)
{
    // Forward the message to any registered listeners
    for (auto* listener : midiInputListeners)
    {
        listener->handleIncomingMidiMessage(source, message);
    }
    
    // If we're not playing a pattern, we can process live MIDI input
    if (!playing)
    {
        // Process the MIDI message through the plugin graph
        MessageManager::callAsync([this, message]()
        {
            MidiBuffer tempBuffer;
            tempBuffer.addEvent(message, 0);
            
            AudioBuffer<float> tempAudioBuffer(2, 512);
            tempAudioBuffer.clear();
            
            if (processorGraph)
                processorGraph->processBlock(tempAudioBuffer, tempBuffer);
        });
    }
}

void Instrument::addMidiInputListener(MidiInputCallback* listener)
{
    if (listener != nullptr && !midiInputListeners.contains(listener))
    {
        midiInputListeners.add(listener);
    }
}

void Instrument::removeMidiInputListener(MidiInputCallback* listener)
{
    if (listener != nullptr)
    {
        midiInputListeners.removeFirstMatchingValue(listener);
    }
}

void Instrument::timerCallback()
{
    // Process MIDI events directly through the processor graph
    if (playing && !paused && currentSequence != nullptr && processorGraph != nullptr)
    {
        // Calculate the current time in seconds
        double currentTimeInSeconds = currentTime;
        
        // Calculate the time for the next block (using proper audio buffer timing)
        // Use a reasonable buffer size equivalent for timer-based processing
        double bufferTime = 512.0 / 44100.0; // ~11.6ms at 44.1kHz, more realistic than 5ms
        double blockEndTime = currentTimeInSeconds + bufferTime;
        
        // Create a MIDI buffer for this time window
        MidiBuffer midiBuffer;
        bool addedEvents = false;
        
        // Add all events that fall within this time window to the buffer
        while (nextEventIndex < currentSequence->getNumEvents())
        {
            auto event = currentSequence->getEventPointer(nextEventIndex);
            double eventTime = event->message.getTimeStamp();
            
            if (eventTime >= blockEndTime)
            {
                break; // This event is for a future block
            }
            
            // Important: Process all events in the time window, not just those >= currentTimeInSeconds
            // This ensures we don't miss any events due to timing precision issues
            
            // Create a copy of the message to modify if needed
            MidiMessage messageCopy = event->message;
            
            // For note-on events, make sure we use the reduced velocity
            if (messageCopy.isNoteOn())
            {
                // Use the velocity that was set in the pattern (already reduced in playMIDIPattern)
                // but create a fresh note-on message to ensure it's properly processed
                messageCopy = MidiMessage::noteOn(
                    messageCopy.getChannel(),
                    messageCopy.getNoteNumber(),
                    messageCopy.getVelocity());
                    
                std::cout << "Processing note on: channel=" << (int)messageCopy.getChannel() 
                          << ", note=" << messageCopy.getNoteNumber() 
                          << ", velocity=" << (int)messageCopy.getVelocity() 
                          << ", pattern time=" << eventTime << std::endl;
            }
            else if (messageCopy.isNoteOff())
            {
                // Create a fresh note-off message to ensure it's properly processed
                messageCopy = MidiMessage::noteOff(
                    messageCopy.getChannel(),
                    messageCopy.getNoteNumber());
                    
                std::cout << "Processing note off: channel=" << (int)messageCopy.getChannel() 
                          << ", note=" << messageCopy.getNoteNumber() 
                          << ", pattern time=" << eventTime << std::endl;
            }
            
            // Add the event to our buffer at sample position 0 (immediate)
            midiBuffer.addEvent(messageCopy, 0);
            addedEvents = true;
            nextEventIndex++;
        }
        
        // If we have events to process, send them directly to the processor graph
        if (!midiBuffer.isEmpty())
        {
            // Get the current buffer size from the audio device
            int bufferSize = 512; // Default buffer size
            auto& deviceManager = audioEngine->getDeviceManager();
            auto* currentDevice = deviceManager.getCurrentAudioDevice();
            if (currentDevice != nullptr)
            {
                bufferSize = currentDevice->getCurrentBufferSizeSamples();
            }
            
            // Create a temporary audio buffer
            AudioBuffer<float> tempBuffer(2, bufferSize);
            tempBuffer.clear();
            
            // Process the MIDI events directly through the processor graph
            processorGraph->processBlock(tempBuffer, midiBuffer);
        }
        
        // Update the current time
        currentTime = blockEndTime;
        
        // If we've reached the end of the sequence, schedule stopping playback
        if (nextEventIndex >= currentSequence->getNumEvents() && !addedEvents)
        {
            // Stop the timer to prevent further callbacks
            stopTimer();
            
            // All events have been processed, stop playback after a short delay
            // to allow the last notes to finish playing
            static bool stopScheduled = false;
            
            if (!stopScheduled)
            {
                stopScheduled = true;
                std::cout << "Scheduling playback to stop after delay..." << std::endl;
                
                Timer::callAfterDelay(1000, [this]()
                {
                    this->stopPlayback();
                    stopScheduled = false;
                });
            }
        }
    }
}

void Instrument::setPatternFinishedCallback(std::function<void()> callback)
{
    onPatternFinished = std::move(callback);
}

void Instrument::processMIDIEvents(MidiBuffer& midiBuffer, int numSamples)
{
    if (currentSequence == nullptr || nextEventIndex >= currentSequence->getNumEvents())
    {
        return;
    }
    
    // Calculate the time range for this block
    double blockStartTime = currentTime;
    double blockEndTime = blockStartTime + (numSamples / sampleRate); // Real-time seconds (no tempo scaling for playback speed)
    
    // Add all events that fall within this time range
    bool addedEvents = false;
    int eventsAdded = 0;
    
    while (nextEventIndex < currentSequence->getNumEvents())
    {
        auto event = currentSequence->getEventPointer(nextEventIndex);
        double eventTime = event->message.getTimeStamp();
        
        if (eventTime >= blockEndTime)
        {
            break;
        }
        
        if (eventTime >= blockStartTime)
        {
            // Calculate the sample position within this block
            double normalizedPosition = (eventTime - blockStartTime) / (blockEndTime - blockStartTime);
            int samplePosition = static_cast<int>(normalizedPosition * numSamples);
            samplePosition = jlimit(0, numSamples - 1, samplePosition);
            
            // Add the event to the buffer
            midiBuffer.addEvent(event->message, samplePosition);
            
            // Log the event (for debugging)
            if (event->message.isNoteOn())
            {
                std::cout << "Added note on event: channel=" << event->message.getChannel() 
                          << ", note=" << event->message.getNoteNumber() 
                          << ", velocity=" << event->message.getVelocity() 
                          << ", position=" << samplePosition 
                          << ", time=" << eventTime << std::endl;
            }
            else if (event->message.isNoteOff())
            {
                std::cout << "Added note off event: channel=" << event->message.getChannel() 
                          << ", note=" << event->message.getNoteNumber() 
                          << ", position=" << samplePosition 
                          << ", time=" << eventTime << std::endl;
            }
            
            addedEvents = true;
            eventsAdded++;
        }
        
        nextEventIndex++;
    }
    
    // If we've reached the end of the sequence and didn't add any events,
    // we should stop playback
    if (nextEventIndex >= currentSequence->getNumEvents() && !addedEvents)
    {
        // Schedule a stop on the message thread to avoid thread issues
        MessageManager::callAsync([this]()
        {
            this->stopPlayback();
        });
    }
    else if (eventsAdded > 0)
    {
        std::cout << "Added " << eventsAdded << " MIDI events to buffer for " << instrumentName << std::endl;
    }
}

void Instrument::updateCurrentTime(int numSamples)
{
    if (playing && !paused)
    {
        // Update the current time based on the number of samples processed
        // Convert to real-time seconds (tempo affects beat interpretation, not playback speed)
        double blockDuration = (numSamples / sampleRate);
        currentTime += blockDuration;
        
        // Log the current time periodically
        static int logCounter = 0;
        if (++logCounter % 100 == 0) // Log every 100th update to avoid flooding the console
        {
            std::cout << "Current playback time for " << instrumentName << ": " 
                      << currentTime << " seconds (" 
                      << (currentTime / (60.0 / tempo)) << " beats)" << std::endl;
        }
    }
}

std::unique_ptr<MidiMessageSequence> Instrument::convertJSONToMIDISequence(const var& pattern)
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    try
    {
        // First, check if this is a pattern object with an events array
        var events = pattern;
        
        if (pattern.hasProperty("events") && pattern["events"].isArray())
        {
            events = pattern["events"];
            std::cout << "Found events array in pattern object" << std::endl;
        }
        else if (!events.isArray())
        {
            throw std::runtime_error("Pattern must contain an events array");
        }
        
        std::cout << "Processing " << events.size() << " MIDI events for pattern" << std::endl;
        
        for (int i = 0; i < events.size(); ++i)
        {
            auto event = events[i];
            
            if (!event.isObject())
            {
                std::cerr << "Event at index " << i << " is not an object, skipping" << std::endl;
                continue;
            }
            
            // Extract event properties
            String type = event.getProperty("type", "note_on").toString();
            int channel = static_cast<int>(static_cast<double>(event.getProperty("channel", 1)));
            int note = static_cast<int>(static_cast<double>(event.getProperty("note", 60)));
            float velocity = static_cast<float>(static_cast<double>(event.getProperty("velocity", 0.5)));
            double timestamp = static_cast<double>(event.getProperty("timestamp", 0.0));
            double duration = static_cast<double>(event.getProperty("duration", 0.25)); // Default quarter note duration
            
            
            // Ensure channel is in valid range (1-16)
            channel = jlimit(1, 16, channel);
            
            // Ensure note is in valid range (0-127)
            note = jlimit(0, 127, note);
            
            // Ensure velocity is in valid range (0.0-1.0)
            velocity = jlimit(0.0f, 1.0f, velocity);
            
            if (type == "note_on")
            {
                // Create note on message
                MidiMessage noteOn = MidiMessage::noteOn(channel, note, velocity);
                noteOn.setTimeStamp(timestamp);
                sequence->addEvent(noteOn);
                
                std::cout << "Added note_on event: channel=" << channel 
                          << ", note=" << note 
                          << ", velocity=" << velocity 
                          << ", timestamp=" << timestamp << std::endl;
                
                // Create corresponding note off message based on duration
                double noteOffTimestamp = timestamp + duration;
                MidiMessage noteOff = MidiMessage::noteOff(channel, note, velocity);
                noteOff.setTimeStamp(noteOffTimestamp);
                sequence->addEvent(noteOff);
                
                std::cout << "Added note_off event: channel=" << channel 
                          << ", note=" << note 
                          << ", velocity=" << velocity 
                          << ", timestamp=" << noteOffTimestamp 
                          << ", duration=" << duration << std::endl;
            }
            else if (type == "note_off")
            {
                // Create note off message
                MidiMessage noteOff = MidiMessage::noteOff(channel, note, velocity);
                noteOff.setTimeStamp(timestamp);
                sequence->addEvent(noteOff);
                
                std::cout << "Added note_off event: channel=" << channel 
                          << ", note=" << note 
                          << ", timestamp=" << timestamp << std::endl;
            }
            else if (type == "controller")
            {
                // Handle controller messages
                int controllerNumber = static_cast<int>(static_cast<double>(event.getProperty("controller", 0)));
                int value = static_cast<int>(static_cast<double>(event.getProperty("value", 0)));
                
                // Ensure controller number is in valid range (0-127)
                controllerNumber = jlimit(0, 127, controllerNumber);
                
                // Ensure value is in valid range (0-127)
                value = jlimit(0, 127, value);
                
                MidiMessage cc = MidiMessage::controllerEvent(channel, controllerNumber, value);
                cc.setTimeStamp(timestamp);
                sequence->addEvent(cc);
                
                std::cout << "Added controller event: channel=" << channel 
                          << ", controller=" << controllerNumber 
                          << ", value=" << value 
                          << ", timestamp=" << timestamp << std::endl;
            }
            else if (type == "program_change")
            {
                // Handle program change messages
                int programNumber = static_cast<int>(static_cast<double>(event.getProperty("program", 0)));
                
                // Ensure program number is in valid range (0-127)
                programNumber = jlimit(0, 127, programNumber);
                
                MidiMessage pc = MidiMessage::programChange(channel, programNumber);
                pc.setTimeStamp(timestamp);
                sequence->addEvent(pc);
                
                std::cout << "Added program_change event: channel=" << channel 
                          << ", program=" << programNumber 
                          << ", timestamp=" << timestamp << std::endl;
            }
            else
            {
                std::cerr << "Unknown event type: " << type << ", skipping" << std::endl;
            }
        }
        
        // Sort the sequence by timestamp
        sequence->sort();
        
        // Add an all notes off message at the end to ensure clean playback
        double endTime = 0.0;
        
        if (sequence->getNumEvents() > 0)
        {
            endTime = sequence->getEndTime() + 0.5; // Add a half-second buffer
        }
        else
        {
            endTime = 1.0; // Default to 1 second if no events
        }
        
        for (int channel = 1; channel <= 16; ++channel)
        {
            MidiMessage allNotesOff = MidiMessage::allNotesOff(channel);
            allNotesOff.setTimeStamp(endTime);
            sequence->addEvent(allNotesOff);
        }
        
        // Re-sort after adding all notes off messages
        sequence->sort();
        
        std::cout << "Converted JSON to MIDI sequence with " 
                 << sequence->getNumEvents() << " events, duration: " 
                 << sequence->getEndTime() << " seconds" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing JSON MIDI events: " << e.what() << std::endl;
    }
    
    return sequence;
}

// This method has been removed since we're now using AudioProcessorPlayer

// These methods have been removed since we're now using AudioProcessorPlayer

} // namespace covertracks
