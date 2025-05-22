#include "EnsembleEditPanel.h"
#include "../../UI/Styling/AppFonts.h"

namespace covertracks {

// RolesTableModel implementation
EnsembleEditPanel::RolesTableModel::RolesTableModel(EnsembleManager* manager)
    : ensembleManager(manager)
{
}

int EnsembleEditPanel::RolesTableModel::getNumRows()
{
    return ensembleManager ? ensembleManager->getMusicianRoles().size() : 0;
}

void EnsembleEditPanel::RolesTableModel::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected) {
        g.fillAll(juce::Colours::lightblue);
    } else if (rowNumber % 2) {
        g.fillAll(juce::Colours::white);
    } else {
        g.fillAll(juce::Colours::lightgrey.withAlpha(0.2f));
    }
}

void EnsembleEditPanel::RolesTableModel::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    if (!ensembleManager) return;
    
    const auto& roles = ensembleManager->getMusicianRoles();
    if (rowNumber >= roles.size()) return;
    
    const auto& role = roles[rowNumber];
    
    g.setColour(juce::Colours::black);
    
    juce::String text;
    switch (columnId) {
        case 1: // Role name
            text = role.name;
            break;
        case 2: // Instrument
            text = role.instrument;
            break;
        case 3: // Player
            text = role.player ? role.player->getId() : "None";
            break;
        case 4: // Pattern status
            text = role.pattern.isEmpty() ? "Not generated" : "Generated";
            break;
    }
    
    g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft);
}

// EnsembleEditPanel implementation
EnsembleEditPanel::EnsembleEditPanel(PlayerManager* playerManagerPtr, EnsembleManager* ensembleManagerPtr)
    : playerManager(playerManagerPtr),
      ensembleManager(nullptr),
      isGenerating(false),
      progressValue(0.0),
      progressBar(progressValue)
{
    // If no ensemble manager was provided, create a new one
    if (ensembleManagerPtr == nullptr) {
        ownedEnsembleManager = std::make_unique<EnsembleManager>(playerManager);
        ensembleManager = ownedEnsembleManager.get();
    } else {
        ensembleManager = ensembleManagerPtr;
    }
    
    // Set up the title label
    titleLabel.setText("Ensemble Generator", juce::dontSendNotification);
    titleLabel.setFont(AppFonts::getTitleFont());
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up the song description editor
    songDescriptionLabel.setText("Song Description:", juce::dontSendNotification);
    songDescriptionLabel.setFont(AppFonts::getHeaderFont());
    addAndMakeVisible(songDescriptionLabel);
    
    songDescriptionEditor.setMultiLine(true);
    songDescriptionEditor.setReturnKeyStartsNewLine(true);
    songDescriptionEditor.setReadOnly(false);
    songDescriptionEditor.setScrollbarsShown(true);
    songDescriptionEditor.setCaretVisible(true);
    songDescriptionEditor.setPopupMenuEnabled(true);
    songDescriptionEditor.setText("Create a blues rock song with a driving rhythm and soulful lead guitar.");
    addAndMakeVisible(songDescriptionEditor);
    
    // Set up the roles section
    rolesLabel.setText("Musician Roles:", juce::dontSendNotification);
    rolesLabel.setFont(AppFonts::getHeaderFont());
    addAndMakeVisible(rolesLabel);
    
    // Set up the roles table
    rolesTableModel = std::make_unique<RolesTableModel>(ensembleManager);
    rolesTable.setModel(rolesTableModel.get());
    
    rolesTable.getHeader().addColumn("Role", 1, 150);
    rolesTable.getHeader().addColumn("Instrument", 2, 150);
    rolesTable.getHeader().addColumn("Player", 3, 150);
    rolesTable.getHeader().addColumn("Pattern Status", 4, 150);
    
    rolesTable.setHeaderHeight(24);
    rolesTable.setRowHeight(24);
    rolesTable.setMultipleSelectionEnabled(false);
    addAndMakeVisible(rolesTable);
    
    // Set up the role management controls
    addRoleButton.setButtonText("Add Role");
    addRoleButton.addListener(this);
    addAndMakeVisible(addRoleButton);
    
    removeRoleButton.setButtonText("Remove Role");
    removeRoleButton.addListener(this);
    addAndMakeVisible(removeRoleButton);
    
    // Set up the combo boxes
    roleNameComboBox.addItem("Lead Guitar", 1);
    roleNameComboBox.addItem("Rhythm Guitar 1", 2);
    roleNameComboBox.addItem("Rhythm Guitar 2", 3);
    roleNameComboBox.addItem("Bass Guitar", 4);
    roleNameComboBox.addItem("Drums", 5);
    roleNameComboBox.addItem("Keyboard", 6);
    roleNameComboBox.addListener(this);
    roleNameComboBox.setSelectedId(1);
    addAndMakeVisible(roleNameComboBox);
    
    instrumentComboBox.addItem("Guitar", 1);
    instrumentComboBox.addItem("Bass", 2);
    instrumentComboBox.addItem("Drums", 3);
    instrumentComboBox.addItem("Keyboard", 4);
    instrumentComboBox.addListener(this);
    instrumentComboBox.setSelectedId(1);
    addAndMakeVisible(instrumentComboBox);
    
    // Populate the player combo box
    updatePlayerComboBox();
    playerComboBox.addListener(this);
    addAndMakeVisible(playerComboBox);
    
    // Set up the action buttons
    generateButton.setButtonText("Generate Patterns");
    generateButton.addListener(this);
    addAndMakeVisible(generateButton);
    
    playButton.setButtonText("Play Ensemble");
    playButton.addListener(this);
    addAndMakeVisible(playButton);
    
    stopButton.setButtonText("Stop");
    stopButton.addListener(this);
    addAndMakeVisible(stopButton);
    
    // Set up the progress bar
    progressBar.setPercentageDisplay(false);
    progressBar.setTextToDisplay("Ready");
    addAndMakeVisible(progressBar);
    
    // Update the UI
    updateUI();
}

