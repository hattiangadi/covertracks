#include "SongEditorComponent.h"
#include "../../Core/Song.h"
#include "../../../UI/Styling/AppFonts.h"

namespace covertracks {

// SongEditAction implementation
SongEditorComponent::SongEditAction::SongEditAction(SongEditorComponent& ownerComponent, std::shared_ptr<Song> oldSongPtr, std::shared_ptr<Song> newSongPtr)
    : owner(ownerComponent), oldSong(oldSongPtr), newSong(newSongPtr)
{
}

bool SongEditorComponent::SongEditAction::perform()
{
    if (newSong)
    {
        owner.currentSong = newSong;
        owner.updateEditorFields();
        return true;
    }
    return false;
}

bool SongEditorComponent::SongEditAction::undo()
{
    if (oldSong)
    {
        owner.currentSong = oldSong;
        owner.updateEditorFields();
        return true;
    }
    return false;
}

// SongEditorComponent implementation
SongEditorComponent::SongEditorComponent(std::shared_ptr<LLMClient> clientPtr)
    : tabbedComponent(juce::TabbedButtonBar::TabsAtTop),
      llmClient(clientPtr)
{
    // Set up the tabbed component
    addAndMakeVisible(tabbedComponent);
    setupTabs();
    
    // Set up common buttons
    saveButton.setButtonText("Save");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);
    
    importImageButton.setButtonText("Import Image");
    importImageButton.addListener(this);
    addAndMakeVisible(importImageButton);
    
    // Set up command manager for keyboard shortcuts
    commandManager = std::make_unique<juce::ApplicationCommandManager>();
    commandManager->registerAllCommandsForTarget(this);
    addKeyListener(commandManager->getKeyMappings());
    
    // Set size constraints
    setSize(800, 600);
}

SongEditorComponent::~SongEditorComponent()
{
    // Remove listeners
    titleEditor.removeListener(this);
    tempoEditor.removeListener(this);
    
    // Unregister as command target
    if (commandManager)
        commandManager->setFirstCommandTarget(nullptr);
    
    // Clean up resources
    imageImportComponent.reset();
}

void SongEditorComponent::setupTabs()
{
    // Set up the tabs
    setupBasicInfoTab();
    setupStructureTab();
    setupImportTab();
    
    // Add tabs to the tabbed component
    tabbedComponent.addTab("Basic Info", juce::Colours::white, &basicInfoTab, false);
    tabbedComponent.addTab("Structure", juce::Colours::white, &structureTab, false);
    tabbedComponent.addTab("Import", juce::Colours::white, &importTab, false);
    
    // Set the current tab
    tabbedComponent.setCurrentTabIndex(0);
}

void SongEditorComponent::setupBasicInfoTab()
{
    // Add components to the basic info tab
    basicInfoTab.addAndMakeVisible(titleLabel);
    basicInfoTab.addAndMakeVisible(titleEditor);
    basicInfoTab.addAndMakeVisible(tempoLabel);
    basicInfoTab.addAndMakeVisible(tempoEditor);
    
    // Set up title components
    titleLabel.setText("Title:", juce::dontSendNotification);
    titleLabel.setFont(AppFonts::getHeaderFont());
    
    titleEditor.setMultiLine(false);
    titleEditor.setReturnKeyStartsNewLine(false);
    titleEditor.setReadOnly(false);
    titleEditor.setScrollbarsShown(false);
    titleEditor.setCaretVisible(true);
    titleEditor.setPopupMenuEnabled(true);
    titleEditor.setText("");
    titleEditor.addListener(this);
    
    // Set up tempo components
    tempoLabel.setText("Tempo (BPM):", juce::dontSendNotification);
    tempoLabel.setFont(AppFonts::getHeaderFont());
    
    tempoEditor.setMultiLine(false);
    tempoEditor.setReturnKeyStartsNewLine(false);
    tempoEditor.setReadOnly(false);
    tempoEditor.setScrollbarsShown(false);
    tempoEditor.setCaretVisible(true);
    tempoEditor.setPopupMenuEnabled(true);
    tempoEditor.setText("");
    tempoEditor.addListener(this);
}

void SongEditorComponent::setupStructureTab()
{
    // Add components to the structure tab
    structureTab.addAndMakeVisible(structureLabel);
    structureTab.addAndMakeVisible(structureEditor);
    
    // Set up structure label
    structureLabel.setText("Song Structure (JSON):", juce::dontSendNotification);
    structureLabel.setFont(AppFonts::getHeaderFont());
    
    // Configure the text editor for JSON editing
    structureEditor.setMultiLine(true);
    structureEditor.setReturnKeyStartsNewLine(true);
    // Use a monospaced font for the JSON editor
    juce::Font monoFont = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), "", 14.0f));
    structureEditor.setFont(monoFont);
    structureEditor.setTabKeyUsedAsCharacter(true);
    structureEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::white);
    structureEditor.setColour(juce::TextEditor::textColourId, juce::Colours::black);
    structureEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::lightgrey);
    structureEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::lightblue);
    structureEditor.addListener(this);
    
    // Add a help label
    structureHelpLabel.setText("Enter the song structure as JSON. This will be used to generate the performance.", juce::dontSendNotification);
    juce::FontOptions helpFontOptions;
    structureHelpLabel.setFont(helpFontOptions.withHeight(12.0f));
    structureTab.addAndMakeVisible(structureHelpLabel);
}

