#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace covertracks {

//==============================================================================
// Forward declaration
class CovertracksPluginWindow;

//==============================================================================
/** A dummy function to represent checking if a plugin is DPI aware.
    In a real app, you'd want to store this state for each plugin.
*/
inline bool isDPIAware (te::Plugin&)
{
	return true;
}

//==============================================================================
/** This is the component that will contain the actual editor GUI from the plugin. */
struct CovertracksAudioProcessorEditorContentComp : public te::Plugin::EditorComponent
{
    CovertracksAudioProcessorEditorContentComp (te::ExternalPlugin& plug) : plugin (plug)
    {
        JUCE_AUTORELEASEPOOL
        {
            if (auto pi = plugin.getAudioPluginInstance())
            {
                editor.reset (pi->createEditorIfNeeded());

                if (editor == nullptr)
                    editor = std::make_unique<GenericAudioProcessorEditor> (*pi);

                addAndMakeVisible (*editor);
            }
        }

        resizeToFitEditor (true);
    }

    bool allowWindowResizing() override { return false; }

    ComponentBoundsConstrainer* getBoundsConstrainer() override
    {
        if (editor == nullptr || allowWindowResizing())
            return {};

        return editor->getConstrainer();
    }

    void resized() override
    {
        if (editor != nullptr)
            editor->setBounds (getLocalBounds());
    }

    void childBoundsChanged (Component* c) override
    {
        if (c == editor.get())
        {
            plugin.edit.pluginChanged (plugin);
            resizeToFitEditor();
        }
    }

    void resizeToFitEditor (bool force = false)
    {
        if (force || ! allowWindowResizing())
            setSize (jmax (8, editor != nullptr ? editor->getWidth() : 0), jmax (8, editor != nullptr ? editor->getHeight() : 0));
    }

    te::ExternalPlugin& plugin;
    std::unique_ptr<AudioProcessorEditor> editor;

    CovertracksAudioProcessorEditorContentComp() = delete;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CovertracksAudioProcessorEditorContentComp)
};

//=============================================================================
/** This is the main window frame that will live on the desktop. */
class CovertracksPluginWindow : public juce::DocumentWindow
{
public:
    CovertracksPluginWindow (te::Plugin&);
    ~CovertracksPluginWindow() override;

    static std::unique_ptr<Component> create (te::Plugin&);

    void show();

    void setEditor (std::unique_ptr<te::Plugin::EditorComponent>);
    te::Plugin::EditorComponent* getEditor() const         { return editor.get(); }

    void recreateEditor();
    void recreateEditorAsync();

private:
    void moved() override;
    void userTriedToCloseWindow() override          { plugin.windowState->closeWindowExplicitly(); }
    void closeButtonPressed() override              { userTriedToCloseWindow(); }
    float getDesktopScaleFactor() const override    { return 1.0f; }

    std::unique_ptr<te::Plugin::EditorComponent> editor;

    te::Plugin& plugin;
    te::PluginWindowState& windowState;
    bool updateStoredBounds = false;
};

//==============================================================================
#if JUCE_LINUX
 constexpr bool shouldAddPluginWindowToDesktop = false;
#else
 constexpr bool shouldAddPluginWindowToDesktop = true;
#endif

inline CovertracksPluginWindow::CovertracksPluginWindow (te::Plugin& plug)
    : DocumentWindow (plug.getName(), Colours::black, DocumentWindow::closeButton, shouldAddPluginWindowToDesktop),
      plugin (plug), windowState (*plug.windowState)
{
    DBG("--- CovertracksPluginWindow CREATED for " + plug.getName());
    getConstrainer()->setMinimumOnscreenAmounts (0x10000, 50, 30, 50);
    setResizeLimits (100, 50, 4000, 4000);

    recreateEditor();

    setBoundsConstrained (getLocalBounds() + plugin.windowState->choosePositionForPluginWindow());

    #if JUCE_LINUX
     setAlwaysOnTop (true);
     addToDesktop();
    #endif

     updateStoredBounds = true;
}

inline CovertracksPluginWindow::~CovertracksPluginWindow()
{
    DBG("--- CovertracksPluginWindow DESTROYED for " + plugin.getName());
    updateStoredBounds = false;
    plugin.edit.flushPluginStateIfNeeded (plugin);
    setEditor (nullptr);
}

