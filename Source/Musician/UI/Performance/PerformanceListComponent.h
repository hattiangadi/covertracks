#pragma once

#include <JuceHeader.h>
#include "../../Core/PerformanceManager.h"
#include "../../Core/Performance.h"

namespace covertracks {

/**
 * @brief Component for displaying and managing a list of performances
 */
class PerformanceListComponent : public juce::Component,
                                public juce::TableListBoxModel,
                                public juce::Button::Listener
{
public:
    /**
     * @brief Construct a new Performance List Component
     * 
     * @param performanceManager The performance manager to use
     */
    PerformanceListComponent(std::shared_ptr<PerformanceManager> performanceManager);
    
    /**
     * @brief Destroy the Performance List Component
     */
    ~PerformanceListComponent() override;
    
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    
    // Button::Listener overrides
    void buttonClicked(juce::Button* button) override;
    
    // TableListBoxModel override for cell clicks
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& event) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& event) override;
    
    // Get the currently selected performance
    std::shared_ptr<Performance> getSelectedPerformance() const;
    
    // Refresh the list
    void refresh();
    
    // Listener interface for receiving events from the PerformanceListComponent
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void performanceSelected(std::shared_ptr<Performance> performance) = 0;
        virtual void newPerformanceRequested() = 0;
        virtual void editPerformanceRequested(std::shared_ptr<Performance> performance) = 0;
        virtual void deletePerformanceRequested(std::shared_ptr<Performance> performance) = 0;
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
private:
    std::shared_ptr<PerformanceManager> performanceManager;
    juce::TableListBox performanceTable;
    juce::TextButton newButton;
    juce::TextButton editButton;
    juce::TextButton deleteButton;
    
    juce::Array<std::shared_ptr<Performance>> displayedPerformances;
    
    juce::ListenerList<Listener> listeners;
    
    enum ColumnIds
    {
        NameColumn = 1,
        EnsembleColumn,
        SongColumn,
        GenreColumn,
        MoodColumn
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceListComponent)
};

} // namespace covertracks
