#pragma once

#include <JuceHeader.h>
#include "PerformanceListComponent.h"
#include "PerformanceEditorComponent.h"
#include "../../Core/PerformanceManager.h"
#include "../../../LLM/Core/LLMClient.h"

namespace covertracks {

/**
 * @brief Container component for performance management
 * 
 * This component manages navigation between performance list and editor views.
 * It follows the same pattern as PlayerLibraryComponent for consistency.
 */
class PerformanceLibraryComponent : public Component,
                                   public PerformanceListComponent::Listener
{
public:
    /**
     * @brief Construct a new Performance Library Component
     * 
     * @param performanceManager The performance manager to use
     * @param llmClient The LLM client for chart generation
     */
    PerformanceLibraryComponent(PerformanceManager* performanceManager, 
                               std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Destroy the Performance Library Component
     */
    ~PerformanceLibraryComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // PerformanceListComponent::Listener implementation
    void performanceSelected(Performance* performance) override;
    void newPerformanceRequested() override;
    void editPerformanceRequested(Performance* performance) override;
    void deletePerformanceRequested(Performance* performance) override;
    
    /**
     * @brief Show the performance list view
     */
    void showList();
    
    /**
     * @brief Show the performance editor view
     * 
     * @param performance Performance to edit (nullptr for new performance)
     */
    void showEditor(Performance* performance = nullptr);

private:
    // Core components
    PerformanceManager* performanceManager;
    std::shared_ptr<LLMClient> llmClient;
    
    // UI components
    std::unique_ptr<PerformanceListComponent> listComponent;
    std::unique_ptr<PerformanceEditorComponent> editorComponent;
    
    // Current state
    enum class ViewMode {
        List,
        Editor
    };
    
    ViewMode currentMode = ViewMode::List;
    Performance* currentPerformance = nullptr;
    
    // Helper methods
    void updateVisibility();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceLibraryComponent)
};

} // namespace covertracks