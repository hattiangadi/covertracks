/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../UI/MainHostWindow.h"
#include "PluginGraph.h"
#include "InternalPlugins.h"
#include "../UI/GraphEditorPanel.h"

namespace covertracks {
static std::unique_ptr<ScopedDPIAwarenessDisabler> makeDPIAwarenessDisablerForPlugin (const PluginDescription& desc)
{
    return shouldAutoScalePlugin (desc) ? std::make_unique<ScopedDPIAwarenessDisabler>()
                                        : nullptr;
}

//==============================================================================
PluginGraph::PluginGraph (AudioPluginFormatManager& fm, KnownPluginList& kpl)
    : FileBasedDocument (getFilenameSuffix(),
                         getFilenameWildcard(),
                         "Load a graph",
                         "Save a graph"),
      formatManager (fm),
      knownPlugins (kpl)
{
    // Don't automatically create a new document with default plugins
    // This will be handled by the GraphDocumentComponent based on loadDefaultGraph parameter
    clear();
    setFile({});
    graph.addListener (this);
}

PluginGraph::~PluginGraph()
{
    graph.removeListener (this);
    graph.removeChangeListener (this);
    graph.clear();
}

PluginGraph::NodeID PluginGraph::getNextUID() noexcept
{
    return PluginGraph::NodeID (++(lastUID.uid));
}

//==============================================================================
void PluginGraph::changeListenerCallback (ChangeBroadcaster*)
{
    changed();

    for (int i = activePluginWindows.size(); --i >= 0;)
        if (! graph.getNodes().contains (activePluginWindows.getUnchecked (i)->node))
            activePluginWindows.remove (i);
}

AudioProcessorGraph::Node::Ptr PluginGraph::getNodeForName (const String& name) const
{
    for (auto* node : graph.getNodes())
        if (auto p = node->getProcessor())
            if (p->getName().equalsIgnoreCase (name))
                return node;

    return nullptr;
}

void PluginGraph::addPlugin (const PluginDescriptionAndPreference& desc, Point<double> pos)
{
    std::shared_ptr<ScopedDPIAwarenessDisabler> dpiDisabler = makeDPIAwarenessDisablerForPlugin (desc.pluginDescription);
    
    // Create a copy of the plugin description to ensure we have the correct format name
    PluginDescription correctedDesc = desc.pluginDescription;
    
    // Find the correct format for this plugin
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        
        // Check if this format can handle the plugin
        if (format->fileMightContainThisPluginType(correctedDesc.fileOrIdentifier))
        {
            // If the format name doesn't match, update it
            if (format->getName() != correctedDesc.pluginFormatName)
            {
                DBG("Correcting plugin format name from '" + correctedDesc.pluginFormatName + "' to '" + format->getName() + "'");
                correctedDesc.pluginFormatName = format->getName();
            }
            break;
        }
    }
    
    // For internal plugins, ensure the format name is correct
    if (correctedDesc.fileOrIdentifier.isEmpty() && correctedDesc.name.isNotEmpty())
    {
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* format = formatManager.getFormat(i);
            if (format->getName() == "Internal")
            {
                if (correctedDesc.pluginFormatName != format->getName())
                {
                    DBG("Correcting internal plugin format name from '" + correctedDesc.pluginFormatName + "' to 'Internal'");
                    correctedDesc.pluginFormatName = format->getName();
                }
                break;
            }
        }
    }
    
    // Create the plugin with the corrected description
    PluginDescriptionAndPreference correctedDescAndPref = desc;
    correctedDescAndPref.pluginDescription = correctedDesc;
    
    formatManager.createPluginInstanceAsync (correctedDesc,
                                             graph.getSampleRate(),
                                             graph.getBlockSize(),
                                             [this, pos, dpiDisabler, useARA = desc.useARA] (std::unique_ptr<AudioPluginInstance> instance, const String& error)
                                             {
                                                 addPluginCallback (std::move (instance), error, pos, useARA);
                                             });
}

