#include "PerformanceListComponent.h"

namespace covertracks {

PerformanceListComponent::PerformanceListComponent(PerformanceManager* pm)
    : performanceManager(pm),
      titleLabel("titleLabel", "Performances"),
      newButton("New"),
      editButton("Edit"),
      deleteButton("Delete"),
      playButton("Play")
{
    // Set up title label
    titleLabel.setFont(Font(FontOptions().withHeight(18.0f).withStyle("bold")));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up table
    performanceTable.setModel(this);
    performanceTable.getHeader().addColumn("Performance", 1, 150);
    performanceTable.getHeader().addColumn("Song", 2, 150);
    performanceTable.getHeader().addColumn("Ensemble", 3, 150);
    performanceTable.setMultipleSelectionEnabled(false);
    addAndMakeVisible(performanceTable);
    
    // Set up buttons
    newButton.addListener(this);
    addAndMakeVisible(newButton);
    
    editButton.addListener(this);
    editButton.setEnabled(false);
    addAndMakeVisible(editButton);
    
    deleteButton.addListener(this);
    deleteButton.setEnabled(false);
    addAndMakeVisible(deleteButton);
    
    playButton.addListener(this);
    playButton.setEnabled(false);
    addAndMakeVisible(playButton);
    
    // Load performances
    loadPerformances();
}

PerformanceListComponent::~PerformanceListComponent()
{
    performanceTable.setModel(nullptr);
    
    newButton.removeListener(this);
    editButton.removeListener(this);
    deleteButton.removeListener(this);
    playButton.removeListener(this);
}

void PerformanceListComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw a border around the table
    g.setColour(Colours::grey);
    g.drawRect(performanceTable.getBounds(), 1);
}

void PerformanceListComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title at the top
    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    
    // Buttons at the bottom
    auto buttonArea = bounds.removeFromBottom(30);
    int buttonWidth = buttonArea.getWidth() / 4;
    
    newButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    deleteButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    playButton.setBounds(buttonArea.reduced(2));
    
    bounds.removeFromBottom(10);
    
    // Table takes remaining space
    performanceTable.setBounds(bounds);
}

void PerformanceListComponent::buttonClicked(Button* button)
{
    if (button == &newButton)
    {
        listeners.call(&Listener::newPerformanceRequested);
    }
    else if (button == &editButton)
    {
        auto* selected = getSelectedPerformance();
        if (selected)
            listeners.call(&Listener::editPerformanceRequested, selected);
    }
    else if (button == &deleteButton)
    {
        auto* selected = getSelectedPerformance();
        if (selected)
            listeners.call(&Listener::deletePerformanceRequested, selected);
    }
    else if (button == &playButton)
    {
        auto* selected = getSelectedPerformance();
        if (selected)
            listeners.call(&Listener::editPerformanceRequested, selected);  // Edit mode can also play
    }
}

int PerformanceListComponent::getNumRows()
{
    return performances.size();
}

void PerformanceListComponent::paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(getLookAndFeel().findColour(TextEditor::highlightColourId));
    else
        g.fillAll(getLookAndFeel().findColour(ListBox::backgroundColourId));
}

void PerformanceListComponent::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    if (rowNumber < 0 || rowNumber >= performances.size())
        return;
    
    auto* performance = performances[rowNumber];
    if (!performance)
        return;
    
    g.setColour(getLookAndFeel().findColour(ListBox::textColourId));
    g.setFont(Font(FontOptions().withHeight(14.0f)));
    
    String text;
    if (columnId == 1) // Performance name
    {
        text = performance->getName();
    }
    else if (columnId == 2) // Song name
    {
        if (auto song = performance->getSong())
            text = song->getTitle();
        else
            text = "No Song";
    }
    else if (columnId == 3) // Ensemble name
    {
        String ensembleName = performance->getEnsembleName();
        if (ensembleName.isEmpty())
            text = "No Ensemble";
        else
            text = ensembleName;
    }
    
    g.drawText(text, Rectangle<int>(4, 0, width - 8, height), Justification::centredLeft, true);
}

void PerformanceListComponent::cellClicked(int rowNumber, int /*columnId*/, const MouseEvent& /*e*/)
{
    selectedRow = rowNumber;
    updateButtonStates();
    
    auto* selected = getSelectedPerformance();
    if (selected)
        listeners.call(&Listener::performanceSelected, selected);
}

void PerformanceListComponent::cellDoubleClicked(int rowNumber, int /*columnId*/, const MouseEvent& /*e*/)
{
    selectedRow = rowNumber;
    auto* selected = getSelectedPerformance();
    if (selected)
        listeners.call(&Listener::editPerformanceRequested, selected);
}

void PerformanceListComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PerformanceListComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void PerformanceListComponent::refreshList()
{
    loadPerformances();
    performanceTable.updateContent();
    updateButtonStates();
}

Performance* PerformanceListComponent::getSelectedPerformance() const
{
    if (selectedRow >= 0 && selectedRow < performances.size())
        return performances[selectedRow];
    return nullptr;
}

void PerformanceListComponent::updateButtonStates()
{
    bool hasSelection = getSelectedPerformance() != nullptr;
    
    editButton.setEnabled(hasSelection);
    deleteButton.setEnabled(hasSelection);
    playButton.setEnabled(hasSelection);
}

void PerformanceListComponent::loadPerformances()
{
    performances.clear();
    selectedRow = -1;
    
    if (performanceManager)
    {
        // Get all performances from the manager
        auto allPerformances = performanceManager->getAllPerformances();
        for (auto* performance : allPerformances)
        {
            performances.add(performance);
        }
    }
}

} // namespace covertracks