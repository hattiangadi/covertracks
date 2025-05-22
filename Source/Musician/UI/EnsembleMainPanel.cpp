#include "EnsembleMainPanel.h"

namespace covertracks {

EnsembleMainPanel::EnsembleMainPanel(PlayerManager* playerManager)
    : resizerBar(&layoutManager, 1, false),
      ensembleListPanel(playerManager),
      ensembleEditPanel(playerManager)
{
    // Set up the layout manager
    layoutManager.setItemLayout(0, 200, 300, 250); // List panel (min, preferred, max)
    layoutManager.setItemLayout(1, 5, 5, 5);       // Resizer bar (fixed at 5 pixels)
    layoutManager.setItemLayout(2, 300, -1, -1);   // Edit panel (min, preferred = unlimited, max = unlimited)
    
    // Add the components
    addAndMakeVisible(ensembleListPanel);
    addAndMakeVisible(resizerBar);
    addAndMakeVisible(ensembleEditPanel);
    
    // Set up the callback for when an ensemble is selected
    ensembleListPanel.setEnsembleEditCallback([this](EnsembleManager* ensemble) {
        handleEnsembleSelected(ensemble);
    });
    
    // Set initial size
    setSize(1000, 700);
}

EnsembleMainPanel::~EnsembleMainPanel()
{
}

void EnsembleMainPanel::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void EnsembleMainPanel::resized()
{
    auto area = getLocalBounds();
    
    // Create an array of component pointers for the layout manager
    juce::Component* components[] = { &ensembleListPanel, &resizerBar, &ensembleEditPanel };
    
    // Apply the layout
    layoutManager.layOutComponents(components, 3, area.getX(), area.getY(), area.getWidth(), area.getHeight(), false, true);
}

void EnsembleMainPanel::handleEnsembleSelected(EnsembleManager* ensemble)
{
    // Update the edit panel with the selected ensemble
    ensembleEditPanel.setEnsemble(ensemble);
}

} // namespace covertracks