void PluginGraph::addPluginCallback (std::unique_ptr<AudioPluginInstance> instance,
                                     const String& error,
                                     Point<double> pos,
                                     PluginDescriptionAndPreference::UseARA useARA)
{
    if (instance == nullptr)
    {
        // Log detailed error information
        DBG("Plugin creation failed with error: " + error);
        
        // Show more detailed error message to the user
        auto options = MessageBoxOptions::makeOptionsOk (MessageBoxIconType::WarningIcon,
                                                         TRANS ("Couldn't create plugin"),
                                                         "Error details: " + error);
        messageBox = AlertWindow::showScopedAsync (options, nullptr);
    }
    else
    {
       #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
        if (useARA == PluginDescriptionAndPreference::UseARA::yes
            && instance->getPluginDescription().hasARAExtension)
        {
            instance = std::make_unique<ARAPluginInstanceWrapper> (std::move (instance));
        }
       #endif

        instance->enableAllBuses();

        if (auto node = graph.addNode (std::move (instance)))
        {
            node->properties.set ("x", pos.x);
            node->properties.set ("y", pos.y);
            node->properties.set ("useARA", useARA == PluginDescriptionAndPreference::UseARA::yes);
            changed();
        }
    }
}

void PluginGraph::setNodePosition (NodeID nodeID, Point<double> pos)
{
    if (auto* n = graph.getNodeForId (nodeID))
    {
        n->properties.set ("x", jlimit (0.0, 1.0, pos.x));
        n->properties.set ("y", jlimit (0.0, 1.0, pos.y));
    }
}

Point<double> PluginGraph::getNodePosition (NodeID nodeID) const
{
    if (auto* n = graph.getNodeForId (nodeID))
        return { static_cast<double> (n->properties ["x"]),
                 static_cast<double> (n->properties ["y"]) };

    return {};
}

//==============================================================================
void PluginGraph::clear()
{
    closeAnyOpenPluginWindows();
    graph.clear();
    changed();
}

PluginWindow* PluginGraph::getOrCreateWindowFor (AudioProcessorGraph::Node* node, PluginWindow::Type type)
{
    jassert (node != nullptr);

   #if JUCE_IOS || JUCE_ANDROID
    closeAnyOpenPluginWindows();
   #else
    for (auto* w : activePluginWindows)
        if (w->node.get() == node && w->type == type)
            return w;
   #endif

    if (auto* processor = node->getProcessor())
    {
        if (auto* plugin = dynamic_cast<AudioPluginInstance*> (processor))
        {
            auto description = plugin->getPluginDescription();

            if (! plugin->hasEditor() && description.pluginFormatName == "Internal")
            {
                getCommandManager().invokeDirectly (CommandIDs::showAudioSettings, false);
                return nullptr;
            }

            auto localDpiDisabler = makeDPIAwarenessDisablerForPlugin (description);
            return activePluginWindows.add (new PluginWindow (node, type, activePluginWindows));
        }
    }

    return nullptr;
}

bool PluginGraph::closeAnyOpenPluginWindows()
{
    bool wasEmpty = activePluginWindows.isEmpty();
    activePluginWindows.clear();
    return ! wasEmpty;
}

//==============================================================================
String PluginGraph::getDocumentTitle()
{
    if (! getFile().exists())
        return "Unnamed";

    return getFile().getFileNameWithoutExtension();
}

void PluginGraph::newDocument(bool createDefaultPlugins)
{
    clear();
    setFile ({});

    graph.removeChangeListener (this);

    if (createDefaultPlugins)
    {
        InternalPluginFormat internalFormat;
        
        jassert (internalFormat.getAllTypes().size() > 3);
        
        addPlugin (PluginDescriptionAndPreference { internalFormat.getAllTypes()[0] }, { 0.5,  0.1 });
        addPlugin (PluginDescriptionAndPreference { internalFormat.getAllTypes()[1] }, { 0.25, 0.1 });
        addPlugin (PluginDescriptionAndPreference { internalFormat.getAllTypes()[2] }, { 0.5,  0.9 });
        addPlugin (PluginDescriptionAndPreference { internalFormat.getAllTypes()[3] }, { 0.25, 0.9 });
    }

    MessageManager::callAsync ([this]
    {
        setChangedFlag (false);
        graph.addChangeListener (this);
    });
}

Result PluginGraph::loadDocument (const File& file)
{
    if (auto xml = parseXMLIfTagMatches (file, "FILTERGRAPH"))
    {
        graph.removeChangeListener (this);
        restoreFromXml (*xml);

        MessageManager::callAsync ([this]
        {
            setChangedFlag (false);
            graph.addChangeListener (this);
        });

        return Result::ok();
    }

    return Result::fail ("Not a valid graph file");
}

