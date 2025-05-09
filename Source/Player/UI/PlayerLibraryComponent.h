// PlayerLibraryComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
#include "PlayerListComponent.h"
#include "PlayerEditorComponent.h"

namespace covertracks {

class PlayerLibraryComponent : public Component,
                               private PlayerListComponent::Listener,
                               private PlayerEditorComponent::Listener
{
public:
    explicit PlayerLibraryComponent(std::shared_ptr<PlayerManager> manager);
    ~PlayerLibraryComponent() override;
    
    void resized() override;
    void paint(Graphics& g) override;
    
public:
    PlayerEditorComponent* getPlayerEditor() const { return playerEditor.get(); }
private:
    // PlayerListComponent::Listener implementation
    void playerSelected(Player* player) override;
    void newPlayerRequested() override;
    void editPlayerRequested(Player* player) override;
    void deletePlayerRequested(Player* player) override;
    
    // PlayerEditorComponent::Listener implementation
    void playerEditorSaveRequested(PlayerEditorComponent* editor) override;
    void playerEditorCancelRequested(PlayerEditorComponent* editor) override;
    
    // UI components
    std::shared_ptr<PlayerManager> playerManager;
    std::unique_ptr<PlayerListComponent> playerList;
    std::unique_ptr<PlayerEditorComponent> playerEditor;
    
    // Helper methods
    void showPlayerEditor(Player* playerToEdit = nullptr);
    void hidePlayerEditor();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerLibraryComponent)
};

} // namespace covertracks
