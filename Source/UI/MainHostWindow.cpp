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
#include "../Audio/AudioEngine.h"
#include "../Musician/Core/Musician.h"
#include "../LLM/Core/ClaudeClient.h"
#include "../Musician/UI/EnsembleEditPanel.h"
#include "../Musician/UI/Performance/PerformanceLibraryComponent.h"
#include "../Musician/UI/Song/SongManagerComponent.h"

namespace covertracks {
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
                                                         owner.getPluginList(),
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
    // Request audio permissions - this will be used when creating the AudioEngine in PlayerManager
    DBG("MainHostWindow:  Requesting audio permissions");
    RuntimePermissions::request (RuntimePermissions::recordAudio,
                                 [] (bool granted) mutable
                                 {
                                     DBG("MainHostWindow:  Audio permissions " << (granted ? "granted" : "denied"));
                                 });

   #if JUCE_IOS || JUCE_ANDROID
    setFullScreen (true);
   #else
    setResizable (true, false);
    setResizeLimits (500, 400, 10000, 10000);
    centreWithSize (800, 600);
   #endif

    // Create a PlayerManager instance that owns the AudioEngine
    // The PlayerManager will create its own AudioEngine, plugin formats, and plugin list
    DBG("MainHostWindow:  Creating PlayerManager");
    playerManager = std::make_shared<PlayerManager>();
    DBG("MainHostWindow:  PlayerManager created");

    
    // Create PlayerLibraryComponent and PlayerModeComponent
    DBG("MainHostWindow:  Creating PlayerLibraryComponent");
    playerLibraryWindow = std::make_unique<PlayerLibraryComponent>(playerManager);
    DBG("MainHostWindow:  PlayerLibraryComponent created");
    
    DBG("MainHostWindow:  Creating PlayerModeComponent");
    playerModeWindow = std::make_unique<PlayerModeComponent>(*playerManager);
    DBG("MainHostWindow:  PlayerModeComponent created");
    
    // Create Musician instance for LLM and Pattern components
    DBG("MainHostWindow:  Creating Musician");
    musician = std::make_shared<Musician>("main_musician", "default");
    DBG("MainHostWindow:  Musician created");
    
