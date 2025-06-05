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

#pragma once

#include <JuceHeader.h>
#include "../Plugins/InternalPlugins.h"
#include "../Player/UI/PlayerLibraryComponent.h"
#include "../Player/UI/PlayerModeComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Musician/UI/Song/SongManagerComponent.h"
#include "../Musician/UI/Performance/PerformanceContainerComponent.h"
#include "../Musician/UI/PatternEditorComponent.h"
#include "../LLM/Core/LLMManager.h"
#include "../Musician/UI/EnsembleMainPanel.h"
#include "../Musician/UI/Song/SongManagerComponent.h"
#include "TestsComponent.h"

namespace covertracks {
//==============================================================================
namespace CommandIDs
{
   #if ! (JUCE_IOS || JUCE_ANDROID)
    static const int open                   = 0x30000;
    static const int save                   = 0x30001;
    static const int saveAs                 = 0x30002;
    static const int newFile                = 0x30003;
   #endif
    static const int showPluginListEditor   = 0x30100;
    static const int showAudioSettings      = 0x30200;
    static const int aboutBox               = 0x30300;
    static const int allWindowsForward      = 0x30400;
    static const int toggleDoublePrecision  = 0x30500;
    static const int autoScalePluginWindows = 0x30600;
    static const int showSongTab            = 0x30700;
}

//==============================================================================
ApplicationCommandManager& getCommandManager();
ApplicationProperties& getAppProperties();
bool isOnTouchDevice();

//==============================================================================
enum class AutoScale
{
    scaled,
    unscaled,
    useDefault
};

constexpr bool autoScaleOptionAvailable =
    #if JUCE_WINDOWS && JUCE_WIN_PER_MONITOR_DPI_AWARE
     true;
    #else
     false;
    #endif

AutoScale getAutoScaleValueForPlugin (const String&);
void setAutoScaleValueForPlugin (const String&, AutoScale);
bool shouldAutoScalePlugin (const PluginDescription&);
void addPluginAutoScaleOptionsSubMenu (AudioPluginInstance*, PopupMenu&);

constexpr const char* processUID = "juceaudiopluginhost";

class PlayerLibraryComponent; // Forward declaration

//==============================================================================
class MainHostWindow final : public DocumentWindow,
                             public MenuBarModel,
                             public ApplicationCommandTarget,
                             public ChangeListener,
                             public FileDragAndDropTarget,
                             public SongManagerComponent::Listener
{
public:
    //==============================================================================
    MainHostWindow();
    ~MainHostWindow() override;

    //==============================================================================
    void closeButtonPressed() override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    
    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const StringArray& files) override;
    void fileDragEnter (const StringArray& files, int x, int y) override;
    void fileDragMove (const StringArray& files, int x, int y) override;
    void fileDragExit (const StringArray& files) override;
    void filesDropped (const StringArray& files, int x, int y) override;
    
    // SongManagerComponent::Listener
    void songSelected(SongManagerComponent* source, std::shared_ptr<Song> song) override;
    
    void menuBarActivated (bool isActive) override;

    StringArray getMenuBarNames() override;
    PopupMenu getMenuForIndex (int topLevelMenuIndex, const String& menuName) override;
    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override;
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>&) override;
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo&) override;

    void tryToQuitApplication();

    // Plugin-related functionality is now handled at the player level
    void addPluginsToMenu (PopupMenu&);
    std::optional<PluginDescriptionAndPreference> getChosenType (int menuID) const;
    
    // Accessor methods for shared resources - these now access through PlayerManager
    AudioPluginFormatManager& getFormatManager() const;
    AudioDeviceManager& getDeviceManager();
    KnownPluginList& getPluginList() const;

    AudioEngine* getAudioEngine() { return playerManager ? playerManager->getAudioEngine() : nullptr; }

    // PlayerManager owns the AudioEngine and all audio resources
    std::shared_ptr<PlayerManager> playerManager; 
    
    // Main components
    std::unique_ptr<PlayerLibraryComponent> playerLibraryWindow;
    std::unique_ptr<PlayerModeComponent> playerModeWindow;
    std::unique_ptr<PerformanceContainerComponent> performanceContainerWindow;
    std::unique_ptr<SongManagerComponent> songManagerWindow;
    std::unique_ptr<PatternEditorComponent> patternEditorWindow;
    std::unique_ptr<EnsembleMainPanel> ensembleMainWindow;
    std::unique_ptr<TestsComponent> testsWindow;
    std::shared_ptr<Musician> musician; // Needed for LLM and Pattern components
    
    // LLM client
    LLMClient* llmClient; // Raw pointer for compatibility with older code
    std::shared_ptr<LLMClient> llmClientShared; // Shared pointer for LLMConversationComponent
    
    // Ensemble manager for LLM conversation
    std::unique_ptr<EnsembleManager> llmConversationEnsembleManager;
    


private:
    //==============================================================================
    static bool isDoublePrecisionProcessingEnabled();
    static bool isAutoScalePluginWindowsEnabled();

    static void updatePrecisionMenuItem (ApplicationCommandInfo& info);
    static void updateAutoScaleMenuItem (ApplicationCommandInfo& info);

    void showAudioSettings();

    //==============================================================================
    KnownPluginList::SortMethod pluginSortMethod;
    Array<PluginDescriptionAndPreference> pluginDescriptionsAndPreference;
    
    class PluginListWindow;
    std::unique_ptr<PluginListWindow> pluginListWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainHostWindow)
};
}
