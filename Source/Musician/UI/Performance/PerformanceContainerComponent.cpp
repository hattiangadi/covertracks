#include "PerformanceContainerComponent.h"

namespace covertracks {

PerformanceContainerComponent::PerformanceContainerComponent(EnsembleManager* ensembleManager, 
                                                           std::shared_ptr<LLMClient> llmClient)
{
    // Create the performance list component
    auto performanceManager = std::make_shared<PerformanceManager>();
    listComponent = std::make_unique<PerformanceListComponent>(performanceManager);
    listComponent->addListener(this);
    addAndMakeVisible(listComponent.get());
    
    // Create the performance editor component
    editorComponent = std::make_unique<PerformanceComponent>(ensembleManager, llmClient);
    editorComponent->addListener(this);
    addAndMakeVisible(editorComponent.get());
    
    // Start with the list view
    showListView();
}

PerformanceContainerComponent::~PerformanceContainerComponent()
{
    // Remove listeners to avoid callbacks during destruction
    if (listComponent)
        listComponent->removeListener(this);
    
    if (editorComponent)
        editorComponent->removeListener(this);
}

void PerformanceContainerComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PerformanceContainerComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Both components take up the full space, but only one is visible at a time
    listComponent->setBounds(bounds);
    editorComponent->setBounds(bounds);
}

void PerformanceContainerComponent::performanceSelected(std::shared_ptr<Performance> performance)
{
    // Just select the performance in the list, don't switch to editor view
    // This is a change from the previous behavior
}

void PerformanceContainerComponent::editPerformanceRequested(std::shared_ptr<Performance> performance)
{
    if (editorComponent && performance)
    {
        // Load the performance into the editor
        editorComponent->loadPerformance(performance);
        
        // Switch to the editor view
        showEditorView();
    }
}

void PerformanceContainerComponent::newPerformanceRequested()
{
    if (editorComponent)
    {
        // Create a new performance in the editor
        editorComponent->createNewPerformance();
        
        // Switch to the editor view
        showEditorView();
    }
}

void PerformanceContainerComponent::deletePerformanceRequested(std::shared_ptr<Performance> performance)
{
    // Delete the performance
    if (listComponent && performance)
    {
        // Get the performance manager from the list component
        auto performanceManager = std::make_shared<PerformanceManager>();
        
        // Delete the performance
        if (performanceManager->deletePerformance(performance))
        {
            // Successfully deleted, refresh the list
            refresh();
        }
        else
        {
            // Failed to delete, show error message
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                "Delete Failed",
                                                "Failed to delete performance: " + performance->getName());
        }
    }
}

void PerformanceContainerComponent::performanceSaved(PerformanceComponent* source, std::shared_ptr<Performance> performance)
{
    // Refresh the list to show the updated performance
    refresh();
    
    // Switch back to the list view
    showListView();
}

void PerformanceContainerComponent::performanceCancelled(PerformanceComponent* source)
{
    // User cancelled editing, just switch back to the list view without saving
    showListView();
}

void PerformanceContainerComponent::performancePlaybackStarted(PerformanceComponent* /*source*/)
{
    // Nothing to do here
}

void PerformanceContainerComponent::performancePlaybackStopped(PerformanceComponent* /*source*/)
{
    // Nothing to do here
}

void PerformanceContainerComponent::showListView()
{
    if (listComponent && editorComponent)
    {
        listComponent->setVisible(true);
        editorComponent->setVisible(false);
        showingListView = true;
        
        // Refresh the list to ensure it's up to date
        refresh();
    }
}

void PerformanceContainerComponent::showEditorView()
{
    if (listComponent && editorComponent)
    {
        listComponent->setVisible(false);
        editorComponent->setVisible(true);
        showingListView = false;
    }
}

void PerformanceContainerComponent::refresh()
{
    if (listComponent)
    {
        listComponent->refresh();
    }
}

void PerformanceContainerComponent::songSelectionChanged(SongListComponent* source, std::shared_ptr<Song> song)
{
    // Forward the song selection to the performance editor component
    if (editorComponent && song)
    {
        editorComponent->setCurrentSong(song);
    }
}

void PerformanceContainerComponent::songListChanged(SongListComponent* source)
{
    // Nothing specific to do here, but we need to implement the interface method
}

void PerformanceContainerComponent::registerWithSongList(SongListComponent* songList)
{
    // Register this container as a listener to the song list
    if (songList)
    {
        songList->addListener(this);
        
        // Also register the editor component if needed
        if (editorComponent)
        {
            editorComponent->registerWithSongList(songList);
        }
    }
}

} // namespace covertracks
