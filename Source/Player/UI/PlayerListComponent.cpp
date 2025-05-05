// PlayerListComponent.cpp
#include "PlayerListComponent.h"

PlayerListComponent::PlayerListComponent(PlayerManager& pm)
    : playerManager(pm),
      table("PlayerTable", this)
{
    // Set up table columns
    table.getHeader().addColumn("Name", NameColumn, 150);
    table.getHeader().addColumn("Category", CategoryColumn, 100);
    table.getHeader().addColumn("Description", DescriptionColumn, 300);
    
    table.setMultipleSelectionEnabled(false);
    
    addAndMakeVisible(table);
    addAndMakeVisible(newButton);
    addAndMakeVisible(editButton);
    addAndMakeVisible(deleteButton);
    
    newButton.addListener(this);
    editButton.addListener(this);
    deleteButton.addListener(this);
    
    // Initialize the displayed players
    refreshList();
    
    // Set initial button states
    updateButtonStates();
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
    
    // Reserve space at the bottom for buttons
    auto buttonArea = area.removeFromBottom(40).reduced(5);
    
    // Position buttons
    auto buttonWidth = jmin(120, buttonArea.getWidth() / 3 - 10);
    newButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(10); // Spacing
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(10); // Spacing
    deleteButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    
    // The rest is for the table
    table.setBounds(area);
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
        g.fillAll(getLookAndFeel().findColour(TextEditor::highlightColourId));
    else
        g.fillAll(rowNumber % 2 == 0 ? Colours::white : Colour(0xffF9F9F9));
}

void PlayerListComponent::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= displayedPlayers.size())
        return;
    
    auto* player = displayedPlayers[rowNumber];
    String text;
    
    switch (columnId)
    {
        case NameColumn:
            text = player->metadata.name;
            break;
        case CategoryColumn:
            text = player->metadata.category;
            break;
        case DescriptionColumn:
            text = player->metadata.description;
            break;
        default:
            break;
    }
    
    g.setColour(rowIsSelected ? Colours::white : Colours::black);
    g.setFont(Font(height * 0.7f));
    g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
}

void PlayerListComponent::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    if (newSortColumnId != 0)
    {
        sortColumnId = newSortColumnId;
        sortForwards = isForwards;
        sortPlayers();
    }
}

void PlayerListComponent::sortPlayers()
{
    switch (sortColumnId)
    {
        case NameColumn:
            std::sort(displayedPlayers.begin(), displayedPlayers.end(),
                     [this](PlayerManager::Player* a, PlayerManager::Player* b) {
                         int result = a->metadata.name.compareIgnoreCase(b->metadata.name);
                         return sortForwards ? result < 0 : result > 0;
                     });
            break;
        case CategoryColumn:
            std::sort(displayedPlayers.begin(), displayedPlayers.end(),
                     [this](PlayerManager::Player* a, PlayerManager::Player* b) {
                         int result = a->metadata.category.compareIgnoreCase(b->metadata.category);
                         return sortForwards ? result < 0 : result > 0;
                     });
            break;
        case DescriptionColumn:
            std::sort(displayedPlayers.begin(), displayedPlayers.end(),
                     [this](PlayerManager::Player* a, PlayerManager::Player* b) {
                         int result = a->metadata.description.compareIgnoreCase(b->metadata.description);
                         return sortForwards ? result < 0 : result > 0;
                     });
            break;
        default:
            break;
    }
    
    table.updateContent();
}

void PlayerListComponent::buttonClicked(Button* button)
{
    if (button == &newButton)
    {
        listeners.call(&Listener::newPlayerRequested);
    }
    else if (button == &editButton)
    {
        auto selectedPlayer = getSelectedPlayer();
        if (selectedPlayer != nullptr)
        {
            listeners.call(&Listener::editPlayerRequested, selectedPlayer);
        }
    }
    else if (button == &deleteButton)
    {
        auto selectedPlayer = getSelectedPlayer();
        if (selectedPlayer != nullptr)
        {
            listeners.call(&Listener::deletePlayerRequested, selectedPlayer);
        }
    }
}

PlayerManager::Player* PlayerListComponent::getSelectedPlayer() const
{
    auto selectedRow = table.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < displayedPlayers.size())
        return displayedPlayers[selectedRow];
    
    return nullptr;
}

void PlayerListComponent::refreshList()
{
    displayedPlayers = playerManager.getPlayers();
    sortPlayers();
    table.updateContent();
    updateButtonStates();
}

void PlayerListComponent::updateButtonStates()
{
    bool hasSelection = table.getSelectedRow() >= 0;
    editButton.setEnabled(hasSelection);
    deleteButton.setEnabled(hasSelection);
}