void SongEditorComponent::setupImportTab()
{
    // Create the image import component
    imageImportComponent = std::make_unique<SongImageImportComponent>(llmClient, [this](const juce::var& extractedStructure) {
        // This is the callback that will be called when a song structure is extracted from an image
        handleExtractedStructure(extractedStructure);
    });
    
    // Add the image import component to the import tab
    importTab.addAndMakeVisible(imageImportComponent.get());
}

void SongEditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("Song Editor", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
}

void SongEditorComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(40); // Space for title (handled by the tabs)
    
    // Tabbed component
    auto tabbedArea = bounds.removeFromTop(bounds.getHeight() - 50);
    tabbedComponent.setBounds(tabbedArea);
    
    // Resize components in the basic info tab
    auto basicInfoBounds = basicInfoTab.getLocalBounds().reduced(10);
    
    auto titleRowArea = basicInfoBounds.removeFromTop(30);
    titleLabel.setBounds(titleRowArea.removeFromLeft(120));
    titleEditor.setBounds(titleRowArea);
    
    basicInfoBounds.removeFromTop(10); // Spacing
    
    auto tempoRowArea = basicInfoBounds.removeFromTop(30);
    tempoLabel.setBounds(tempoRowArea.removeFromLeft(120));
    tempoEditor.setBounds(tempoRowArea);
    
    // Resize components in the structure tab
    auto structureBounds = structureTab.getLocalBounds().reduced(10);
    
    // Position the label at the top
    structureLabel.setBounds(structureBounds.removeFromTop(30));
    
    // Position the help label at the bottom
    auto helpLabelArea = structureBounds.removeFromBottom(30);
    structureHelpLabel.setBounds(helpLabelArea);
    
    // The text editor takes up the rest of the space
    structureEditor.setBounds(structureBounds);
    
    // Resize components in the import tab
    if (imageImportComponent != nullptr)
    {
        imageImportComponent->setBounds(importTab.getLocalBounds().reduced(10));
    }
    
    // Position the common buttons
    auto commonButtonArea = bounds.removeFromBottom(40);
    int commonButtonWidth = (commonButtonArea.getWidth() - 20) / 3;
    
    saveButton.setBounds(commonButtonArea.removeFromLeft(commonButtonWidth).reduced(5, 0));
    cancelButton.setBounds(commonButtonArea.removeFromLeft(commonButtonWidth).reduced(5, 0));
    importImageButton.setBounds(commonButtonArea.removeFromLeft(commonButtonWidth).reduced(5, 0));
}

void SongEditorComponent::buttonClicked(juce::Button* button)
{
    if (button == &saveButton)
    {
        // Save changes to the song
        auto oldSong = currentSong ? currentSong->clone() : nullptr;
        applySongChanges();
        
        // Add to undo manager
        undoManager.beginNewTransaction();
        undoManager.perform(new SongEditAction(*this, oldSong, currentSong));
        
        // Notify listeners
        listeners.call(&Listener::songEdited, this, currentSong);
    }
    else if (button == &cancelButton)
    {
        listeners.call(&Listener::editingCancelled, this);
    }
    else if (button == &importImageButton)
    {
        showImageImportDialog();
    }
}

void SongEditorComponent::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    // For single-line editors, treat return as a commit
    if (&editor == &titleEditor || &editor == &tempoEditor)
    {
        // Move focus to the next field
        if (&editor == &titleEditor)
        {
            tempoEditor.grabKeyboardFocus();
        }
        else if (&editor == &tempoEditor)
        {
            structureEditor.grabKeyboardFocus();
        }
    }
}

void SongEditorComponent::textEditorFocusLost(juce::TextEditor& editor)
{
    // Update the song when focus is lost
    if (currentSong)
    {
        if (&editor == &titleEditor)
        {
            currentSong->setTitle(titleEditor.getText());
        }
        else if (&editor == &tempoEditor)
        {
            currentSong->setTempo(tempoEditor.getText().getDoubleValue());
        }
    }
}

// ApplicationCommandTarget implementation
juce::ApplicationCommandTarget* SongEditorComponent::getNextCommandTarget()
{
    return nullptr;
}

void SongEditorComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    // Add our commands to the list
    commands.add(undoCmd);
    commands.add(redoCmd);
    commands.add(importImageCmd);
}

void SongEditorComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case undoCmd:
            result.setInfo("Undo", "Undo the last action", "Editing", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            result.setActive(undoManager.canUndo());
            break;
            
        case redoCmd:
            result.setInfo("Redo", "Redo the last undone action", "Editing", 0);
            result.addDefaultKeypress('y', juce::ModifierKeys::commandModifier);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            result.setActive(undoManager.canRedo());
            break;
            
        case importImageCmd:
            result.setInfo("Import from Image", "Import song structure from an image", "Import", 0);
            result.addDefaultKeypress('i', juce::ModifierKeys::commandModifier);
            result.setActive(true);
            break;
            
        default:
            break;
    }
}

