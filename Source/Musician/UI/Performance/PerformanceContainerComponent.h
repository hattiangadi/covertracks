#pragma once

#include <JuceHeader.h>
#include "PerformanceListComponent.h"
#include "../PerformanceComponent.h"

namespace covertracks {

/**
 * @brief Container component that manages both the performance list and editor views
 * 
 * This component serves as a container for both the PerformanceListComponent and
 * PerformanceComponent, showing only one at a time and handling transitions between them.
 */
class PerformanceContainerComponent : public juce::Component,
                                     public PerformanceListComponent::Listener,
                                     public PerformanceComponent::Listener,
                                     public SongListComponent::Listener
{
public:
    /**
     * @brief Construct a new Performance Container Component
     * 
     * @param ensembleManager The ensemble manager to use
     * @param llmClient The LLM client to use
     */
    PerformanceContainerComponent(EnsembleManager* ensembleManager, 
                                 std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Destroy the Performance Container Component
     */
    ~PerformanceContainerComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // PerformanceListComponent::Listener implementation
    void performanceSelected(std::shared_ptr<Performance> performance) override;
    void editPerformanceRequested(std::shared_ptr<Performance> performance) override;
    void newPerformanceRequested() override;
    void deletePerformanceRequested(std::shared_ptr<Performance> performance) override;
    
    // PerformanceComponent::Listener implementation
    void performanceSaved(PerformanceComponent* source, std::shared_ptr<Performance> performance) override;
    void performancePlaybackStarted(PerformanceComponent* source) override;
    void performancePlaybackStopped(PerformanceComponent* source) override;
    
    // SongListComponent::Listener implementation
    void songSelectionChanged(SongListComponent* source, std::shared_ptr<Song> song) override;
    void songListChanged(SongListComponent* source) override;
    
    /**
     * @brief Register with a song list component to receive updates
     * 
     * @param songList The song list component to register with
     */
    void registerWithSongList(SongListComponent* songList);
    
    /**
     * @brief Show the list view
     */
    void showListView();
    
    /**
     * @brief Show the editor view
     */
    void showEditorView();
    
    /**
     * @brief Refresh the performance list
     */
    void refresh();
    
private:
    std::unique_ptr<PerformanceListComponent> listComponent;
    std::unique_ptr<PerformanceComponent> editorComponent;
    
    // Current view state
    bool showingListView = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceContainerComponent)
};

} // namespace covertracks
