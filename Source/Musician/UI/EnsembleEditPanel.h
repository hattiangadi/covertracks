#pragma once

#include <JuceHeader.h>
#include "../Core/EnsembleManager.h"
#include "../../Player/Core/PlayerManager.h"

namespace covertracks {

/**
 * @brief UI panel for editing a specific ensemble of musicians
 */
class EnsembleEditPanel : public juce::Component,
                         public juce::Button::Listener,
                         public juce::ComboBox::Listener
{
public:
    /**
     * @brief Construct a new Ensemble Edit Panel
     * 
     * @param playerManager The player manager to use
     * @param ensembleManager The ensemble manager to edit (nullptr to create a new one)
     */
    EnsembleEditPanel(PlayerManager* playerManager, EnsembleManager* ensembleManager = nullptr);
    
    /**
     * @brief Destroy the Ensemble Edit Panel
     */
    ~EnsembleEditPanel() override;
    
    /**
     * @brief Set the ensemble to edit
     * 
     * @param ensembleManager The ensemble manager to edit
     */
    void setEnsemble(EnsembleManager* ensembleManager);
    
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
    
    /**
     * @brief Handle combo box changes
     * 
     * @param comboBox The combo box that changed
     */
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    
private:
    /**
     * @brief Update the UI based on the current state
     */
    void updateUI();
    
    /**
     * @brief Update the player combo box with available players
     */
    void updatePlayerComboBox();
    
    /**
     * @brief Add a new musician role to the ensemble
     */
    void addMusicianRole();
    
    /**
     * @brief Remove the selected musician role from the ensemble
     */
    void removeMusicianRole();
    
    /**
     * @brief Save the current ensemble data
     */
    void saveEnsembleData();
    
    // UI Components
    juce::Label titleLabel;
    juce::TextEditor nameEditor;
    juce::Label nameLabel;
    juce::TextEditor descriptionEditor;
    juce::Label descriptionLabel;
    
    juce::Label rolesLabel;
    juce::TableListBox rolesTable;
    juce::TextButton addRoleButton;
    juce::TextButton removeRoleButton;
    
    juce::ComboBox roleNameComboBox;
    juce::ComboBox instrumentComboBox;
    juce::ComboBox playerComboBox;
    
    juce::TextButton saveButton;
    
    // Data model
    class RolesTableModel : public juce::TableListBoxModel
    {
    public:
        RolesTableModel(EnsembleManager* manager, EnsembleEditPanel* panel);
        
        int getNumRows() override;
        void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
        void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& event) override;
        
    private:
        EnsembleManager* ensembleManager;
        EnsembleEditPanel* parentPanel;
    };
    
    std::unique_ptr<RolesTableModel> rolesTableModel;
    
    // Managers
    PlayerManager* playerManager;
    EnsembleManager* ensembleManager; // Reference to the ensemble being edited
    std::unique_ptr<EnsembleManager> ownedEnsembleManager; // Owned ensemble if we created it
    
    // State
    std::function<void()> saveCallback; // Called when the ensemble is saved
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnsembleEditPanel)
};

} // namespace covertracks
