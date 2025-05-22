// Source/Musician/UI/PatternEditorComponent.cpp
#include "PatternEditorComponent.h"
#include <iostream>

namespace covertracks {

PatternEditorComponent::PatternEditorComponent(Musician* musicianPtr)
    : headerLabel("HeaderLabel", "Pattern Editor"),
      playButton("Play"),
      editButton("Edit"),
      parseButton("Parse"),
      generateButton("Generate"),
      musician(musicianPtr)
{
    // Set up header label
    headerLabel.setFont(Font(18.0f, Font::bold));
    headerLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(headerLabel);
    
    // Set up pattern editor
    patternEditor.setMultiLine(true);
    patternEditor.setReturnKeyStartsNewLine(true);
    patternEditor.setReadOnly(false);
    patternEditor.setScrollbarsShown(true);
    patternEditor.setCaretVisible(true);
    patternEditor.setPopupMenuEnabled(true);
    patternEditor.setText(""); // Start with an empty editor instead of a hardcoded pattern
    addAndMakeVisible(patternEditor);
    
    // Set up buttons
    playButton.addListener(this);
    addAndMakeVisible(playButton);
    
    editButton.addListener(this);
    addAndMakeVisible(editButton);
    
    parseButton.addListener(this);
    addAndMakeVisible(parseButton);
    
    generateButton.addListener(this);
    addAndMakeVisible(generateButton);
    
    // Set up visualization panel
    addAndMakeVisible(visualizationPanel);
    
    // Parse the initial pattern
    parseCurrentPattern();
    
    // Listen for changes from the musician
    if (musician)
        musician->addChangeListener(this);
}

PatternEditorComponent::~PatternEditorComponent()
{
    // Remove listeners
    playButton.removeListener(this);
    editButton.removeListener(this);
    parseButton.removeListener(this);
    generateButton.removeListener(this);
    
    if (musician)
        musician->removeChangeListener(this);
    
    // Explicitly clear all child components to prevent leaks
    visualizationPanel.removeAllChildren();
}

void PatternEditorComponent::paint(Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);
    
    // Draw the visualization area
    auto visualizationArea = visualizationPanel.getBounds();
    g.setColour(Colours::lightgrey);
    g.fillRect(visualizationArea);
    g.setColour(Colours::grey);
    g.drawRect(visualizationArea, 1);
}

void PatternEditorComponent::resized()
{
    auto area = getLocalBounds();
    
    // Header at the top
    headerLabel.setBounds(area.removeFromTop(30));
    
    // Buttons at the bottom
    auto buttonArea = area.removeFromBottom(40);
    int buttonWidth = buttonArea.getWidth() / 4;
    
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    parseButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    generateButton.setBounds(buttonArea.reduced(5));
    
    // Split remaining area between the editor and visualization
    auto editorArea = area.withWidth(area.getWidth() / 2);
    auto visArea = area.withX(editorArea.getRight());
    
    patternEditor.setBounds(editorArea.reduced(5));
    visualizationPanel.setBounds(visArea.reduced(5));
}

void PatternEditorComponent::buttonClicked(Button* button)
{
    if (button == &playButton)
    {
        // Notify listeners that playback is requested
        listeners.call(&Listener::patternPlayRequested, this);
        
        // Try to play the pattern via the musician
        if (musician && !currentPatternNotation.isEmpty())
        {
            // Use a default tempo of 120 BPM if not specified in the pattern
            double tempo = 120.0;
            
            // Let the musician handle the pattern parsing and playback
            var events = musician->convertNotationToMidiEvents(currentPatternNotation, tempo);
            musician->playPattern(events, tempo);
        }
    }
    else if (button == &editButton)
    {
        // Notify listeners that editing is requested
        listeners.call(&Listener::patternEditRequested, this);
    }
    else if (button == &parseButton)
    {
        // Parse the current pattern
        parseCurrentPattern();
    }
    else if (button == &generateButton)
    {
        // Notify listeners that generation is requested
        listeners.call(&Listener::patternGenerateRequested, this);
    }
}

void PatternEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == musician)
    {
        // Musician state has changed (e.g., new pattern generated)
        // We can update the UI if needed
    }
}

void PatternEditorComponent::setPattern(const String& patternNotation)
{
    // Set the pattern text
    patternEditor.setText(patternNotation);
    
    // Parse the pattern
    parseCurrentPattern();
}

String PatternEditorComponent::getPatternNotation() const
{
    return patternEditor.getText();
}

void PatternEditorComponent::patternUpdated()
{
    // Parse the current pattern
    parseCurrentPattern();
}

void PatternEditorComponent::parseCurrentPattern()
{
    currentPatternNotation = patternEditor.getText();
    
    // We don't need to parse the pattern here anymore
    // Just update the visualization with the current notation
    updateVisualization();
}

// Define PatternVisualizer as a member class instead of a local class
class PatternEditorComponent::PatternVisualizer : public Component 
{
public:
    PatternVisualizer(const String& patternText) : patternNotation(patternText) {}
    
    void paint(Graphics& g) override 
    {
        g.fillAll(Colours::white);
        
        // Draw pattern notation directly
        Font font(14.0f);
        g.setFont(font);
        g.setColour(Colours::black);
        
        g.drawText("Pattern Notation:", 10, 10, getWidth() - 20, 20, Justification::centredLeft);
        
        // Draw the pattern notation with line breaks
        StringArray lines;
        lines.addLines(patternNotation);
        
        int y = 40;
        for (int i = 0; i < lines.size(); ++i)
        {
            g.drawText(lines[i], 10, y, getWidth() - 20, 20, Justification::centredLeft);
            y += 20;
        }
    }
    
private:
    String patternNotation;
};

void PatternEditorComponent::updateVisualization()
{
    // Clear any existing content
    visualizationPanel.removeAllChildren();
    
    // Create the visualizer - will be owned by visualizationPanel
    auto visualizer = new PatternVisualizer(currentPatternNotation);
    visualizer->setBounds(visualizationPanel.getLocalBounds());
    visualizationPanel.addAndMakeVisible(visualizer);
    
    // Force a repaint
    visualizationPanel.repaint();
}

void PatternEditorComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PatternEditorComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace covertracks