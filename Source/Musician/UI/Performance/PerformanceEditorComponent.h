#pragma once

#include <JuceHeader.h>
#include "../../Core/Performance.h"
#include "../../Core/PerformanceManager.h"
#include "../../Core/EnsembleManager.h"
#include "../../Core/Song.h"
#include "../../../Player/Core/PlayerManager.h"
#include "../../../LLM/Core/LLMClient.h"

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
                                  public ComboBox::Listener
{
public:
    /**
     * @brief Construct a new Performance Editor Component
     * 
     * @param performanceManager The performance manager to use
     * @param llmClient The LLM client for chart generation
     */
    PerformanceEditorComponent(PerformanceManager* performanceManager,
                              std::shared_ptr<LLMClient> llmClient);
    
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
    std::unique_ptr<EnsembleManager> ensembleManager;
    
    // Current state
    Performance* currentPerformance = nullptr;
    bool isNewPerformance = false;
    std::function<void()> backCallback;
    
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
    };
    
    std::unique_ptr<PlayerTable> playerTable;
    
    // UI components - Coordination
    Label coordinationLabel;
    TextButton loadAllButton;
    TextButton playAllButton;
    TextButton stopAllButton;
    
    // UI components - Actions
    TextButton saveButton;
    TextButton backButton;
    
    // Status display
    TextEditor statusDisplay;
    
    // Helper methods
    void createNewPerformance();
    void updateUI();
    void updateTitle();
    void saveCurrentPerformance();
    
    // Player actions
    void generateChartForPlayer(int playerIndex);
    void loadChartForPlayer(int playerIndex);
    void playPlayer(int playerIndex);
    
    // Coordination actions
    void loadAllCharts();
    void playAllPlayers();
    void stopAllPlayers();
    
    // Song selection
    void updateSongSelector();
    void songSelectionChanged();
    
    // Ensemble selection
    void updateEnsembleSelector();
    void ensembleSelectionChanged();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceEditorComponent)
};

} // namespace covertracks