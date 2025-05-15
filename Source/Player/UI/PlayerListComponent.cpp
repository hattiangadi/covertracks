// PlayerListComponent.cpp
#include "PlayerListComponent.h"

namespace covertracks {

enum TableColumns {
    NameColumn = 1,
    CategoryColumn,
    DescriptionColumn
};

PlayerListComponent::PlayerListComponent(PlayerManager* pm)
    : playerManager(pm),
      playerTable("PlayerTable", this)
{
    // Set up table columns
    playerTable.getHeader().addColumn("Name", NameColumn, 150);
    playerTable.getHeader().addColumn("Category", CategoryColumn, 100);
    playerTable.getHeader().addColumn("Description", DescriptionColumn, 300);
    
    playerTable.setMultipleSelectionEnabled(false);
    playerTable.setClickingTogglesRowSelection(true);
    
    // Set up the table to respond to double-clicks
    playerTable.getHeader().setPopupMenuActive(false);
    playerTable.setRowSelectedOnMouseDown(true);
    
    addAndMakeVisible(playerTable);
    
    // Set up buttons
    newButton.setButtonText("New");
    editButton.setButtonText("Edit");
    deleteButton.setButtonText("Delete");
    
    addAndMakeVisible(newButton);
    addAndMakeVisible(editButton);
    addAndMakeVisible(deleteButton);
    
    newButton.addListener(this);
    editButton.addListener(this);
    deleteButton.addListener(this);
    
    // Initialize the displayed players
    refresh();
}

PlayerListComponent::~PlayerListComponent()
{
    newButton.removeListener(this);
    editButton.removeListener(this);
    deleteButton.removeListener(this);
}

void PlayerListComponent::resized()
{
    auto area = getLocalBounds();
    
    // Reserve space for buttons at the bottom
    auto buttonArea = area.removeFromBottom(40);
    
    // Divide the button area into three equal parts
    auto buttonWidth = buttonArea.getWidth() / 3;
    
    newButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    deleteButton.setBounds(buttonArea.reduced(5));
    
    // Use the remaining area for the table
    playerTable.setBounds(area);
}

void PlayerListComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

int PlayerListComponent::getNumRows()
{
    return displayedPlayers.size();
}

void PlayerListComponent::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(getLookAndFeel().findColour(ListBox::backgroundColourId).contrasting(0.2f));
    }
    else if (rowNumber % 2)
    {
        g.fillAll(getLookAndFeel().findColour(ListBox::backgroundColourId).contrasting(0.05f));
    }
}

void PlayerListComponent::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < displayedPlayers.size())
    {
        auto* player = displayedPlayers[rowNumber];
        
        if (player != nullptr)
        {
            g.setColour(rowIsSelected ? Colours::white : getLookAndFeel().findColour(ListBox::textColourId));
            
            // Default text in case we can't access metadata
            String text = "";
            
            // Use the new safe method that handles all error cases internally
            if (player != nullptr)
            {
                switch (columnId)
                {
                    case NameColumn:
                        text = player->getSafeMetadataValue(1); // 1 = name
                        break;
                        
                    case CategoryColumn:
                        text = player->getSafeMetadataValue(2); // 2 = category
                        break;
                        
                    case DescriptionColumn:
                        text = player->getSafeMetadataValue(3); // 3 = description
                        break;
                        
                    default:
                        text = "";
                        break;
                }
            }
            else
            {
                // Player is null, use default text
                text = columnId == NameColumn ? "[Unnamed]" : 
                       columnId == CategoryColumn ? "[No Category]" : 
                       columnId == DescriptionColumn ? "[No Description]" : "";
            }
            
            g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }
    }
}

void PlayerListComponent::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    // Sort the players based on the column
    if (newSortColumnId != 0)
    {
        try
        {
            std::sort(displayedPlayers.begin(), displayedPlayers.end(), [newSortColumnId, isForwards](Player* a, Player* b) {
                // Handle null pointers
                if (a == nullptr && b == nullptr) return false;
                if (a == nullptr) return !isForwards; // Null players at the end if sorting ascending
                if (b == nullptr) return isForwards;  // Null players at the end if sorting ascending
                
                // Use the safe metadata accessor that handles all error cases internally
                String aValue, bValue;
                
                switch (newSortColumnId)
                {
                    case NameColumn:
                        aValue = a->getSafeMetadataValue(1); // 1 = name
                        bValue = b->getSafeMetadataValue(1);
                        break;
                        
                    case CategoryColumn:
                        aValue = a->getSafeMetadataValue(2); // 2 = category
                        bValue = b->getSafeMetadataValue(2);
                        break;
                        
                    case DescriptionColumn:
                        aValue = a->getSafeMetadataValue(3); // 3 = description
                        bValue = b->getSafeMetadataValue(3);
                        break;
                        
                    default:
                        return false;
                }
                
                return isForwards ? (aValue.compareIgnoreCase(bValue) < 0) : (aValue.compareIgnoreCase(bValue) > 0);
            });
            
            playerTable.updateContent();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception during player sorting: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception during player sorting" << std::endl;
        }
    }
}

void PlayerListComponent::buttonClicked(Button* button)
{
    if (button == &newButton)
    {
        // Notify listeners that a new player has been requested
        listeners.call(&Listener::newPlayerRequested);
    }
    else if (button == &editButton)
    {
        auto* selectedPlayer = getSelectedPlayer();
        
        if (selectedPlayer != nullptr)
        {
            // Notify listeners that a player edit has been requested
            listeners.call(&Listener::editPlayerRequested, selectedPlayer);
        }
    }
    else if (button == &deleteButton)
    {
        auto* selectedPlayer = getSelectedPlayer();
        
        if (selectedPlayer != nullptr)
        {
            // Notify listeners that a player deletion has been requested
            listeners.call(&Listener::deletePlayerRequested, selectedPlayer);
        }
    }
}

void PlayerListComponent::cellClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    // Handle cell clicks
    if (rowNumber < displayedPlayers.size())
    {
        auto* player = displayedPlayers[rowNumber];
        
        if (player != nullptr)
        {
            // Notify listeners that a player has been selected
            listeners.call(&Listener::playerSelected, player);
            
            // If double-clicked, also trigger edit
            if (event.getNumberOfClicks() > 1)
            {
                listeners.call(&Listener::editPlayerRequested, player);
            }
        }
    }
}

Player* PlayerListComponent::getSelectedPlayer() const
{
    auto selectedRow = playerTable.getSelectedRow();
    
    if (selectedRow >= 0 && selectedRow < displayedPlayers.size())
    {
        return displayedPlayers[selectedRow];
    }
    
    return nullptr;
}

void PlayerListComponent::refresh()
{
    // Clear the current list
    displayedPlayers.clear();
    
    // Get all players from the manager
    if (playerManager != nullptr)
    {
        displayedPlayers = playerManager->getPlayers();
    }
    
    // Update the table
    playerTable.updateContent();
    playerTable.repaint();
}

void PlayerListComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PlayerListComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace covertracks
