#include "PerformanceLibraryComponent.h"

namespace covertracks {

PerformanceLibraryComponent::PerformanceLibraryComponent(PerformanceManager* pm, 
                                                       std::shared_ptr<LLMClient> client,
                                                       PlayerManager* pm2)
    : performanceManager(pm), llmClient(client), playerManager(pm2)
{
    // Create the list component
    listComponent = std::make_unique<PerformanceListComponent>(performanceManager);
    listComponent->addListener(this);
    addAndMakeVisible(*listComponent);
    
    // Editor component will be created when needed
    
    // Start in list mode
    showList();
}

PerformanceLibraryComponent::~PerformanceLibraryComponent()
{
    if (listComponent)
        listComponent->removeListener(this);
}

void PerformanceLibraryComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void PerformanceLibraryComponent::resized()
{
    auto bounds = getLocalBounds();
    
    if (listComponent && listComponent->isVisible())
        listComponent->setBounds(bounds);
    
    if (editorComponent && editorComponent->isVisible())
        editorComponent->setBounds(bounds);
}

void PerformanceLibraryComponent::performanceSelected(Performance* performance)
{
    currentPerformance = performance;
    // For now, just store the selection. Could show preview later.
}

void PerformanceLibraryComponent::newPerformanceRequested()
{
    showEditor(nullptr);  // nullptr means create new performance
}

void PerformanceLibraryComponent::editPerformanceRequested(Performance* performance)
{
    showEditor(performance);
}

void PerformanceLibraryComponent::deletePerformanceRequested(Performance* performance)
{
    if (performance && performanceManager)
    {
        // Show confirmation dialog
        int result = AlertWindow::showYesNoCancelBox(
            AlertWindow::QuestionIcon,
            "Delete Performance",
            "Are you sure you want to delete performance '" + performance->getName() + "'?",
            "Delete", "Cancel", String(), nullptr, nullptr
        );
        
        if (result == 1)  // Yes
        {
            performanceManager->removePerformance(performance->getUniqueID());
            
            // Refresh the list
            if (listComponent)
                listComponent->refreshList();
        }
    }
}

void PerformanceLibraryComponent::showList()
{
    currentMode = ViewMode::List;
    currentPerformance = nullptr;
    
    // Hide editor if it exists
    if (editorComponent)
        editorComponent->setVisible(false);
    
    // Show list
    if (listComponent)
    {
        listComponent->setVisible(true);
        listComponent->refreshList();
    }
    
    updateVisibility();
}

void PerformanceLibraryComponent::showEditor(Performance* performance)
{
    currentMode = ViewMode::Editor;
    currentPerformance = performance;
    
    // Create editor if it doesn't exist
    if (!editorComponent)
    {
        editorComponent = std::make_unique<PerformanceEditorComponent>(performanceManager, llmClient, playerManager);
        addAndMakeVisible(*editorComponent);
        
        // Add callback to return to list
        editorComponent->setBackCallback([this]() {
            showList();
        });
    }
    
    // Load the performance into the editor
    editorComponent->loadPerformance(performance);
    
    // Hide list
    if (listComponent)
        listComponent->setVisible(false);
    
    // Show editor
    editorComponent->setVisible(true);
    
    updateVisibility();
}

void PerformanceLibraryComponent::updateVisibility()
{
    resized();  // Update layout
}

} // namespace covertracks