// PlayerLibraryComponent.cpp
#include "PlayerLibraryComponent.h"
#include "../../UI/MainHostWindow.h"

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
        try
        {
            bool success = false;
            
            // Check if this is a new player that the editor owns
            if (editor->hasOwnedPlayer())
            {
                // Get the owned player from the editor and transfer ownership to the PlayerManager
                auto ownedPlayer = editor->releaseOwnedPlayer();
                if (ownedPlayer != nullptr)
                {
                    std::cout << "Saving new player and transferring ownership to PlayerManager" << std::endl;
                    success = playerManager->savePlayerAndTakeOwnership(std::move(ownedPlayer));
                }
            }
            else
            {
                // This is an existing player that the PlayerManager already owns
                auto player = editor->getPlayer();
                if (player != nullptr)
                {
                    std::cout << "Saving existing player: " << player->getMetadata().name << std::endl;
                    success = playerManager->savePlayer(player);
                }
            }
            
            if (success)
                std::cout << "Player saved successfully" << std::endl;
            else
                std::cout << "Failed to save player" << std::endl;
            
            // Refresh the list
            playerList->refresh();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception when saving player: " << e.what() << std::endl;
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                           "Save Error",
                                           "There was an error saving the player: " + String(e.what()));
        }
        catch (...)
        {
            std::cerr << "Unknown exception when saving player" << std::endl;
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                           "Save Error",
                                           "There was an unknown error saving the player.");
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
    
    // Get the AudioEngine from the PlayerManager
    AudioEngine* audioEngine = playerManager->getAudioEngine();
    
    // Create the editor component with the AudioEngine reference
    playerEditor = std::make_unique<PlayerEditorComponent>(playerToEdit, audioEngine);
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