Result PluginGraph::saveDocument (const File& file)
{
    auto xml = createXml();

    if (! xml->writeTo (file, {}))
        return Result::fail ("Couldn't write to the file");

    return Result::ok();
}

File PluginGraph::getLastDocumentOpened()
{
    RecentlyOpenedFilesList recentFiles;
    recentFiles.restoreFromString (getAppProperties().getUserSettings()
                                        ->getValue ("recentFilterGraphFiles"));

    return recentFiles.getFile (0);
}

void PluginGraph::setLastDocumentOpened (const File& file)
{
    RecentlyOpenedFilesList recentFiles;
    recentFiles.restoreFromString (getAppProperties().getUserSettings()
                                        ->getValue ("recentFilterGraphFiles"));

    recentFiles.addFile (file);

    getAppProperties().getUserSettings()
        ->setValue ("recentFilterGraphFiles", recentFiles.toString());
}

//==============================================================================
static void readBusLayoutFromXml (AudioProcessor::BusesLayout& busesLayout, AudioProcessor& plugin,
                                  const XmlElement& xml, bool isInput)
{
    auto& targetBuses = (isInput ? busesLayout.inputBuses
                                 : busesLayout.outputBuses);
    int maxNumBuses = 0;

    if (auto* buses = xml.getChildByName (isInput ? "INPUTS" : "OUTPUTS"))
    {
        for (auto* e : buses->getChildWithTagNameIterator ("BUS"))
        {
            const int busIdx = e->getIntAttribute ("index");
            maxNumBuses = jmax (maxNumBuses, busIdx + 1);

            // the number of buses on busesLayout may not be in sync with the plugin after adding buses
            // because adding an input bus could also add an output bus
            for (int actualIdx = plugin.getBusCount (isInput) - 1; actualIdx < busIdx; ++actualIdx)
                if (! plugin.addBus (isInput))
                    return;

            for (int actualIdx = targetBuses.size() - 1; actualIdx < busIdx; ++actualIdx)
                targetBuses.add (plugin.getChannelLayoutOfBus (isInput, busIdx));

            auto layout = e->getStringAttribute ("layout");

            if (layout.isNotEmpty())
                targetBuses.getReference (busIdx) = AudioChannelSet::fromAbbreviatedString (layout);
        }
    }

    // if the plugin has more buses than specified in the xml, then try to remove them!
    while (maxNumBuses < targetBuses.size())
    {
        if (! plugin.removeBus (isInput))
            return;

        targetBuses.removeLast();
    }
}

//==============================================================================
static XmlElement* createBusLayoutXml (const AudioProcessor::BusesLayout& layout, const bool isInput)
{
    auto& buses = isInput ? layout.inputBuses
                          : layout.outputBuses;

    auto* xml = new XmlElement (isInput ? "INPUTS" : "OUTPUTS");

    for (int busIdx = 0; busIdx < buses.size(); ++busIdx)
    {
        auto& set = buses.getReference (busIdx);

        auto* bus = xml->createNewChildElement ("BUS");
        bus->setAttribute ("index", busIdx);
        bus->setAttribute ("layout", set.isDisabled() ? "disabled" : set.getSpeakerArrangementAsString());
    }

    return xml;
}

static XmlElement* createNodeXml (AudioProcessorGraph::Node* const node) noexcept
{
    if (auto* plugin = dynamic_cast<AudioPluginInstance*> (node->getProcessor()))
    {
        auto e = new XmlElement ("FILTER");

        e->setAttribute ("uid",      (int) node->nodeID.uid);
        e->setAttribute ("x",        node->properties ["x"].toString());
        e->setAttribute ("y",        node->properties ["y"].toString());
        e->setAttribute ("useARA",   node->properties ["useARA"].toString());

        for (int i = 0; i < (int) PluginWindow::Type::numTypes; ++i)
        {
            auto type = (PluginWindow::Type) i;

            if (node->properties.contains (PluginWindow::getOpenProp (type)))
            {
                e->setAttribute (PluginWindow::getLastXProp (type), node->properties[PluginWindow::getLastXProp (type)].toString());
                e->setAttribute (PluginWindow::getLastYProp (type), node->properties[PluginWindow::getLastYProp (type)].toString());
                e->setAttribute (PluginWindow::getOpenProp (type),  node->properties[PluginWindow::getOpenProp (type)].toString());
            }
        }

        {
            PluginDescription pd;
            plugin->fillInPluginDescription (pd);
            e->addChildElement (pd.createXml().release());
        }

        {
            MemoryBlock m;
            node->getProcessor()->getStateInformation (m);
            e->createNewChildElement ("STATE")->addTextElement (m.toBase64Encoding());
        }

        auto layout = plugin->getBusesLayout();

        auto layouts = e->createNewChildElement ("LAYOUT");
        layouts->addChildElement (createBusLayoutXml (layout, true));
        layouts->addChildElement (createBusLayoutXml (layout, false));

        return e;
    }

    jassertfalse;
    return nullptr;
}

