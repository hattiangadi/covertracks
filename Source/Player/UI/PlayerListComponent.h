// PlayerListComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"

class PlayerListComponent : public Component,
                            public TableListBoxModel,
                            public Button::Listener,
                            public TableHeaderComponent::Listener
{
public:
    PlayerListComponent(PlayerManager& pm);
    ~PlayerListComponent() override;
    
    void resized() override;
    void paint(Graphics& g) override;
    
    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    
    // Button::Listener overrides
    void buttonClicked(Button* button) override;
    
    // TableHeaderComponent::Listener overrides
    void tableColumnsChanged(TableHeaderComponent*) override {}
    void tableColumnsResized(TableHeaderComponent*) override {}
    void tableSortOrderChanged(TableHeaderComponent*) override { updateButtonStates(); }
    
    // TableListBoxModel override for cell clicks
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    
    // Get the currently selected player
    PlayerManager::Player* getSelectedPlayer() const;
    
    // Refresh the list content
    void refreshList();
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void playerSelected(PlayerManager::Player* player) = 0;
        virtual void newPlayerRequested() = 0;
        virtual void editPlayerRequested(PlayerManager::Player* player) = 0;
        virtual void deletePlayerRequested(PlayerManager::Player* player) = 0;
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
private:
    enum ColumnIds
    {
        NameColumn = 1,
        CategoryColumn,
        DescriptionColumn
    };
    
    PlayerManager& playerManager;
    TableListBox table;
    
    TextButton newButton {"New Player"};
    TextButton editButton {"Edit"};
    TextButton deleteButton {"Delete"};
    
    Array<PlayerManager::Player*> displayedPlayers;
    int sortColumnId = NameColumn;
    bool sortForwards = true;
    
    ListenerList<Listener> listeners;
    
    void updateButtonStates();
    void sortPlayers();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerListComponent)
};
