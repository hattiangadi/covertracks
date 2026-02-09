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
#include "UI/MainHostWindow.h"
#include "Plugins/InternalPlugins.h"
#include "LLM/Core/LLMManager.h"

#if ! (JUCE_PLUGINHOST_VST || JUCE_PLUGINHOST_VST3 || JUCE_PLUGINHOST_AU)
 #error "If you're building the audio plugin host, you probably want to enable VST and/or AU support"
#endif

namespace covertracks {
    
class PluginScannerSubprocess final : private ChildProcessWorker,
                                      private AsyncUpdater
{
public:
    PluginScannerSubprocess()
    {
        formatManager.addDefaultFormats();
    }

    using ChildProcessWorker::initialiseFromCommandLine;

private:
    void handleMessageFromCoordinator (const MemoryBlock& mb) override
    {
        if (mb.isEmpty())
            return;

        const std::lock_guard<std::mutex> lock (mutex);

        if (const auto results = doScan (mb); ! results.isEmpty())
        {
            sendResults (results);
        }
        else
        {
            pendingBlocks.emplace (mb);
            triggerAsyncUpdate();
        }
    }

    void handleConnectionLost() override
    {
        JUCEApplicationBase::quit();
    }

    void handleAsyncUpdate() override
    {
        for (;;)
        {
            const std::lock_guard<std::mutex> lock (mutex);

            if (pendingBlocks.empty())
                return;

            sendResults (doScan (pendingBlocks.front()));
            pendingBlocks.pop();
        }
    }

    OwnedArray<PluginDescription> doScan (const MemoryBlock& block)
    {
        MemoryInputStream stream { block, false };
        const auto formatName = stream.readString();
        const auto identifier = stream.readString();

        PluginDescription pd;
        pd.fileOrIdentifier = identifier;
        pd.uniqueId = pd.deprecatedUid = 0;

        const auto matchingFormat = [&]() -> AudioPluginFormat*
        {
            for (auto* format : formatManager.getFormats())
                if (format->getName() == formatName)
                    return format;

            return nullptr;
        }();

        OwnedArray<PluginDescription> results;

        if (matchingFormat != nullptr
            && (MessageManager::getInstance()->isThisTheMessageThread()
                || matchingFormat->requiresUnblockedMessageThreadDuringCreation (pd)))
        {
            matchingFormat->findAllTypesForFile (results, identifier);
        }

        return results;
    }

    void sendResults (const OwnedArray<PluginDescription>& results)
    {
        XmlElement xml ("LIST");

        for (const auto& desc : results)
            xml.addChildElement (desc->createXml().release());

        const auto str = xml.toString();
        sendMessageToCoordinator ({ str.toRawUTF8(), str.getNumBytesAsUTF8() });
    }

    std::mutex mutex;
    std::queue<MemoryBlock> pendingBlocks;
    AudioPluginFormatManager formatManager;
};

//==============================================================================
class PluginHostApp final : public JUCEApplication,
                            private AsyncUpdater
{
public:
    PluginHostApp() = default;

    void initialise (const String& commandLine) override
    {
        std::cout << "[PluginHostApp] Starting initialization..." << std::endl;
        
        // Check for test mode first
        auto args = getCommandLineParameterArray();
        if (args.contains("--run-tests") || args.contains("--test"))
        {
            runTestsAndExit(args);
            return;
        }
        
        auto scannerSubprocess = std::make_unique<PluginScannerSubprocess>();

        if (scannerSubprocess->initialiseFromCommandLine (commandLine, processUID))
        {
            std::cout << "[PluginHostApp] Initializing as scanner subprocess" << std::endl;
            storedScannerSubprocess = std::move (scannerSubprocess);
            return;
        }

        std::cout << "[PluginHostApp] Initializing settings file" << std::endl;
        // initialise our settings file..

        PropertiesFile::Options options;
        options.applicationName     = "Covertracks";
        options.filenameSuffix      = "settings";
        options.osxLibrarySubFolder = "Preferences";

        appProperties.reset (new ApplicationProperties());
        appProperties->setStorageParameters (options);

        std::cout << "[PluginHostApp] Creating MainHostWindow" << std::endl;
        mainWindow.reset (new MainHostWindow());
        std::cout << "[PluginHostApp] MainHostWindow created" << std::endl;

        std::cout << "[PluginHostApp] Registering commands" << std::endl;
        commandManager.registerAllCommandsForTarget (this);
        commandManager.registerAllCommandsForTarget (mainWindow.get());

        std::cout << "[PluginHostApp] Updating menu items" << std::endl;
        mainWindow->menuItemsChanged();
        std::cout << "[PluginHostApp] Menu items updated" << std::endl;

        // Important note! We're going to use an async update here so that if we need
        // to re-open a file and instantiate some plugins, it will happen AFTER this
        // initialisation method has returned.
        // On Windows this probably won't make a difference, but on OSX there's a subtle event loop
        // issue that can happen if a plugin runs one of those irritating modal dialogs while it's
        // being loaded. If that happens inside this method, the OSX event loop seems to be in some
        // kind of special "initialisation" mode and things get confused. But if we load the plugin
        // later when the normal event loop is running, everything's fine.
        std::cout << "[PluginHostApp] Triggering async update" << std::endl;
        triggerAsyncUpdate();
        std::cout << "[PluginHostApp] Initialization complete" << std::endl;
    }

    void handleAsyncUpdate() override
    {
        File fileToOpen;

       #if JUCE_ANDROID || JUCE_IOS
        // In the Tracktion architecture, default documents are handled by Player loading
       #else
        for (int i = 0; i < getCommandLineParameterArray().size(); ++i)
        {
            fileToOpen = File::getCurrentWorkingDirectory().getChildFile (getCommandLineParameterArray()[i]);

            if (fileToOpen.existsAsFile())
                break;
        }
       #endif

        if (! fileToOpen.existsAsFile())
        {
            RecentlyOpenedFilesList recentFiles;
            recentFiles.restoreFromString (getAppProperties().getUserSettings()->getValue ("recentFilterGraphFiles"));

            if (recentFiles.getNumFiles() > 0)
                fileToOpen = recentFiles.getFile (0);
        }

        // In the instrument/player approach, graph files are loaded by the PlayerEditorComponent
        // The main window no longer directly loads graph files
    }

    void shutdown() override
    {
        // Shut down LLM clients before static destruction to avoid thread crashes
        LLMManager::getInstance().shutdown();

        mainWindow = nullptr;
        appProperties = nullptr;
        LookAndFeel::setDefaultLookAndFeel (nullptr);
        
        // Clean up MouseCursor to prevent leak
        // Force all mouse sources to reset their cursors
        for (int i = 0; i < Desktop::getInstance().getNumMouseSources(); ++i)
        {
            Desktop::getInstance().getMouseSource(i)->forceMouseCursorUpdate();
        }
        
        // Set a null cursor to release any custom cursors
        MouseCursor nullCursor;
        Desktop::getInstance().setMousePosition(Point<int>(-1000, -1000));
    }

    void suspended() override
    {
       #if JUCE_ANDROID || JUCE_IOS
        // In the instrument/player approach, graph files are saved by the PlayerEditorComponent
        // The main window no longer directly manages graph files
       #endif
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->tryToQuitApplication();
        else
            JUCEApplicationBase::quit();
    }

    bool backButtonPressed() override
    {
        // In the instrument/player approach, side panels are managed differently
        // The main window no longer has a graphHolder
        
        return true;
    }

    const String getApplicationName() override       { return "Covertracks"; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    ApplicationCommandManager commandManager;
    std::unique_ptr<ApplicationProperties> appProperties;

private:
    void runTestsAndExit(const juce::StringArray& args)
    {
        std::cout << "=== Covertracks Unit Tests ===" << std::endl;
        std::cout << "Running in CLI test mode..." << std::endl;
        
        // Check for list categories option
        if (args.contains("--list-categories"))
        {
            listTestCategories();
            return;
        }
        
        // Check for category filter
        juce::String categoryFilter;
        for (int i = 0; i < args.size(); ++i)
        {
            if (args[i] == "--category" && i + 1 < args.size())
            {
                categoryFilter = args[i + 1];
                break;
            }
            if (args[i].startsWith("--category="))
            {
                categoryFilter = args[i].fromFirstOccurrenceOf("=", false, false);
                break;
            }
        }
        
        // Run tests with optional category filter
        juce::UnitTestRunner runner;
        if (categoryFilter.isNotEmpty())
        {
            std::cout << "Filtering tests by category: " << categoryFilter << std::endl;
            runner.runTestsInCategory(categoryFilter);
        }
        else
        {
            std::cout << "Running all test categories..." << std::endl;
            runner.runAllTests();
        }
        
        // Get results and exit with appropriate code
        int totalTests = 0;
        int totalFailures = 0;
        int totalPasses = 0;
        
        for (int i = 0; i < runner.getNumResults(); ++i)
        {
            const auto* result = runner.getResult(i);
            if (result != nullptr)
            {
                totalPasses += result->passes;
                totalFailures += result->failures;
                totalTests += (result->passes + result->failures);
            }
        }
        
        std::cout << std::endl;
        std::cout << "=== Test Results ===" << std::endl;
        std::cout << "Tests run: " << totalTests << std::endl;
        std::cout << "Failures: " << totalFailures << std::endl;
        
        if (totalFailures == 0)
        {
            std::cout << "✅ All tests passed!" << std::endl;
        }
        else
        {
            std::cout << "❌ " << totalFailures << " test(s) failed" << std::endl;
        }
        
        // Exit with failure code if tests failed (for CI/CD)
        setApplicationReturnValue(totalFailures > 0 ? 1 : 0);
        quit();
    }
    
    void listTestCategories()
    {
        std::cout << "=== Available Test Categories ===" << std::endl;
        
        // Get all available test categories by running tests with dummy category
        // and inspecting the registered tests
        auto allTests = juce::UnitTest::getAllTests();
        juce::StringArray categories;
        
        for (auto* test : allTests)
        {
            juce::String category = test->getCategory();
            if (category.isNotEmpty() && !categories.contains(category))
                categories.add(category);
        }
        
        categories.sort(false);
        
        for (const auto& category : categories)
        {
            std::cout << "  " << category << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  --run-tests                    Run all tests" << std::endl;
        std::cout << "  --run-tests --category=NAME    Run tests in specific category" << std::endl;
        std::cout << "  --run-tests --list-categories  Show this list" << std::endl;
        
        setApplicationReturnValue(0);
        quit();
    }

    std::unique_ptr<MainHostWindow> mainWindow;
    std::unique_ptr<PluginScannerSubprocess> storedScannerSubprocess;
};

static PluginHostApp& getApp()                    { return *dynamic_cast<PluginHostApp*> (JUCEApplication::getInstance()); }

ApplicationProperties& getAppProperties()         { return *getApp().appProperties; }
ApplicationCommandManager& getCommandManager()    { return getApp().commandManager; }

bool isOnTouchDevice()
{
    static bool isTouch = Desktop::getInstance().getMainMouseSource().isTouch();
    return isTouch;
}

//==============================================================================
static AutoScale autoScaleFromString (StringRef str)
{
    if (str.isEmpty())                     return AutoScale::useDefault;
    if (str == CharPointer_ASCII { "0" })  return AutoScale::scaled;
    if (str == CharPointer_ASCII { "1" })  return AutoScale::unscaled;

    jassertfalse;
    return AutoScale::useDefault;
}

static const char* autoScaleToString (AutoScale autoScale)
{
    if (autoScale == AutoScale::scaled)    return "0";
    if (autoScale == AutoScale::unscaled)  return "1";

    return {};
}

AutoScale getAutoScaleValueForPlugin (const String& identifier)
{
    if (identifier.isNotEmpty())
    {
        auto plugins = StringArray::fromLines (getAppProperties().getUserSettings()->getValue ("autoScalePlugins"));
        plugins.removeEmptyStrings();

        for (auto& plugin : plugins)
        {
            auto fromIdentifier = plugin.fromFirstOccurrenceOf (identifier, false, false);

            if (fromIdentifier.isNotEmpty())
                return autoScaleFromString (fromIdentifier.fromFirstOccurrenceOf (":", false, false));
        }
    }

    return AutoScale::useDefault;
}

void setAutoScaleValueForPlugin (const String& identifier, AutoScale s)
{
    auto plugins = StringArray::fromLines (getAppProperties().getUserSettings()->getValue ("autoScalePlugins"));
    plugins.removeEmptyStrings();

    auto index = [identifier, plugins]
    {
        auto it = std::find_if (plugins.begin(), plugins.end(),
                                [&] (const String& str) { return str.startsWith (identifier); });

        return (int) std::distance (plugins.begin(), it);
    }();

    if (s == AutoScale::useDefault && index != plugins.size())
    {
        plugins.remove (index);
    }
    else
    {
        auto str = identifier + ":" + autoScaleToString (s);

        if (index != plugins.size())
            plugins.getReference (index) = str;
        else
            plugins.add (str);
    }

    getAppProperties().getUserSettings()->setValue ("autoScalePlugins", plugins.joinIntoString ("\n"));
}

static bool isAutoScaleAvailableForPlugin (const PluginDescription& description)
{
    return autoScaleOptionAvailable
          && (description.pluginFormatName.containsIgnoreCase ("VST")
              || description.pluginFormatName.containsIgnoreCase ("LV2"));
}

bool shouldAutoScalePlugin (const PluginDescription& description)
{
    if (! isAutoScaleAvailableForPlugin (description))
        return false;

    const auto scaleValue = getAutoScaleValueForPlugin (description.fileOrIdentifier);

    return (scaleValue == AutoScale::scaled
              || (scaleValue == AutoScale::useDefault
                    && getAppProperties().getUserSettings()->getBoolValue ("autoScalePluginWindows")));
}

void addPluginAutoScaleOptionsSubMenu (AudioPluginInstance* pluginInstance,
                                       PopupMenu& menu)
{
    if (pluginInstance == nullptr)
        return;

    auto description = pluginInstance->getPluginDescription();

    if (! isAutoScaleAvailableForPlugin (description))
        return;

    auto identifier = description.fileOrIdentifier;

    PopupMenu autoScaleMenu;

    autoScaleMenu.addItem ("Default",
                           true,
                           getAutoScaleValueForPlugin (identifier) == AutoScale::useDefault,
                           [identifier] { setAutoScaleValueForPlugin (identifier, AutoScale::useDefault); });

    autoScaleMenu.addItem ("Enabled",
                           true,
                           getAutoScaleValueForPlugin (identifier) == AutoScale::scaled,
                           [identifier] { setAutoScaleValueForPlugin (identifier, AutoScale::scaled); });

    autoScaleMenu.addItem ("Disabled",
                           true,
                           getAutoScaleValueForPlugin (identifier) == AutoScale::unscaled,
                           [identifier] { setAutoScaleValueForPlugin (identifier, AutoScale::unscaled); });

    menu.addSubMenu ("Auto-scale window", autoScaleMenu);
}

}   // namespace covertracks

// This kicks the whole thing off..
START_JUCE_APPLICATION (covertracks::PluginHostApp)
