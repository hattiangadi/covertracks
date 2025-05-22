#pragma once

#include <JuceHeader.h>
#include "EnsembleListPanel.h"
#include "EnsembleEditPanel.h"

namespace covertracks {

/**
 * @brief Main UI panel for managing ensembles
 * 
 * This panel combines the ensemble list and edit panels into a single component
 */
class EnsembleMainPanel : public juce::Component
{
public:
    /**
     * @brief Construct a new Ensemble Main Panel
     * 
     * @param playerManager The player manager to use
     */
    EnsembleMainPanel(PlayerManager* playerManager);
    
    /**
     * @brief Destroy the Ensemble Main Panel
     */
    ~EnsembleMainPanel() override;
    
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
    
private:
    // UI Components
    juce::StretchableLayoutManager layoutManager;
    juce::StretchableLayoutResizerBar resizerBar;
    
    EnsembleListPanel ensembleListPanel;
    EnsembleEditPanel ensembleEditPanel;
    
    // Callbacks
    void handleEnsembleSelected(EnsembleManager* ensemble);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnsembleMainPanel)
};

} // namespace covertracks
