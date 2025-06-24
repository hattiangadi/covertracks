#pragma once

#include <JuceHeader.h>
#include <set>
#include "../../Core/Performance.h"
#include "../../Core/PerformanceManager.h"
#include "../../Core/EnsembleManager.h"
#include "../../Core/Song.h"
#include "../../../Player/Core/PlayerManager.h"
#include "../../../Player/Core/TracktionPlayer.h"
#include "../../../LLM/Core/LLMClient.h"
#include "../PatternEditorComponent.h"

namespace covertracks {

/**
 * @brief Component for editing and playing performances
 * 
 * This component allows editing performance details, managing the ensemble,
 * generating charts for each player, and coordinating synchronized playback.
 */
class PerformanceEditorComponent : public Component,
                                  public Button::Listener,
                                  public TextEditor::Listener,
                                  public ComboBox::Listener,
                                  public PatternEditorComponent::Listener
{
public:
    /**
     * @brief Construct a new Performance Editor Component
     * 
     * @param performanceManager The performance manager to use
     * @param llmClient The LLM client for chart generation
     * @param playerManager The player manager for loading and playing charts
     */
    PerformanceEditorComponent(PerformanceManager* performanceManager,
                              std::shared_ptr<LLMClient> llmClient,
                              PlayerManager* playerManager = nullptr);
    
    /**
     * @brief Destroy the Performance Editor Component
     */
    ~PerformanceEditorComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // Button::Listener implementation
    void buttonClicked(Button* button) override;
    
    // TextEditor::Listener implementation
    void textEditorReturnKeyPressed(TextEditor& editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;
    void textEditorFocusLost(TextEditor& editor) override;
    
    // ComboBox::Listener implementation
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    
    // PatternEditorComponent::Listener implementation
    void patternPlayRequested(PatternEditorComponent* editor) override;
    void patternParseRequested(PatternEditorComponent* editor) override;
    void patternLoadRequested(PatternEditorComponent* editor) override;
    void performancePatternSaveRequested(PatternEditorComponent* editor) override;
    void performancePatternCancelRequested(PatternEditorComponent* editor) override;
    
    /**
     * @brief Load a performance for editing
     * 
     * @param performance The performance to load (nullptr for new performance)
     */
    void loadPerformance(Performance* performance);
    
    /**
     * @brief Set callback for returning to list view
     * 
     * @param callback Function to call when user wants to go back
     */
    void setBackCallback(std::function<void()> callback);

private:
    // Core components
    PerformanceManager* performanceManager;
    std::shared_ptr<LLMClient> llmClient;
    PlayerManager* playerManager;
    std::unique_ptr<EnsembleManager> ensembleManager;
    
    // Current state
    Performance* currentPerformance = nullptr;
    bool isNewPerformance = false;
    std::function<void()> backCallback;
    std::set<String> generatingRoles; // Track which roles are currently generating
    
    // Pattern editing mode
    bool isInPatternEditMode = false;
    String editingRoleName;
    Player* editingPlayer = nullptr;
    
    // Temporary storage for new performance data
    String pendingEnsembleName;
    std::shared_ptr<Song> pendingSong;
    
    // Cached ensemble roles with proper player assignments
    mutable juce::Array<MusicianRole> cachedEnsembleRoles;
    
    // UI components - Header
    Label titleLabel;
    TextEditor nameEditor;
    TextEditor descriptionEditor;
    
    // UI components - Song Selection
    Label songLabel;
    ComboBox songSelector;
    
    // UI components - Ensemble Setup
    Label ensembleLabel;
    ComboBox ensembleSelector;
    
    // UI components - Style and Character
    Label styleLabel;
    ComboBox styleSelector;
    Label characterLabel;
    TextEditor characterEditor;
    
    // UI components - Player Table
    class PlayerTable : public Component,
                       public TableListBoxModel,
                       public Button::Listener
    {
    public:
        PlayerTable(PerformanceEditorComponent* parent);
        ~PlayerTable() override;
        
        // Component overrides
        void resized() override;
        
        // TableListBoxModel implementation
        int getNumRows() override;
        void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
        Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
        
        // Button::Listener implementation
        void buttonClicked(Button* button) override;
        
        void updateContent();
        
    private:
        PerformanceEditorComponent* parent;
        TableListBox table;
        Array<TextButton*> generateButtons;
        Array<TextButton*> loadButtons;
        Array<TextButton*> playButtons;
        
        void clearButtons();
        Array<MusicianRole> getEnsembleRolesFromName(const String& ensembleName);
    };
    
    std::unique_ptr<PlayerTable> playerTable;
    
    // UI components - Coordination
    Label coordinationLabel;
    TextButton generateAllButton;
    TextButton prepAllButton;
    TextButton loadAllButton;
    TextButton playAllButton;
    TextButton stopAllButton;
    
    // UI components - Actions
    TextButton saveButton;
    TextButton backButton;
    
    // Status display
    TextEditor statusDisplay;
    
    // Pattern editor for editing LLM patterns
    std::unique_ptr<PatternEditorComponent> patternEditor;
    
    // Active TracktionPlayers for individual playback (keeps them alive during playback)
    std::map<juce::String, std::unique_ptr<TracktionPlayer>> activeTracktionPlayers;
    
    // Helper methods
    void createNewPerformance();
    void updateUI();
    void updateTitle();
    void saveCurrentPerformance();
    
    // Player actions
    void generateChartForPlayer(int playerIndex);
    void prepChartForPlayer(int playerIndex);
    void editPatternForPlayer(int playerIndex);
    void loadChartForPlayer(int playerIndex);
    void playPlayer(int playerIndex);
    
    // Coordination actions
    void generateAllCharts();
    void prepAllCharts();
    void loadAllCharts();
    void playAllPlayers();
    void stopAllPlayers();
    
    // Song selection
    void updateSongSelector();
    void songSelectionChanged();
    
    // Ensemble selection
    void updateEnsembleSelector();
    void ensembleSelectionChanged();
    
    // Ensemble caching with PlayerManager
    void refreshPerformanceEnsembleCache();
    juce::Array<MusicianRole> getPerformanceEnsemble() const;
    
    // Pattern editing mode
    void enterPatternEditMode(const String& roleName, Player* player, const String& llmPattern);
    void exitPatternEditMode(bool saveChanges);
    
    // Level control
    void updateRoleLevel(int roleIndex, float level);
    
    // JSON continuation handling
    void requestJsonContinuation(const String& roleName, const String& incompleteJson);
    
    // TracktionPlayer creation
    std::unique_ptr<TracktionPlayer> createTracktionPlayerFromPlayer(Player* player);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceEditorComponent)
};

} // namespace covertracks