#include "TracktionPlayerEditor.h"
#include <iostream>

namespace covertracks {

TracktionPlayerEditor::TracktionPlayerEditor()
{
    // Set up title
    titleLabel.setText("TracktionPlayer Editor", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up metadata fields
    nameLabel.setText("Name:", juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(nameLabel);
    
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    addAndMakeVisible(nameEditor);
    
    categoryLabel.setText("Category:", juce::dontSendNotification);
    categoryLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(categoryLabel);
    
    categoryCombo.addItem("Keyboard", 1);
    categoryCombo.addItem("Drums", 2);
    categoryCombo.addItem("Bass", 3);
    categoryCombo.addItem("Guitar", 4);
    categoryCombo.addItem("Strings", 5);
    categoryCombo.addItem("Brass", 6);
    categoryCombo.addItem("Woodwinds", 7);
    categoryCombo.addItem("Percussion", 8);
    categoryCombo.addItem("Effects", 9);
    categoryCombo.addItem("Other", 10);
    categoryCombo.setTextWhenNothingSelected("Select category");
    categoryCombo.addListener(this);
    addAndMakeVisible(categoryCombo);
    
    descriptionLabel.setText("Description:", juce::dontSendNotification);
    descriptionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(descriptionLabel);
    
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    addAndMakeVisible(descriptionEditor);
    
    // Set up action buttons
    saveButton.setButtonText("Save");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);
    
    testPlayerButton.setButtonText("Test Player");
    testPlayerButton.addListener(this);
    addAndMakeVisible(testPlayerButton);
    
    // Set up plugin chain editor
    pluginChainEditor = std::make_unique<PluginChainEditor>();
    addAndMakeVisible(pluginChainEditor.get());
    
    std::cout << "TracktionPlayerEditor initialized" << std::endl;
}

TracktionPlayerEditor::~TracktionPlayerEditor()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
    testPlayerButton.removeListener(this);
    categoryCombo.removeListener(this);
}

void TracktionPlayerEditor::setPlayer(TracktionPlayer* player)
{
    currentPlayer = player;
    
    // Update plugin chain editor
    if (pluginChainEditor)
    {
        pluginChainEditor->setPlayer(player);
    }
    
    // Update UI from player
    updateUIFromPlayer();
    
    std::cout << "TracktionPlayerEditor: Set player " << (player ? player->getName().toStdString() : "nullptr") << std::endl;
}

void TracktionPlayerEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 2);
    
    // Draw section separators
    g.setColour(juce::Colours::lightgrey);
    
    // Separator between metadata and plugin editor
    auto metadataHeight = 140;
    g.drawHorizontalLine(metadataHeight, 10, getWidth() - 10);
}

void TracktionPlayerEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title
    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);
    
    // Metadata section
    auto metadataArea = area.removeFromTop(100);
    auto labelWidth = 80;
    
    // Name row
    auto nameRow = metadataArea.removeFromTop(25);
    nameLabel.setBounds(nameRow.removeFromLeft(labelWidth));
    nameEditor.setBounds(nameRow.reduced(5, 2));
    
    // Category row
    auto categoryRow = metadataArea.removeFromTop(25);
    categoryLabel.setBounds(categoryRow.removeFromLeft(labelWidth));
    categoryCombo.setBounds(categoryRow.removeFromLeft(150).reduced(5, 2));
    
    // Description row
    auto descRow = metadataArea.removeFromTop(40);
    descriptionLabel.setBounds(descRow.removeFromLeft(labelWidth));
    descriptionEditor.setBounds(descRow.reduced(5, 2));
    
    area.removeFromTop(20); // Space after metadata
    
    // Button row
    auto buttonArea = area.removeFromBottom(40);
    auto buttonWidth = buttonArea.getWidth() / 3;
    
    saveButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    cancelButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    testPlayerButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    
    // Plugin chain editor fills remaining space
    if (pluginChainEditor)
    {
        pluginChainEditor->setBounds(area.reduced(0, 5));
    }
}