    // Create LLM client
    DBG("MainHostWindow:  Creating LLM client");
    llmClient = LLMManager::getInstance().getDefaultClient();
    if (llmClient)
    {
        // Create a shared_ptr that wraps the raw pointer but doesn't own it
        llmClientShared = std::shared_ptr<LLMClient>(llmClient, [](LLMClient*){});
    }
    else
    {
        DBG("MainHostWindow:  No LLM client available, creating dummy client");
        // Create a dummy client that returns error messages
        // Create a dummy LLM client for testing
        class DummyLLMClient : public LLMClient {
        public:
            void sendMessage(const String& message, ResponseCallback callback) override {
                // Use the message parameter to avoid unused parameter warning
                String response = "This is a dummy response to: " + message;
                callback(true, response);
            }
            
            void sendMessageWithHistory(const String& message, const Array<std::pair<String, String>>& history, ResponseCallback callback) override {
                // Use the parameters to avoid unused parameter warnings
                String response = "This is a dummy response to: " + message;
                response += "\nWith " + String(history.size()) + " history items.";
                callback(true, response);
            }
            
            void sendMessageWithImage(const String& message, const String& imageBase64, const String& imageType, ResponseCallback callback) override {
                // Use the parameters to avoid unused parameter warnings
                String response = "This is a dummy response to image analysis request: " + message;
                response += "\nImage type: " + imageType;
                response += "\nImage size: " + String(imageBase64.length() / 1024) + " KB";
                
                // Return a dummy JSON response that matches the expected format for song structure
                response += "\n\n{\n";
                response += "  \"title\": \"Dummy Song\",\n";
                response += "  \"composer\": \"Dummy Composer\",\n";
                response += "  \"tempo\": 120,\n";
                response += "  \"timeSignature\": \"4/4\",\n";
                response += "  \"key\": \"C\",\n";
                response += "  \"sections\": [\n";
                response += "    {\n";
                response += "      \"name\": \"Verse\",\n";
                response += "      \"chords\": \"C | Am | F | G\",\n";
                response += "      \"repeats\": 2\n";
                response += "    },\n";
                response += "    {\n";
                response += "      \"name\": \"Chorus\",\n";
                response += "      \"chords\": \"F | G | C | C\",\n";
                response += "      \"repeats\": 1\n";
                response += "    }\n";
                response += "  ],\n";
                response += "  \"form\": \"Verse Chorus Verse Chorus\",\n";
                response += "  \"style\": \"Pop\"\n";
                response += "}";
                
                callback(true, response);
            }
            
            bool isConnected() const override { return false; }
            
            String getServiceName() const override { return "Dummy LLM"; }
            
            // Implement the clone method required by the interface
            LLMClient* clone() const override {
                return new DummyLLMClient(*this);
            }
        };
        
        // Create a shared_ptr to the DummyLLMClient
        auto dummyClient = std::make_shared<DummyLLMClient>();
        llmClient = dummyClient.get();
        
        // Store the shared_ptr for use with LLMConversationComponent
        llmClientShared = dummyClient;
    }
    // Create Song Manager component
    DBG("MainHostWindow:  Creating SongManagerComponent");
    songManagerWindow = std::make_unique<SongManagerComponent>();
    // Initialize SongEditorComponent with the LLM client
    songManagerWindow->initializeWithLLMClient(llmClientShared);
    // Register this window as a listener for song selection events
    songManagerWindow->addListener(this);
    DBG("MainHostWindow:  SongManagerComponent created");
    
    // Create Performance Library component
    DBG("MainHostWindow:  Creating PerformanceLibraryComponent");
    // Create a PerformanceManager instance
    performanceManager = std::make_unique<PerformanceManager>();
    
    performanceWindow = std::make_unique<PerformanceLibraryComponent>(performanceManager.get(), llmClientShared, playerManager.get());
    
    DBG("MainHostWindow:  PerformanceLibraryComponent created");
    
    // Create Pattern Editor component
    DBG("MainHostWindow:  Creating PatternEditorComponent");
    patternEditorWindow = std::make_unique<PatternEditorComponent>(musician.get(), playerManager.get());
    DBG("MainHostWindow:  PatternEditorComponent created");
    
    // Create Ensemble Main Panel component
    DBG("MainHostWindow:  Creating EnsembleMainPanel");
    ensembleMainWindow = std::make_unique<EnsembleMainPanel>(playerManager.get());
    DBG("MainHostWindow:  EnsembleMainPanel created");
    
    // Create Tests component
    DBG("MainHostWindow:  Creating TestsComponent");
    testsWindow = std::make_unique<TestsComponent>();
    DBG("MainHostWindow:  TestsComponent created");
    
