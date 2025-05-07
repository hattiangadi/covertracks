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
    void mouseDown(const MouseEvent& e) override;
    
    // Button::Listener overrides
    void buttonClicked(Button* button) override;
    
    void createPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos);
    
    void showPopupMenu(Point<int> position);
    
    void addPluginsToMenu(PopupMenu& menu);
    
    void addToMenu(const KnownPluginList::PluginTree& tree,
                  PopupMenu& m,
                  const Array<PluginDescription>& allPlugins,
                  Array<PluginDescriptionAndPreference>& addedPlugins);

    std::optional<PluginDescriptionAndPreference> getChosenType(int menuID) const;

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
    
    // Getter for the graph editor component
    GraphDocumentComponent* getGraphEditor() const { return graphEditor.get(); }
    
private:
    void makeDefaultConnections(PluginGraph* graph);
    void createDefaultGraph(PluginGraph* graph);

    Array<PluginDescriptionAndPreference> pluginDescriptionsAndPreference;
    std::vector<PluginDescription> internalTypes;
    static constexpr int menuIDBase = 0x324503f4;
    
    // Plugin system components
    AudioPluginFormatManager formatManager;
    AudioDeviceManager deviceManager;
    KnownPluginList knownPluginList;
    Label nameLabel {"nameLabel", "Name:"};
    TextEditor nameEditor;
    
    Label categoryLabel {"categoryLabel", "Category:"};
    ComboBox categoryCombo;
    
    Label descriptionLabel {"descriptionLabel", "Description:"};
    TextEditor descriptionEditor;
    
    static bool containsDuplicateNames(const Array<PluginDescription>& plugins, const String& name);

    // Plugin graph editor
    Label pluginLabel {"pluginLabel", "Plugin Configuration:"};
    std::unique_ptr<GraphDocumentComponent> graphEditor;
    
    TextButton loadGraphButton {"Load Graph"};
    TextButton saveGraphButton {"Save Graph"};
    TextButton saveButton {"Save Player"};
    TextButton cancelButton {"Cancel"};
    
    std::unique_ptr<PlayerManager::Player> player;
    
    ListenerList<Listener> listeners;
    
    // For asynchronous file operations
    std::unique_ptr<FileChooser> fileChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerEditorComponent)
};