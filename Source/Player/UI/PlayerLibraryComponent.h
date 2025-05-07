// PlayerLibraryComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
#include "PlayerListComponent.h"
#include "PlayerEditorComponent.h"

class PlayerLibraryComponent : public Component,
                               private PlayerListComponent::Listener,
                               private PlayerEditorComponent::Listener
{
public:
    PlayerLibraryComponent();
    ~PlayerLibraryComponent() override;
    
    void resized() override;
    void paint(Graphics& g) override;
   // In PlayerLibraryComponent.h:
    PlayerEditorComponent* getActiveEditorComponent() const { return playerEditorComponent.get(); } 
    
private:
    // PlayerListComponent::Listener implementation
    void playerSelected(PlayerManager::Player* player) override;
    void newPlayerRequested() override;
    void editPlayerRequested(PlayerManager::Player* player) override;
    void deletePlayerRequested(PlayerManager::Player* player) override;
    
    // PlayerEditorComponent::Listener implementation
    void playerEditorSaveRequested(PlayerEditorComponent* editor) override;
    void playerEditorCancelRequested(PlayerEditorComponent* editor) override;
    
    // UI components
    PlayerManager playerManager;
    std::unique_ptr<PlayerListComponent> playerListComponent;
    std::unique_ptr<PlayerEditorComponent> playerEditorComponent;
    
    // Helper methods
    void showPlayerEditor(PlayerManager::Player* playerToEdit = nullptr);
    void hidePlayerEditor();
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerLibraryComponent)
};
