// PlayerEditorComponent.cpp
#include "PlayerEditorComponent.h"

PlayerEditorComponent::PlayerEditorComponent(PlayerManager::Player* playerToEdit)
{
    // Initialize the player data
    if (playerToEdit != nullptr)
    {
        player = std::make_unique<PlayerManager::Player>();
        player->metadata = playerToEdit->metadata;
        player->capabilities = playerToEdit->capabilities;
        
        if (playerToEdit->graphState != nullptr)
            player->graphState = std::make_unique<XmlElement>(*playerToEdit->graphState);
    }
    else
    {
        // Create a new player with default settings
        player = std::make_unique<PlayerManager::Player>();
        player->metadata.name = "New Player";
        player->metadata.category = "Keyboard";
        player->metadata.description = "A new player configuration";
        player->metadata.created = Time::getCurrentTime();
        player->metadata.lastModified = player->metadata.created;
        
        player->capabilities.pitchRange = {36, 96}; // MIDI range C2-C8
        player->capabilities.techniques.add("legato");
        
        player->graphState = std::make_unique<XmlElement>("FILTERGRAPH");
    }
    
    // Set up name editor
    nameEditor.setText(player->metadata.name);
    nameEditor.setSelectAllWhenFocused(true);
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(nameEditor);
    
    // Set up category combo
    categoryCombo.addItem("Keyboard", 1);
    categoryCombo.addItem("Guitar", 2);
    categoryCombo.addItem("Bass", 3);
    categoryCombo.addItem("Drums", 4);
    categoryCombo.addItem("Vocals", 5);
    categoryCombo.addItem("Synth", 6);
    categoryCombo.addItem("Brass", 7);
    categoryCombo.addItem("Strings", 8);
    categoryCombo.addItem("Woodwind", 9);
    categoryCombo.addItem("Other", 10);
    
    categoryCombo.setText(player->metadata.category);
    addAndMakeVisible(categoryLabel);
    addAndMakeVisible(categoryCombo);
    
    // Set up description editor
    descriptionEditor.setText(player->metadata.description);
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.setSelectAllWhenFocused(true);
    addAndMakeVisible(descriptionLabel);
    addAndMakeVisible(descriptionEditor);
    
    // Set up techniques toggles
    legatoToggle.setToggleState(player->capabilities.techniques.contains("legato"), dontSendNotification);
    staccatoToggle.setToggleState(player->capabilities.techniques.contains("staccato"), dontSendNotification);
    chordToggle.setToggleState(player->capabilities.techniques.contains("chord"), dontSendNotification);
    
    addAndMakeVisible(techniquesLabel);
    addAndMakeVisible(legatoToggle);
    addAndMakeVisible(staccatoToggle);
    addAndMakeVisible(chordToggle);
    
    // Set up pitch range sliders
    lowPitchSlider.setRange(0, 127, 1);
    lowPitchSlider.setValue(player->capabilities.pitchRange.getStart(), dontSendNotification);
    lowPitchSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    lowPitchSlider.setTextBoxStyle(Slider::TextBoxRight, false, 50, 20);
    lowPitchSlider.onValueChange = [this] { updateSliderLabels(); };
    
    highPitchSlider.setRange(0, 127, 1);
    highPitchSlider.setValue(player->capabilities.pitchRange.getEnd(), dontSendNotification);
    highPitchSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    highPitchSlider.setTextBoxStyle(Slider::TextBoxRight, false, 50, 20);
    highPitchSlider.onValueChange = [this] { updateSliderLabels(); };
    
    addAndMakeVisible(pitchRangeLabel);
    addAndMakeVisible(lowPitchSlider);
    addAndMakeVisible(highPitchSlider);
    addAndMakeVisible(lowPitchValueLabel);
    addAndMakeVisible(highPitchValueLabel);
    
    updateSliderLabels();
    
    // Set up buttons
    addAndMakeVisible(saveButton);
    addAndMakeVisible(cancelButton);
    
    saveButton.addListener(this);
    cancelButton.addListener(this);
}

PlayerEditorComponent::~PlayerEditorComponent()
{
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
}

void PlayerEditorComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto fieldHeight = 24;
    auto labelWidth = 100;
    
    // Name field
    auto nameArea = area.removeFromTop(fieldHeight);
    nameLabel.setBounds(nameArea.removeFromLeft(labelWidth));
    nameEditor.setBounds(nameArea);
    
    area.removeFromTop(10); // Spacing
    
    // Category field
    auto categoryArea = area.removeFromTop(fieldHeight);
    categoryLabel.setBounds(categoryArea.removeFromLeft(labelWidth));
    categoryCombo.setBounds(categoryArea);
    
    area.removeFromTop(10); // Spacing
    
    // Description field
    auto descriptionArea = area.removeFromTop(fieldHeight * 3);
    descriptionLabel.setBounds(descriptionArea.removeFromLeft(labelWidth).withHeight(fieldHeight));
    descriptionEditor.setBounds(descriptionArea);
    
    area.removeFromTop(20); // Spacing
    
    // Techniques
    auto techniquesArea = area.removeFromTop(fieldHeight);
    techniquesLabel.setBounds(techniquesArea.removeFromLeft(labelWidth));
    auto toggleWidth = (techniquesArea.getWidth() - 20) / 3;
    legatoToggle.setBounds(techniquesArea.removeFromLeft(toggleWidth));
    techniquesArea.removeFromLeft(10); // Spacing
    staccatoToggle.setBounds(techniquesArea.removeFromLeft(toggleWidth));
    techniquesArea.removeFromLeft(10); // Spacing
    chordToggle.setBounds(techniquesArea);
    
    area.removeFromTop(20); // Spacing
    
    // Pitch range
    auto pitchRangeArea = area.removeFromTop(fieldHeight);
    pitchRangeLabel.setBounds(pitchRangeArea.removeFromLeft(labelWidth));
    auto sliderArea = pitchRangeArea.reduced(0, 2);
    lowPitchValueLabel.setBounds(sliderArea.removeFromLeft(50));
    lowPitchSlider.setBounds(sliderArea.removeFromLeft((sliderArea.getWidth() - 100) / 2));
    highPitchValueLabel.setBounds(sliderArea.removeFromRight(50));
    highPitchSlider.setBounds(sliderArea);
    
    area.removeFromTop(20); // Spacing
    
    // Buttons
    auto buttonHeight = 30;
    auto buttonArea = area.removeFromBottom(buttonHeight);
    auto buttonWidth = 100;
    
    cancelButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(10); // Spacing
    saveButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
}

void PlayerEditorComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    g.setColour(Colours::black);
    Font f;
    f = f.withHeight(15.0f).withStyle(Font::bold);
    g.setFont(f);
    g.drawText("Player Details", getLocalBounds().reduced(10).removeFromTop(30),
               Justification::centredLeft, true);
    
    g.drawLine(10, 40, getWidth() - 10, 40, 1.0f);
}

void PlayerEditorComponent::buttonClicked(Button* button)
{
    if (button == &saveButton)
    {
        // Update player data from UI
        player->metadata.name = nameEditor.getText();
        player->metadata.category = categoryCombo.getText();
        player->metadata.description = descriptionEditor.getText();
        player->metadata.lastModified = Time::getCurrentTime();
        
        // Update capabilities
        player->capabilities.pitchRange = {
            static_cast<int>(lowPitchSlider.getValue()),
            static_cast<int>(highPitchSlider.getValue())
        };
        
        player->capabilities.techniques.clear();
        if (legatoToggle.getToggleState())
            player->capabilities.techniques.add("legato");
        if (staccatoToggle.getToggleState())
            player->capabilities.techniques.add("staccato");
        if (chordToggle.getToggleState())
            player->capabilities.techniques.add("chord");
        
        // Notify listeners
        listeners.call(&Listener::playerEditorSaveRequested, this);
    }
    else if (button == &cancelButton)
    {
        // Notify listeners
        listeners.call(&Listener::playerEditorCancelRequested, this);
    }
}

std::unique_ptr<PlayerManager::Player> PlayerEditorComponent::getPlayer()
{
    return std::move(player);
}

void PlayerEditorComponent::updateSliderLabels()
{
    lowPitchValueLabel.setText(getMidiNoteName(static_cast<int>(lowPitchSlider.getValue())), dontSendNotification);
    highPitchValueLabel.setText(getMidiNoteName(static_cast<int>(highPitchSlider.getValue())), dontSendNotification);
}

String PlayerEditorComponent::getMidiNoteName(int midiNote)
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    String name = noteNames[midiNote % 12];
    int octave = (midiNote / 12) - 1;  // MIDI note 0 is C-1
    
    return name + String(octave);
}