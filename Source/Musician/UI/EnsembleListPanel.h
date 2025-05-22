#pragma once

#include <JuceHeader.h>
#include "../Core/EnsembleManager.h"
#include "../../Player/Core/PlayerManager.h"

namespace covertracks {

/**
 * @brief UI panel for managing a list of ensembles
 */
class EnsembleListPanel : public juce::Component,
                         public juce::Button::Listener,
                         public juce::ListBoxModel
{
public:
    /**
     * @brief Construct a new Ensemble List Panel
     * 
     * @param playerManager The player manager to use
     */
    EnsembleListPanel(PlayerManager* playerManager);
    
    /**
     * @brief Destroy the Ensemble List Panel
     */
    ~EnsembleListPanel() override;
    
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
    
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    
    /**
     * @brief Get the selected ensemble
     * 
     * @return EnsembleManager* The selected ensemble (nullptr if none selected)
     */
    EnsembleManager* getSelectedEnsemble();
    
    /**
     * @brief Set the ensemble edit callback
     * 
     * @param callback The callback to invoke when an ensemble is selected for editing
     */
    void setEnsembleEditCallback(std::function<void(EnsembleManager*)> callback);
    
    /**
     * @brief Add a new ensemble
     */
    void addEnsemble();
    
    /**
     * @brief Remove the selected ensemble
     */
    void removeEnsemble();
    
    /**
     * @brief Refresh the ensemble list
     */
    void refreshEnsembleList();
    
    /**
     * @brief Load ensembles from disk
     */
    void loadEnsemblesFromDisk();
    
private:
    // UI Components
    juce::Label titleLabel;
    juce::ListBox ensembleListBox;
    juce::TextButton addButton;
    juce::TextButton removeButton;
    juce::TextButton editButton;
    
    // Data
    PlayerManager* playerManager;
    juce::OwnedArray<EnsembleManager> ensembles; // OwnedArray handles ownership of pointers
    std::function<void(EnsembleManager*)> ensembleEditCallback;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnsembleListPanel)
};

} // namespace covertracks
