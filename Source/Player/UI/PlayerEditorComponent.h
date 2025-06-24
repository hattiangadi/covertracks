#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
#include "../../UI/GraphEditorPanel.h"
#include "../../Plugins/InternalPlugins.h"
#include "../Core/Player.h"
#include "PluginChainEditor.h"

namespace covertracks {

    class PlayerEditorComponent : public Component,
    public Button::Listener,
    public ComboBox::Listener,
    public Timer,
    public MidiKeyboardStateListener,
    public MidiInputCallback
{
public:
// Modified constructor with engine reference
PlayerEditorComponent(Player* playerToEdit = nullptr, 
AudioEngineImpl* audioEngine = nullptr);
~PlayerEditorComponent() override;

// Resource management tied to UI
void loadPlayerForEditing();
void saveAndReleaseResources();

// Existing methods
void resized() override;
void paint(Graphics& g) override;
void mouseDown(const MouseEvent& e) override;
void buttonClicked(Button* button) override;
void comboBoxChanged(ComboBox* comboBox) override;

// Plugin creation and management
void createPlugin(const PluginDescriptionAndPreference& desc, Point<int> pos);
void addPluginsToMenu(PopupMenu& menu);

// Player management
Player* getPlayer() const { return player; }
GraphDocumentComponent* getGraphEditor() const { return graphEditor.get(); }
std::optional<PluginDescriptionAndPreference> getChosenType(int menuID) const;
void takeOwnershipOfPlayer(std::unique_ptr<Player> newPlayer);
bool hasOwnedPlayer() const { return ownedPlayer != nullptr; }
std::unique_ptr<Player> releaseOwnedPlayer();

// Listener interface
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
// State values
Player* player = nullptr;
std::unique_ptr<Player> ownedPlayer;

// UI components
TextEditor nameEditor;
ComboBox categoryEditor;
TextEditor descriptionEditor;
Label nameLabel;
Label categoryLabel;
Label descriptionLabel;
TextButton saveButton;
TextButton cancelButton;

// Instrument capabilities UI
Label capabilitiesLabel;
ComboBox instrumentTypeCombo;
Label instrumentTypeLabel;
ComboBox capabilityTypeCombo;
Label capabilityTypeLabel;

// Note range controls
Label noteRangeLabel;
Slider minNoteSlider;
Label minNoteLabel;
Slider maxNoteSlider;
Label maxNoteLabel;

// Sound mapping controls
Label soundMappingLabel;
TextEditor soundMappingEditor;
TextButton addPresetButton;

// Graph editor (old system)
std::unique_ptr<GraphDocumentComponent> graphEditor;

// New Player system
std::unique_ptr<Player> playerInstance;
std::unique_ptr<PluginChainEditor> pluginChainEditor;
juce::MidiKeyboardState keyboardState;
std::unique_ptr<juce::MidiKeyboardComponent> midiKeyboard;

// Listeners
ListenerList<Listener> listeners;

// Reference to AudioEngine
AudioEngineImpl* audioEngine = nullptr;

// Player state update methods
void updatePlayerFromUI();
void updateUIFromPlayer();

// Instrument capabilities methods
void setupCapabilitiesUI();
void updateCapabilitiesUIVisibility();
void updateCapabilitiesFromUI();
void updateCapabilitiesInUI();

// Timer callback
void timerCallback() override;

// MIDI keyboard support
void connectMidiKeyboardToGraph();
void setupNewPlayerSystem();
void switchToPluginChainEditor();

// MidiInputCallback implementation
void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) override;

// MidiKeyboardStateListener implementation
void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayerEditorComponent)
};

} // namespace covertracks
