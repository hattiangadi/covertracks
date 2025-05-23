#pragma once

#include <JuceHeader.h>
#include "../Core/EnsembleManager.h"
#include "../../LLM/Core/LLMClient.h"
#include "../Core/MusicPatternComponents.h"

namespace covertracks {

/**
 * @brief Component for managing LLM conversations with an ensemble
 * 
 * This component coordinates LLM conversations with multiple musicians in an ensemble,
 * allowing them to generate patterns for a song based on performance instructions.
 */
class EnsembleLLMConversationComponent : public juce::Component,
                                        public juce::Button::Listener,
                                        public juce::TextEditor::Listener,
                                        public juce::ComboBox::Listener,
                                        private juce::Timer
{
public:
    /**
     * @brief Construct a new Ensemble LLM Conversation Component
     * 
     * @param ensembleManager The ensemble manager to use
     * @param llmClient The LLM client to use
     */
    EnsembleLLMConversationComponent(EnsembleManager* ensembleManager, std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Destroy the Ensemble LLM Conversation Component
     */
    ~EnsembleLLMConversationComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    
    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    
    // Timer
    void timerCallback() override;
    
    /**
     * @brief Set the current song for performance instructions
     * 
     * @param song The song to use for performance
     */
    void setCurrentSong(std::shared_ptr<Song> song);
    
    /**
     * @brief Get the current song
     * 
     * @return std::shared_ptr<Song> The current song
     */
    std::shared_ptr<Song> getCurrentSong() const;
    
private:
    // UI components
    juce::Label ensembleLabel;
    juce::ComboBox ensembleSelector;
    
    juce::Label songLabel;
    juce::ComboBox songSelector;
    
    juce::Label instructionsLabel;
    juce::TextEditor instructionsEditor;
    
    juce::TextButton generateButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    
    // Status table for musicians
    class MusicianStatusTable : public juce::Component,
                               public juce::TableListBoxModel
    {
    public:
        MusicianStatusTable();
        ~MusicianStatusTable() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // TableListBoxModel overrides
        int getNumRows() override;
        void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
        juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
        
        // Update the table with musician roles
        void updateMusicianRoles(const juce::Array<MusicianRole>& roles);
        
        // Update the status of a musician
        void updateMusicianStatus(int index, const juce::String& status);
        
        // Clear all statuses
        void clearStatuses();
        
    private:
        juce::TableListBox table;
        juce::Array<MusicianRole> musicianRoles;
        juce::StringArray statuses;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MusicianStatusTable)
    };
    
    MusicianStatusTable musicianStatusTable;
    
    // References
    EnsembleManager* ensembleManager;
    std::shared_ptr<LLMClient> llmClient;
    
    // State
    std::shared_ptr<Song> currentSong;
    juce::String currentInstructions;
    bool isGenerating = false;
    
    // Helper methods
    void updateEnsembleSelector();
    void updateSongSelector();
    void updateMusicianStatusTable();
    void startGeneration();
    void cancelGeneration();
    void playGeneratedPatterns();
    void stopPlayback();
    
    // Generate a prompt for a specific musician
    juce::String generateMusicianPrompt(const MusicianRole& role, 
                                      std::shared_ptr<Song> song,
                                      const juce::String& performanceInstructions);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnsembleLLMConversationComponent)
};

} // namespace covertracks
