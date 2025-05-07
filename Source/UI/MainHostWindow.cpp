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
#include "MainHostWindow.h"
#include "../Plugins/InternalPlugins.h"
#include "../Player/UI/PlayerLibraryComponent.h"

constexpr const char* scanModeKey = "pluginScanMode";

//==============================================================================
class Superprocess final : private ChildProcessCoordinator
{
public:
    Superprocess()
    {
        launchWorkerProcess (File::getSpecialLocation (File::currentExecutableFile), processUID, 0, 0);
    }

    enum class State
    {
        timeout,
        gotResult,
        connectionLost,
    };

    struct Response
    {
        State state;
        std::unique_ptr<XmlElement> xml;
    };

    Response getResponse()
    {
        std::unique_lock<std::mutex> lock { mutex };

        if (! condvar.wait_for (lock, std::chrono::milliseconds { 50 }, [&] { return gotResult || connectionLost; }))
            return { State::timeout, nullptr };

        const auto state = connectionLost ? State::connectionLost : State::gotResult;
        connectionLost = false;
        gotResult = false;

        return { state, std::move (pluginDescription) };
    }

    using ChildProcessCoordinator::sendMessageToWorker;

private:
    void handleMessageFromWorker (const MemoryBlock& mb) override
    {
        const std::lock_guard<std::mutex> lock { mutex };
        pluginDescription = parseXML (mb.toString());
        gotResult = true;
        condvar.notify_one();
    }

    void handleConnectionLost() override
    {
        const std::lock_guard<std::mutex> lock { mutex };
        connectionLost = true;
        condvar.notify_one();
    }

    std::mutex mutex;
    std::condition_variable condvar;

    std::unique_ptr<XmlElement> pluginDescription;
    bool connectionLost = false;
    bool gotResult = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Superprocess)
};

//==============================================================================
class CustomPluginScanner final : public KnownPluginList::CustomScanner,
                                  private ChangeListener
{
public:
    CustomPluginScanner()
    {
        if (auto* file = getAppProperties().getUserSettings())
            file->addChangeListener (this);

        handleChange();
    }

    ~CustomPluginScanner() override
    {
        if (auto* file = getAppProperties().getUserSettings())
            file->removeChangeListener (this);
    }

    bool findPluginTypesFor (AudioPluginFormat& format,
                             OwnedArray<PluginDescription>& result,
                             const String& fileOrIdentifier) override
    {
        if (scanInProcess)
        {
            superprocess = nullptr;
            format.findAllTypesForFile (result, fileOrIdentifier);
            return true;
        }

        if (addPluginDescriptions (format.getName(), fileOrIdentifier, result))
            return true;

        superprocess = nullptr;
        return false;
    }

    void scanFinished() override
    {
        superprocess = nullptr;
    }

private:
    /*  Scans for a plugin with format 'formatName' and ID 'fileOrIdentifier' using a subprocess,
        and adds discovered plugin descriptions to 'result'.

        Returns true on success.

        Failure indicates that the subprocess is unrecoverable and should be terminated.
    */
    bool addPluginDescriptions (const String& formatName,
                                const String& fileOrIdentifier,
                                OwnedArray<PluginDescription>& result)
    {
        if (superprocess == nullptr)
            superprocess = std::make_unique<Superprocess>();

        MemoryBlock block;
        MemoryOutputStream stream { block, true };
        stream.writeString (formatName);
        stream.writeString (fileOrIdentifier);

        if (! superprocess->sendMessageToWorker (block))
            return false;

        for (;;)
        {
            if (shouldExit())
                return true;

            const auto response = superprocess->getResponse();

            if (response.state == Superprocess::State::timeout)
                continue;

            if (response.xml != nullptr)
            {
                for (const auto* item : response.xml->getChildIterator())
                {
                    auto desc = std::make_unique<PluginDescription>();

                    if (desc->loadFromXml (*item))
                        result.add (std::move (desc));
                }
            }

            return (response.state == Superprocess::State::gotResult);
        }
    }

    void handleChange()
    {
        if (auto* file = getAppProperties().getUserSettings())
            scanInProcess = (file->getIntValue (scanModeKey) == 0);
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        handleChange();
    }

    std::unique_ptr<Superprocess> superprocess;

    std::atomic<bool> scanInProcess { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomPluginScanner)
};

