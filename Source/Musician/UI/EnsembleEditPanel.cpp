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
            // Ensure we properly display the player name
            if (role.player && role.player->getMetadata().name.isNotEmpty()) {
                text = role.player->getMetadata().name;
            } else {
                text = "None";
            }
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
      ensembleManager(nullptr)
{
    // If no ensemble manager was provided, create a new one
    if (ensembleManagerPtr == nullptr) {
        ownedEnsembleManager = std::make_unique<EnsembleManager>(playerManager);
        ensembleManager = ownedEnsembleManager.get();
    } else {
        ensembleManager = ensembleManagerPtr;
    }
    
    // Set up the title label
    titleLabel.setText("Ensemble Editor", juce::dontSendNotification);
    titleLabel.setFont(AppFonts::getTitleFont());
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up the name editor
    nameLabel.setText("Ensemble Name:", juce::dontSendNotification);
    nameLabel.setFont(AppFonts::getHeaderFont());
    addAndMakeVisible(nameLabel);
    
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.setReadOnly(false);
    nameEditor.setScrollbarsShown(false);
    nameEditor.setCaretVisible(true);
    nameEditor.setPopupMenuEnabled(true);
    nameEditor.setText(ensembleManager->getName());
    addAndMakeVisible(nameEditor);
    
    // Set up the description editor
    descriptionLabel.setText("Ensemble Description:", juce::dontSendNotification);
    descriptionLabel.setFont(AppFonts::getHeaderFont());
    addAndMakeVisible(descriptionLabel);
    
    descriptionEditor.setMultiLine(true);
    descriptionEditor.setReturnKeyStartsNewLine(true);
    descriptionEditor.setReadOnly(false);
    descriptionEditor.setScrollbarsShown(true);
    descriptionEditor.setCaretVisible(true);
    descriptionEditor.setPopupMenuEnabled(true);
    descriptionEditor.setText(ensembleManager->getDescription());
    addAndMakeVisible(descriptionEditor);
    
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
    
    // Set up the role selection controls
    roleNameComboBox.addItem("Lead Guitar", 1);
    roleNameComboBox.addItem("Rhythm Guitar", 2);
    roleNameComboBox.addItem("Solo Guitar", 3);
    roleNameComboBox.addItem("Bass", 4);
    roleNameComboBox.addItem("Drums", 5);
    roleNameComboBox.addItem("Keyboard", 6);
    roleNameComboBox.setEditableText(true);
    roleNameComboBox.setText("Lead Guitar");
    roleNameComboBox.addListener(this);
    addAndMakeVisible(roleNameComboBox);
    
    instrumentComboBox.addItem("Guitar", 1);
    instrumentComboBox.addItem("Bass", 2);
    instrumentComboBox.addItem("Drums", 3);
    instrumentComboBox.addItem("Keyboard", 4);
    instrumentComboBox.setEditableText(true);
    instrumentComboBox.setText("Guitar");
    addAndMakeVisible(instrumentComboBox);
    
    // Set up the player selection
    updatePlayerComboBox();
    playerComboBox.addListener(this);
    addAndMakeVisible(playerComboBox);
    
    // Set up the save button
    saveButton.setButtonText("Save Ensemble");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);
    
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
    
    // Title at the top
    titleLabel.setBounds(area.removeFromTop(40));
    
    // Name editor
    auto nameArea = area.removeFromTop(50);
    nameLabel.setBounds(nameArea.removeFromTop(20));
    nameEditor.setBounds(nameArea);
    
    // Description editor
    auto descriptionArea = area.removeFromTop(100);
    descriptionLabel.setBounds(descriptionArea.removeFromTop(20));
    descriptionEditor.setBounds(descriptionArea);
    
    // Add a gap
    area.removeFromTop(10);
    
    // Roles section
    auto rolesArea = area.removeFromTop(300);
    rolesLabel.setBounds(rolesArea.removeFromTop(20));
    
    // Roles table
    auto tableArea = rolesArea.removeFromTop(150);
    rolesTable.setBounds(tableArea);
    
    // Role controls
    auto roleControlsArea = rolesArea.removeFromTop(30);
    auto buttonWidth = roleControlsArea.getWidth() / 2;
    addRoleButton.setBounds(roleControlsArea.removeFromLeft(buttonWidth).reduced(5, 0));
    removeRoleButton.setBounds(roleControlsArea.reduced(5, 0));
    
    // Role editor
    auto roleEditorArea = rolesArea.removeFromTop(30);
    auto editorWidth = roleEditorArea.getWidth() / 3;
    roleNameComboBox.setBounds(roleEditorArea.removeFromLeft(editorWidth).reduced(5, 0));
    instrumentComboBox.setBounds(roleEditorArea.removeFromLeft(editorWidth).reduced(5, 0));
    playerComboBox.setBounds(roleEditorArea.reduced(5, 0));
    
    // Add a gap
    area.removeFromTop(10);
    
    // Save button
    saveButton.setBounds(area.removeFromTop(30).reduced(5, 0));
}

void EnsembleEditPanel::buttonClicked(juce::Button* button)
{
    if (button == &addRoleButton) {
        addMusicianRole();
    } else if (button == &removeRoleButton) {
        removeMusicianRole();
    } else if (button == &saveButton) {
        saveEnsembleData();
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
    
    // Update the button states
    removeRoleButton.setEnabled(rolesTable.getSelectedRow() >= 0);
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
            playerComboBox.addItem(player->getMetadata().name, id++);
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
    
    // Store the role name before removal
    juce::String roleName = roles[selectedRow].name;
    
    // Remove the role
    if (ensembleManager->removeMusicianRole(roleName)) {
        // Update the UI
        updateUI();
        
        // If there are still roles, select an appropriate row
        if (ensembleManager->getMusicianRoles().size() > 0) {
            int newSelectedRow = juce::jmin(selectedRow, ensembleManager->getMusicianRoles().size() - 1);
            rolesTable.selectRow(newSelectedRow);
        } else {
            // No roles left, ensure remove button is disabled
            removeRoleButton.setEnabled(false);
        }
    }
}

void EnsembleEditPanel::saveEnsembleData()
{
    if (!ensembleManager) return;
    
    // Save the ensemble data in memory
    ensembleManager->setName(nameEditor.getText());
    ensembleManager->setDescription(descriptionEditor.getText());
    
    // Create the file to save to
    juce::File ensemblesDir = EnsembleManager::getEnsemblesDirectory();
    juce::File ensembleFile = ensemblesDir.getChildFile(ensembleManager->getName() + ".ensemble");
    
    // Save to disk
    bool saveSuccess = ensembleManager->saveToFile(ensembleFile);
    
    // Call the save callback if provided
    if (saveCallback) {
        saveCallback();
    }
    
    // Show a confirmation message
    if (saveSuccess) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                             "Ensemble Saved",
                                             "Ensemble data has been saved to:\n" + ensembleFile.getFullPathName());
    } else {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                             "Save Failed",
                                             "Failed to save ensemble data to:\n" + ensembleFile.getFullPathName());
    }
}

} // namespace covertracks
