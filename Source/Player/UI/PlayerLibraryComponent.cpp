// PlayerLibraryComponent.cpp
#include "PlayerLibraryComponent.h"

namespace covertracks {

PlayerLibraryComponent::PlayerLibraryComponent(std::shared_ptr<PlayerManager> manager)
    : playerManager(std::move(manager))
{
    playerList = std::make_unique<PlayerListComponent>(playerManager.get());
    playerList->addListener(this);
    addAndMakeVisible(playerList.get());
}

PlayerLibraryComponent::~PlayerLibraryComponent()
{
    if (playerList != nullptr)
        playerList->removeListener(this);
    
    if (playerEditor != nullptr)
        playerEditor->removeListener(this);
}

void PlayerLibraryComponent::resized()
{
    auto area = getLocalBounds();
    
    if (playerEditor != nullptr)
    {
        playerEditor->setBounds(area);
    }
    else if (playerList != nullptr)
    {
        playerList->setBounds(area);
    }
}

void PlayerLibraryComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void PlayerLibraryComponent::playerSelected(Player* player)
{
    // Handle player selection
}

void PlayerLibraryComponent::newPlayerRequested()
{
    showPlayerEditor(nullptr);
}

void PlayerLibraryComponent::editPlayerRequested(Player* player)
{
    showPlayerEditor(player);
}

void PlayerLibraryComponent::deletePlayerRequested(Player* player)
{
    // Confirm deletion
    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon,
        "Delete Player",
        "Are you sure you want to delete this player?",
        "Delete",
        "Cancel",
        this,
        ModalCallbackFunction::create([this, player](int result) {
            if (result == 1) // OK button was pressed
            {
                playerManager->deletePlayer(player);
                playerList->refresh();
            }
        })
    );
}

void PlayerLibraryComponent::playerEditorSaveRequested(PlayerEditorComponent* editor)
{
    if (editor != nullptr)
    {
        auto player = editor->getPlayer();
        
        if (player != nullptr)
        {
            // Save the player
            playerManager->savePlayer(player);
            
            // Refresh the list
            playerList->refresh();
        }
    }
    
    hidePlayerEditor();
}

void PlayerLibraryComponent::playerEditorCancelRequested(PlayerEditorComponent* editor)
{
    hidePlayerEditor();
}

void PlayerLibraryComponent::showPlayerEditor(Player* playerToEdit)
{
    // Create a new player if none was provided
    std::unique_ptr<Player> newPlayer;
    
    if (playerToEdit == nullptr)
    {
        newPlayer = playerManager->createPlayer();
        playerToEdit = newPlayer.get();
    }
    
    // Create the editor component
    playerEditor = std::make_unique<PlayerEditorComponent>(playerToEdit);
    playerEditor->addListener(this);
    
    // If we created a new player, transfer ownership to the editor
    if (newPlayer != nullptr)
    {
        playerEditor->takeOwnershipOfPlayer(std::move(newPlayer));
    }
    
    // Show the editor
    addAndMakeVisible(playerEditor.get());
    
    // Hide the list
    if (playerList != nullptr)
    {
        playerList->setVisible(false);
    }
    
    // Resize to fit
    resized();
}

void PlayerLibraryComponent::hidePlayerEditor()
{
    // Remove the editor
    if (playerEditor != nullptr)
    {
        playerEditor->removeListener(this);
        removeChildComponent(playerEditor.get());
        playerEditor = nullptr;
    }
    
    // Show the list
    if (playerList != nullptr)
    {
        playerList->setVisible(true);
        playerList->refresh();
    }
    
    // Resize to fit
    resized();
}

} // namespace covertracks
