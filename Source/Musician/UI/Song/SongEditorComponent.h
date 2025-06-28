#pragma once

#include <JuceHeader.h>
#include "../../Core/Song.h"
#include "../../../LLM/Core/LLMClient.h"
#include "SongImageImportComponent.h"

namespace covertracks {

/**
 * @brief Component for editing a song's details and structure
 * 
 * Supports importing song structure from images and provides undo/redo functionality
 */
class SongEditorComponent : public juce::Component,
                           public juce::Button::Listener,
                           public juce::TextEditor::Listener,
                           public juce::ApplicationCommandTarget
{
public:
    /**
     * @brief Construct a new Song Editor Component
     * 
     * @param llmClient The LLM client to use for image analysis
     */
    SongEditorComponent(std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Destroy the Song Editor Component
     */
    ~SongEditorComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    
    /**
     * @brief Set the song to edit
     * 
     * @param song The song to edit
     */
    void setSong(std::shared_ptr<Song> song);
    
    /**
     * @brief Get the edited song
     * 
     * @return std::shared_ptr<Song> The edited song
     */
    std::shared_ptr<Song> getSong() const;
    
    /**
     * @brief Set the LLM client to use for image analysis
     * 
     * @param client The LLM client
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
    /**
     * @brief Listener interface for song edit events
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void songEdited(SongEditorComponent* source, std::shared_ptr<Song> song) = 0;
        virtual void editingCancelled(SongEditorComponent* source) = 0;
    };
    
    /**
     * @brief Add a listener
     * 
     * @param listener The listener to add
     */
    void addListener(Listener* listener);
    
    /**
     * @brief Remove a listener
     * 
     * @param listener The listener to remove
     */
    void removeListener(Listener* listener);
    
    // ApplicationCommandTarget implementation
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;
    
    // Handle extracted song structure
    void handleExtractedStructure(const juce::var& extractedStructure);
    
    // Command IDs for undo/redo
    enum CommandIDs
    {
        undoCmd = 0x2001,
        redoCmd = 0x2002,
        importImageCmd = 0x2003
    };
    
private:
    // UI components
    juce::TabbedComponent tabbedComponent;
    
    // Basic info tab
    juce::Component basicInfoTab;
    juce::Label titleLabel;
    juce::TextEditor titleEditor;
    juce::Label tempoLabel;
    juce::TextEditor tempoEditor;
    
    // Structure tab
    juce::Component structureTab;
    juce::Label structureLabel;
    juce::TextEditor structureEditor;
    juce::Label structureHelpLabel;
    
    // Import tab
    juce::Component importTab;
    std::unique_ptr<SongImageImportComponent> imageImportComponent;
    
    // Common buttons
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
    juce::TextButton importImageButton;
    
    // Data
    std::shared_ptr<Song> currentSong;
    
    // Store the source structure JSON string (from Claude/sheet music analysis)
    juce::String structureJson;
    std::shared_ptr<LLMClient> llmClient;
    
    // Undo/redo support
    juce::UndoManager undoManager;
    
    // Command manager
    std::unique_ptr<juce::ApplicationCommandManager> commandManager;
    
    // No longer need section table model
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    // Helper methods
    void updateEditorFields();
    void applySongChanges();
    void setupTabs();
    void setupBasicInfoTab();
    void setupStructureTab();
    void setupImportTab();
    void showImageImportDialog();
    void processSongStructureFromJson(const juce::var& json);
    
    // Helper method removed - now using Song::clone() backend method
    
    // Undo/redo actions
    class SongEditAction : public juce::UndoableAction
    {
    public:
        SongEditAction(SongEditorComponent& owner, std::shared_ptr<Song> oldSong, std::shared_ptr<Song> newSong);
        bool perform() override;
        bool undo() override;
        
    private:
        SongEditorComponent& owner;
        std::shared_ptr<Song> oldSong;
        std::shared_ptr<Song> newSong;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongEditorComponent)
};

} // namespace covertracks
