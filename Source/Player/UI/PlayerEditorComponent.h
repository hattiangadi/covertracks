// PlayerEditorComponent.h
#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
#include "../../UI/GraphEditorPanel.h"
#include "../../Plugins/InternalPlugins.h" 

class PlayerEditorComponent : public Component,
                              public Button::Listener
{
public:
    PlayerEditorComponent(PlayerManager::Player* playerToEdit = nullptr);
    ~PlayerEditorComponent() override;
    
    void resized() override;
    void paint(Graphics& g) override;
    
    // Button::Listener overrides
    void buttonClicked(Button* button) override;
    
    // Handle mouse events
    void mouseDown(const MouseEvent& e) override;
    
    // Get the edited player data
    std::unique_ptr<PlayerManager::Player> getPlayer();
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        virtual void playerEditorSaveRequested(PlayerEditorComponent* editor) = 0;
        virtual void playerEditorCancelRequested(PlayerEditorComponent* editor) = 0;
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
private:
    void makeDefaultConnections(PluginGraph* graph);
    void createDefaultGraph(PluginGraph* graph);
    void showPluginMenu(Point<int> position);
    
    void addPluginsToMenu(const KnownPluginList::PluginTree& tree,
                         PopupMenu& menu,
                         const Array<PluginDescription>& allPlugins,
                         Array<PluginDescriptionAndPreference>& addedPlugins,
                         int& nextMenuID);


    Label nameLabel {"nameLabel", "Name:"};
    TextEditor nameEditor;
    
    Label categoryLabel {"categoryLabel", "Category:"};
    ComboBox categoryCombo;
    
    Label descriptionLabel {"descriptionLabel", "Description:"};
    TextEditor descriptionEditor;
    
    // Techniques section has been removed
    
    // Plugin graph editor
    Label pluginLabel {"pluginLabel", "Plugin Configuration:"};
    std::unique_ptr<GraphDocumentComponent> graphEditor;
    
    TextButton saveButton {"Save"};
    TextButton cancelButton {"Cancel"};
    
    std::unique_ptr<PlayerManager::Player> player;
    
    ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerEditorComponent)
};