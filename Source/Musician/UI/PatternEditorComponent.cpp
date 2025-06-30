// Source/Musician/UI/PatternEditorComponent.cpp
#include "PatternEditorComponent.h"
#include <iostream>

namespace covertracks {

PatternEditorComponent::PatternEditorComponent(PlayerManager* playerMgr)
    : headerLabel("HeaderLabel", "Pattern Editor"),
      performanceContextLabel("PerformanceContextLabel", ""),
      playerLabel("PlayerLabel", "Player:"),
      parseButton("Parse"),
      loadButton("Load"),
      playButton("Play"),
      stopButton("Stop"),
      playerManager(playerMgr),
      selectedPlayer(nullptr)
{
    // Set up header label
    headerLabel.setFont(Font(FontOptions().withHeight(18.0f).withStyle("bold")));
    headerLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(headerLabel);
    
    // Set up performance context label (initially hidden)
    performanceContextLabel.setFont(Font(FontOptions().withHeight(14.0f)));
    performanceContextLabel.setJustificationType(Justification::centred);
    performanceContextLabel.setColour(Label::textColourId, Colours::darkgrey);
    performanceContextLabel.setVisible(false);
    addAndMakeVisible(performanceContextLabel);
    
    // Set up pattern editor
    patternEditor.setMultiLine(true);
    patternEditor.setReturnKeyStartsNewLine(true);
    patternEditor.setReadOnly(false);
    patternEditor.setScrollbarsShown(true);
    patternEditor.setCaretVisible(true);
    patternEditor.setPopupMenuEnabled(true);
    patternEditor.setText(""); // Start with an empty editor instead of a hardcoded pattern
    addAndMakeVisible(patternEditor);
    
    // Set up player selection
    playerLabel.setFont(Font(FontOptions().withHeight(14.0f)));
    addAndMakeVisible(playerLabel);
    
    playerSelector.addListener(this);
    addAndMakeVisible(playerSelector);
    updatePlayerSelector();
    
    // Set up buttons with clearer labels for new workflow
    parseButton.setButtonText("1. Parse + Convert");
    parseButton.addListener(this);
    addAndMakeVisible(parseButton);
    
    loadButton.setButtonText("2. Load to Player");
    loadButton.addListener(this);
    addAndMakeVisible(loadButton);
    
    playButton.setButtonText("3. Play");
    playButton.addListener(this);
    addAndMakeVisible(playButton);
    
    stopButton.addListener(this);
    addAndMakeVisible(stopButton);
    
    // Set up one-click button (new state machine workflow)
    oneClickButton.setButtonText("Parse + Load + Play");
    oneClickButton.addListener(this);
    oneClickButton.setColour(TextButton::buttonColourId, Colours::green.darker());
    addAndMakeVisible(oneClickButton);
    
    // Set up performance mode buttons (initially hidden)
    saveButton.setButtonText("Save");
    saveButton.addListener(this);
    saveButton.setVisible(false);
    addAndMakeVisible(saveButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    cancelButton.setVisible(false);
    addAndMakeVisible(cancelButton);
    
    // Set up piano roll visualization
    pianoRoll = std::make_unique<PianoRollComponent>();
    addAndMakeVisible(*pianoRoll);
    
    // Don't parse initial pattern - wait until a player is selected
    // parseCurrentPattern(); // Removed - this was causing "No player selected" errors
    
    // No longer listening to musician directly - we work through Player
}

PatternEditorComponent::~PatternEditorComponent()
{
    // Remove listeners
    parseButton.removeListener(this);
    loadButton.removeListener(this);
    playButton.removeListener(this);
    stopButton.removeListener(this);
    saveButton.removeListener(this);
    cancelButton.removeListener(this);
    playerSelector.removeListener(this);
    
    // No longer listening to musician directly
    
    // Clean up piano roll
    pianoRoll.reset();
}

void PatternEditorComponent::paint(Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    
    // Draw a border around the component
    g.setColour(Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void PatternEditorComponent::resized()
{
    auto area = getLocalBounds();
    
    // Header at the top
    headerLabel.setBounds(area.removeFromTop(30));
    
    // Performance context (song/role info) if in performance mode
    if (performanceEditMode && performanceContextLabel.isVisible())
    {
        performanceContextLabel.setBounds(area.removeFromTop(25));
        area.removeFromTop(5); // Small gap
    }
    
    // Player selection
    auto playerArea = area.removeFromTop(30);
    playerLabel.setBounds(playerArea.removeFromLeft(60).reduced(5));
    playerSelector.setBounds(playerArea.reduced(5));
    
    // Buttons at the bottom
    if (performanceEditMode)
    {
        // Performance mode: Show all buttons (parse/load/play/stop + save/cancel)
        auto normalButtonArea = area.removeFromBottom(40);
        auto performanceButtonArea = area.removeFromBottom(40);
        
        // First row: Parse, Load, Play, Stop buttons
        int normalButtonWidth = normalButtonArea.getWidth() / 4;
        parseButton.setBounds(normalButtonArea.removeFromLeft(normalButtonWidth).reduced(5));
        loadButton.setBounds(normalButtonArea.removeFromLeft(normalButtonWidth).reduced(5));
        playButton.setBounds(normalButtonArea.removeFromLeft(normalButtonWidth).reduced(5));
        stopButton.setBounds(normalButtonArea.reduced(5));
        
        // Second row: Save and Cancel buttons
        int performanceButtonWidth = performanceButtonArea.getWidth() / 2;
        saveButton.setBounds(performanceButtonArea.removeFromLeft(performanceButtonWidth).reduced(5));
        cancelButton.setBounds(performanceButtonArea.reduced(5));
    }
    else
    {
        // Normal mode: Show Parse, Load, Play, Stop buttons and new One-Click button
        auto buttonArea = area.removeFromBottom(80);  // More space for two rows
        
        // First row: One-click button (prominent)
        auto oneClickRow = buttonArea.removeFromTop(35);
        oneClickButton.setBounds(oneClickRow.reduced(5));
        
        buttonArea.removeFromTop(5); // Small gap
        
        // Second row: Individual step buttons
        auto stepButtonRow = buttonArea.removeFromTop(35);
        int stepButtonWidth = stepButtonRow.getWidth() / 4;
        parseButton.setBounds(stepButtonRow.removeFromLeft(stepButtonWidth).reduced(5));
        loadButton.setBounds(stepButtonRow.removeFromLeft(stepButtonWidth).reduced(5));
        playButton.setBounds(stepButtonRow.removeFromLeft(stepButtonWidth).reduced(5));
        stopButton.setBounds(stepButtonRow.reduced(5));
    }
    
    // Split remaining area between the editor and piano roll
    auto editorArea = area.removeFromLeft(area.getWidth() / 2);
    auto pianoRollArea = area; // Use remaining area
    
    patternEditor.setBounds(editorArea.reduced(5));
    pianoRoll->setBounds(pianoRollArea.reduced(5));
}

void PatternEditorComponent::buttonClicked(Button* button)
{
    if (button == &parseButton)
    {
        // Parse the current pattern
        parseCurrentPattern();
        listeners.call(&Listener::patternParseRequested, this);
    }
    else if (button == &loadButton)
    {
        // Load the parsed pattern into the selected player
        loadParsedPattern();
        listeners.call(&Listener::patternLoadRequested, this);
    }
    else if (button == &playButton)
    {
        // Play the loaded pattern
        playLoadedPattern();
        listeners.call(&Listener::patternPlayRequested, this);
    }
    else if (button == &stopButton)
    {
        // Stop the current playback
        stopCurrentPlayback();
    }
    else if (button == &saveButton)
    {
        // Save the pattern in performance mode
        listeners.call(&Listener::performancePatternSaveRequested, this);
    }
    else if (button == &cancelButton)
    {
        // Cancel editing in performance mode
        listeners.call(&Listener::performancePatternCancelRequested, this);
    }
    else if (button == &oneClickButton)
    {
        // One-click workflow using backend method
        std::cout << "Starting one-click workflow (Parse + Load + Play) using backend..." << std::endl;
        
        if (!selectedPlayer)
        {
            std::cout << "No player selected. Please select a player first." << std::endl;
            return;
        }
        
        currentPatternNotation = patternEditor.getText();
        if (currentPatternNotation.isEmpty())
        {
            std::cout << "No pattern to play. Please enter a pattern first." << std::endl;
            return;
        }
        
        // Use Player's state machine for complete workflow
        bool prepared = selectedPlayer->preparePattern(currentPatternNotation);
        if (!prepared)
        {
            std::cout << "Failed to prepare pattern in Player" << std::endl;
            return;
        }
        
        bool converted = selectedPlayer->convertToMidi();
        if (!converted)
        {
            std::cout << "Failed to convert pattern to MIDI" << std::endl;
            return;
        }
        
        bool loaded = selectedPlayer->loadPreparedMidi();
        if (!loaded)
        {
            std::cout << "Failed to load pattern into Player" << std::endl;
            return;
        }
        
        bool started = selectedPlayer->play();
        
        if (started)
        {
            std::cout << "✅ One-click workflow completed successfully using Player state machine!" << std::endl;
            
            // Get tempo from chart data for UI tracking
            var chartData = selectedPlayer->getLastChart();
            if (chartData.hasProperty("tempo"))
            {
                double tempo = static_cast<double>(chartData["tempo"]);
                startPlaybackTracking(tempo);
            }
            else
            {
                startPlaybackTracking(120.0); // Default tempo
            }
            
            // Update visualization
            if (!chartData.isVoid())
            {
                currentParsedChart = chartData;
                updatePianoRollVisualization();
            }
        }
        else
        {
            std::cout << "❌ One-click workflow failed to start playback" << std::endl;
        }
    }
}

void PatternEditorComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    // No longer listening to Musician directly
    // All state changes go through Player interface
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

void PatternEditorComponent::setPlaybackPosition(double positionInBeats)
{
    if (pianoRoll)
    {
        pianoRoll->setPlaybackPosition(positionInBeats);
    }
}

void PatternEditorComponent::startPlaybackTracking(double tempo)
{
    playbackTempo = tempo;
    
    // Calculate audio latency compensation
    double audioLatencyMs = 0.0;
    if (playerManager && playerManager->getAudioEngine())
    {
        auto* audioEngine = playerManager->getAudioEngine();
        double sampleRate = audioEngine->getSampleRate();
        int outputLatencySamples = audioEngine->getOutputLatencySamples();
        int bufferSize = audioEngine->getBufferSize();
        
        // Total latency = output latency + buffer latency
        double outputLatencyMs = (outputLatencySamples / sampleRate) * 1000.0;
        double bufferLatencyMs = (bufferSize / sampleRate) * 1000.0;
        audioLatencyMs = outputLatencyMs + bufferLatencyMs;
        
        std::cout << "Audio latency compensation: " << audioLatencyMs << "ms "
                  << "(output: " << outputLatencyMs << "ms + buffer: " << bufferLatencyMs << "ms)" << std::endl;
    }
    
    // Start playhead tracking with latency compensation
    playbackStartTime = Time::getMillisecondCounterHiRes() + audioLatencyMs;
    playbackStartBeat = 0.0; // Start from beat 0 to match 0-based MIDI timing
    isTrackingPlayback = true;
    
    // Start timer at 30 FPS for smooth updates
    startTimer(33); // ~30 FPS (1000ms / 30 = 33ms)
    
    std::cout << "Started playback tracking at " << tempo << " BPM with " << audioLatencyMs << "ms latency compensation" << std::endl;
}

void PatternEditorComponent::stopPlaybackTracking()
{
    isTrackingPlayback = false;
    stopTimer();
    
    // Clear playback position indicator
    if (pianoRoll)
    {
        pianoRoll->setPlaybackPosition(-1.0); // -1 means not playing
    }
    
    std::cout << "Stopped playback tracking" << std::endl;
}

void PatternEditorComponent::timerCallback()
{
    if (isTrackingPlayback)
    {
        // Calculate current playback position based on elapsed time
        double currentTime = Time::getMillisecondCounterHiRes();
        double elapsedTimeMs = currentTime - playbackStartTime;
        double elapsedTimeSeconds = elapsedTimeMs / 1000.0;
        
        // Convert elapsed time to beats using tempo
        // BPM = beats per minute, so beats per second = BPM / 60
        double beatsPerSecond = playbackTempo / 60.0;
        double currentBeat = playbackStartBeat + (elapsedTimeSeconds * beatsPerSecond);
        
        // Update the playback position
        if (pianoRoll)
        {
            pianoRoll->setPlaybackPosition(currentBeat);
            
            // Auto-scroll if playback position is outside visible area
            autoScrollToPlaybackPosition(currentBeat);
        }
    }
}

void PatternEditorComponent::autoScrollToPlaybackPosition(double currentBeat)
{
    if (!pianoRoll) return;
    
    // Get current scroll parameters
    int beatsPerRow = 16; // 4 bars per row
    int rows = 4; // 4 visible rows
    double currentScrollOffset = pianoRoll->getScrollOffset();
    
    // Calculate visible beat range
    double visibleStartBeat = currentScrollOffset * 4.0; // scrollOffset is in bars
    double visibleEndBeat = visibleStartBeat + (rows * beatsPerRow);
    
    // Check if playback position is outside visible area
    if (currentBeat < visibleStartBeat || currentBeat >= visibleEndBeat)
    {
        // Calculate which bar contains the current beat (0-based for internal calculation)
        double targetBar = std::floor(currentBeat / 4.0);
        
        // Scroll to show this bar in the first row
        pianoRoll->setScrollOffset(targetBar);
        
        std::cout << "Auto-scrolled to bar " << (targetBar + 1) << " (beat " << currentBeat << ")" << std::endl;
    }
}

void PatternEditorComponent::parseCurrentPattern()
{
    currentPatternNotation = patternEditor.getText();
    
    if (!selectedPlayer)
    {
        // Silently return if no player is selected - this is normal during initialization
        return;
    }
    
    if (!currentPatternNotation.isEmpty())
    {
        // Use Player's state machine for pattern processing
        bool prepared = selectedPlayer->preparePattern(currentPatternNotation);
        
        if (prepared)
        {
            // Convert to MIDI for visualization
            bool converted = selectedPlayer->convertToMidi();
            
            if (converted)
            {
                std::cout << "Pattern processed successfully using Player state machine." << std::endl;
                
                // Get chart data for visualization
                var chartData = selectedPlayer->getLastChart();
                if (!chartData.isVoid())
                {
                    std::cout << "Chart data received: " << JSON::toString(chartData) << std::endl;
                    currentParsedChart = chartData;
                    updatePianoRollVisualization();
                }
                else
                {
                    std::cout << "No chart data available from getLastChart()" << std::endl;
                }
            }
            else
            {
                std::cout << "Failed to convert pattern to MIDI" << std::endl;
                currentParsedChart = var();
            }
        }
        else
        {
            std::cout << "Failed to prepare pattern in Player" << std::endl;
            currentParsedChart = var();
        }
    }
}

void PatternEditorComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &playerSelector)
    {
        onPlayerSelectionChanged();
    }
}

void PatternEditorComponent::updatePlayerSelector()
{
    playerSelector.clear();
    playerSelector.addItem("No Player Selected", 1);
    
    if (playerManager)
    {
        auto players = playerManager->getPlayers();
        int itemIndex = 2;
        for (auto* player : players)
        {
            if (player)
            {
                playerSelector.addItem(player->getMetadata().name, itemIndex++);
            }
        }
    }
    
    playerSelector.setSelectedItemIndex(0);
    selectedPlayer = nullptr;
}

void PatternEditorComponent::onPlayerSelectionChanged()
{
    int selectedIndex = playerSelector.getSelectedItemIndex();
    selectedPlayer = nullptr;
    
    if (selectedIndex > 0 && playerManager)
    {
        auto players = playerManager->getPlayers();
        int playerIndex = selectedIndex - 1; // Adjust for "No Player Selected" option
        
        if (playerIndex >= 0 && playerIndex < players.size())
        {
            selectedPlayer = players[playerIndex];
            std::cout << "Selected player: " << selectedPlayer->getMetadata().name << std::endl;
        }
    }
}

void PatternEditorComponent::loadParsedPattern()
{
    if (!selectedPlayer)
    {
        std::cout << "No player selected for loading pattern" << std::endl;
        return;
    }
    
    // Use Player's state machine for pattern loading
    bool loaded = selectedPlayer->loadPreparedMidi();
    
    if (loaded)
    {
        std::cout << "Pattern loaded successfully using Player state machine." << std::endl;
        
        // Update visualization with the loaded chart data
        var chartData = selectedPlayer->getLastChart();
        if (!chartData.isVoid())
        {
            currentParsedChart = chartData;
            updatePianoRollVisualization();
            std::cout << "Updated piano roll visualization after loading." << std::endl;
        }
        else
        {
            std::cout << "Warning: No chart data available after loading pattern." << std::endl;
        }
    }
    else
    {
        std::cout << "Failed to load pattern into Player. Make sure pattern is processed first." << std::endl;
    }
}

void PatternEditorComponent::playLoadedPattern()
{
    if (!selectedPlayer)
    {
        std::cout << "No player selected for playback" << std::endl;
        return;
    }
    
    // Use Player's state machine for playback
    bool started = selectedPlayer->play();
    
    if (started)
    {
        std::cout << "Pattern playback started successfully using Player state machine." << std::endl;
        
        // Get tempo from chart data for UI tracking
        var chartData = selectedPlayer->getLastChart();
        if (chartData.hasProperty("tempo"))
        {
            double tempo = static_cast<double>(chartData["tempo"]);
            startPlaybackTracking(tempo);
        }
        else
        {
            startPlaybackTracking(120.0); // Default tempo
        }
        
        // Update visualization with the latest chart data
        if (!chartData.isVoid())
        {
            currentParsedChart = chartData;
            updatePianoRollVisualization();
        }
    }
    else
    {
        std::cout << "Failed to start playback. Make sure pattern is loaded first." << std::endl;
    }
}

void PatternEditorComponent::stopCurrentPlayback()
{
    // Stop visual tracking first
    stopPlaybackTracking();
    
    if (selectedPlayer)
    {
        selectedPlayer->stop();
        std::cout << "Stopped playback on " << selectedPlayer->getMetadata().name << std::endl;
    }
    else
    {
        std::cout << "No player selected to stop" << std::endl;
    }
}

void PatternEditorComponent::updatePianoRollVisualization()
{
    if (pianoRoll)
    {
        pianoRoll->setMidiEvents(currentParsedChart);
        pianoRoll->repaint();
    }
}

void PatternEditorComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PatternEditorComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

// Piano Roll Visualization Component Implementation
PatternEditorComponent::PianoRollComponent::PianoRollComponent() = default;

void PatternEditorComponent::PianoRollComponent::setMidiEvents(const var& chart)
{
    events.clear();
    
    std::cout << "PianoRoll setMidiEvents called with: " << JSON::toString(chart) << std::endl;
    
    var eventsArray;
    
    // Handle both formats: direct array or object with "events" property
    if (chart.isArray())
    {
        // Direct array format
        eventsArray = chart;
        std::cout << "Using direct array format with " << eventsArray.size() << " events" << std::endl;
    }
    else if (chart.hasProperty("events") && chart["events"].isArray())
    {
        // Object with "events" property
        eventsArray = chart["events"];
        std::cout << "Using events property format with " << eventsArray.size() << " events" << std::endl;
    }
    
    if (!eventsArray.isVoid() && eventsArray.isArray())
    {
        
        for (int i = 0; i < eventsArray.size(); ++i)
        {
            var event = eventsArray[i];
            
            if (event.hasProperty("type") && event.hasProperty("note") && event.hasProperty("timestamp"))
            {
                String type = event["type"].toString();
                int note = static_cast<int>(static_cast<double>(event["note"]));
                double timestamp = static_cast<double>(event["timestamp"]);
                double duration = event.hasProperty("duration") ? static_cast<double>(event["duration"]) : 0.25;
                String source = event.hasProperty("source") ? event["source"].toString() : "pattern";
                
                if (type == "note_on")
                {
                    // MIDI charts use 1-based note numbering, so convert to 0-based for internal use
                    int adjustedNote = note - 1;
                    events.add({adjustedNote, timestamp, duration, source});
                    std::cout << "Added note: " << adjustedNote << " at " << timestamp << " duration " << duration << std::endl;
                }
            }
        }
    }
    
    std::cout << "Total events processed: " << events.size() << std::endl;
    repaint();
}

void PatternEditorComponent::PianoRollComponent::paint(Graphics& g)
{
    g.fillAll(Colours::white);
    
    // Draw background grid
    g.setColour(Colours::lightgrey);
    
    auto bounds = getLocalBounds();
    int width = bounds.getWidth();
    int height = bounds.getHeight();
    
    // Time grid - vertical lines every beat (each row shows 4 bars = 16 beats)
    int rows = 4; // 4 rows visible at once
    int beatsPerRow = 16; // 4 bars per row (4 beats × 4 bars)
    int gapHeight = 10; // Gap between rows
    double availableHeight = height - (gapHeight * (rows - 1)); // Subtract gaps
    double rowHeight = availableHeight / rows;
    double pixelsPerBeat = width / beatsPerRow;
    
    // Draw grid for each row
    for (int row = 0; row < rows; ++row)
    {
        double rowY = row * (rowHeight + gapHeight); // Include gap in row positioning
        
        // Draw vertical beat lines for this row
        for (double beat = 0; beat <= beatsPerRow; beat += 1.0) // Beat lines every beat
        {
            int x = static_cast<int>(beat * pixelsPerBeat);
            bool isBarLine = (static_cast<int>(beat) % 4 == 0);
            g.setColour(isBarLine ? Colours::darkgrey : Colours::lightgrey);
            g.drawVerticalLine(x, static_cast<float>(rowY), static_cast<float>(rowY + rowHeight));
        }
        
        // Draw quarter note subdivisions (lighter)
        for (double beat = 0; beat <= beatsPerRow; beat += 0.25)
        {
            int x = static_cast<int>(beat * pixelsPerBeat);
            if (static_cast<int>(beat * 4) % 4 != 0) // Skip if it's already drawn as a beat line
            {
                g.setColour(Colours::lightgrey.withAlpha(0.3f));
                g.drawVerticalLine(x, static_cast<float>(rowY), static_cast<float>(rowY + rowHeight));
            }
        }
        
        // Draw horizontal line to separate rows
        g.setColour(Colours::darkgrey);
        g.drawHorizontalLine(static_cast<int>(rowY), 0.0f, static_cast<float>(width));
    }
    
    // Note grid - horizontal lines for piano keys within each row (reduced range)
    // Adjust for 0-based internal representation (MIDI charts are 1-based)
    int minNote = 35; // C2 (36 in MIDI chart becomes 35 internally)
    int maxNote = 71; // C5 (72 in MIDI chart becomes 71 internally)
    int noteRange = maxNote - minNote;
    double pixelsPerNote = rowHeight / noteRange;
    
    for (int row = 0; row < rows; ++row)
    {
        double rowY = row * (rowHeight + gapHeight); // Include gap in row positioning
        
        for (int note = minNote; note <= maxNote; note += 12) // Every octave for cleaner grid
        {
            double y = rowY + ((maxNote - note) * pixelsPerNote);
            g.setColour(Colours::lightgrey.withAlpha(0.4f));
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(width));
        }
    }
    
    // Draw notes with source-based colors
    for (const auto& event : events)
    {
        int note = event.note;
        double timestamp = event.timestamp;
        double duration = event.duration;
        String source = event.source;
        
        if (note >= minNote && note <= maxNote)
        {
            // Timestamps are now 0-based from MIDI conversion, no adjustment needed
            // Apply scroll offset to timestamp
            double adjustedTimestamp = timestamp - (scrollOffset * 4.0); // scrollOffset is in bars (4 beats each)
            
            // Calculate which row this note belongs to based on adjusted timestamp
            int row = static_cast<int>(adjustedTimestamp / beatsPerRow);
            double beatInRow = adjustedTimestamp - (row * beatsPerRow);
            
            // Skip if note is outside our visible range
            if (row < 0 || row >= rows || beatInRow < 0 || beatInRow > beatsPerRow) continue;
            
            double rowY = row * (rowHeight + gapHeight); // Include gap in row positioning
            int x = static_cast<int>(beatInRow * pixelsPerBeat);
            int y = static_cast<int>(rowY + ((maxNote - note) * pixelsPerNote));
            int noteWidth = jmax(2, static_cast<int>(duration * pixelsPerBeat)); // Full duration width
            int noteHeight = static_cast<int>(pixelsPerNote * 0.8); // Standard note height
            
            Rectangle<int> noteRect(x, y, noteWidth, noteHeight);
            
            // Set color based on source
            Colour noteColour = getSourceColour(source);
            g.setColour(noteColour.withAlpha(0.8f));
            g.fillRect(noteRect);
            
            // Draw note border
            g.setColour(noteColour.darker());
            g.drawRect(noteRect, 1);
            
            // Draw a small end line to show the exact ending point
            int endLineHeight = static_cast<int>(noteHeight * 0.1); // 10% of note height
            int endLineY = y + (noteHeight - endLineHeight) / 2; // Center vertically
            g.setColour(noteColour.darker(0.5f)); // Slightly darker than border
            g.drawVerticalLine(x + noteWidth - 1, static_cast<float>(endLineY), static_cast<float>(endLineY + endLineHeight));
        }
    }
    
    // Draw time labels for each row (show bar numbers)
    g.setColour(Colours::black);
    g.setFont(Font(FontOptions().withHeight(12.0f)));
    
    for (int row = 0; row < rows; ++row)
    {
        double rowY = row * (rowHeight + gapHeight); // Include gap in row positioning
        
        // Show bar numbers (every 4 beats) - show exactly 4 bars per row
        for (int barInRow = 0; barInRow < 4; ++barInRow)
        {
            double beat = barInRow * 4.0; // Each bar is 4 beats
            int x = static_cast<int>(beat * pixelsPerBeat);
            int absoluteBar = static_cast<int>(scrollOffset) + (row * 4) + barInRow + 1; // Start from bar 1
            g.drawText("Bar " + String(absoluteBar), x + 2, static_cast<int>(rowY + 2), 60, 20, Justification::centredLeft);
        }
    }
    
    // Draw note labels on the left for each row
    for (int row = 0; row < rows; ++row)
    {
        double rowY = row * (rowHeight + gapHeight); // Include gap in row positioning
        
        for (int note = minNote; note <= maxNote; note += 12) // Every octave
        {
            double y = rowY + ((maxNote - note) * pixelsPerNote);
            // Convert back to 1-based MIDI note number for display
            String noteName = getMidiNoteName(note + 1);
            g.drawText(noteName, 2, static_cast<int>(y - 6), 30, 12, Justification::centredLeft);
        }
    }
    
    // Draw playback position indicator
    if (playbackPosition >= 0.0)
    {
        // Playback position is now 0-based to match MIDI timing, no adjustment needed
        // Apply scroll offset to playback position
        double adjustedPlaybackPosition = playbackPosition - (scrollOffset * 4.0);
        
        // Calculate which row the playback position is in
        int playbackRow = static_cast<int>(adjustedPlaybackPosition / beatsPerRow);
        double beatInRow = adjustedPlaybackPosition - (playbackRow * beatsPerRow);
        
        // Only draw if playback position is visible
        if (playbackRow >= 0 && playbackRow < rows && beatInRow >= 0 && beatInRow <= beatsPerRow)
        {
            double rowY = playbackRow * (rowHeight + gapHeight);
            int x = static_cast<int>(beatInRow * pixelsPerBeat);
            
            // Draw a bright vertical line for playback position
            g.setColour(Colours::red);
            g.drawVerticalLine(x, static_cast<float>(rowY), static_cast<float>(rowY + rowHeight));
            
            // Draw a small triangle at the top
            Path triangle;
            triangle.addTriangle(static_cast<float>(x - 5), static_cast<float>(rowY - 5),
                               static_cast<float>(x + 5), static_cast<float>(rowY - 5),
                               static_cast<float>(x), static_cast<float>(rowY + 5));
            g.fillPath(triangle);
        }
    }
    
    // Draw scroll position indicator in bottom left
    g.setColour(Colours::black);
    g.setFont(Font(FontOptions().withHeight(10.0f)));
    double endBar = scrollOffset + (rows * 4); // Each row shows 4 bars
    g.drawText("Showing bars " + String(scrollOffset + 1, 1) + "-" + String(endBar, 1), // Start from bar 1 
               5, height - 25, 150, 20, Justification::centredLeft);
    g.drawText("Scroll wheel to navigate", 5, height - 12, 150, 10, Justification::centredLeft);
    
    // Draw color legend in the bottom right
    int legendX = width - 180;
    int legendY = height - 120;
    g.setFont(Font(FontOptions().withHeight(10.0f)));
    
    Array<std::pair<String, Colour>> legendItems;
    legendItems.add({"Layers:", Colours::black});
    legendItems.add({"Drums", Colours::red});
    legendItems.add({"Bass", Colours::green});
    legendItems.add({"Melody", Colours::blue});
    legendItems.add({"Harmony", Colours::purple});
    legendItems.add({"Variations:", Colours::black});
    legendItems.add({"Fill", Colours::orange});
    legendItems.add({"Break", Colours::yellow});
    legendItems.add({"Base Pattern", Colours::lightblue});
    
    for (int i = 0; i < legendItems.size(); ++i)
    {
        int itemY = legendY + (i * 12);
        g.setColour(legendItems[i].second);
        g.drawText(legendItems[i].first, legendX, itemY, 120, 12, Justification::centredLeft);
    }
}

bool PatternEditorComponent::PianoRollComponent::isWhiteKeyNote(int midiNote)
{
    int noteInOctave = midiNote % 12;
    return (noteInOctave == 0 || noteInOctave == 2 || noteInOctave == 4 || 
            noteInOctave == 5 || noteInOctave == 7 || noteInOctave == 9 || noteInOctave == 11);
}

String PatternEditorComponent::PianoRollComponent::getMidiNoteName(int midiNote)
{
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteInOctave = midiNote % 12;
    return String(noteNames[noteInOctave]) + String(octave);
}

Colour PatternEditorComponent::PianoRollComponent::getSourceColour(const String& source)
{
    // Color-code based on source type
    if (source.startsWith("layer:"))
    {
        String layerName = source.substring(6); // Remove "layer:" prefix
        
        // Use different colors for different layers
        if (layerName == "drums" || layerName.contains("drum"))
            return Colours::red;
        else if (layerName == "bass" || layerName.contains("bass"))
            return Colours::green;
        else if (layerName == "melody" || layerName.contains("melody"))
            return Colours::blue;
        else if (layerName == "harmony" || layerName.contains("harmony"))
            return Colours::purple;
        else if (layerName == "rhythm" || layerName.contains("rhythm"))
            return Colours::orange;
        else
            return Colours::cyan; // Default for other layers
    }
    else if (source.startsWith("variation:"))
    {
        String variationName = source.substring(10); // Remove "variation:" prefix
        
        // Use warm colors for variations
        if (variationName.contains("fill"))
            return Colours::orange;
        else if (variationName.contains("break"))
            return Colours::yellow;
        else if (variationName.contains("solo"))
            return Colours::magenta;
        else
            return Colours::coral; // Default for variations
    }
    else if (source.startsWith("base:"))
    {
        // Cool colors for base patterns
        return Colours::lightblue;
    }
    else
    {
        // Default pattern color
        return Colours::blue;
    }
}

void PatternEditorComponent::PianoRollComponent::setPlaybackPosition(double positionInBeats)
{
    playbackPosition = positionInBeats;
    repaint(); // Redraw to show new playback position
}

void PatternEditorComponent::PianoRollComponent::setScrollOffset(double offset)
{
    scrollOffset = jmax(0.0, offset); // Prevent negative scroll
    repaint(); // Redraw with new scroll position
}

double PatternEditorComponent::PianoRollComponent::getScrollOffset() const
{
    return scrollOffset;
}

void PatternEditorComponent::PianoRollComponent::mouseWheelMove(const MouseEvent& /*event*/, const MouseWheelDetails& wheel)
{
    // Scroll discretely by 4 bars (one full row) per wheel notch
    if (wheel.deltaY > 0.1f) // Scroll up
    {
        scrollOffset = jmax(0.0, scrollOffset - 4.0); // Move up by 4 bars
    }
    else if (wheel.deltaY < -0.1f) // Scroll down  
    {
        scrollOffset += 4.0; // Move down by 4 bars
    }
    
    repaint(); // Redraw with new scroll position
}

void PatternEditorComponent::setPerformanceEditMode(bool enabled)
{
    performanceEditMode = enabled;
    
    // Keep all buttons visible - parse/load/play/stop are still needed
    // Just add save/cancel buttons in performance mode
    saveButton.setVisible(enabled);
    cancelButton.setVisible(enabled);
    
    // Set player selector read-only in performance mode
    playerSelector.setEnabled(!enabled);
    
    // Show/hide performance context label
    performanceContextLabel.setVisible(enabled);
    
    // Update layout
    resized();
}

void PatternEditorComponent::setSelectedPlayer(Player* player)
{
    selectedPlayer = player;
    
    if (player && playerManager)
    {
        // Find the player in the selector and select it
        auto players = playerManager->getPlayers();
        for (int i = 0; i < players.size(); ++i)
        {
            if (players[i] == player)
            {
                // Index + 1 because item 0 is "No Player Selected"
                playerSelector.setSelectedItemIndex(i + 1, dontSendNotification);
                break;
            }
        }
    }
    else
    {
        // No player selected
        playerSelector.setSelectedItemIndex(0, dontSendNotification);
    }
}

void PatternEditorComponent::setPerformanceContext(const String& songName, const String& roleName)
{
    String contextText;
    if (songName.isNotEmpty() && roleName.isNotEmpty())
    {
        contextText = "Song: " + songName + " | Role: " + roleName;
    }
    else if (roleName.isNotEmpty())
    {
        contextText = "Role: " + roleName;
    }
    else if (songName.isNotEmpty())
    {
        contextText = "Song: " + songName;
    }
    
    performanceContextLabel.setText(contextText, dontSendNotification);
}

} // namespace covertracks
