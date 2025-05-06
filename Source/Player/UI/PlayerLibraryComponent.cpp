// PlayerLibraryComponent.cpp
#include "PlayerLibraryComponent.h"

PlayerLibraryComponent::PlayerLibraryComponent()
{
    playerListComponent = std::make_unique<PlayerListComponent>(playerManager);
    playerListComponent->addListener(this);
    addAndMakeVisible(playerListComponent.get());
}

PlayerLibraryComponent::~PlayerLibraryComponent()
{
    if (playerListComponent != nullptr)
        playerListComponent->removeListener(this);
    
    if (playerEditorComponent != nullptr)
        playerEditorComponent->removeListener(this);
}

void PlayerLibraryComponent::resized()
{
    auto area = getLocalBounds();
    
    if (playerEditorComponent != nullptr)
    {
        playerEditorComponent->setBounds(area);
    }
    else if (playerListComponent != nullptr)
    {
        playerListComponent->setBounds(area);
    }
}

void PlayerLibraryComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void PlayerLibraryComponent::playerSelected(PlayerManager::Player* player)
{
    // Handle player selection
    // For now, we don't need to do anything special here
}

void PlayerLibraryComponent::newPlayerRequested()
{
    showPlayerEditor(nullptr);
}

void PlayerLibraryComponent::editPlayerRequested(PlayerManager::Player* player)
{
    showPlayerEditor(player);
}

void PlayerLibraryComponent::deletePlayerRequested(PlayerManager::Player* player)
{
    // Show confirmation dialog
    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon,
        "Delete Player",
        "Are you sure you want to delete the player '" + player->metadata.name + "'?",
        "Delete",
        "Cancel",
        this,
        ModalCallbackFunction::create([this, player](int result) {
            if (result == 1) // OK was clicked
            {
                playerManager.deletePlayer(player);
                playerListComponent->refreshList();
            }
        })
    );
}

// PlayerLibraryComponent.cpp - Update to ensure both graph structure and plugin states are preserved

void PlayerLibraryComponent::playerEditorSaveRequested(PlayerEditorComponent* editor)
{
    // Get the player from the editor (includes graph and plugin states)
    auto playerPtr = editor->getPlayer();
    
    if (playerPtr == nullptr)
        return;
    
    // Save the player
    bool saved = playerManager.savePlayer(playerPtr.get());
    
    if (saved)
    {
        // Add to manager's collection if it's not already there
        playerManager.addPlayer(std::move(playerPtr));
        
        // Refresh the player list
        if (playerListComponent != nullptr)
            playerListComponent->refreshList();
    }
    
    // Hide the editor
    hidePlayerEditor();
}

void PlayerLibraryComponent::playerEditorCancelRequested(PlayerEditorComponent* editor)
{
    // Just hide the editor without saving
    hidePlayerEditor();
}

// In PlayerLibraryComponent.cpp, update the showPlayerEditor method:

void PlayerLibraryComponent::showPlayerEditor(PlayerManager::Player* playerToEdit)
{
    // Remove existing editor if any
    if (playerEditorComponent != nullptr)
    {
        playerEditorComponent->removeListener(this);
        removeChildComponent(playerEditorComponent.get());
        playerEditorComponent = nullptr;
    }
    
    // Create and show new editor
    playerEditorComponent = std::make_unique<PlayerEditorComponent>(playerToEdit);
    playerEditorComponent->addListener(this);
    addAndMakeVisible(playerEditorComponent.get());
    
    // Hide the list component
    if (playerListComponent != nullptr)
        playerListComponent->setVisible(false);
    
    // Resize to fit - ensure the editor has enough space
    resized();
    
    // Resize parent window if needed to give enough space for the graph editor
    if (auto* parentComponent = getParentComponent())
    {
        if (auto* dialogWindow = dynamic_cast<DialogWindow*>(parentComponent))
        {
            // Make sure the dialog is large enough
            const int minWidth = 800;
            const int minHeight = 700;
            
            auto currentBounds = dialogWindow->getBounds();
            if (currentBounds.getWidth() < minWidth || currentBounds.getHeight() < minHeight)
            {
                dialogWindow->centreWithSize(minWidth, minHeight);
            }
        }
    }
}

void PlayerLibraryComponent::hidePlayerEditor()
{
    // Remove the editor
    if (playerEditorComponent != nullptr)
    {
        playerEditorComponent->removeListener(this);
        removeChildComponent(playerEditorComponent.get());
        playerEditorComponent = nullptr;
    }
    
    // Show the list component
    if (playerListComponent != nullptr)
        playerListComponent->setVisible(true);
    
    // Resize to fit
    resized();
}