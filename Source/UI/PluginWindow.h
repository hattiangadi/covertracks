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

#include "../Plugins/IOConfigurationWindow.h"
#include "../Plugins/ARAPlugin.h"

namespace covertracks {
inline String getFormatSuffix (const AudioProcessor* plugin)
{
    const auto format = [plugin]()
    {
        if (auto* instance = dynamic_cast<const AudioPluginInstance*> (plugin))
            return instance->getPluginDescription().pluginFormatName;

        return String();
    }();

    return format.isNotEmpty() ? (" (" + format + ")") : format;
}

class PluginGraph;

/**
    A window that shows a log of parameter change messages sent by the plugin.
*/
class PluginDebugWindow final : public AudioProcessorEditor,
                                public AudioProcessorParameter::Listener,
                                public ListBoxModel,
                                public AsyncUpdater
{
public:
    PluginDebugWindow (AudioProcessor& proc)
        : AudioProcessorEditor (proc), audioProc (proc)
    {
        setSize (500, 200);
        addAndMakeVisible (list);

        for (auto* p : audioProc.getParameters())
            p->addListener (this);

        log.add ("Parameter debug log started");
    }

    ~PluginDebugWindow() override
    {
        for (auto* p : audioProc.getParameters())
            p->removeListener (this);
    }

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        auto* param = audioProc.getParameters()[parameterIndex];
        auto value = param->getCurrentValueAsText().quoted() + " (" + String (newValue, 4) + ")";

        appendToLog ("parameter change", *param, value);
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
    {
        auto* param = audioProc.getParameters()[parameterIndex];
        appendToLog ("gesture", *param, gestureIsStarting ? "start" : "end");
    }

private:
    void appendToLog (StringRef action, AudioProcessorParameter& param, StringRef value)
    {
        String entry (action + " " + param.getName (30).quoted() + " [" + String (param.getParameterIndex()) + "]: " + value);

        {
            ScopedLock lock (pendingLogLock);
            pendingLogEntries.add (entry);
        }

        triggerAsyncUpdate();
    }

    void resized() override
    {
        list.setBounds (getLocalBounds());
    }

    int getNumRows() override
    {
        return log.size();
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool) override
    {
        g.setColour (getLookAndFeel().findColour (TextEditor::textColourId));

        if (isPositiveAndBelow (rowNumber, log.size()))
            g.drawText (log[rowNumber], Rectangle<int> { 0, 0, width, height }, Justification::left, true);
    }

    void handleAsyncUpdate() override
    {
        if (log.size() > logSizeTrimThreshold)
            log.removeRange (0, log.size() - maxLogSize);

        {
            ScopedLock lock (pendingLogLock);
            log.addArray (pendingLogEntries);
            pendingLogEntries.clear();
        }

        list.updateContent();
        list.scrollToEnsureRowIsOnscreen (log.size() - 1);
    }

    constexpr static const int maxLogSize = 300;
    constexpr static const int logSizeTrimThreshold = 400;

    ListBox list { "Log", this };

    StringArray log;
    StringArray pendingLogEntries;
    CriticalSection pendingLogLock;

    AudioProcessor& audioProc;
};

//==============================================================================
/**
    A desktop window containing a plugin's GUI.
*/
class PluginWindow final : public DocumentWindow
{
public:
    enum class Type
    {
        normal = 0,
        generic,
        programs,
        audioIO,
        debug,
        araHost,
        numTypes
    };