void PluginGraph::createNodeFromXml (const XmlElement& xml)
{
    PluginDescriptionAndPreference pd;
    const auto nodeUsesARA = xml.getBoolAttribute ("useARA");

    for (auto* e : xml.getChildIterator())
    {
        if (pd.pluginDescription.loadFromXml (*e))
        {
            pd.useARA = nodeUsesARA ? PluginDescriptionAndPreference::UseARA::yes
                                    : PluginDescriptionAndPreference::UseARA::no;
            break;
        }
    }

    auto createInstanceWithFallback = [&]() -> std::unique_ptr<AudioPluginInstance>
    {
        auto createInstance = [this] (const PluginDescriptionAndPreference& description) -> std::unique_ptr<AudioPluginInstance>
        {
            String errorMessage;

            auto localDpiDisabler = makeDPIAwarenessDisablerForPlugin (description.pluginDescription);

            auto instance = formatManager.createPluginInstance (description.pluginDescription,
                                                                graph.getSampleRate(),
                                                                graph.getBlockSize(),
                                                                errorMessage);

           #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
            if (instance
                && description.useARA == PluginDescriptionAndPreference::UseARA::yes
                && description.pluginDescription.hasARAExtension)
            {
                return std::make_unique<ARAPluginInstanceWrapper> (std::move (instance));
            }
           #endif

            return instance;
        };

        if (auto instance = createInstance (pd))
            return instance;

        const auto allFormats = formatManager.getFormats();
        const auto matchingFormat = std::find_if (allFormats.begin(), allFormats.end(),
                                                  [&] (const AudioPluginFormat* f) { return f->getName() == pd.pluginDescription.pluginFormatName; });

        if (matchingFormat == allFormats.end())
            return nullptr;

        const auto plugins = knownPlugins.getTypesForFormat (**matchingFormat);
        const auto matchingPlugin = std::find_if (plugins.begin(), plugins.end(),
                                                  [&] (const PluginDescription& desc) { return pd.pluginDescription.uniqueId == desc.uniqueId; });

        if (matchingPlugin == plugins.end())
            return nullptr;

        return createInstance (PluginDescriptionAndPreference { *matchingPlugin });
    };

    if (auto instance = createInstanceWithFallback())
    {
        if (auto* layoutEntity = xml.getChildByName ("LAYOUT"))
        {
            auto layout = instance->getBusesLayout();

            readBusLayoutFromXml (layout, *instance, *layoutEntity, true);
            readBusLayoutFromXml (layout, *instance, *layoutEntity, false);

            instance->setBusesLayout (layout);
        }

        if (auto node = graph.addNode (std::move (instance), NodeID ((uint32) xml.getIntAttribute ("uid"))))
        {
            if (auto* state = xml.getChildByName ("STATE"))
            {
                MemoryBlock m;
                m.fromBase64Encoding (state->getAllSubText());

                node->getProcessor()->setStateInformation (m.getData(), (int) m.getSize());
            }

            node->properties.set ("x", xml.getDoubleAttribute ("x"));
            node->properties.set ("y", xml.getDoubleAttribute ("y"));
            node->properties.set ("useARA", xml.getBoolAttribute ("useARA"));

            for (int i = 0; i < (int) PluginWindow::Type::numTypes; ++i)
            {
                auto type = (PluginWindow::Type) i;

                if (xml.hasAttribute (PluginWindow::getOpenProp (type)))
                {
                    node->properties.set (PluginWindow::getLastXProp (type), xml.getIntAttribute (PluginWindow::getLastXProp (type)));
                    node->properties.set (PluginWindow::getLastYProp (type), xml.getIntAttribute (PluginWindow::getLastYProp (type)));
                    node->properties.set (PluginWindow::getOpenProp  (type), xml.getIntAttribute (PluginWindow::getOpenProp (type)));

                    if (node->properties[PluginWindow::getOpenProp (type)])
                    {
                        jassert (node->getProcessor() != nullptr);

                        if (auto w = getOrCreateWindowFor (node, type))
                            w->toFront (true);
                    }
                }
            }
        }
    }
}