//==============================================================================
class CustomPluginListComponent final : public PluginListComponent
{
public:
    CustomPluginListComponent (AudioPluginFormatManager& manager,
                               KnownPluginList& listToRepresent,
                               const File& pedal,
                               PropertiesFile* props,
                               bool async)
        : PluginListComponent (manager, listToRepresent, pedal, props, async)
    {
        addAndMakeVisible (validationModeLabel);
        addAndMakeVisible (validationModeBox);

        validationModeLabel.attachToComponent (&validationModeBox, true);
        validationModeLabel.setJustificationType (Justification::right);
        validationModeLabel.setSize (100, 30);

        auto unusedId = 1;

        for (const auto mode : { "In-process", "Out-of-process" })
            validationModeBox.addItem (mode, unusedId++);

        validationModeBox.setSelectedItemIndex (getAppProperties().getUserSettings()->getIntValue (scanModeKey));

        validationModeBox.onChange = [this]
        {
            getAppProperties().getUserSettings()->setValue (scanModeKey, validationModeBox.getSelectedItemIndex());
        };

        handleResize();
    }

    void resized() override
    {
        handleResize();
    }

private:
    void handleResize()
    {
        PluginListComponent::resized();

        const auto& buttonBounds = getOptionsButton().getBounds();
        validationModeBox.setBounds (buttonBounds.withWidth (130).withRightX (getWidth() - buttonBounds.getX()));
    }


    Label validationModeLabel { {}, "Scan mode" };
    ComboBox validationModeBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomPluginListComponent)
};

//==============================================================================
class MainHostWindow::PluginListWindow final : public DocumentWindow
{
public:
    PluginListWindow (MainHostWindow& mw, AudioPluginFormatManager& pluginFormatManager)
        : DocumentWindow ("Available Plugins",
                          LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                          DocumentWindow::minimiseButton | DocumentWindow::closeButton),
          owner (mw)
    {
        auto deadMansPedalFile = getAppProperties().getUserSettings()
                                   ->getFile().getSiblingFile ("RecentlyCrashedPluginsList");

        setContentOwned (new CustomPluginListComponent (pluginFormatManager,
                                                        owner.knownPluginList,
                                                        deadMansPedalFile,
                                                        getAppProperties().getUserSettings(),
                                                        true), true);

        setResizable (true, false);
        setResizeLimits (300, 400, 800, 1500);
        setTopLeftPosition (60, 60);

        restoreWindowStateFromString (getAppProperties().getUserSettings()->getValue ("listWindowPos"));
        setVisible (true);
    }

    ~PluginListWindow() override
    {
        getAppProperties().getUserSettings()->setValue ("listWindowPos", getWindowStateAsString());
        clearContentComponent();
    }

    void closeButtonPressed() override
    {
        owner.pluginListWindow = nullptr;
    }

private:
    MainHostWindow& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListWindow)
};

//==============================================================================
MainHostWindow::MainHostWindow()
    : DocumentWindow (JUCEApplication::getInstance()->getApplicationName(),
                      LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                      DocumentWindow::allButtons)
{
    formatManager.addDefaultFormats();
    formatManager.addFormat (new InternalPluginFormat());

    auto safeThis = SafePointer<MainHostWindow> (this);
    RuntimePermissions::request (RuntimePermissions::recordAudio,
                                 [safeThis] (bool granted) mutable
                                 {
                                     auto savedState = getAppProperties().getUserSettings()->getXmlValue ("audioDeviceState");
                                     safeThis->deviceManager.initialise (granted ? 256 : 0, 256, savedState.get(), true);
                                 });

   #if JUCE_IOS || JUCE_ANDROID
    setFullScreen (true);
   #else
    setResizable (true, false);
    setResizeLimits (500, 400, 10000, 10000);
    centreWithSize (800, 600);
   #endif

    knownPluginList.setCustomScanner (std::make_unique<CustomPluginScanner>());

    playerLibraryWindow.reset (new PlayerLibraryComponent());

    setContentNonOwned (playerLibraryWindow.get(), false);

    setUsingNativeTitleBar (true);

    restoreWindowStateFromString (getAppProperties().getUserSettings()->getValue ("mainWindowPos"));

    setVisible (true);

    InternalPluginFormat internalFormat;
    internalTypes = internalFormat.getAllTypes();

    if (auto savedPluginList = getAppProperties().getUserSettings()->getXmlValue ("pluginList"))
        knownPluginList.recreateFromXml (*savedPluginList);

    for (auto& t : internalTypes)
        knownPluginList.addType (t);

    pluginSortMethod = (KnownPluginList::SortMethod) getAppProperties().getUserSettings()
                            ->getIntValue ("pluginSortMethod", KnownPluginList::sortByManufacturer);

    knownPluginList.addChangeListener (this);

    addKeyListener (getCommandManager().getKeyMappings());

    Process::setPriority (Process::HighPriority);

  #if ! (JUCE_IOS || JUCE_ANDROID)
   #if JUCE_MAC
    setMacMainMenu (this);
   #else
    setMenuBar (this);
   #endif
  #endif

    getCommandManager().setFirstCommandTarget (this);
}