EnsembleEditPanel::~EnsembleEditPanel()
{
    if (ensembleManager) {
        ensembleManager->stopPlaying();
    }
}

void EnsembleEditPanel::setEnsemble(EnsembleManager* ensembleManagerPtr)
{
    // Clear any owned ensemble manager
    ownedEnsembleManager.reset();
    
    // Set the new ensemble manager
    ensembleManager = ensembleManagerPtr;
    
    // Update the UI with the new ensemble data
    if (ensembleManager) {
        nameEditor.setText(ensembleManager->getName(), false);
        descriptionEditor.setText(ensembleManager->getDescription(), false);
        songDescriptionEditor.setText(ensembleManager->getCurrentSongDescription(), false);
    }
    
    // Update the roles table model
    rolesTableModel = std::make_unique<RolesTableModel>(ensembleManager);
    rolesTable.setModel(rolesTableModel.get());
    
    // Update the UI
    updateUI();
}

void EnsembleEditPanel::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void EnsembleEditPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title
    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);
    
    // Song description
    auto descriptionArea = area.removeFromTop(120);
    songDescriptionLabel.setBounds(descriptionArea.removeFromTop(20));
    songDescriptionEditor.setBounds(descriptionArea);
    area.removeFromTop(10);
    
    // Roles table and controls
    auto rolesArea = area.removeFromTop(200);
    rolesLabel.setBounds(rolesArea.removeFromTop(20));
    
    auto roleControlsArea = rolesArea.removeFromBottom(30);
    removeRoleButton.setBounds(roleControlsArea.removeFromRight(120));
    roleControlsArea.removeFromRight(10);
    addRoleButton.setBounds(roleControlsArea.removeFromRight(120));
    
    rolesTable.setBounds(rolesArea);
    area.removeFromTop(10);
    
    // Role selection controls
    auto selectionArea = area.removeFromTop(30);
    auto thirdWidth = selectionArea.getWidth() / 3;
    
    roleNameComboBox.setBounds(selectionArea.removeFromLeft(thirdWidth));
    selectionArea.removeFromLeft(10);
    instrumentComboBox.setBounds(selectionArea.removeFromLeft(thirdWidth));
    selectionArea.removeFromLeft(10);
    playerComboBox.setBounds(selectionArea);
    area.removeFromTop(10);
    
    // Action buttons
    auto actionArea = area.removeFromTop(30);
    auto actionWidth = actionArea.getWidth() / 3;
    
    generateButton.setBounds(actionArea.removeFromLeft(actionWidth));
    actionArea.removeFromLeft(10);
    playButton.setBounds(actionArea.removeFromLeft(actionWidth));
    actionArea.removeFromLeft(10);
    stopButton.setBounds(actionArea);
    area.removeFromTop(10);
    
    // Progress bar
    progressBar.setBounds(area.removeFromTop(30));
}

