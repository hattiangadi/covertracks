#include "EnsembleListPanel.h"
#include "../../UI/Styling/AppFonts.h"

namespace covertracks {

EnsembleListPanel::EnsembleListPanel(PlayerManager* playerManagerPtr)
    : playerManager(playerManagerPtr)
{
    // Set up the title label
    titleLabel.setText("Ensemble Library", juce::dontSendNotification);
    titleLabel.setFont(AppFonts::getTitleFont());
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up the ensemble list box
    ensembleListBox.setModel(this);
    ensembleListBox.setRowHeight(30);
    ensembleListBox.setMultipleSelectionEnabled(false);
    addAndMakeVisible(ensembleListBox);
    
    // Set up the buttons
    addButton.setButtonText("Add Ensemble");
    addButton.addListener(this);
    addAndMakeVisible(addButton);
    
    removeButton.setButtonText("Remove Ensemble");
    removeButton.addListener(this);
    addAndMakeVisible(removeButton);
    
    editButton.setButtonText("Edit Ensemble");
    editButton.addListener(this);
    addAndMakeVisible(editButton);
    
    // Load saved ensembles from disk
    loadEnsemblesFromDisk();
    
    // Log the number of ensembles loaded
    std::cout << "[EnsembleListPanel] Loaded " << ensembles.size() << " ensemble(s) from disk" << std::endl;
    
    // Update the UI
    refreshEnsembleList();
}

EnsembleListPanel::~EnsembleListPanel()
{
}

void EnsembleListPanel::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void EnsembleListPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title at the top
    titleLabel.setBounds(area.removeFromTop(40));
    
    // Buttons at the bottom
    auto buttonArea = area.removeFromBottom(40);
    auto buttonWidth = buttonArea.getWidth() / 3;
    
    addButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5, 0));
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5, 0));
    removeButton.setBounds(buttonArea.reduced(5, 0));
    
    // List box in the middle
    ensembleListBox.setBounds(area.reduced(0, 10));
}

void EnsembleListPanel::buttonClicked(juce::Button* button)
{
    if (button == &addButton) {
        addEnsemble();
    } else if (button == &removeButton) {
        removeEnsemble();
    } else if (button == &editButton) {
        auto selectedEnsemble = getSelectedEnsemble();
        if (selectedEnsemble && ensembleEditCallback) {
            ensembleEditCallback(selectedEnsemble);
        }
    }
}

int EnsembleListPanel::getNumRows()
{
    return ensembles.size();
}

void EnsembleListPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int /*height*/, bool rowIsSelected)
{
    if (rowNumber >= 0 && rowNumber < ensembles.size()) {
        if (rowIsSelected) {
            g.fillAll(juce::Colours::lightblue);
        } else if (rowNumber % 2) {
            g.fillAll(juce::Colours::white);
        } else {
            g.fillAll(juce::Colours::lightgrey.withAlpha(0.2f));
        }
        
        const auto& ensemble = ensembles[rowNumber];
        
        g.setColour(juce::Colours::black);
        g.setFont(AppFonts::getHeaderFont());
        g.drawText(ensemble->getName(), 10, 2, width - 20, 20, juce::Justification::centredLeft, true);
        
        g.setFont(AppFonts::getSmallFont());
        g.drawText(ensemble->getDescription(), 10, 24, width - 20, 20, juce::Justification::centredLeft, true);
    }
}

void EnsembleListPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < ensembles.size() && ensembleEditCallback) {
        ensembleEditCallback(ensembles[row]);
    }
}

EnsembleManager* EnsembleListPanel::getSelectedEnsemble()
{
    int selectedRow = ensembleListBox.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < ensembles.size()) {
        return ensembles[selectedRow];
    }
    return nullptr;
}

void EnsembleListPanel::setEnsembleEditCallback(std::function<void(EnsembleManager*)> callback)
{
    ensembleEditCallback = std::move(callback);
}

void EnsembleListPanel::addEnsemble()
{
    // Create a new empty ensemble
    auto newEnsemble = new EnsembleManager(playerManager, nullptr);
    newEnsemble->setName("New Ensemble " + juce::String(ensembles.size() + 1));
    newEnsemble->setDescription("A new ensemble");
    
    // Add it to the list
    ensembles.add(newEnsemble);
    
    // Update the UI
    refreshEnsembleList();
    
    // Select the new ensemble
    ensembleListBox.selectRow(ensembles.size() - 1);
    
    // Edit the new ensemble
    if (ensembleEditCallback) {
        ensembleEditCallback(ensembles.getLast());
    }
}

void EnsembleListPanel::removeEnsemble()
{
    int selectedRow = ensembleListBox.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < ensembles.size()) {
        // Get the ensemble to remove
        EnsembleManager* ensemble = ensembles[selectedRow];
        
        // Delete the file if it exists
        juce::File ensembleFile = EnsembleManager::getEnsemblesDirectory()
                                .getChildFile(ensemble->getName() + ".ensemble");
        if (ensembleFile.exists()) {
            ensembleFile.deleteFile();
        }
        
        // Remove from the list
        ensembles.remove(selectedRow);
        refreshEnsembleList();
    }
}

void EnsembleListPanel::refreshEnsembleList()
{
    ensembleListBox.updateContent();
    ensembleListBox.repaint();
}

void EnsembleListPanel::loadEnsemblesFromDisk()
{
    // Clear existing ensembles
    ensembles.clear();
    
    // Get the ensembles directory
    juce::File ensemblesDir = EnsembleManager::getEnsemblesDirectory();
    if (!ensemblesDir.exists()) {
        ensemblesDir.createDirectory();
        return;
    }
    
    // Get all ensemble files
    juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    // Load each ensemble
    for (const auto& file : ensembleFiles)
    {
        std::cout << "[EnsembleListPanel] Loading ensemble from: " << file.getFullPathName() << std::endl;
        
        auto ensemble = EnsembleManager::loadFromFile(file, playerManager);
        if (ensemble)
        {
            ensembles.add(ensemble.release()); // Transfer ownership to the OwnedArray
            std::cout << "[EnsembleListPanel] Loaded ensemble: " << ensembles.getLast()->getName() << std::endl;
        }
        else
        {
            std::cout << "[EnsembleListPanel] Failed to load ensemble from: " << file.getFullPathName() << std::endl;
        }
    }
}

} // namespace covertracks