MainHostWindow::~MainHostWindow()
{
    pluginListWindow = nullptr;
    knownPluginList.removeChangeListener (this);

    setLookAndFeel (nullptr);

  #if JUCE_MAC
    MenuBarModel::setMacMainMenu (nullptr);
  #else
    setMenuBar (nullptr);
  #endif

    deviceManager.closeAudioDevice();
}

// These methods have been removed as part of the graph editor removal

void MainHostWindow::closeButtonPressed()
{
    tryToQuitApplication();
}

struct AsyncQuitRetrier final : private Timer
{
    AsyncQuitRetrier()   { startTimer (500); }

    void timerCallback() override
    {
        stopTimer();
        delete this;

        if (auto app = JUCEApplicationBase::getInstance())
            app->systemRequestedQuit();
    }
};

void MainHostWindow::tryToQuitApplication()
{
    if (ModalComponentManager::getInstance()->cancelAllModalComponents())
    {
        new AsyncQuitRetrier();
        return;
    }

    // Simply quit the application
    JUCEApplication::quit();
}
void MainHostWindow::changeListenerCallback (ChangeBroadcaster* changed)
{
    if (changed == &knownPluginList)
    {
        menuItemsChanged();

        // save the plugin list every time it gets changed, so that if we're scanning
        // and it crashes, we've still saved the previous ones
        if (auto savedPluginList = std::unique_ptr<XmlElement> (knownPluginList.createXml()))
        {
            getAppProperties().getUserSettings()->setValue ("pluginList", savedPluginList.get());
            getAppProperties().getUserSettings()->saveIfNeeded();
        }
    }
}

StringArray MainHostWindow::getMenuBarNames()
{
    StringArray names;
    names.add ("File");
    names.add ("Plugins");
    names.add ("Options");
    names.add ("Windows");
    return names;
}

