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
    patternEditor.setText("[SONG:\n  <VERSE: {NEW: [4/4: C3 E3 G3 C4]}>\n  <CHORUS: {NEW: [4/4: F3 A3 C4 F4]}>\n]");
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
        if (musician && currentSong)
        {
            var events = musician->convertNotationToMidiEvents(currentPatternNotation, currentSong->getTempo());
            musician->playPattern(events, currentSong->getTempo());
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
    
    if (musician)
    {
        // Parse the pattern
        PatternParser parser(musician->getPatternSystem());
        currentSong = parser.parseLLMResponse(currentPatternNotation);
        
        // Update the visualization
        updateVisualization();
    }
}

void PatternEditorComponent::updateVisualization()
{
    // Clear any existing content
    visualizationPanel.removeAllChildren();
    
    // Create a visualizer component to render the song structure
    class PatternVisualizer : public Component 
    {
    public:
        PatternVisualizer(std::shared_ptr<Song> songPtr) : song(songPtr) {}
        
        void paint(Graphics& g) override 
        {
            g.fillAll(Colours::white);
            
            if (!song)
                return;
            
            // Draw song structure
            Font font(14.0f);
            g.setFont(font);
            g.setColour(Colours::black);
            
            String songName = song->getTitle().isEmpty() ? "Untitled Song" : song->getTitle();
            String tempoText = "Tempo: " + String(song->getTempo()) + " BPM";
            
            g.drawText(songName, 10, 10, getWidth() - 20, 20, Justification::centredLeft);
            g.drawText(tempoText, 10, 30, getWidth() - 20, 20, Justification::centredLeft);
            
            // Draw sections
            int y = 60;
            for (int i = 0; i < song->getNumSections(); ++i)
            {
                auto section = song->getSection(i);
                String sectionName = section->getName().isEmpty() ? "Section " + String(i + 1) : section->getName();
                
                g.setColour(Colours::darkblue);
                g.drawText(sectionName, 10, y, getWidth() - 20, 20, Justification::centredLeft);
                y += 25;
                
                // Draw phrases
                g.setColour(Colours::blue);
                for (int j = 0; j < section->getNumPhrases(); ++j)
                {
                    auto phrase = section->getPhrase(j);
                    String phraseName = phrase->getName().isEmpty() ? "Phrase " + String(j + 1) : phrase->getName();
                    
                    g.drawText(phraseName, 30, y, getWidth() - 40, 20, Justification::centredLeft);
                    y += 20;
                    
                    // Draw bars
                    g.setColour(Colours::black);
                    for (int k = 0; k < phrase->getNumBars(); ++k)
                    {
                        auto bar = phrase->getBar(k);
                        String barText = "Bar " + String(k + 1) + " (" + String(bar->getTimeSignatureNumerator()) + "/" + String(bar->getTimeSignatureDenominator()) + ")";
                        
                        g.drawText(barText, 50, y, getWidth() - 60, 20, Justification::centredLeft);
                        y += 20;
                        
                        // Draw events (limit to first 5 for brevity)
                        g.setColour(Colours::grey);
                        // Since we can't access the private events array directly,
                        // just display a generic message about the bar
                        String eventText = "Time Signature: " + String(bar->getTimeSignatureNumerator()) + "/" + String(bar->getTimeSignatureDenominator());
                        g.drawText(eventText, 70, y, getWidth() - 80, 20, Justification::centredLeft);
                        y += 15;
                        
                        // Add a placeholder message about events
                        g.drawText("Contains MIDI events (not shown)", 
                                      70, y, getWidth() - 80, 20, Justification::centredLeft);
                        y += 15;
                    }
                    
                    y += 5; // Extra spacing between bars
                }
                
                y += 5; // Extra spacing between phrases
            }
            
            y += 10; // Extra spacing between sections
        }
        
    private:
        std::shared_ptr<Song> song;
    };
    
    // Create the visualizer
    auto visualizer = new PatternVisualizer(currentSong);
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