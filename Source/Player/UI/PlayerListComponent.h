// PlayerListComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"

namespace covertracks {

class PlayerListComponent : public Component,
                            public TableListBoxModel,
                            public Button::Listener
{
public:
    PlayerListComponent(PlayerManager* pm);
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
    
    // TableListBoxModel override for cell clicks
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    
    // Get the currently selected player
    Player* getSelectedPlayer() const;
    
    // Refresh the list
    void refresh();
    
    // Listener interface for receiving events from the PlayerListComponent
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void playerSelected(Player* player) = 0;
        virtual void newPlayerRequested() = 0;
        virtual void editPlayerRequested(Player* player) = 0;
        virtual void deletePlayerRequested(Player* player) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    PlayerManager* playerManager;
    TableListBox playerTable;
    TextButton newButton;
    TextButton editButton;
    TextButton deleteButton;
    
    Array<Player*> displayedPlayers;
    
    ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerListComponent)
};

} // namespace covertracks