    // Create a tabbed component to hold all views
    auto* tabbedComponent = new TabbedComponent(TabbedButtonBar::TabsAtTop);
    tabbedComponent->addTab("Player Library", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), playerLibraryWindow.get(), false);
    tabbedComponent->addTab("Player Mode", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), playerModeWindow.get(), false);
    tabbedComponent->addTab("Songs", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), songManagerWindow.get(), false);
    tabbedComponent->addTab("Performances", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), performanceWindow.get(), false);
    tabbedComponent->addTab("Pattern Editor", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), patternEditorWindow.get(), false);
    tabbedComponent->addTab("Ensembles", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), ensembleMainWindow.get(), false);
    tabbedComponent->addTab("Tests", getLookAndFeel().findColour(ResizableWindow::backgroundColourId), testsWindow.get(), false);
    
    // Set it as the main content
    DBG("MainHostWindow:  Setting content");
    setContentOwned(tabbedComponent, true);

    setUsingNativeTitleBar (true);

    DBG("MainHostWindow:  Restoring window state");
    restoreWindowStateFromString (getAppProperties().getUserSettings()->getValue ("mainWindowPos"));

    setVisible (true);
    
    // Load saved plugin list into the PlayerManager's KnownPluginList
    DBG("MainHostWindow:  Loading saved plugin list");
    if (auto savedPluginList = getAppProperties().getUserSettings()->getXmlValue ("pluginList"))
    {
        DBG("MainHostWindow:  Recreating plugin list from XML");
        getPluginList().recreateFromXml (*savedPluginList);
    }
    
    // The internal plugin types are now added by the PlayerManager during initialization
    DBG("MainHostWindow:  Setting plugin sort method");
    // Set plugin sort method
    pluginSortMethod = (KnownPluginList::SortMethod) getAppProperties().getUserSettings()
                            ->getIntValue ("pluginSortMethod", KnownPluginList::sortByManufacturer);

    // Register as a change listener for the plugin list
    DBG("MainHostWindow:  Adding change listener for plugin list");
    getPluginList().addChangeListener (this);

    DBG("MainHostWindow:  Adding key listener");
    addKeyListener (getCommandManager().getKeyMappings());

    DBG("MainHostWindow:  Setting process priority");
    Process::setPriority (Process::HighPriority);

  #if JUCE_IOS || JUCE_ANDROID
    // Mobile menu setup - removed graph holder reference
  #else
   #if JUCE_MAC
    DBG("MainHostWindow:  Setting Mac main menu");
    setMacMainMenu (this);
   #else
    DBG("MainHostWindow:  Setting menu bar");
    setMenuBar (this);
   #endif
  #endif

    DBG("MainHostWindow:  Setting first command target");
    getCommandManager().setFirstCommandTarget (this);
    DBG("MainHostWindow:  Constructor complete");
}

MainHostWindow::~MainHostWindow()
{
    DBG("MainHostWindow destructor: Starting cleanup");
    
    // Save window position
    getAppProperties().getUserSettings()->setValue ("mainWindowPos", getWindowStateAsString());
    
    // First, clean up the plugin list window
    pluginListWindow = nullptr;
    
    // Remove ourselves as a change listener from the plugin list
    if (playerManager)
        getPluginList().removeChangeListener(this);
    
    // Remove ourselves as a listener from the SongManagerComponent
    if (songManagerWindow)
    {
        songManagerWindow->removeListener(this);
    }
    
    // Clear the content component
    clearContentComponent();
    
    // IMPORTANT: We've reordered the member variables in the header file
    // so that playerManager is destroyed before audioEngine
    // We'll let the natural C++ destruction order handle this
    // Just add some logging to verify the order
    
    DBG("MainHostWindow destructor: Class members will be destroyed in reverse declaration order");
    DBG("MainHostWindow destructor: PlayerManager will be destroyed first, then AudioEngine");
    
  #if ! (JUCE_ANDROID || JUCE_IOS)
   #if JUCE_MAC
    setMacMainMenu (nullptr);
   #else
    setMenuBar (nullptr);
   #endif
  #endif
  
    DBG("MainHostWindow destructor: Cleanup complete");
}

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
    // In the instrument/player approach, we don't need to handle plugin windows or graph files
    // at the MainHostWindow level, as they're now managed by the PlayerEditorComponent
    
    if (ModalComponentManager::getInstance()->cancelAllModalComponents())
    {
        // If there are modal components active, wait a moment and try again
        new AsyncQuitRetrier();
        return;
    }
    
    // Simply quit the application
    JUCEApplication::quit();
}