bool SongEditorComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& info)
{
    switch (info.commandID)
    {
        case undoCmd:
            if (undoManager.canUndo())
            {
                undoManager.undo();
                return true;
            }
            break;
            
        case redoCmd:
            if (undoManager.canRedo())
            {
                undoManager.redo();
                return true;
            }
            break;
            
        case importImageCmd:
            showImageImportDialog();
            return true;
            
        default:
            break;
    }
    
    return false;
}

// Handle extracted song structure
void SongEditorComponent::handleExtractedStructure(const juce::var& extractedStructure)
{
    // Store the JSON as a string
    structureJson = juce::JSON::toString(extractedStructure, true);
    
    // Process the extracted song structure
    processSongStructureFromJson(extractedStructure);
}

void SongEditorComponent::showImageImportDialog()
{
    // Switch to the import tab
    tabbedComponent.setCurrentTabIndex(2);
}

// cloneSong method removed - now using Song::clone() backend method

void SongEditorComponent::processSongStructureFromJson(const juce::var& json)
{
    // Create a backup of the current song for undo
    auto oldSong = currentSong ? currentSong->clone() : nullptr;
    
    // Create a new song from the extracted structure
    auto newSong = std::make_shared<Song>();
    
    // Use backend method to process extracted structure
    juce::Result result = newSong->processExtractedStructure(json);
    
    if (result.failed())
    {
        // Show error message from backend processing
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                            "Error",
                                            result.getErrorMessage());
        return;
    }
    
    // Switch to the structure tab
    tabbedComponent.setCurrentTabIndex(1);
    
    // Update the current song
    currentSong = newSong;
    
    // Update the editor fields from the processed song
    titleEditor.setText(currentSong->getTitle());
    tempoEditor.setText(juce::String(currentSong->getTempo()));
    structureEditor.setText(currentSong->getSourceStructureJson());
    
    // Store for UI state
    structureJson = currentSong->getSourceStructureJson();
    
    // Add to undo stack
    undoManager.beginNewTransaction();
    undoManager.perform(new SongEditAction(*this, oldSong, currentSong));
    
    // Notify listeners that the song has changed
    listeners.call(&Listener::songEdited, this, currentSong);
}

void SongEditorComponent::setSong(std::shared_ptr<Song> song)
{
    // Store the current song for undo
    auto oldSong = currentSong ? currentSong->clone() : nullptr;
    
    // Update the current song
    currentSong = song;
    
    // Add to undo manager
    if (oldSong)
    {
        undoManager.beginNewTransaction();
        undoManager.perform(new SongEditAction(*this, oldSong, currentSong));
    }
    
    // Update the UI
    updateEditorFields();
}

std::shared_ptr<Song> SongEditorComponent::getSong() const
{
    return currentSong;
}

void SongEditorComponent::setLLMClient(std::shared_ptr<LLMClient> client)
{
    llmClient = client;
    
    // Update the image import component if it exists
    if (imageImportComponent)
    {
        imageImportComponent->setLLMClient(llmClient);
    }
}

void SongEditorComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void SongEditorComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void SongEditorComponent::updateEditorFields()
{
    if (currentSong)
    {
        titleEditor.setText(currentSong->getTitle());
        tempoEditor.setText(juce::String(currentSong->getTempo()));
        
        // First check if the Song object has a saved structure JSON
        juce::String savedStructureJson = currentSong->getSourceStructureJson();
        
        // If the Song has saved structure JSON, use it
        if (savedStructureJson.isNotEmpty())
        {
            structureEditor.setText(savedStructureJson);
            structureJson = savedStructureJson; // Update our local copy too
        }
        // Otherwise, if we have stored JSON structure in the component, use it
        else if (structureJson.isNotEmpty())
        {
            structureEditor.setText(structureJson);
        }

        
        // Enable UI elements
        titleEditor.setEnabled(true);
        tempoEditor.setEnabled(true);
        structureEditor.setEnabled(true);
        saveButton.setEnabled(true);
    }
    else
    {
        // Clear fields
        titleEditor.setText("");
        tempoEditor.setText("");
        structureEditor.setText("");
        
        // Disable UI elements until a song is loaded or created
        titleEditor.setEnabled(false);
        tempoEditor.setEnabled(false);
        structureEditor.setEnabled(false);
        saveButton.setEnabled(false);
    }
}

void SongEditorComponent::applySongChanges()
{
    if (!currentSong)
    {
        currentSong = std::make_shared<Song>();
    }
    
    // Use backend method to update from UI data
    juce::Result result = currentSong->updateFromUIData(
        titleEditor.getText(),
        tempoEditor.getText().getDoubleValue(),
        structureEditor.getText()
    );
    
    if (result.failed())
    {
        // Show error message from backend validation
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                             "Error",
                                             result.getErrorMessage());
        return;
    }
    
    // Store the JSON text in our component for UI state
    structureJson = structureEditor.getText();
    
    // Log what we're saving
    DBG("Saving song with source structure JSON: " + structureJson.substring(0, 100) + "...");
}

} // namespace covertracks