PopupMenu MainHostWindow::getMenuForIndex (int topLevelMenuIndex, const String& /*menuName*/)
{
    PopupMenu menu;

    if (topLevelMenuIndex == 0)
    {
        // "File" menu
       #if ! (JUCE_IOS || JUCE_ANDROID)
        menu.addCommandItem (&getCommandManager(), CommandIDs::newFile);
        menu.addCommandItem (&getCommandManager(), CommandIDs::open);
       #endif

        RecentlyOpenedFilesList recentFiles;
        recentFiles.restoreFromString (getAppProperties().getUserSettings()
                                            ->getValue ("recentFilterGraphFiles"));

        PopupMenu recentFilesMenu;
        recentFiles.createPopupMenuItems (recentFilesMenu, 100, true, true);
        menu.addSubMenu ("Open recent file", recentFilesMenu);

       #if ! (JUCE_IOS || JUCE_ANDROID)
        menu.addCommandItem (&getCommandManager(), CommandIDs::save);
        menu.addCommandItem (&getCommandManager(), CommandIDs::saveAs);
       #endif

        menu.addSeparator();
        menu.addCommandItem (&getCommandManager(), StandardApplicationCommandIDs::quit);
    }
    else if (topLevelMenuIndex == 1)
    {
        // "Plugins" menu
        menu.addCommandItem (&getCommandManager(), CommandIDs::showPluginListEditor);
        // Add a separator before plugin list
        menu.addSeparator();

        // Add "Create Plugin" submenu with all available plugins
        PopupMenu pluginsMenu;
        addPluginsToMenu(pluginsMenu);
        menu.addSubMenu("Create Plugin", pluginsMenu);

}
    else if (topLevelMenuIndex == 2)
    {
        // "Options" menu

        PopupMenu sortTypeMenu;
        sortTypeMenu.addItem (200, "List Plug-ins in Default Order",      true, pluginSortMethod == KnownPluginList::defaultOrder);
        sortTypeMenu.addItem (201, "List Plug-ins in Alphabetical Order", true, pluginSortMethod == KnownPluginList::sortAlphabetically);
        sortTypeMenu.addItem (202, "List Plug-ins by Category",           true, pluginSortMethod == KnownPluginList::sortByCategory);
        sortTypeMenu.addItem (203, "List Plug-ins by Manufacturer",       true, pluginSortMethod == KnownPluginList::sortByManufacturer);
        sortTypeMenu.addItem (204, "List Plug-ins Based on the Directory Structure", true, pluginSortMethod == KnownPluginList::sortByFileSystemLocation);
        menu.addSubMenu ("Plug-in Menu Type", sortTypeMenu);

        menu.addSeparator();
        menu.addCommandItem (&getCommandManager(), CommandIDs::showAudioSettings);
        menu.addCommandItem (&getCommandManager(), CommandIDs::toggleDoublePrecision);

        if (autoScaleOptionAvailable)
            menu.addCommandItem (&getCommandManager(), CommandIDs::autoScalePluginWindows);
        
        menu.addSeparator();
        menu.addCommandItem(&getCommandManager(), CommandIDs::showPlayerLibrary);
        
        menu.addSeparator();
        menu.addCommandItem (&getCommandManager(), CommandIDs::aboutBox);
    }
    else if (topLevelMenuIndex == 3)
    {
        menu.addCommandItem (&getCommandManager(), CommandIDs::allWindowsForward);
    }

    return menu;
}

void MainHostWindow::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    if (menuItemID == 250)
    {
        // Delete all plugins (existing code)
    }
   #if ! (JUCE_ANDROID || JUCE_IOS)
    else if (menuItemID >= 100 && menuItemID < 200)
    {
        // Recent files handling (existing code)
    }
   #endif
    else if (menuItemID >= 200 && menuItemID < 210)
    {
        // Plugin sorting methods (existing code)
    }
    else
    {
        // Check if this is a plugin selection
        auto optionalPlugin = getChosenType(menuItemID);
        if (optionalPlugin.has_value())
        {
            // Find active player editor
            PlayerEditorComponent* activeEditor = nullptr;
            
            if (playerLibraryWindow != nullptr)
            {
                if (auto* editorComponent = playerLibraryWindow->getActiveEditorComponent())
                {
                    activeEditor = editorComponent;
                }
            }
            
            if (activeEditor != nullptr)
            {
                // Add plugin to active editor
                Point<int> pos(400, 300); // Default position - center of typical window
                activeEditor->createPlugin(*optionalPlugin, pos);
            }
            else
            {
                // No active editor - show message
                AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
                                               "Add Plugin",
                                               "Please open a player editor first before adding plugins.");
            }
        }
    }
}

void MainHostWindow::menuBarActivated (bool isActivated)
{
    if (isActivated)
        Component::unfocusAllComponents();
}

// Plugin creation is now handled by PlayerEditorComponent

static bool containsDuplicateNames (const Array<PluginDescription>& plugins, const String& name)
{
    int matches = 0;

    for (auto& p : plugins)
        if (p.name == name && ++matches > 1)
            return true;

    return false;
}

static constexpr int menuIDBase = 0x324503f4;

