#pragma once

#include <JuceHeader.h>
#include "../../Core/EnsembleManager.h"
#include "../../../LLM/Core/LLMClient.h"
#include "../../Core/MusicPatternComponents.h"
#include "../../Core/Song.h"
#include "../Song/SongListComponent.h"
#include "../../Core/Performance.h"

namespace covertracks {

/**
 * @brief Component for editing a performance
 * 
 * This component allows editing a performance's properties and generating/loading/playing patterns.
 */
class PerformanceEditorComponent : public juce::Component,
                                  public juce::Button::Listener,
                                  public juce::TextEditor::Listener,
                                  public juce::ComboBox::Listener,
                                  public SongListComponent::Listener,
                                  private juce::Timer
{
public:
    /**
     * @brief Construct a new Performance Editor Component
     * 
     * @param ensembleManager The ensemble manager to use
     * @param llmClient The LLM client to use
     */
    PerformanceEditorComponent(EnsembleManager* ensembleManager, std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Destroy the Performance Editor Component
     */
    ~PerformanceEditorComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    
    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    
    // Timer
    void timerCallback() override;
    
    // SongListComponent::Listener
    void songSelectionChanged(SongListComponent* source, std::shared_ptr<Song> song) override;
    void songListChanged(SongListComponent* source) override;
    
    /**
     * @brief Set the current song for performance
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
    
    /**
     * @brief Get the chart generated from the current song
     * 
     * @return std::shared_ptr<Chart> The current chart
     */
    std::shared_ptr<Chart> getCurrentChart() const;
    
    /**
     * @brief Register with a SongListComponent to receive updates
     * 
     * @param songList The SongListComponent to register with
     */
    void registerWithSongList(SongListComponent* songList);
    
    /**
     * @brief Set the performance to edit
     * 
     * @param performance The performance to edit
     */
    void setPerformance(std::shared_ptr<Performance> performance);
    
    /**
     * @brief Get the current performance
     * 
     * @return std::shared_ptr<Performance> The current performance
     */
    std::shared_ptr<Performance> getPerformance() const { return currentPerformance; }
    
    /**
     * @brief Listener interface for performance editor events
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /**
         * @brief Called when a performance is saved
         * 
         * @param source The PerformanceEditorComponent that triggered the event
         * @param performance The saved performance
         */
        virtual void performanceEditorSaved(PerformanceEditorComponent* source, std::shared_ptr<Performance> performance) = 0;
        
        /**
         * @brief Called when the editor is canceled
         * 
         * @param source The PerformanceEditorComponent that triggered the event
         */
        virtual void performanceEditorCanceled(PerformanceEditorComponent* source) = 0;
        
        /**
         * @brief Called when performance playback starts
         * 
         * @param source The PerformanceEditorComponent that triggered the event
         */
        virtual void performancePlaybackStarted(PerformanceEditorComponent* source) {}
        
        /**
         * @brief Called when performance playback stops
         * 
         * @param source The PerformanceEditorComponent that triggered the event
         */
        virtual void performancePlaybackStopped(PerformanceEditorComponent* source) {}
    };
    
    /**
     * @brief Add a listener
     * 
     * @param listener The listener to add
     */
    void addListener(Listener* listener) { listeners.add(listener); }
    
    /**
     * @brief Remove a listener
     * 
     * @param listener The listener to remove
     */
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
private:
    // UI components
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    
    juce::Label ensembleLabel;
    juce::ComboBox ensembleSelector;
    
    juce::Label songLabel;
    juce::ComboBox songSelector;
    
    // Performance attributes
    juce::Label keyLabel;
    juce::ComboBox keySelector;
    
    juce::Label genreLabel;
    juce::ComboBox genreSelector;
    
    juce::Label moodLabel;
    juce::ComboBox moodSelector;
    
    juce::Label instructionsLabel;
    juce::TextEditor instructionsEditor;
    
    // Action buttons with clear workflow
    juce::TextButton generateButton;  // Generate patterns from LLM
    juce::TextButton loadButton;      // Load patterns into players
    juce::TextButton playButton;      // Play loaded patterns
    juce::TextButton stopButton;      // Stop playback
    
    // Save/Cancel buttons
    juce::TextButton saveButton;
    juce::TextButton cancelButton;
    
    juce::Label statusLabel;
    
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
        
        // Get the status message for a musician
        juce::String getStatusMessage(int index) const;
        
        // Get the musician roles
        const juce::Array<MusicianRole>& getMusicianRoles() const { return musicianRoles; }
        
        // Add a generate button for a specific musician
        void addGenerateButton(int rowNumber, juce::Button::Listener* listener);
        
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
    SongListComponent* registeredSongList = nullptr;
    
    // State
    std::shared_ptr<Song> currentSong;
    std::shared_ptr<Performance> currentPerformance;
    juce::String currentInstructions;
    bool isGenerating = false;
    bool patternsGenerated = false;
    bool patternsLoaded = false;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    // Helper methods
    void updateEnsembleSelector();
    void updateSongSelector();
    void updateMusicianStatusTable();
    void updateUIState();
    void startGeneration();
    void startGenerationForRole(const MusicianRole& role);
    
    // Pattern handling methods
    void generatePatterns();
    void loadPatterns();
    void playPatterns();
    void stopPlayback();
    bool isPlaying() const;
    void cancelGeneration();
    
    // Save/cancel methods
    void savePerformance();
    void cancelEditing();
    
    // Generate a prompt for a specific musician
    juce::String generateMusicianPrompt(const MusicianRole& role, 
                                      std::shared_ptr<Song> song,
                                      const juce::String& performanceInstructions);
    
    // Parse a JSON pattern from an LLM response using PatternParser
    std::shared_ptr<Chart> parseJSONPatternFromResponse(const juce::String& response);
    
    // Convert a note name to a MIDI note number
    int getMidiNoteFromName(const juce::String& name);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceEditorComponent)
};

} // namespace covertracks