    PluginWindow (AudioProcessorGraph::Node* n, Type t, OwnedArray<PluginWindow>& windowList)
        : DocumentWindow (n->getProcessor()->getName() + getFormatSuffix (n->getProcessor()),
                          LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                          DocumentWindow::minimiseButton | DocumentWindow::closeButton),
          activeWindowList (windowList),
          node (n), type (t)
    {
        std::cout << "[PluginWindow] Constructor starting for " << n->getProcessor()->getName() 
                  << ", format: " << getFormatSuffix(n->getProcessor()) 
                  << ", type: " << static_cast<int>(type) << std::endl;
        
        setSize (400, 300);

        AudioProcessorEditor* ui = nullptr;
        try
        {
            std::cout << "[PluginWindow] Attempting to create processor editor" << std::endl;
            ui = createProcessorEditor (*node->getProcessor(), type);
            
            if (ui != nullptr)
            {
                std::cout << "[PluginWindow] Editor created successfully, size: " 
                          << ui->getWidth() << "x" << ui->getHeight() << std::endl;
                setContentOwned (ui, true);
                setResizable (ui->isResizable(), false);
            }
            else
            {
                std::cout << "[PluginWindow] Failed to create editor - createProcessorEditor returned nullptr" << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[PluginWindow] Exception creating editor: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[PluginWindow] Unknown exception creating editor" << std::endl;
        }

        setConstrainer (&constrainer);

       #if JUCE_IOS || JUCE_ANDROID
        const auto screenBounds = Desktop::getInstance().getDisplays().getTotalBounds (true).toFloat();
        const auto scaleFactor = jmin ((screenBounds.getWidth()  - 50.0f) / (float) getWidth(),
                                       (screenBounds.getHeight() - 50.0f) / (float) getHeight());

        if (scaleFactor < 1.0f)
        {
            setSize ((int) (scaleFactor * (float) getWidth()),
                     (int) (scaleFactor * (float) getHeight()));
        }

        setTopLeftPosition (20, 20);
       #else
        setTopLeftPosition (node->properties.getWithDefault (getLastXProp (type), Random::getSystemRandom().nextInt (500)),
                            node->properties.getWithDefault (getLastYProp (type), Random::getSystemRandom().nextInt (500)));
       #endif

        node->properties.set (getOpenProp (type), true);

        std::cout << "[PluginWindow] Making window visible, size: " << getWidth() << "x" << getHeight() << std::endl;
        setVisible (true);
        toFront (true);
        std::cout << "[PluginWindow] Constructor complete" << std::endl;
    }

    ~PluginWindow() override
    {
        clearContentComponent();
    }

    void moved() override
    {
        node->properties.set (getLastXProp (type), getX());
        node->properties.set (getLastYProp (type), getY());
    }

    void closeButtonPressed() override
    {
        node->properties.set (getOpenProp (type), false);
        activeWindowList.removeObject (this);
    }

    static String getLastXProp (Type type)    { return "uiLastX_" + getTypeName (type); }
    static String getLastYProp (Type type)    { return "uiLastY_" + getTypeName (type); }
    static String getOpenProp  (Type type)    { return "uiopen_"  + getTypeName (type); }

    OwnedArray<PluginWindow>& activeWindowList;
    const AudioProcessorGraph::Node::Ptr node;
    const Type type;

    BorderSize<int> getBorderThickness() const override
    {
       #if JUCE_IOS || JUCE_ANDROID
        const int border = 10;
        return { border, border, border, border };
       #else
        return DocumentWindow::getBorderThickness();
       #endif
    }

private:
    class DecoratorConstrainer final : public BorderedComponentBoundsConstrainer
    {
    public:
        explicit DecoratorConstrainer (DocumentWindow& windowIn)
            : window (windowIn) {}

        ComponentBoundsConstrainer* getWrappedConstrainer() const override
        {
            auto* editor = dynamic_cast<AudioProcessorEditor*> (window.getContentComponent());
            return editor != nullptr ? editor->getConstrainer() : nullptr;
        }

        BorderSize<int> getAdditionalBorder() const override
        {
            const auto nativeFrame = [&]() -> BorderSize<int>
            {
                if (auto* peer = window.getPeer())
                    if (const auto frameSize = peer->getFrameSizeIfPresent())
                        return *frameSize;

                return {};
            }();

            return nativeFrame.addedTo (window.getContentComponentBorder());
        }

    private:
        DocumentWindow& window;
    };

    DecoratorConstrainer constrainer { *this };

    float getDesktopScaleFactor() const override     { return 1.0f; }

    static AudioProcessorEditor* createProcessorEditor (AudioProcessor& processor,
                                                         PluginWindow::Type type)
    {
        std::cout << "[PluginWindow] createProcessorEditor called for " << processor.getName() 
                  << ", type: " << static_cast<int>(type) << std::endl;
        
        if (type == PluginWindow::Type::normal)
        {
            if (processor.hasEditor())
            {
                std::cout << "[PluginWindow] Processor has editor, calling createEditorIfNeeded()" << std::endl;
                try
                {
                    auto* ui = processor.createEditorIfNeeded();
                    if (ui != nullptr)
                    {
                        std::cout << "[PluginWindow] Editor created successfully, size: " 
                                  << ui->getWidth() << "x" << ui->getHeight() << std::endl;
                        return ui;
                    }
                    else
                    {
                        std::cout << "[PluginWindow] createEditorIfNeeded() returned nullptr" << std::endl;
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[PluginWindow] Exception in createEditorIfNeeded(): " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "[PluginWindow] Unknown exception in createEditorIfNeeded()" << std::endl;
                }
            }
            else
            {
                std::cout << "[PluginWindow] Processor doesn't have an editor" << std::endl;
            }

            std::cout << "[PluginWindow] Falling back to generic editor" << std::endl;
            type = PluginWindow::Type::generic;
        }

        if (type == PluginWindow::Type::araHost)
        {
           #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
            std::cout << "[PluginWindow] Attempting to create ARA host editor" << std::endl;
            if (auto* araPluginInstanceWrapper = dynamic_cast<ARAPluginInstanceWrapper*> (&processor))
                if (auto* ui = araPluginInstanceWrapper->createARAHostEditor())
                    return ui;
           #endif
            std::cout << "[PluginWindow] ARA host editor creation failed" << std::endl;
            return {};
        }

        if (type == PluginWindow::Type::generic)
        {
            std::cout << "[PluginWindow] Creating GenericAudioProcessorEditor" << std::endl;
            try
            {
                auto* result = new GenericAudioProcessorEditor (processor);
                result->setResizeLimits (200, 300, 1'000, 10'000);
                std::cout << "[PluginWindow] GenericAudioProcessorEditor created successfully" << std::endl;
                return result;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[PluginWindow] Exception creating GenericAudioProcessorEditor: " << e.what() << std::endl;
                return {};
            }
            catch (...)
            {
                std::cerr << "[PluginWindow] Unknown exception creating GenericAudioProcessorEditor" << std::endl;
                return {};
            }
        }

        if (type == PluginWindow::Type::programs)
        {
            std::cout << "[PluginWindow] Creating ProgramAudioProcessorEditor" << std::endl;
            return new ProgramAudioProcessorEditor (processor);
        }

        if (type == PluginWindow::Type::audioIO)
        {
            std::cout << "[PluginWindow] Creating IOConfigurationWindow" << std::endl;
            return new IOConfigurationWindow (processor);
        }

        if (type == PluginWindow::Type::debug)
        {
            std::cout << "[PluginWindow] Creating PluginDebugWindow" << std::endl;
            return new PluginDebugWindow (processor);
        }

        std::cerr << "[PluginWindow] Invalid window type: " << static_cast<int>(type) << std::endl;
        jassertfalse;
        return {};
    }

    static String getTypeName (Type type)
    {
        switch (type)
        {
            case Type::normal:     return "Normal";
            case Type::generic:    return "Generic";
            case Type::programs:   return "Programs";
            case Type::audioIO:    return "IO";
            case Type::debug:      return "Debug";
            case Type::araHost:    return "ARAHost";
            case Type::numTypes:
            default:               return {};
        }
    }

    //==============================================================================
    struct ProgramAudioProcessorEditor final : public AudioProcessorEditor
    {
        explicit ProgramAudioProcessorEditor (AudioProcessor& p)
            : AudioProcessorEditor (p)
        {
            setOpaque (true);

            addAndMakeVisible (listBox);
            listBox.updateContent();

            const auto rowHeight = listBox.getRowHeight();

            setSize (400, jlimit (rowHeight, 400, p.getNumPrograms() * rowHeight));
        }

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::grey);
        }

        void resized() override
        {
            listBox.setBounds (getLocalBounds());
        }

    private:
        class Model : public ListBoxModel
        {
        public:
            Model (Component& o, AudioProcessor& p)
                : owner (o), proc (p) {}

            int getNumRows() override
            {
                return proc.getNumPrograms();
            }

            void paintListBoxItem (int rowNumber,
                                   Graphics& g,
                                   int width,
                                   int height,
                                   bool rowIsSelected) override
            {
                const auto textColour = owner.findColour (ListBox::textColourId);

                if (rowIsSelected)
                {
                    const auto defaultColour = owner.findColour (ListBox::backgroundColourId);
                    const auto c = rowIsSelected ? defaultColour.interpolatedWith (textColour, 0.5f)
                                                 : defaultColour;

                    g.fillAll (c);
                }

                g.setColour (textColour);
                g.drawText (proc.getProgramName (rowNumber),
                            Rectangle<int> { width, height }.reduced (2),
                            Justification::left,
                            true);
            }

            void selectedRowsChanged (int row) override
            {
                if (0 <= row)
                    proc.setCurrentProgram (row);
            }

        private:
            Component& owner;
            AudioProcessor& proc;
        };

        Model model { *this, *getAudioProcessor() };
        ListBox listBox { "Programs", &model };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramAudioProcessorEditor)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
};
}   // namespace covertracks