void EnsembleEditPanel::buttonClicked(juce::Button* button)
{
    if (button == &addRoleButton) {
        addMusicianRole();
    } else if (button == &removeRoleButton) {
        removeMusicianRole();
    } else if (button == &generateButton) {
        generatePatterns();
    } else if (button == &playButton) {
        startPlaying();
    } else if (button == &stopButton) {
        stopPlaying();
    }
}

void EnsembleEditPanel::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &roleNameComboBox) {
        // Update the instrument combo box based on the selected role
        int roleId = roleNameComboBox.getSelectedId();
        
        if (roleId == 1 || roleId == 2 || roleId == 3) { // Guitar roles
            instrumentComboBox.setSelectedId(1);
        } else if (roleId == 4) { // Bass role
            instrumentComboBox.setSelectedId(2);
        } else if (roleId == 5) { // Drums role
            instrumentComboBox.setSelectedId(3);
        } else if (roleId == 6) { // Keyboard role
            instrumentComboBox.setSelectedId(4);
        }
    }
}

void EnsembleEditPanel::updateUI()
{
    // Update the roles table
    rolesTable.updateContent();
    rolesTable.repaint();
    
    // Update button states
    bool hasRoles = rolesTable.getNumRows() > 0;
    bool isPlaying = ensembleManager->isPlaying();
    
    generateButton.setEnabled(hasRoles && !isGenerating && !isPlaying);
    playButton.setEnabled(hasRoles && !isGenerating && !isPlaying);
    stopButton.setEnabled(isPlaying);
    removeRoleButton.setEnabled(hasRoles && rolesTable.getSelectedRow() >= 0);
    
    // Update the progress bar
    if (isGenerating) {
        progressBar.setTextToDisplay("Generating patterns...");
    } else if (isPlaying) {
        progressBar.setTextToDisplay("Playing ensemble");
    } else {
        progressBar.setTextToDisplay("Ready");
    }
}

void EnsembleEditPanel::updatePlayerComboBox()
{
    playerComboBox.clear();
    
    // Add a "None" option
    playerComboBox.addItem("None", 1);
    
    // Add all available players
    if (playerManager) {
        int id = 2;
        for (auto* player : playerManager->getPlayers()) {
            // Use getId() instead of getName() since Player doesn't have getName()
            playerComboBox.addItem(player->getId(), id++);
        }
    }
    
    playerComboBox.setSelectedId(1);
}

void EnsembleEditPanel::addMusicianRole()
{
    // Get the selected role, instrument, and player
    juce::String roleName = roleNameComboBox.getText();
    juce::String instrumentType = instrumentComboBox.getText();
    
    Player* player = nullptr;
    int playerId = playerComboBox.getSelectedId();
    if (playerId > 1 && playerManager) {
        // Adjust for the "None" option
        player = playerManager->getPlayers()[playerId - 2];
    }
    
    // Add the role to the ensemble
    if (ensembleManager->addMusicianRole(roleName, instrumentType, player)) {
        // Update the UI
        updateUI();
    }
}

void EnsembleEditPanel::removeMusicianRole()
{
    // Get the selected role
    int selectedRow = rolesTable.getSelectedRow();
    if (selectedRow < 0) return;
    
    const auto& roles = ensembleManager->getMusicianRoles();
    if (selectedRow >= roles.size()) return;
    
    // Remove the role
    if (ensembleManager->removeMusicianRole(roles[selectedRow].name)) {
        // Update the UI
        updateUI();
    }
}

void EnsembleEditPanel::generatePatterns()
{
    // Get the song description
    juce::String songDescription = songDescriptionEditor.getText();
    if (songDescription.isEmpty()) {
        // Show an error message
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                             "Error",
                                             "Please enter a song description.");
        return;
    }
    
    // Update the UI
    isGenerating = true;
    updateUI();
    
    // Generate the patterns
    ensembleManager->generateEnsemblePatterns(songDescription, [this](bool success) {
        // Update the UI
        juce::MessageManager::callAsync([this, success]() {
            isGenerating = false;
            
            if (success) {
                // Show a success message
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Success",
                                                     "Patterns generated successfully.");
            } else {
                // Show an error message
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                     "Error",
                                                     "Failed to generate patterns.");
            }
            
            updateUI();
        });
    });
}

void EnsembleEditPanel::startPlaying()
{
    if (ensembleManager->startPlaying()) {
        updateUI();
    } else {
        // Show an error message
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                             "Error",
                                             "Failed to start playing.");
    }
}

void EnsembleEditPanel::stopPlaying()
{
    ensembleManager->stopPlaying();
    updateUI();
}

} // namespace covertracks
