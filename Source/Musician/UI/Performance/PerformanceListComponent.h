#pragma once

#include <JuceHeader.h>
#include "../../Core/Performance.h"
#include "../../Core/PerformanceManager.h"

namespace covertracks {

/**
 * @brief Component for listing and managing performances
 * 
 * This component shows a list of saved performances and provides
 * buttons for creating, editing, and deleting performances.
 */
class PerformanceListComponent : public Component,
                                public Button::Listener,
                                public TableListBoxModel
{
public:
    /**
     * @brief Listener interface for performance list events
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void performanceSelected(Performance* performance) = 0;
        virtual void newPerformanceRequested() = 0;
        virtual void editPerformanceRequested(Performance* performance) = 0;
        virtual void deletePerformanceRequested(Performance* performance) = 0;
    };
    
    /**
     * @brief Construct a new Performance List Component
     * 
     * @param performanceManager The performance manager to use
     */
    PerformanceListComponent(PerformanceManager* performanceManager);
    
    /**
     * @brief Destroy the Performance List Component
     */
    ~PerformanceListComponent() override;
    
    // Component overrides
    void paint(Graphics& g) override;
    void resized() override;
    
    // Button::Listener implementation
    void buttonClicked(Button* button) override;
    
    // TableListBoxModel implementation
    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const MouseEvent& e) override;
    void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& e) override;
    
    /**
     * @brief Add a listener for performance list events
     * 
     * @param listener The listener to add
     */
    void addListener(Listener* listener);
    
    /**
     * @brief Remove a listener for performance list events
     * 
     * @param listener The listener to remove
     */
    void removeListener(Listener* listener);
    
    /**
     * @brief Refresh the performance list
     */
    void refreshList();
    
    /**
     * @brief Get the currently selected performance
     * 
     * @return Performance* The selected performance, or nullptr if none selected
     */
    Performance* getSelectedPerformance() const;

private:
    // Core components
    PerformanceManager* performanceManager;
    
    // UI components
    Label titleLabel;
    TableListBox performanceTable;
    TextButton newButton;
    TextButton editButton;
    TextButton deleteButton;
    TextButton playButton;
    
    // State
    ListenerList<Listener> listeners;
    Array<Performance*> performances;
    int selectedRow = -1;
    
    // Helper methods
    void updateButtonStates();
    void loadPerformances();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceListComponent)
};

} // namespace covertracks