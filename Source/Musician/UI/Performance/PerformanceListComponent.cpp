#include "PerformanceListComponent.h"

namespace covertracks {

PerformanceListComponent::PerformanceListComponent(std::shared_ptr<PerformanceManager> manager)
    : performanceManager(manager)
{
    // Set up the table
    performanceTable.setModel(this);
    performanceTable.setMultipleSelectionEnabled(false);
    performanceTable.setHeaderHeight(24);
    performanceTable.getHeader().addColumn("Name", NameColumn, 150);
    performanceTable.getHeader().addColumn("Ensemble", EnsembleColumn, 150);
    performanceTable.getHeader().addColumn("Song", SongColumn, 150);
    performanceTable.getHeader().addColumn("Genre", GenreColumn, 100);
    performanceTable.getHeader().addColumn("Mood", MoodColumn, 100);
    performanceTable.getHeader().setSortColumnId(NameColumn, true);
    addAndMakeVisible(performanceTable);
    
    // Set up buttons
    newButton.setButtonText("New");
    newButton.addListener(this);
    addAndMakeVisible(newButton);
    
    editButton.setButtonText("Edit");
    editButton.addListener(this);
    addAndMakeVisible(editButton);
    
    deleteButton.setButtonText("Delete");
    deleteButton.addListener(this);
    addAndMakeVisible(deleteButton);
    
    // Initial refresh
    refresh();
}

PerformanceListComponent::~PerformanceListComponent()
{
    performanceTable.setModel(nullptr);
}

void PerformanceListComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PerformanceListComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    
    // Button area at the bottom
    auto buttonArea = bounds.removeFromBottom(30);
    
    // Arrange buttons
    int buttonWidth = 80;
    int spacing = 10;
    
    newButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    deleteButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    
    // Table takes the rest
    performanceTable.setBounds(bounds);
}

int PerformanceListComponent::getNumRows()
{
    return displayedPerformances.size();
}

void PerformanceListComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    // Suppress unused parameter warnings
    juce::ignoreUnused(width, height);
    
    if (rowIsSelected)
    {
        // Use a consistent blue color for selected rows
        g.fillAll(getLookAndFeel().findColour(juce::TextEditor::highlightColourId));
    }
    else
    {
        // Use alternating colors for unselected rows
        g.fillAll(rowNumber % 2 ? juce::Colour(0xffeeeeee) : getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
}

void PerformanceListComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= displayedPerformances.size())
        return;
        
    auto performance = displayedPerformances[rowNumber];
    if (!performance)
        return;
        
    // Use white text for selected rows, black for unselected
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colours::black);
    g.setFont(14.0f);
    
    juce::String text;
    
    switch (columnId)
    {
        case NameColumn:
            text = performance->getName();
            break;
        case EnsembleColumn:
            text = performance->getEnsembleName();
            break;
        case SongColumn:
            text = performance->getSong() ? performance->getSong()->getTitle() : "No Song";
            break;
        case GenreColumn:
            text = performance->getGenre();
            break;
        case MoodColumn:
            text = performance->getMood();
            break;
        default:
            break;
    }
    
    g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
}

void PerformanceListComponent::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    // Sort the performances based on the column
    if (newSortColumnId == 0)
        return;
        
    std::stable_sort(displayedPerformances.begin(), displayedPerformances.end(),
        [newSortColumnId, isForwards](const std::shared_ptr<Performance>& a, const std::shared_ptr<Performance>& b)
        {
            int result = 0;
            
            switch (newSortColumnId)
            {
                case NameColumn:
                    result = a->getName().compareIgnoreCase(b->getName());
                    break;
                case EnsembleColumn:
                    result = a->getEnsembleName().compareIgnoreCase(b->getEnsembleName());
                    break;
                case SongColumn:
                {
                    juce::String aName = a->getSong() ? a->getSong()->getTitle() : "";
                    juce::String bName = b->getSong() ? b->getSong()->getTitle() : "";
                    result = aName.compareIgnoreCase(bName);
                    break;
                }
                case GenreColumn:
                    result = a->getGenre().compareIgnoreCase(b->getGenre());
                    break;
                case MoodColumn:
                    result = a->getMood().compareIgnoreCase(b->getMood());
                    break;
                default:
                    break;
            }
            
            return isForwards ? (result < 0) : (result > 0);
        });
        
    performanceTable.updateContent();
}

void PerformanceListComponent::buttonClicked(juce::Button* button)
{
    if (button == &newButton)
    {
        listeners.call(&Listener::newPerformanceRequested);
    }
    else if (button == &editButton)
    {
        auto selectedPerformance = getSelectedPerformance();
        if (selectedPerformance)
        {
            listeners.call(&Listener::editPerformanceRequested, selectedPerformance);
        }
    }
    else if (button == &deleteButton)
    {
        auto selectedPerformance = getSelectedPerformance();
        if (selectedPerformance)
        {
            // Confirm deletion
            if (juce::NativeMessageBox::showOkCancelBox(
                juce::MessageBoxIconType::QuestionIcon,
                "Delete Performance",
                "Are you sure you want to delete \"" + selectedPerformance->getName() + "\"?",
                this,
                juce::ModalCallbackFunction::create([this, selectedPerformance](int result) {
                    if (result)
                    {
                        listeners.call(&Listener::deletePerformanceRequested, selectedPerformance);
                    }
                })))
            {
                // Handled in the callback
            }
        }
    }
}

void PerformanceListComponent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& event)
{
    // Suppress unused parameter warnings
    juce::ignoreUnused(columnId, event);
    
    if (rowNumber >= 0 && rowNumber < displayedPerformances.size())
    {
        performanceTable.selectRow(rowNumber);
        auto selectedPerformance = displayedPerformances[rowNumber];
        listeners.call(&Listener::performanceSelected, selectedPerformance);
    }
}

void PerformanceListComponent::cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& event)
{
    // Suppress unused parameter warnings
    juce::ignoreUnused(columnId, event);
    
    if (rowNumber >= 0 && rowNumber < displayedPerformances.size())
    {
        auto selectedPerformance = displayedPerformances[rowNumber];
        listeners.call(&Listener::editPerformanceRequested, selectedPerformance);
    }
}

std::shared_ptr<Performance> PerformanceListComponent::getSelectedPerformance() const
{
    int selectedRow = performanceTable.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < displayedPerformances.size())
    {
        return displayedPerformances[selectedRow];
    }
    
    return nullptr;
}

void PerformanceListComponent::refresh()
{
    displayedPerformances.clear();
    
    if (performanceManager)
    {
        auto performances = performanceManager->loadPerformancesFromDefaultLocation();
        for (auto& performance : performances)
        {
            displayedPerformances.add(performance);
        }
    }
    
    performanceTable.updateContent();
    performanceTable.repaint();
    
    // Enable/disable buttons based on selection
    bool hasSelection = (performanceTable.getSelectedRow() >= 0);
    editButton.setEnabled(hasSelection);
    deleteButton.setEnabled(hasSelection);
}

} // namespace covertracks
