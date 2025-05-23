#pragma once

#include <JuceHeader.h>
#include "../../Core/MusicPatternComponents.h"

namespace covertracks {

/**
 * @brief Dialog for editing a section's properties
 */
class SectionEditorDialog : public juce::DialogWindow
{
public:
    /**
     * @brief Callback for when a section is edited
     */
    using SectionEditedCallback = std::function<void(std::shared_ptr<Section> section)>;
    
    /**
     * @brief Show a section editor dialog
     * 
     * @param section The section to edit
     * @param callback Callback to call when the section is edited
     * @param parentComponent The parent component
     */
    static void show(std::shared_ptr<Section> section, SectionEditedCallback callback, juce::Component* parentComponent);
    
private:
    /**
     * @brief Content component for the section editor dialog
     */
    class ContentComponent : public juce::Component,
                            public juce::Button::Listener
    {
    public:
        /**
         * @brief Construct a new Content Component
         * 
         * @param section The section to edit
         * @param callback Callback to call when the section is edited
         * @param dialog The parent dialog
         */
        ContentComponent(std::shared_ptr<Section> section, SectionEditedCallback callback, SectionEditorDialog* dialog);
        
        /**
         * @brief Destroy the Content Component
         */
        ~ContentComponent() override;
        
        /**
         * @brief Paint the component
         * 
         * @param g The graphics context
         */
        void paint(juce::Graphics& g) override;
        
        /**
         * @brief Handle component resizing
         */
        void resized() override;
        
        /**
         * @brief Handle button clicks
         * 
         * @param button The button that was clicked
         */
        void buttonClicked(juce::Button* button) override;
        
    private:
        // UI components
        juce::Label nameLabel;
        juce::TextEditor nameEditor;
        
        juce::Label chordsLabel;
        juce::TextEditor chordsEditor;
        
        juce::Label repeatsLabel;
        juce::TextEditor repeatsEditor;
        
        juce::TextButton saveButton;
        juce::TextButton cancelButton;
        
        // Data
        std::shared_ptr<Section> section;
        
        // Callback
        SectionEditedCallback callback;
        
        // Parent dialog
        SectionEditorDialog* dialog;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentComponent)
    };
    
    /**
     * @brief Construct a new Section Editor Dialog
     * 
     * @param section The section to edit
     * @param callback Callback to call when the section is edited
     */
    SectionEditorDialog(std::shared_ptr<Section> section, SectionEditedCallback callback);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionEditorDialog)
};

} // namespace covertracks