std::unique_ptr<XmlElement> PluginGraph::createXml() const
{
    auto xml = std::make_unique<XmlElement> ("FILTERGRAPH");

    // Save all nodes
    int nodeCount = 0;
    for (auto* node : graph.getNodes())
    {
        xml->addChildElement (createNodeXml (node));
        nodeCount++;
    }
    std::cout << "Saving " << nodeCount << " nodes to XML" << std::endl;

    // Then save all connections
    int connectionCount = 0;
    int midiConnectionCount = 0;
    int audioConnectionCount = 0;
    
    for (auto& connection : graph.getConnections())
    {
        auto* e = xml->createNewChildElement ("CONNECTION");

        e->setAttribute ("srcFilter", (int) connection.source.nodeID.uid);
        e->setAttribute ("srcChannel", connection.source.channelIndex);
        e->setAttribute ("dstFilter", (int) connection.destination.nodeID.uid);
        e->setAttribute ("dstChannel", connection.destination.channelIndex);
        
        // Determine if this is a MIDI or audio connection
        bool isMidi = connection.source.isMIDI() || connection.destination.isMIDI();
        if (isMidi)
        {
            e->setAttribute ("type", "midi");
            midiConnectionCount++;
        }
        else
        {
            e->setAttribute ("type", "audio");
            audioConnectionCount++;
        }
        
        connectionCount++;
        std::cout << "Saving connection " << connectionCount << ": " 
                  << connection.source.nodeID.uid << "/" << connection.source.channelIndex
                  << " -> " 
                  << connection.destination.nodeID.uid << "/" << connection.destination.channelIndex
                  << " (" << (isMidi ? "MIDI" : "Audio") << ")"
                  << std::endl;
    }
    
    std::cout << "Total connections saved: " << connectionCount 
              << " (MIDI: " << midiConnectionCount 
              << ", Audio: " << audioConnectionCount << ")" << std::endl;

    return xml;
}

