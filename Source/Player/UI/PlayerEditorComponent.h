#pragma once

#include <JuceHeader.h>
#include "../Core/PlayerManager.h"
// Note: GraphEditorPanel removed - using Tracktion track editing instead
#include "../../Plugins/InternalPlugins.h"
#include "../Core/Player.h"
#include "PluginChainEditor.h"

namespace covertracks {

    class PlayerEditorComponent : public juce::Component,
    public juce::Button::Listener,
    public juce::ComboBox::Listener,
    public juce::Timer,
    public juce::MidiKeyboardStateListener,
    public juce::MidiInputCallback
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
void paint(juce::Graphics& g) override;
void mouseDown(const juce::MouseEvent& e) override;
void buttonClicked(juce::Button* button) override;
void comboBoxChanged(juce::ComboBox* comboBox) override;

// Plugin creation and management
void createPlugin(const juce::PluginDescription& desc, juce::Point<int> pos);
void addPluginsToMenu(juce::PopupMenu& menu);

// Player management
Player* getPlayer() const { return player; }
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

// Graph editor removed - using Tracktion track editing instead

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