static void addToMenu (const KnownPluginList::PluginTree& tree,
                       PopupMenu& m,
                       const Array<PluginDescription>& allPlugins,
                       Array<PluginDescriptionAndPreference>& addedPlugins)
{
    for (auto* sub : tree.subFolders)
    {
        PopupMenu subMenu;
        addToMenu (*sub, subMenu, allPlugins, addedPlugins);

        m.addSubMenu (sub->folder, subMenu, true, nullptr, false, 0);
    }

    auto addPlugin = [&] (const auto& descriptionAndPreference, const auto& pluginName)
    {
        addedPlugins.add (descriptionAndPreference);
        const auto menuID = addedPlugins.size() - 1 + menuIDBase;
        m.addItem (menuID, pluginName, true, false);
    };

    for (auto& plugin : tree.plugins)
    {
        auto name = plugin.name;

        if (containsDuplicateNames (tree.plugins, name))
            name << " (" << plugin.pluginFormatName << ')';

        addPlugin (PluginDescriptionAndPreference { plugin, PluginDescriptionAndPreference::UseARA::no }, name);

       #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
        if (plugin.hasARAExtension)
        {
            name << " (ARA)";
            addPlugin (PluginDescriptionAndPreference { plugin }, name);
        }
       #endif
    }
}

//==============================================================================
ApplicationCommandTarget* MainHostWindow::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void MainHostWindow::getAllCommands (Array<CommandID>& commands)
{
    // this returns the set of all commands that this target can perform..
    const CommandID ids[] = {
                             #if ! (JUCE_IOS || JUCE_ANDROID)
                              CommandIDs::newFile,
                              CommandIDs::open,
                              CommandIDs::save,
                              CommandIDs::saveAs,
                             #endif
                              CommandIDs::showPluginListEditor,
                              CommandIDs::showAudioSettings,
                              CommandIDs::toggleDoublePrecision,
                              CommandIDs::aboutBox,
                              CommandIDs::allWindowsForward,
                              CommandIDs::autoScalePluginWindows,
                              CommandIDs::showPlayerLibrary
                            };

    commands.addArray (ids, numElementsInArray (ids));
}