void TracktionPlayerEditor::buttonClicked(juce::Button* button)
{
    if (button == &saveButton)
    {
        updatePlayerFromUI();
        listeners.call([this](Listener& l) { l.tracktionPlayerEditorSaveRequested(this); });
    }
    else if (button == &cancelButton)
    {
        listeners.call([this](Listener& l) { l.tracktionPlayerEditorCancelRequested(this); });
    }
    else if (button == &testPlayerButton)
    {
        testPlayer();
    }
}

void TracktionPlayerEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &categoryCombo)
    {
        // Update player immediately when category changes
        updatePlayerFromUI();
    }
}

void TracktionPlayerEditor::addListener(Listener* listener)
{
    listeners.add(listener);
}

void TracktionPlayerEditor::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void TracktionPlayerEditor::updateUIFromPlayer()
{
    if (!currentPlayer)
    {
        nameEditor.setText("");
        categoryCombo.setSelectedId(0);
        descriptionEditor.setText("");
        return;
    }
    
    // Update metadata fields from player
    nameEditor.setText(currentPlayer->getName());
    
    // Set category (simplified mapping)
    auto playerName = currentPlayer->getName().toLowerCase();
    if (playerName.contains("piano") || playerName.contains("keyboard"))
        categoryCombo.setSelectedId(1);
    else if (playerName.contains("drum"))
        categoryCombo.setSelectedId(2);
    else if (playerName.contains("bass"))
        categoryCombo.setSelectedId(3);
    else if (playerName.contains("guitar"))
        categoryCombo.setSelectedId(4);
    else
        categoryCombo.setSelectedId(10); // Other
    
    // Set description
    juce::String description = "TracktionPlayer: " + currentPlayer->getName();
    if (auto* instrument = currentPlayer->getInstrument())
    {
        description += "\nPlugins: " + juce::String(instrument->getPluginCount());
        description += "\nRegistered: " + juce::String(instrument->isRegisteredWithAudioEngine() ? "Yes" : "No");
    }
    descriptionEditor.setText(description);
}

void TracktionPlayerEditor::updatePlayerFromUI()
{
    if (!currentPlayer)
        return;
        
    // Note: TracktionPlayer doesn't have setters for metadata yet
    // This is where we'd update the player's metadata from the UI
    
    std::cout << "TracktionPlayerEditor: Updated player metadata (setters not yet implemented)" << std::endl;
    std::cout << "  Name: " << nameEditor.getText().toStdString() << std::endl;
    std::cout << "  Category: " << categoryCombo.getText().toStdString() << std::endl;
    std::cout << "  Description: " << descriptionEditor.getText().substring(0, 50).toStdString() << "..." << std::endl;
}

void TracktionPlayerEditor::testPlayer()
{
    if (!currentPlayer)
    {
        std::cout << "No player available for testing" << std::endl;
        return;
    }
    
    std::cout << "Testing TracktionPlayer: " << currentPlayer->getName().toStdString() << std::endl;
    
    // Create a simple test chart
    juce::Array<juce::var> midiEventsArray;
    
    // Play a C major chord
    std::vector<int> chordNotes = {60, 64, 67}; // C, E, G
    
    for (int note : chordNotes)
    {
        // Note on
        juce::DynamicObject::Ptr noteOnEvent = new juce::DynamicObject();
        noteOnEvent->setProperty("type", "noteOn");
        noteOnEvent->setProperty("time", 0.0);
        noteOnEvent->setProperty("channel", 1);
        noteOnEvent->setProperty("note", note);
        noteOnEvent->setProperty("velocity", 80);
        midiEventsArray.add(juce::var(noteOnEvent.get()));
        
        // Note off
        juce::DynamicObject::Ptr noteOffEvent = new juce::DynamicObject();
        noteOffEvent->setProperty("type", "noteOff");
        noteOffEvent->setProperty("time", 2.0);
        noteOffEvent->setProperty("channel", 1);
        noteOffEvent->setProperty("note", note);
        noteOffEvent->setProperty("velocity", 0);
        midiEventsArray.add(juce::var(noteOffEvent.get()));
    }
    
    juce::var chart = juce::var(midiEventsArray);
    
    if (currentPlayer->playChart(chart, 120.0))
    {
        std::cout << "Test player started - C major chord for 2 seconds" << std::endl;
    }
    else
    {
        std::cout << "Failed to start test player" << std::endl;
    }
}

} // namespace covertracks