inline void CovertracksPluginWindow::show()
{
    setVisible (true);
    toFront (false);
    setBoundsConstrained (getBounds());
}

inline void CovertracksPluginWindow::setEditor (std::unique_ptr<te::Plugin::EditorComponent> newEditor)
{
    JUCE_AUTORELEASEPOOL
    {
        setConstrainer (nullptr);
        editor.reset();

        if (newEditor != nullptr)
        {
            editor = std::move (newEditor);
            setContentNonOwned (editor.get(), true);
        }

        setResizable (editor == nullptr || editor->allowWindowResizing(), false);

        if (editor != nullptr && editor->allowWindowResizing())
            setConstrainer (editor->getBoundsConstrainer());
    }
}

inline std::unique_ptr<Component> CovertracksPluginWindow::create (te::Plugin& plugin)
{
    // For external plugins, check if they're still initializing asynchronously
    if (auto externalPlugin = dynamic_cast<te::ExternalPlugin*> (&plugin))
    {
        if (externalPlugin->isInitialisingAsync())
        {
            DBG("--- External plugin is still initializing asynchronously, cannot create window yet");
            return nullptr;
        }
        
        if (externalPlugin->getAudioPluginInstance() == nullptr)
        {
            DBG("--- External plugin AudioPluginInstance is null, cannot create window");
            return nullptr;
        }
    }

    std::unique_ptr<CovertracksPluginWindow> w;

    {
        struct Blocker : public Component { void inputAttemptWhenModal() override {} };

        Blocker blocker;
        blocker.enterModalState (false);

       #if JUCE_WINDOWS && JUCE_WIN_PER_MONITOR_DPI_AWARE
        if (! isDPIAware (plugin))
        {
            ScopedDPIAwarenessDisabler disableDPIAwareness;
            w = std::make_unique<CovertracksPluginWindow> (plugin);
        }
        else
       #endif
        {
            w = std::make_unique<CovertracksPluginWindow> (plugin);
        }
    }

    if (w == nullptr || w->getEditor() == nullptr)
    {
        DBG("--- CovertracksPluginWindow creation failed or editor is null");
        return {};
    }

    w->show();
    return w;
}

inline void CovertracksPluginWindow::recreateEditor()
{
    setEditor (nullptr);
    setEditor (plugin.createEditor());
}

inline void CovertracksPluginWindow::recreateEditorAsync()
{
    DBG("--- CovertracksPluginWindow::recreateEditorAsync called ---");
    setEditor (nullptr);

    Timer::callAfterDelay (50, [this, sp = SafePointer<Component> (this)]
                               {
                                   if (sp != nullptr)
                                       recreateEditor();
                               });
}

inline void CovertracksPluginWindow::moved()
{
    if (updateStoredBounds)
    {
        plugin.windowState->lastWindowBounds = getBounds();
        plugin.edit.pluginChanged (plugin);
    }
}

//==============================================================================
/** This is the hook that tells the Tracktion Engine to use our custom window class. */
class CovertracksUIBehaviour : public te::UIBehaviour
{
public:
    CovertracksUIBehaviour() = default;

    std::unique_ptr<Component> createPluginWindow (te::PluginWindowState& pws) override
    {
        DBG("--- CovertracksUIBehaviour::createPluginWindow called ---");

        if (auto ws = dynamic_cast<te::Plugin::WindowState*> (&pws))
            return CovertracksPluginWindow::create (ws->plugin);

        return {};
    }

    void recreatePluginWindowContentAsync (te::Plugin& p) override
    {
        DBG("--- CovertracksUIBehaviour::recreatePluginWindowContentAsync called for " + p.getName());
        auto* windowComponent = p.windowState->pluginWindow.get();

        if (windowComponent == nullptr)
        {
            DBG("--- FAILED: pluginWindow is null.");
            return;
        }

        DBG(juce::String("--- pluginWindow type: ") + typeid(*windowComponent).name());

        if (auto* w = dynamic_cast<CovertracksPluginWindow*> (windowComponent))
        {
            DBG("--- dynamic_cast to CovertracksPluginWindow SUCCESSFUL. Calling recreateEditorAsync.");
            return w->recreateEditorAsync();
        }

        DBG("--- dynamic_cast to CovertracksPluginWindow FAILED. Falling back to base class.");
        UIBehaviour::recreatePluginWindowContentAsync (p);
    }
};

} // namespace covertracks
