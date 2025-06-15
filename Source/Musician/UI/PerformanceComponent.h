#pragma once

#include <JuceHeader.h>
#include "../Core/EnsembleManager.h"
#include "../../Player/Core/PlayerManager.h"
#include "../Core/MusicPatternComponents.h"
#include "../Core/Song.h"
#include "Song/SongListComponent.h"
#include "../Core/Performance.h"
#include "../Core/PerformanceManager.h"
#include "../Core/MusicPatternSystem.h"
#include "../Core/PatternParser.h"
#include "PatternEditorComponent.h"

namespace covertracks {

/**
 * @brief Component for managing musical performances
 * 
 * This component allows creating, editing, and playing performances.
 * A performance consists of a song, an ensemble, and performance instructions.
 * The LLM generates patterns for each musician in the ensemble based on the instructions.
 */
class PerformanceComponent : public juce::Component,
                            public juce::Button::Listener,
                            public juce::TextEditor::Listener,
                            public juce::ComboBox::Listener,
                            public SongListComponent::Listener,
                            private juce::Timer,
                            public PatternEditorComponent::Listener
{
public:
    /**
     * @brief Construct a new Performance Component
     * 
     * @param ensembleManager The ensemble manager to use
     * @param playerManager The player manager to use for proper architecture flow
     */
    PerformanceComponent(EnsembleManager* ensembleManager, PlayerManager* playerManager);
    
    /**
     * @brief Destroy the Performance Component
     */
    ~PerformanceComponent() override;
    
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
    
    // PatternEditorComponent::Listener
    void patternPlayRequested(PatternEditorComponent* editor) override;
    void patternParseRequested(PatternEditorComponent* editor) override;
    void patternLoadRequested(PatternEditorComponent* editor) override;
    void performancePatternSaveRequested(PatternEditorComponent* editor) override;
    void performancePatternCancelRequested(PatternEditorComponent* editor) override;
    
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
     * @brief Listener interface for performance events
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /**
         * @brief Called when a performance is saved
         * 
         * @param source The PerformanceComponent that triggered the event
         * @param performance The saved performance
         */
        virtual void performanceSaved(PerformanceComponent* source, std::shared_ptr<Performance> performance) = 0;
        
        /**
         * @brief Called when performance playback starts
         * 
         * @param source The PerformanceComponent that triggered the event
         */
        virtual void performancePlaybackStarted(PerformanceComponent* /*source*/) {}
        
        /**
         * @brief Called when performance playback stops
         * 
         * @param source The PerformanceComponent that triggered the event
         */
        virtual void performancePlaybackStopped(PerformanceComponent* /*source*/) {}
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
    juce::Label performanceLabel;
    juce::ComboBox performanceSelector;
    juce::TextButton newPerformanceButton;
    juce::TextButton savePerformanceButton;
    
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
    
    juce::Label descriptionLabel;
    juce::TextEditor descriptionEditor;
    
    juce::Label instructionsLabel;
    juce::TextEditor instructionsEditor;
    
    juce::TextButton generateButton;
    juce::TextButton prepAllButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    
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
    
    // Pattern editor for editing LLM patterns
    std::unique_ptr<PatternEditorComponent> patternEditor;
    
    // References
    EnsembleManager* ensembleManager;
    PlayerManager* playerManager;
    std::shared_ptr<PerformanceManager> performanceManager;
    SongListComponent* registeredSongList = nullptr;
    
    // State
    std::shared_ptr<Song> currentSong;
    std::shared_ptr<Performance> currentPerformance;
    juce::String currentInstructions;
    bool isGenerating = false;
    
    // Pattern editing mode
    bool isInPatternEditMode = false;
    juce::String editingRoleName;
    Player* editingPlayer = nullptr;
    
    // Performance manager for saving/loading performances
    juce::Array<std::shared_ptr<Performance>> loadedPerformances;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    // Helper methods
    void updatePerformanceSelector();
    void updateEnsembleSelector();
    void updateSongSelector();
    void updateMusicianStatusTable();
    void startGeneration();
    void startGenerationForRole(const MusicianRole& role);
    void prepAllPatterns();
    void prepPatternForRole(const MusicianRole& role);
    void editPatternForRole(const MusicianRole& role);
    void loadPatternForRole(const MusicianRole& role);
    void enterPatternEditMode(const juce::String& roleName, Player* player, const juce::String& llmPattern);
    void exitPatternEditMode(bool saveChanges);
    
    // Playback methods
    void playPerformance();
    void stopPerformance();
    bool isPlaying() const;
    void cancelGeneration();
    void playGeneratedPatterns();
    void stopPlayback();
    void saveCurrentPerformance();
    void createNewPerformance();
    void loadPerformance(std::shared_ptr<Performance> performance);
    
    
    // Convert a note name to a MIDI note number
    int getMidiNoteFromName(const juce::String& name);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceComponent)
};

} // namespace covertracks