void MainHostWindow::changeListenerCallback (ChangeBroadcaster* changed)
{
    try
    {
        // Check if the change came from the plugin list
        if (playerManager && changed == &getPluginList())
        {
            DBG("MainHostWindow:  Change from plugin list detected");
            
            // Update menu items
            menuItemsChanged();

            // Save the plugin list every time it gets changed, so that if we're scanning
            // and it crashes, we've still saved the previous ones
            try
            {
                if (auto savedPluginList = std::unique_ptr<XmlElement>(getPluginList().createXml()))
                {
                    auto* settings = getAppProperties().getUserSettings();
                    if (settings != nullptr)
                    {
                        settings->setValue("pluginList", savedPluginList.get());
                        settings->saveIfNeeded();
                        DBG("MainHostWindow:  Plugin list saved");
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[MainHostWindow] Error saving plugin list: " << e.what();
            }
        }
        // We no longer need to handle audio device changes here as that's now fully handled by the AudioEngine
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MainHostWindow] Unhandled exception in changeListenerCallback:" << e.what();
    }
    
    // Set the window title to the application name
    if (auto* app = JUCEApplication::getInstance())
    {
        auto title = app->getApplicationName();
        setName(title);
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
        // "File" menu - graph file commands removed for instrument/player approach
        
        menu.addSeparator();
        menu.addCommandItem (&getCommandManager(), StandardApplicationCommandIDs::quit);
    }
    else if (topLevelMenuIndex == 1)
    {
        // "Plugins" menu
        PopupMenu pluginsMenu;
        addPluginsToMenu (pluginsMenu);
        menu.addSubMenu ("Create Plug-in", pluginsMenu);
        menu.addSeparator();
        menu.addItem (250, "Delete All Plug-ins");
    }
    else if (topLevelMenuIndex == 2)
    {
        // "Options" menu

        menu.addCommandItem (&getCommandManager(), CommandIDs::showPluginListEditor);

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
        menu.addCommandItem (&getCommandManager(), CommandIDs::aboutBox);
    }
    else if (topLevelMenuIndex == 3)
    {
        menu.addCommandItem (&getCommandManager(), CommandIDs::allWindowsForward);
    }

    return menu;
}

void MainHostWindow::menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/)
{
    if (menuItemID == 250)
    {
        // Clearing plugins is now handled at the player level
        AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Plugin Management", 
            "Plugin management is now handled at the player level.");
    }
    // Recent files handling removed - no longer needed with instrument/player approach
    else if (menuItemID >= 200 && menuItemID < 210)
    {
             if (menuItemID == 200)     pluginSortMethod = KnownPluginList::defaultOrder;
        else if (menuItemID == 201)     pluginSortMethod = KnownPluginList::sortAlphabetically;
        else if (menuItemID == 202)     pluginSortMethod = KnownPluginList::sortByCategory;
        else if (menuItemID == 203)     pluginSortMethod = KnownPluginList::sortByManufacturer;
        else if (menuItemID == 204)     pluginSortMethod = KnownPluginList::sortByFileSystemLocation;

        getAppProperties().getUserSettings()->setValue ("pluginSortMethod", (int) pluginSortMethod);

        menuItemsChanged();
    }
    else
    {
        // In the instrument/player approach, plugin creation is handled at the player level
        // This code is left as a placeholder for future implementation
        if (const auto chosen = getChosenType (menuItemID))
        {
            // Try to forward the plugin creation to the PlayerEditorComponent if open
            if (playerLibraryWindow != nullptr)
            {
                if (auto* playerEditor = playerLibraryWindow->getPlayerEditor())
                {
                    // Use a default position (e.g., center of editor or {100,100})
                    playerEditor->createPlugin(*chosen, {100, 100});
                    return;
                }
            }
            // If no editor is open, show the info message
            String message = "Please use the Player Editor to add plugins.\n\n"
                            "Plugin: " + chosen->pluginDescription.name + " (" + chosen->pluginDescription.pluginFormatName + ")"; 
            AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Plugin Selection", message);
        }
    }
}

