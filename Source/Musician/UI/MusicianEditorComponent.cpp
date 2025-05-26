// Source/Musician/UI/MusicianEditorComponent.cpp
#include "MusicianEditorComponent.h"
#include "../../LLM/Core/ClaudeClient.h"
#include <iostream>

namespace covertracks {

MusicianEditorComponent::MusicianEditorComponent(Musician* musicianPtr, std::shared_ptr<LLMClient> llmClientPtr)
    : styleLabel("StyleLabel", "Style:"),
      apiKeyLabel("APIKeyLabel", "API Key:"),
      connectButton("Connect"),
      disconnectButton("Disconnect"),
      saveButton("Save Settings"),
      statusLabel("StatusLabel", "Not connected to LLM"),
      musician(musicianPtr),
      llmClient(llmClientPtr),
      connected(false)
{
    // Set up style label and combo box
    styleLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(styleLabel);
    
    styleComboBox.addItem("Jazz", 1);
    styleComboBox.addItem("Rock", 2);
    styleComboBox.addItem("Classical", 3);
    styleComboBox.addItem("Funk", 4);
    styleComboBox.addItem("Blues", 5);
    styleComboBox.addItem("Electronic", 6);
    styleComboBox.addItem("Pop", 7);
    styleComboBox.addListener(this);
    
    // Set the selected style if the musician has one
    if (musician && musician->getStyle().isNotEmpty())
    {
        for (int i = 0; i < styleComboBox.getNumItems(); ++i)
        {
            if (styleComboBox.getItemText(i).equalsIgnoreCase(musician->getStyle()))
            {
                styleComboBox.setSelectedItemIndex(i, dontSendNotification);
                break;
            }
        }
    }
    else
    {
        styleComboBox.setSelectedId(1); // Default to Jazz
    }
    
    addAndMakeVisible(styleComboBox);
    
    // Set up API key label and editor
    apiKeyLabel.setJustificationType(Justification::centredRight);
    addAndMakeVisible(apiKeyLabel);
    
    apiKeyEditor.setMultiLine(false);
    apiKeyEditor.setPasswordCharacter('*');
    apiKeyEditor.setText("Insert your Claude API key here");
    addAndMakeVisible(apiKeyEditor);
    
    // Set up buttons
    connectButton.addListener(this);
    addAndMakeVisible(connectButton);
    
    disconnectButton.addListener(this);
    disconnectButton.setEnabled(false);
    addAndMakeVisible(disconnectButton);
    
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
    // Set up status label
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(statusLabel);
    
    // Update the connection status if we have a musician
    if (musician)
    {
        connected = musician->isConnectedToLLM();
        updateConnectionStatus();
    }
    
    // Listen for changes from the musician
    if (musician)
        musician->addChangeListener(this);
}

MusicianEditorComponent::~MusicianEditorComponent()
{
    // Remove listeners
    styleComboBox.removeListener(this);
    connectButton.removeListener(this);
    disconnectButton.removeListener(this);
    saveButton.removeListener(this);
    
    if (musician)
        musician->removeChangeListener(this);
}

void MusicianEditorComponent::paint(Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void MusicianEditorComponent::resized()
{
    auto area = getLocalBounds();
    
    // Top controls
    auto topArea = area.removeFromTop(100);
    
    // Style selector
    auto styleArea = topArea.removeFromTop(30).reduced(5);
    styleLabel.setBounds(styleArea.removeFromLeft(80));
    styleComboBox.setBounds(styleArea.reduced(5, 0));
    
    // API key editor
    auto apiKeyArea = topArea.removeFromTop(30).reduced(5);
    apiKeyLabel.setBounds(apiKeyArea.removeFromLeft(80));
    apiKeyEditor.setBounds(apiKeyArea.reduced(5, 0));
    
    // Buttons
    auto buttonArea = topArea.removeFromTop(30).reduced(5);
    int buttonWidth = (buttonArea.getWidth() - 20) / 3;
    
    connectButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5, 0));
    buttonArea.removeFromLeft(10);
    disconnectButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5, 0));
    buttonArea.removeFromLeft(10);
    saveButton.setBounds(buttonArea.reduced(5, 0));
    
    // Status label
    statusLabel.setBounds(topArea.reduced(5));
    
    // Main area is now available for other components
}

void MusicianEditorComponent::buttonClicked(Button* button)
{
    if (button == &connectButton)
    {
        handleConnect();
    }
    else if (button == &disconnectButton)
    {
        handleDisconnect();
    }
    else if (button == &saveButton)
    {
        handleSave();
    }
}

void MusicianEditorComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &styleComboBox)
    {
        // Update the musician's style
        if (musician)
        {
            musician->setStyle(styleComboBox.getText());
        }
    }
}

void MusicianEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == musician)
    {
        // Update the connection status
        connected = musician->isConnectedToLLM();
        updateConnectionStatus();
    }
}

void MusicianEditorComponent::updateConnectionStatus()
{
    if (connected)
    {
        statusLabel.setText("Connected to " + musician->getLLMServiceName(), dontSendNotification);
        statusLabel.setColour(Label::textColourId, Colours::green);
        connectButton.setEnabled(false);
        disconnectButton.setEnabled(true);
    }
    else
    {
        statusLabel.setText("Not connected to LLM", dontSendNotification);
        statusLabel.setColour(Label::textColourId, Colours::grey);
        connectButton.setEnabled(true);
        disconnectButton.setEnabled(false);
    }
}

void MusicianEditorComponent::handleConnect()
{
    // Get the API key
    String apiKey = apiKeyEditor.getText().trim();
    
    if (apiKey.isEmpty())
    {
        statusLabel.setText("API key cannot be empty", dontSendNotification);
        statusLabel.setColour(Label::textColourId, Colours::red);
        return;
    }
    
    // Create a new Claude client
    auto claudeClient = std::make_shared<ClaudeClient>(apiKey);
    
    // Store the client
    llmClient = claudeClient;
    
    // Connect the musician to the LLM
    if (musician)
    {
        musician->setLLMClient(llmClient);
        
        // Update the UI
        resized();
    }
    
    // Update the connection status
    connected = true;
    updateConnectionStatus();
}

void MusicianEditorComponent::handleDisconnect()
{
    // Disconnect the musician from the LLM
    if (musician)
    {
        musician->setLLMClient(nullptr);
    }
    
    // Clear the LLM client
    llmClient.reset();
    
    // Update the conversation component
    // No longer using conversation component
    
    // Update the connection status
    connected = false;
    updateConnectionStatus();
}

void MusicianEditorComponent::handleSave()
{
    // Save the musician's settings
    if (musician)
    {
        musician->setStyle(styleComboBox.getText());
        
        // Notify that settings were saved
        statusLabel.setText("Settings saved", dontSendNotification);
        statusLabel.setColour(Label::textColourId, Colours::green);
        
        // Reset the status after a delay
        Timer::callAfterDelay(2000, [this]() {
            updateConnectionStatus();
        });
    }
}

} // namespace covertracks