#include <iostream>
#include "Source/Player/UI/PluginChainEditor.h"
#include "Source/Player/Core/TracktionPlayer.h"
#include "Source/Player/Core/TracktionInstrument.h"

using namespace covertracks;

int main()
{
    std::cout << "=== Testing Plugin Fixes ===" << std::endl;
    
    // Test 1: Plugin scanning
    std::cout << "\n1. Testing plugin scanning..." << std::endl;
    
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    formatManager.addDefaultFormats();
    std::cout << "   Format manager has " << formatManager.getNumFormats() << " formats:" << std::endl;
    for (auto* format : formatManager.getFormats())
    {
        std::cout << "     - " << format->getName() << std::endl;
    }
    
    // Test 2: PluginChainEditor creation
    std::cout << "\n2. Testing PluginChainEditor creation..." << std::endl;
    
    try {
        PluginChainEditor editor;
        std::cout << "   ✓ PluginChainEditor created successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✗ PluginChainEditor creation failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 3: TracktionPlayer creation
    std::cout << "\n3. Testing TracktionPlayer creation..." << std::endl;
    
    try {
        auto player = std::make_unique<TracktionPlayer>("test", "Test Player", formatManager, knownPluginList);
        std::cout << "   ✓ TracktionPlayer created successfully" << std::endl;
        
        // Test 4: Plugin creation (built-in)
        std::cout << "\n4. Testing built-in plugin creation..." << std::endl;
        
        if (player->initialize()) {
            std::cout << "   ✓ TracktionPlayer initialized" << std::endl;
            
            // Create a built-in plugin description
            juce::PluginDescription desc;
            desc.pluginFormatName = "TracktionBuiltIn";  // Use proper built-in format name
            desc.name = "4-Oscillator Synth";
            desc.category = "FourOscPlugin";  // Use proper category
            desc.isInstrument = true;
            
            if (player->addPlugin(desc)) {
                std::cout << "   ✓ Built-in plugin added successfully" << std::endl;
            } else {
                std::cout << "   ✗ Failed to add built-in plugin" << std::endl;
            }
        } else {
            std::cout << "   ✗ TracktionPlayer initialization failed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ TracktionPlayer test failed: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Plugin Tests Complete ===" << std::endl;
    return 0;
}