void covertracks::MainHostWindow::menuBarActivated (bool isActivated)
{
    if (isActivated)
        Component::unfocusAllComponents();
}

// createPlugin method removed - plugin creation is now handled at the player level

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

void covertracks::MainHostWindow::addPluginsToMenu (PopupMenu& m)
{
    // In the instrument/player approach, we still want to show available plugins in the menu
    // even though they'll be added through the player interface
    int i = 0;
    
    // Access internal types through the PlayerManager
    if (playerManager)
    {
        const auto& internalTypes = playerManager->getInternalPluginTypes();
        for (auto& t : internalTypes)
            m.addItem (++i, t.name + " (" + t.pluginFormatName + ")");
    }
        
    m.addSeparator();

    // Make sure we have a valid playerManager before proceeding
    if (!playerManager)
    {
        std::cerr << "Warning: playerManager is null in addPluginsToMenu";
        return;
    }
    
    auto pluginDescriptions = getPluginList().getTypes();
    
    // Check if we have any plugin descriptions
    if (pluginDescriptions.isEmpty())
    {
        std::cerr << "Warning: No plugin descriptions available";
        return;
    }

    // This avoids showing the internal types again later on in the list
    pluginDescriptions.removeIf ([] (PluginDescription& desc)
    {
        return desc.pluginFormatName == InternalPluginFormat::getIdentifier();
    });

    auto tree = KnownPluginList::createTree (pluginDescriptions, pluginSortMethod);
    pluginDescriptionsAndPreference = {};
    addToMenu (*tree, m, pluginDescriptions, pluginDescriptionsAndPreference);
}

std::optional<covertracks::PluginDescriptionAndPreference> covertracks::MainHostWindow::getChosenType (const int menuID) const
{
    const auto internalIndex = menuID - 1;

    // Access internal types through the PlayerManager
    if (playerManager && isPositiveAndBelow(internalIndex, playerManager->getInternalPluginTypes().size()))
        return PluginDescriptionAndPreference { playerManager->getInternalPluginTypes()[(size_t) internalIndex] };

    const auto externalIndex = menuID - menuIDBase;

    if (isPositiveAndBelow (externalIndex, pluginDescriptionsAndPreference.size()))
        return pluginDescriptionsAndPreference[externalIndex];

    return {};
}

//==============================================================================
ApplicationCommandTarget* covertracks::MainHostWindow::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void covertracks::MainHostWindow::getAllCommands (Array<CommandID>& commands)
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
                              CommandIDs::showSongTab
                            };

    commands.addArray (ids, numElementsInArray (ids));
}

void covertracks::MainHostWindow::getCommandInfo (const CommandID commandID, ApplicationCommandInfo& result)
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
        
    case CommandIDs::showSongTab:
        result.setInfo ("Show Songs", "Shows the Songs tab", category, 0);
        break;

    default:
        break;
    }
}

bool covertracks::MainHostWindow::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
   #if ! (JUCE_IOS || JUCE_ANDROID)
    // Graph file commands removed - no longer needed with instrument/player approach
   #endif

    case CommandIDs::showPluginListEditor:
        if (pluginListWindow == nullptr)
            pluginListWindow.reset (new PluginListWindow (*this, getFormatManager()));

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
            
            // Use AudioEngine to set double precision
            if (auto* engine = getAudioEngine())
                engine->setDoublePrecisionProcessing(newIsDoublePrecision);
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
    
    case CommandIDs::showSongTab:
    {
        // Switch to the Songs tab
        if (auto* tabbedComponent = dynamic_cast<TabbedComponent*>(getContentComponent()))
        {
            // Find the Songs tab index
            for (int i = 0; i < tabbedComponent->getNumTabs(); ++i)
            {
                if (tabbedComponent->getTabNames()[i] == "Songs")
                {
                    tabbedComponent->setCurrentTabIndex(i);
                    break;
                }
            }
        }
        break;
    }

    // Player Library command removed - now integrated into main window

    default:
        return false;
    }

    return true;
}

