#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
#include "../../UI/GraphEditorPanel.h"
#include "../../Plugins/InternalPlugins.h"
#include "../Core/Player.h"

namespace covertracks {

class PlayerEditorComponent : public Component,
                              public Button::Listener
{
public:
    PlayerEditorComponent(Player* playerToEdit = nullptr);
    ~PlayerEditorComponent() override;
    
    void resized() override;
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& e) override;
    
    // Button::Listener overrides
    void buttonClicked(Button* button) override;
    
    void createPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos);
    
    void addPluginsToMenu(PopupMenu& menu);
    
    void addToMenu(const KnownPluginList::PluginTree& tree,
                  PopupMenu& m,
                  const Array<PluginDescription>& allPlugins,
                  Array<PluginDescriptionAndPreference>& addedPlugins);
    
    Player* getPlayer() const { return player; }
    
    GraphDocumentComponent* getGraphEditor() const { return graphEditor.get(); }
    
    // Get the chosen plugin type from a menu ID
    std::optional<PluginDescriptionAndPreference> getChosenType(int menuID) const;
    
    // Take ownership of a player (for new players)
    void takeOwnershipOfPlayer(std::unique_ptr<Player> newPlayer);
    
    // Listener interface for receiving events from the PlayerEditorComponent
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void playerEditorSaveRequested(PlayerEditorComponent* editor) = 0;
        virtual void playerEditorCancelRequested(PlayerEditorComponent* editor) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    Player* player = nullptr;
    std::unique_ptr<Player> ownedPlayer;

    TextEditor nameEditor;
    TextEditor categoryEditor;
    TextEditor descriptionEditor;

    // Metadata labels
    Label nameLabel;
    Label categoryLabel;
    Label descriptionLabel;

    TextButton saveButton;
    TextButton cancelButton;
    TextButton loadGraphButton;

    std::unique_ptr<GraphDocumentComponent> graphEditor;

    ListenerList<Listener> listeners;

    void updatePlayerFromUI();
    void updateUIFromPlayer();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerEditorComponent)

};

} // namespace covertracks
