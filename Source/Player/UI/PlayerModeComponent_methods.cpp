void PlayerModeComponent::convertToMidiChart()
{
    if (currentPatternJson.isEmpty() || selectedPlayer == nullptr)
    {
        std::cerr << "PlayerMode::convertToMidiChart - No pattern or player selected" << std::endl;
        return;
    }
    
    std::cout << "Step 1: Preparing pattern..." << std::endl;
    
    // Step 1: Prepare the pattern
    if (!selectedPlayer->preparePattern(currentPatternJson))
    {
        std::cerr << "Failed to prepare pattern" << std::endl;
        patternViewer.setText("ERROR: Failed to prepare pattern!");
        return;
    }
    
    std::cout << "Step 2: Converting to MIDI..." << std::endl;
    
    // Step 2: Convert to MIDI (handled internally by Player)
    if (!selectedPlayer->convertToMidi())
    {
        std::cerr << "Failed to convert to MIDI" << std::endl;
        patternViewer.setText("ERROR: Failed to convert to MIDI!");
        return;
    }
    
    // Mark that we have a MIDI chart ready
    currentMidiChart = juce::var(true); // Just a flag to enable the load button
    
    // Update the pattern viewer to show conversion results
    patternViewer.setText("Pattern successfully converted to MIDI!\n\nClick 'Load' to prepare for playback.");
    
    std::cout << "Conversion successful - ready to load" << std::endl;
    
    // Update button states
    updateButtonStates();
}

void PlayerModeComponent::loadMidiChart()
{
    if (currentMidiChart.isVoid() || selectedPlayer == nullptr)
    {
        std::cerr << "PlayerMode::loadMidiChart - No MIDI chart or player available" << std::endl;
        return;
    }
    
    std::cout << "Step 3: Loading MIDI into instrument..." << std::endl;
    
    // Step 3: Load the prepared MIDI (handled internally by Player)
    if (!selectedPlayer->loadPreparedMidi())
    {
        std::cerr << "Failed to load MIDI" << std::endl;
        patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to load MIDI!");
        return;
    }
    
    // Update pattern viewer
    patternViewer.setText(patternViewer.getText() + "\n\nMIDI loaded successfully! Click 'Play' to start.");
    
    std::cout << "MIDI loaded - ready to play" << std::endl;
    
    // Update button states
    updateButtonStates();
}

void PlayerModeComponent::playLoadedChart()
{
    std::cout << "🎵 PlayerModeComponent::playLoadedChart() called!" << std::endl;
    
    if (selectedPlayer == nullptr)
    {
        std::cerr << "❌ PlayerMode::playLoadedChart - No player available" << std::endl;
        return;
    }
    
    std::cout << "✅ Starting playback on player..." << std::endl;
    
    // Simply play the loaded chart
    if (selectedPlayer->play())
    {
        // Update button states
        playButton.setEnabled(false);
        stopButton.setEnabled(true);
        
        std::cout << "Started playback on player " << selectedPlayer->getMetadata().name << std::endl;
        
        // Update pattern viewer
        patternViewer.setText(patternViewer.getText() + "\n\nPlayback started!");
    }
    else
    {
        std::cerr << "Failed to start playback - no chart loaded?" << std::endl;
        patternViewer.setText(patternViewer.getText() + "\n\nERROR: Failed to start playback!");
    }
}