void PluginGraph::restoreFromXml (const XmlElement& xml)
{
    clear();

    // First restore all nodes
    int nodeCount = 0;
    for (auto* e : xml.getChildWithTagNameIterator ("FILTER"))
    {
        createNodeFromXml (*e);
        changed();
        nodeCount++;
    }
    std::cout << "Restored " << nodeCount << " nodes" << std::endl;
    
    // Check for audio output nodes and log their channel configuration
    // This is just for debugging purposes now that we have a proper initialization sequence
    for (auto* node : graph.getNodes())
    {
        if (auto* processor = node->getProcessor())
        {
            if (auto* ioProcessor = dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*>(processor))
            {
                if (ioProcessor->getType() == AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode)
                {
                    std::cout << "Found audio output node (ID: " << node->nodeID.uid 
                              << "), current inputs: " << ioProcessor->getTotalNumInputChannels() << std::endl;
                }
            }
        }
    }

    // Then restore all connections
    int connectionCount = 0;
    int successCount = 0;
    int midiConnectionCount = 0;
    int audioConnectionCount = 0;
    
    for (auto* e : xml.getChildWithTagNameIterator ("CONNECTION"))
    {
        auto srcFilter = (uint32) e->getIntAttribute ("srcFilter");
        auto srcChannel = e->getIntAttribute ("srcChannel");
        auto dstFilter = (uint32) e->getIntAttribute ("dstFilter");
        auto dstChannel = e->getIntAttribute ("dstChannel");
        
        // Get connection type if available
        String type = e->hasAttribute("type") ? e->getStringAttribute("type") : "unknown";
        bool isMidi = (type == "midi") || srcChannel == 4096 || dstChannel == 4096;
        
        if (isMidi)
            midiConnectionCount++;
        else
            audioConnectionCount++;
        
        connectionCount++;
        std::cout << "Restoring connection " << connectionCount << ": " 
                  << srcFilter << "/" << srcChannel
                  << " -> " 
                  << dstFilter << "/" << dstChannel
                  << " (" << (isMidi ? "MIDI" : "Audio") << ")"
                  << std::endl;
        
        // Create the connection
        AudioProcessorGraph::Connection connection = { 
            { NodeID(srcFilter), srcChannel },
            { NodeID(dstFilter), dstChannel }
        };
        
        // Check if the connection can be made
        if (graph.canConnect(connection))
        {
            bool result = graph.addConnection(connection);
            if (result)
            {
                successCount++;
                std::cout << "  Connection added successfully" << std::endl;
            }
            else
            {
                std::cout << "  Failed to add connection" << std::endl;
                
                // Debug info about the nodes
                auto sourceNode = graph.getNodeForId(NodeID(srcFilter));
                auto destNode = graph.getNodeForId(NodeID(dstFilter));
                
                if (sourceNode != nullptr && destNode != nullptr)
                {
                    std::cout << "  Source node: " << sourceNode->getProcessor()->getName() 
                              << " (outputs: " << sourceNode->getProcessor()->getTotalNumOutputChannels() << ")" << std::endl;
                    std::cout << "  Dest node: " << destNode->getProcessor()->getName() 
                              << " (inputs: " << destNode->getProcessor()->getTotalNumInputChannels() << ")" << std::endl;
                }
                else
                {
                    std::cout << "  Source or destination node not found" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "  Cannot make connection (incompatible types or missing nodes)" << std::endl;
            
            // Debug info about the nodes
            auto sourceNode = graph.getNodeForId(NodeID(srcFilter));
            auto destNode = graph.getNodeForId(NodeID(dstFilter));
            
            if (sourceNode != nullptr && destNode != nullptr)
            {
                std::cout << "  Source node: " << sourceNode->getProcessor()->getName() 
                          << " (outputs: " << sourceNode->getProcessor()->getTotalNumOutputChannels() << ")" << std::endl;
                std::cout << "  Dest node: " << destNode->getProcessor()->getName() 
                          << " (inputs: " << destNode->getProcessor()->getTotalNumInputChannels() << ")" << std::endl;
                
                // Check if the channel indices are valid
                bool validSrcChannel = srcChannel < sourceNode->getProcessor()->getTotalNumOutputChannels();
                bool validDstChannel = dstChannel < destNode->getProcessor()->getTotalNumInputChannels();
                
                if (!validSrcChannel || !validDstChannel)
                {
                    std::cout << "  Invalid channel indices: src channel " << srcChannel 
                              << (validSrcChannel ? " is valid" : " is invalid")
                              << ", dst channel " << dstChannel 
                              << (validDstChannel ? " is valid" : " is invalid") << std::endl;
                }
            }
            else
            {
                std::cout << "  Source or destination node not found" << std::endl;
            }
        }
    }
    
    std::cout << "Successfully restored " << successCount << " out of " << connectionCount 
              << " connections (MIDI: " << midiConnectionCount 
              << ", Audio: " << audioConnectionCount << ")" << std::endl;

    // Clean up any illegal connections
    graph.removeIllegalConnections();
    
    // Debug: Print all connections after restoration
    auto connections = graph.getConnections();
    std::cout << "After restoration, graph has " << connections.size() << " connections:" << std::endl;
    int index = 0;
    for (auto& connection : connections)
    {
        bool isMidi = connection.source.isMIDI() || connection.destination.isMIDI();
        std::cout << "  Connection " << ++index << ": " 
                  << connection.source.nodeID.uid << "/" << connection.source.channelIndex
                  << " -> " 
                  << connection.destination.nodeID.uid << "/" << connection.destination.channelIndex
                  << " (" << (isMidi ? "MIDI" : "Audio") << ")" << std::endl;
    }
}

File PluginGraph::getDefaultGraphDocumentOnMobile()
{
    auto persistantStorageLocation = File::getSpecialLocation (File::userApplicationDataDirectory);
    return persistantStorageLocation.getChildFile ("state.filtergraph");
}

}   // namespace covertracks