void MainHostWindow::getCommandInfo (const CommandID commandID, ApplicationCommandInfo& result)
{
    const String category ("General");

    switch (commandID)
    {
   #if ! (JUCE_IOS || JUCE_ANDROID)
    case CommandIDs::newFile:
        result.setInfo ("New", "Creates a new filter graph file", category, 0);
        result.defaultKeypresses.add (KeyPress ('n', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::open:
        result.setInfo ("Open...", "Opens a filter graph file", category, 0);
        result.defaultKeypresses.add (KeyPress ('o', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::save:
        result.setInfo ("Save", "Saves the current graph to a file", category, 0);
        result.defaultKeypresses.add (KeyPress ('s', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::saveAs:
        result.setInfo ("Save As...",
                        "Saves a copy of the current graph to a file",
                        category, 0);
        result.defaultKeypresses.add (KeyPress ('s', ModifierKeys::shiftModifier | ModifierKeys::commandModifier, 0));
        break;
   #endif

    case CommandIDs::showPluginListEditor:
        result.setInfo ("Edit the List of Available Plug-ins...", {}, category, 0);
        result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
        break;

    case CommandIDs::showAudioSettings:
        result.setInfo ("Change the Audio Device Settings", {}, category, 0);
        result.addDefaultKeypress ('a', ModifierKeys::commandModifier);
        break;

    case CommandIDs::toggleDoublePrecision:
        updatePrecisionMenuItem (result);
        break;

    case CommandIDs::aboutBox:
        result.setInfo ("About...", {}, category, 0);
        break;

    case CommandIDs::allWindowsForward:
        result.setInfo ("All Windows Forward", "Bring all plug-in windows forward", category, 0);
        result.addDefaultKeypress ('w', ModifierKeys::commandModifier);
        break;

    case CommandIDs::autoScalePluginWindows:
        updateAutoScaleMenuItem (result);
        break;

    case CommandIDs::showPlayerLibrary:
        result.setInfo("Player Library", "Manage player configurations", category, 0);
        result.addDefaultKeypress('l', ModifierKeys::commandModifier);
        break;

    default:
        break;
    }
}

bool MainHostWindow::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
    #if ! (JUCE_IOS || JUCE_ANDROID)
    case CommandIDs::newFile:
    case CommandIDs::open:
    case CommandIDs::save:
    case CommandIDs::saveAs:
        // File operations are now handled by PlayerEditorComponent
        break;
   #endif

    case CommandIDs::showPluginListEditor:
        if (pluginListWindow == nullptr)
            pluginListWindow.reset (new PluginListWindow (*this, formatManager));

        pluginListWindow->toFront (true);
        break;

    case CommandIDs::showAudioSettings:
        showAudioSettings();
        break;

    case CommandIDs::toggleDoublePrecision:
        if (auto* props = getAppProperties().getUserSettings())
        {
            auto newIsDoublePrecision = ! isDoublePrecisionProcessingEnabled();
            props->setValue ("doublePrecisionProcessing", var (newIsDoublePrecision));

            ApplicationCommandInfo cmdInfo (info.commandID);
            updatePrecisionMenuItem (cmdInfo);
            menuItemsChanged();

            // Double precision setting is now only relevant for PlayerEditorComponent
        }
        break;

    case CommandIDs::autoScalePluginWindows:
        if (auto* props = getAppProperties().getUserSettings())
        {
            auto newAutoScale = ! isAutoScalePluginWindowsEnabled();
            props->setValue ("autoScalePluginWindows", var (newAutoScale));

            ApplicationCommandInfo cmdInfo (info.commandID);
            updateAutoScaleMenuItem (cmdInfo);
            menuItemsChanged();
        }
        break;

    case CommandIDs::aboutBox:
        // TODO
        break;

    case CommandIDs::allWindowsForward:
    {
        auto& desktop = Desktop::getInstance();

        for (int i = 0; i < desktop.getNumComponents(); ++i)
            desktop.getComponent (i)->toBehind (this);

        break;
    }

    case CommandIDs::showPlayerLibrary:
    {
        // Create the component
        auto* component = new PlayerLibraryComponent();
        component->setSize(800, 600);
        
        DialogWindow::LaunchOptions o;
        o.content.setNonOwned(component);
        o.dialogTitle = "Player Library";
        o.componentToCentreAround = this;
        o.dialogBackgroundColour = getLookAndFeel().findColour(ResizableWindow::backgroundColourId);
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = false;
        o.resizable = true;
        
        // Launch the dialog and have it delete the component when closed
        DialogWindow* dialog = o.launchAsync();
        if (dialog != nullptr)
            dialog->setVisible(true);
        break;
    }

    default:
        return false;
    }

    return true;
}

void MainHostWindow::showAudioSettings()
{
    auto* audioSettingsComp = new AudioDeviceSelectorComponent (deviceManager,
                                                                0, 256,
                                                                0, 256,
                                                                true, true,
                                                                true, false);

    audioSettingsComp->setSize (500, 450);

    DialogWindow::LaunchOptions o;
    o.content.setOwned (audioSettingsComp);
    o.dialogTitle                   = "Audio Settings";
    o.componentToCentreAround       = this;
    o.dialogBackgroundColour        = getLookAndFeel().findColour (ResizableWindow::backgroundColourId);
    o.escapeKeyTriggersCloseButton  = true;
    o.useNativeTitleBar             = false;
    o.resizable                     = false;

     auto* w = o.create();
     auto safeThis = SafePointer<MainHostWindow> (this);

     w->enterModalState (true,
                         ModalCallbackFunction::create
                         ([safeThis] (int)
                         {
                             auto audioState = safeThis->deviceManager.createStateXml();

                             getAppProperties().getUserSettings()->setValue ("audioDeviceState", audioState.get());
                             getAppProperties().getUserSettings()->saveIfNeeded();

                              // Graph connections are now handled by PlayerEditorComponent
                         }), true);
}

bool MainHostWindow::isInterestedInFileDrag (const StringArray&)
{
    return true;
}

void MainHostWindow::fileDragEnter (const StringArray&, int, int)
{
}

void MainHostWindow::fileDragMove (const StringArray&, int, int)
{
}

void MainHostWindow::fileDragExit (const StringArray&)
{
}

void MainHostWindow::filesDropped (const StringArray& files, int x, int y)
{
    // File drops are now handled by PlayerEditorComponent
}

bool MainHostWindow::isDoublePrecisionProcessingEnabled()
{
    if (auto* props = getAppProperties().getUserSettings())
        return props->getBoolValue ("doublePrecisionProcessing", false);

    return false;
}

bool MainHostWindow::isAutoScalePluginWindowsEnabled()
{
    if (auto* props = getAppProperties().getUserSettings())
        return props->getBoolValue ("autoScalePluginWindows", false);

    return false;
}

void MainHostWindow::updatePrecisionMenuItem (ApplicationCommandInfo& info)
{
    info.setInfo ("Double Floating-Point Precision Rendering", {}, "General", 0);
    info.setTicked (isDoublePrecisionProcessingEnabled());
}

void MainHostWindow::updateAutoScaleMenuItem (ApplicationCommandInfo& info)
{
    info.setInfo ("Auto-Scale Plug-in Windows", {}, "General", 0);
    info.setTicked (isAutoScalePluginWindowsEnabled());
}

// In MainHostWindow.cpp, implement these methods:

void MainHostWindow::addPluginsToMenu(PopupMenu& m)
{
    // Clear any previous descriptions
    pluginDescriptionsAndPreference.clear();
    
    // Add internal plugins first
    int menuID = menuIDBase;
    
    // Create a submenu for internal plugins
    PopupMenu internalPluginsMenu;
    
    for (auto& type : internalTypes)
    {
        pluginDescriptionsAndPreference.add(PluginDescriptionAndPreference{type});
        internalPluginsMenu.addItem(menuID++, type.name);
    }
    
    m.addSubMenu("Internal Plugins", internalPluginsMenu);
    m.addSeparator();
    
    // Add external plugins from the known plugin list
    auto pluginDescriptions = knownPluginList.getTypes();
    
    // Remove internal plugins from this list to avoid duplication
    pluginDescriptions.removeIf([](PluginDescription& desc)
    {
        return desc.pluginFormatName == InternalPluginFormat::getIdentifier();
    });
    
    // Create a tree structure for the plugins using the current sort method
    auto tree = KnownPluginList::createTree(pluginDescriptions, pluginSortMethod);
    
    // Add the tree to the menu
    addPluginTreeToMenu(*tree, m, pluginDescriptions);
}

void MainHostWindow::addPluginTreeToMenu(const KnownPluginList::PluginTree& tree, 
                                        PopupMenu& m,
                                        const Array<PluginDescription>& allPlugins)
{
    for (auto* sub : tree.subFolders)
    {
        PopupMenu subMenu;
        addPluginTreeToMenu(*sub, subMenu, allPlugins);
        
        m.addSubMenu(sub->folder, subMenu, true, nullptr, false, 0);
    }
    
    auto addPlugin = [&] (const auto& descriptionAndPreference, const auto& pluginName)
    {
        pluginDescriptionsAndPreference.add(descriptionAndPreference);
        const auto menuID = pluginDescriptionsAndPreference.size() - 1 + menuIDBase;
        m.addItem(menuID, pluginName, true, false);
    };
    
    for (auto& plugin : tree.plugins)
    {
        auto name = plugin.name;
        
        // Check if there are duplicate names in this folder
        bool hasDuplicates = false;
        for (const auto& otherPlugin : tree.plugins)
        {
            if (&otherPlugin != &plugin && otherPlugin.name == plugin.name)
            {
                hasDuplicates = true;
                break;
            }
        }
        
        // If there are duplicates, add the format name to distinguish them
        if (hasDuplicates)
            name << " (" << plugin.pluginFormatName << ')';
        
        addPlugin(PluginDescriptionAndPreference{plugin, PluginDescriptionAndPreference::UseARA::no}, name);
        
       #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
        if (plugin.hasARAExtension)
        {
            name << " (ARA)";
            addPlugin(PluginDescriptionAndPreference{plugin}, name);
        }
       #endif
    }
}

std::optional<PluginDescriptionAndPreference> MainHostWindow::getChosenType(int menuID) const
{
    const int index = menuID - menuIDBase;
    
    if (isPositiveAndBelow(index, pluginDescriptionsAndPreference.size()))
        return pluginDescriptionsAndPreference[index];
    
    return {};
}