void covertracks::MainHostWindow::showAudioSettings()
{
    // Use AudioEngine's method instead of directly implementing it here
    if (auto* engine = getAudioEngine())
        engine->showAudioSettings(this);
}

bool covertracks::MainHostWindow::isInterestedInFileDrag (const StringArray&)
{
    return true;
}

void covertracks::MainHostWindow::fileDragEnter (const StringArray&, int, int)
{
}

void covertracks::MainHostWindow::fileDragMove (const StringArray&, int, int)
{
}

void covertracks::MainHostWindow::fileDragExit (const StringArray&)
{
}

void covertracks::MainHostWindow::filesDropped (const StringArray& files, int /*x*/, int /*y*/)
{
    // In the instrument/player approach, plugin files should be handled by the PlayerEditorComponent
    // This is just a stub that adds the plugins to the known plugin list
    if (playerManager != nullptr)
    {
        OwnedArray<PluginDescription> typesFound;
        playerManager->getKnownPluginList().scanAndAddDragAndDroppedFiles(playerManager->getFormatManager(), files, typesFound);
        
        // Notify the user that plugins were added to the list
        if (typesFound.size() > 0)
        {
            String message = String(typesFound.size()) + " plugin(s) added to the list.";
            AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Plugins Added", message);
        }
    }
}

void covertracks::MainHostWindow::songSelected(SongManagerComponent* /*source*/, std::shared_ptr<Song> song)
{
    // The new PerformanceLibraryComponent manages its own state and doesn't need
    // external song selection integration - it can select songs internally
    // This method is kept for interface compatibility but no longer performs any action
    (void)song; // Suppress unused parameter warning
}

bool covertracks::MainHostWindow::isDoublePrecisionProcessingEnabled()
{
    if (auto* props = getAppProperties().getUserSettings())
        return props->getBoolValue ("doublePrecisionProcessing", false);

    return false;
}

bool covertracks::MainHostWindow::isAutoScalePluginWindowsEnabled()
{
    if (auto* props = getAppProperties().getUserSettings())
        return props->getBoolValue ("autoScalePluginWindows", false);

    return false;
}

// Accessor method implementations
AudioPluginFormatManager& covertracks::MainHostWindow::getFormatManager() const
{
    if (playerManager != nullptr)
        return playerManager->getFormatManager();
        
    // This should never happen in practice since we create PlayerManager in the constructor
    jassertfalse;
    static AudioPluginFormatManager fallbackManager;
    return fallbackManager;
}

AudioDeviceManager& covertracks::MainHostWindow::getDeviceManager()
{
    // PlayerManager should always be created in the constructor before this is called
    jassert(playerManager != nullptr);
    
    if (playerManager != nullptr)
        return playerManager->getAudioEngine()->getDeviceManager();
        
    // This should never happen, but we need to return something
    static AudioDeviceManager fallbackManager;
    return fallbackManager;
}

KnownPluginList& covertracks::MainHostWindow::getPluginList() const
{
    if (playerManager != nullptr)
        return playerManager->getKnownPluginList();
        
    // This should never happen in practice since we create PlayerManager in the constructor
    jassertfalse;
    static KnownPluginList fallbackList;
    return fallbackList;
}

void covertracks::MainHostWindow::updatePrecisionMenuItem (ApplicationCommandInfo& info)
{
    info.setInfo ("Double Floating-Point Precision Rendering", {}, "General", 0);
    info.setTicked (isDoublePrecisionProcessingEnabled());
}

void covertracks::MainHostWindow::updateAutoScaleMenuItem (ApplicationCommandInfo& info)
{
    info.setInfo ("Auto-Scale Plugin Windows", {}, "General", 0);
    info.setTicked (isAutoScalePluginWindowsEnabled());
}

}//namespace covertracks
