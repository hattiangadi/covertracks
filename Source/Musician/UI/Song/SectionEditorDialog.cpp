#include "SectionEditorDialog.h"

namespace covertracks {

// Static method to show the dialog
void SectionEditorDialog::show(std::shared_ptr<Section> section, SectionEditedCallback callback, juce::Component* parentComponent)
{
    auto dialog = new SectionEditorDialog(section, callback);
    
    dialog->centreAroundComponent(parentComponent, dialog->getWidth(), dialog->getHeight());
    dialog->setVisible(true);
}

// Dialog constructor
SectionEditorDialog::SectionEditorDialog(std::shared_ptr<Section> section, SectionEditedCallback callback)
    : DialogWindow("Edit Section", juce::Colours::lightgrey, true, true)
{
    auto content = new ContentComponent(section, callback, this);
    setContentOwned(content, true);
    
    centreWithSize(400, 300);
    setResizable(false, false);
    setUsingNativeTitleBar(true);
}

// Content component implementation
SectionEditorDialog::ContentComponent::ContentComponent(std::shared_ptr<Section> section, SectionEditedCallback callback, SectionEditorDialog* dialog)
    : section(section), callback(callback), dialog(dialog)
{
    // Set up name components
    nameLabel.setText("Name:", juce::dontSendNotification);
    juce::FontOptions fontOptions;
    nameLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(nameLabel);
    
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.setReadOnly(false);
    nameEditor.setScrollbarsShown(false);
    nameEditor.setCaretVisible(true);
    nameEditor.setPopupMenuEnabled(true);
    nameEditor.setText(section->getName());
    addAndMakeVisible(nameEditor);
    
    // Set up chords components
    chordsLabel.setText("Chord Progression:", juce::dontSendNotification);
    chordsLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(chordsLabel);
    
    chordsEditor.setMultiLine(false);
    chordsEditor.setReturnKeyStartsNewLine(false);
    chordsEditor.setReadOnly(false);
    chordsEditor.setScrollbarsShown(false);
    chordsEditor.setCaretVisible(true);
    chordsEditor.setPopupMenuEnabled(true);
    chordsEditor.setText("C | Am | F | G"); // Default chord progression
    addAndMakeVisible(chordsEditor);
    
    // Set up repeats components
    repeatsLabel.setText("Repeats:", juce::dontSendNotification);
    repeatsLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    addAndMakeVisible(repeatsLabel);
    
    repeatsEditor.setMultiLine(false);
    repeatsEditor.setReturnKeyStartsNewLine(false);
    repeatsEditor.setReadOnly(false);
    repeatsEditor.setScrollbarsShown(false);
    repeatsEditor.setCaretVisible(true);
    repeatsEditor.setPopupMenuEnabled(true);
    repeatsEditor.setText("1");
    addAndMakeVisible(repeatsEditor);
    
    // Set up buttons
    saveButton.setButtonText("Save");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);
    
    // Set size
    setSize(400, 300);
}

SectionEditorDialog::ContentComponent::~ContentComponent()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
}

void SectionEditorDialog::ContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SectionEditorDialog::ContentComponent::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Name field
    auto nameArea = bounds.removeFromTop(30);
    nameLabel.setBounds(nameArea.removeFromLeft(150));
    nameEditor.setBounds(nameArea);
    
    bounds.removeFromTop(10); // Spacing
    
    // Chords field
    auto chordsArea = bounds.removeFromTop(30);
    chordsLabel.setBounds(chordsArea.removeFromLeft(150));
    chordsEditor.setBounds(chordsArea);
    
    bounds.removeFromTop(10); // Spacing
    
    // Repeats field
    auto repeatsArea = bounds.removeFromTop(30);
    repeatsLabel.setBounds(repeatsArea.removeFromLeft(150));
    repeatsEditor.setBounds(repeatsArea);
    
    bounds.removeFromTop(20); // Spacing
    
    // Buttons
    auto buttonArea = bounds.removeFromBottom(40);
    auto buttonWidth = (buttonArea.getWidth() - 10) / 2;
    
    saveButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(0, 5));
    cancelButton.setBounds(buttonArea.reduced(0, 5));
}

void SectionEditorDialog::ContentComponent::buttonClicked(juce::Button* button)
{
    if (button == &saveButton)
    {
        // Update the section
        section->setName(nameEditor.getText());
        
        // Store chord progression and repeats as properties
        // In a real implementation, we would create phrases and bars based on the chord progression
        
        // Call the callback
        if (callback)
        {
            callback(section);
        }
        
        // Close the dialog
        if (dialog)
        {
            dialog->exitModalState(0);
        }
    }
    else if (button == &cancelButton)
    {
        // Close the dialog without saving
        if (dialog)
        {
            dialog->exitModalState(0);
        }
    }
}

} // namespace covertracks
