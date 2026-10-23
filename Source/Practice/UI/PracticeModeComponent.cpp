#include "PracticeModeComponent.h"
#include "../Core/LessonLoader.h"

namespace covertracks {
namespace practice {

// =============================================================================
// Free, pure helper functions
// =============================================================================

juce::String formatPosition (double lessonBeats, double beatsPerBar)
{
    if (lessonBeats < 0.0)
        return "Count-in";

    if (beatsPerBar <= 0.0)
        beatsPerBar = 4.0;

    const double epsilon = 1.0e-9;
    int barIndex = (int) std::floor (lessonBeats / beatsPerBar + epsilon);
    double beatInBar = lessonBeats - (double) barIndex * beatsPerBar;
    int beatNumber = (int) std::floor (beatInBar + epsilon) + 1;

    return "Bar " + juce::String (barIndex + 1) + juce::String (" ") + juce::String::fromUTF8 ("\xc2\xb7") + " beat " + juce::String (beatNumber);
}

juce::File findLessonsDirectory (const juce::File& startFrom)
{
    juce::File dir = startFrom.isDirectory() ? startFrom : startFrom.getParentDirectory();

    for (int level = 0; level < 8; ++level)
    {
        if (! dir.exists())
            break;

        auto candidate = dir.getChildFile ("Lessons");
        if (candidate.isDirectory())
            return candidate;

        auto parent = dir.getParentDirectory();
        if (parent == dir)
            break;

        dir = parent;
    }

    return juce::File();
}

PitchPoint pitchSampleToPoint (const PitchSample& sample, double takeStartBeat, double tempoBpm, double micLatencySeconds)
{
    PitchPoint point;
    point.beat = takeStartBeat + (sample.timeSeconds - micLatencySeconds) * tempoBpm / 60.0;
    point.midiPitch = sample.midiPitch;
    point.confidence = sample.voiced ? sample.confidence : 0.0f;
    return point;
}

std::vector<juce::File> listLessonFiles (const juce::File& dir)
{
    std::vector<juce::File> result;

    if (! dir.isDirectory())
        return result;

    for (const auto& entry : juce::RangedDirectoryIterator (dir, true, "*.mid", juce::File::findFiles))
        result.push_back (entry.getFile());

    std::sort (result.begin(), result.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFullPathName() < b.getFullPathName();
    });

    return result;
}

namespace
{
    juce::PropertiesFile::Options makePracticePropertiesOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "Covertracks";
        options.filenameSuffix = "settings";
        options.osxLibrarySubFolder = "Application Support";
        return options;
    }

    // Lazily-created, process-wide settings store for Practice mode. Deliberately
    // separate from the main app's getAppProperties() (declared in MainHostWindow.h),
    // which is only constructed when the GUI app initialises and is left null when
    // running under --run-tests - using it here would crash the test binary.
    juce::PropertiesFile& getPracticeProperties()
    {
        static juce::PropertiesFile properties (makePracticePropertiesOptions());
        return properties;
    }

    constexpr const char* lessonsDirPropertyKey = "practice.lessonsDir";
}

// =============================================================================
// PracticeModeComponent
// =============================================================================

PracticeModeComponent::PracticeModeComponent (PlayerManager* playerManagerToUse)
    : playerManager (playerManagerToUse)
{
    addAndMakeVisible (lessonSelector);
    lessonSelector.onChange = [this] { onLessonSelectorChanged(); };

    addAndMakeVisible (openButton);
    openButton.onClick = [this] { onOpenButtonClicked(); };

    addAndMakeVisible (playerSelector);
    playerSelector.onChange = [this] { onPlayerSelectorChanged(); };

    addAndMakeVisible (tempoSlider);
    tempoSlider.setRange (40.0, 200.0, 1.0);
    tempoSlider.setSliderStyle (juce::Slider::LinearBar);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    tempoSlider.setTextValueSuffix (" bpm");
    tempoSlider.setValue (120.0, juce::dontSendNotification);
    tempoSlider.onValueChange = [this] { onTempoChanged(); };

    addAndMakeVisible (tempoResetButton);
    tempoResetButton.onClick = [this] { onTempoResetClicked(); };

    addAndMakeVisible (transposeSlider);
    transposeSlider.setRange (-12.0, 12.0, 1.0);
    transposeSlider.setSliderStyle (juce::Slider::IncDecButtons);
    transposeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    transposeSlider.setValue (0.0, juce::dontSendNotification);
    transposeSlider.onValueChange = [this] { onTransposeChanged(); };

    addAndMakeVisible (transposeLabel);
    transposeLabel.setText ("+0 st", juce::dontSendNotification);
    transposeLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (countInSelector);
    countInSelector.addItem ("No count-in", 1);
    countInSelector.addItem ("1 bar", 2);
    countInSelector.addItem ("2 bars", 3);
    countInSelector.setSelectedId (2, juce::dontSendNotification);
    countInSelector.onChange = [this] { onCountInChanged(); };

    addAndMakeVisible (loopToggle);
    loopToggle.onClick = [this] { onLoopToggled(); };

    addAndMakeVisible (loopStartBar);
    loopStartBar.setRange (1.0, 1.0, 1.0);
    loopStartBar.setSliderStyle (juce::Slider::IncDecButtons);
    loopStartBar.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 40, 20);
    loopStartBar.setValue (1.0, juce::dontSendNotification);
    loopStartBar.onValueChange = [this] { onLoopRangeChanged(); };

    addAndMakeVisible (loopEndBar);
    loopEndBar.setRange (1.0, 1.0, 1.0);
    loopEndBar.setSliderStyle (juce::Slider::IncDecButtons);
    loopEndBar.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 40, 20);
    loopEndBar.setValue (1.0, juce::dontSendNotification);
    loopEndBar.onValueChange = [this] { onLoopRangeChanged(); };

    addAndMakeVisible (playButton);
    playButton.onClick = [this] { onPlayButtonClicked(); };

    addAndMakeVisible (rewindButton);
    rewindButton.onClick = [this] { onRewindButtonClicked(); };

    addAndMakeVisible (statusLabel);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setFont (juce::Font (juce::FontOptions (13.0f)));

    addAndMakeVisible (pianoRoll);

    addAndMakeVisible (micToggle);
    micToggle.setToggleState (true, juce::dontSendNotification);
    micToggle.onClick = [this] { onMicToggleChanged(); };

    addAndMakeVisible (pitchReadout);
    pitchReadout.setText (juce::String::fromUTF8 ("\xe2\x80\x94"), juce::dontSendNotification);
    pitchReadout.setJustificationType (juce::Justification::centredLeft);

    lastOpenDirectory = getLessonsDirectory();

    refreshPlayerSelector();
    refreshLessonSelector();
    recreateAccompanimentPlayerForSelectedPlayer();
    updateStatusLabel();
    updateMicListenerRegistration();
}

PracticeModeComponent::~PracticeModeComponent()
{
    stopTimer();

    auto& engine = TracktionAudioEngine::getInstance();
    if (engine.getAudioInputListener() == this)
        engine.setAudioInputListener (nullptr);

    takeActive.store (false);
    waitForAudioThreadToLeavePush();
}

void PracticeModeComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PracticeModeComponent::resized()
{
    auto area = getLocalBounds().reduced (6);

    auto row1 = area.removeFromTop (28);
    lessonSelector.setBounds (row1.removeFromLeft (260));
    row1.removeFromLeft (4);
    openButton.setBounds (row1.removeFromLeft (80));
    row1.removeFromLeft (12);
    playerSelector.setBounds (row1.removeFromLeft (160));

    area.removeFromTop (4);
    auto row2 = area.removeFromTop (28);
    tempoSlider.setBounds (row2.removeFromLeft (220));
    row2.removeFromLeft (4);
    tempoResetButton.setBounds (row2.removeFromLeft (100));
    row2.removeFromLeft (16);
    transposeLabel.setBounds (row2.removeFromLeft (48));
    transposeSlider.setBounds (row2.removeFromLeft (90));
    row2.removeFromLeft (16);
    countInSelector.setBounds (row2.removeFromLeft (140));

    area.removeFromTop (4);
    auto row3 = area.removeFromTop (28);
    loopToggle.setBounds (row3.removeFromLeft (100));
    loopStartBar.setBounds (row3.removeFromLeft (90));
    row3.removeFromLeft (4);
    loopEndBar.setBounds (row3.removeFromLeft (90));
    row3.removeFromLeft (16);
    rewindButton.setBounds (row3.removeFromLeft (36));
    row3.removeFromLeft (4);
    playButton.setBounds (row3.removeFromLeft (90));
    row3.removeFromLeft (16);
    micToggle.setBounds (row3.removeFromLeft (130));
    row3.removeFromLeft (8);
    pitchReadout.setBounds (row3.removeFromLeft (100));

    area.removeFromTop (4);
    statusLabel.setBounds (area.removeFromTop (22));

    area.removeFromTop (4);
    pianoRoll.setBounds (area);
}

void PracticeModeComponent::timerCallback()
{
    if (accompanimentPlayer == nullptr)
        return;

    auto beats = accompanimentPlayer->getPositionBeats();
    pianoRoll.setPlayheadBeat (beats);
    updateStatusLabel();

    if (takeActive.load() && pitchTracker != nullptr)
    {
        pitchTracker->analyse();
        ingestPitchSamples (pitchTracker->takeNewSamples());
    }
}

// -----------------------------------------------------------------------------
// Lessons directory / selector
// -----------------------------------------------------------------------------

juce::File PracticeModeComponent::getLessonsDirectory() const
{
    auto savedPath = getPracticeProperties().getValue (lessonsDirPropertyKey);
    if (savedPath.isNotEmpty())
    {
        juce::File saved (savedPath);
        if (saved.isDirectory())
            return saved;
    }

    auto fromApp = findLessonsDirectory (juce::File::getSpecialLocation (juce::File::currentApplicationFile));
    if (fromApp.isDirectory())
        return fromApp;

    auto fromSource = findLessonsDirectory (juce::File (__FILE__));
    if (fromSource.isDirectory())
        return fromSource;

    return juce::File();
}

void PracticeModeComponent::rememberLessonsDirectory (const juce::File& dir)
{
    if (! dir.isDirectory())
        return;

    getPracticeProperties().setValue (lessonsDirPropertyKey, dir.getFullPathName());
}

void PracticeModeComponent::refreshLessonSelector()
{
    lessonSelector.clear (juce::dontSendNotification);
    lessonFiles.clear();

    auto dir = getLessonsDirectory();
    if (! dir.isDirectory())
        return;

    lessonFiles = listLessonFiles (dir);

    int itemId = 1;
    for (auto& file : lessonFiles)
        lessonSelector.addItem (file.getFileNameWithoutExtension(), itemId++);
}

void PracticeModeComponent::onLessonSelectorChanged()
{
    auto index = lessonSelector.getSelectedItemIndex();
    if (index < 0 || index >= (int) lessonFiles.size())
        return;

    loadLessonFile (lessonFiles[(size_t) index]);
}

void PracticeModeComponent::onOpenButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Open a Practice lesson MIDI file...",
                                                         lastOpenDirectory,
                                                         "*.mid");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (! file.existsAsFile())
            return;

        lastOpenDirectory = file.getParentDirectory();
        rememberLessonsDirectory (lastOpenDirectory);
        loadLessonFile (file);
    });
}

// -----------------------------------------------------------------------------
// Player selector
// -----------------------------------------------------------------------------

void PracticeModeComponent::refreshPlayerSelector()
{
    playerSelector.clear (juce::dontSendNotification);

    if (playerManager == nullptr)
        return;

    auto players = playerManager->getPlayers();

    int defaultIndexToSelect = -1;
    int itemId = 1;
    for (int i = 0; i < players.size(); ++i)
    {
        auto* player = players[i];
        playerSelector.addItem (player->getName(), itemId);

        if (player->getName() == "Piano")
            defaultIndexToSelect = itemId;

        ++itemId;
    }

    if (players.isEmpty())
        return;

    playerSelector.setSelectedId (defaultIndexToSelect > 0 ? defaultIndexToSelect : 1, juce::dontSendNotification);
}

Player* PracticeModeComponent::findSelectedPlayer() const
{
    if (playerManager == nullptr)
        return nullptr;

    auto players = playerManager->getPlayers();
    auto index = playerSelector.getSelectedItemIndex();

    if (index < 0 || index >= players.size())
        return nullptr;

    return players[index];
}

void PracticeModeComponent::onPlayerSelectorChanged()
{
    recreateAccompanimentPlayerForSelectedPlayer();

    if (currentLesson.has_value())
        applyLessonToPlayerAndRoll();

    updateStatusLabel();
}

void PracticeModeComponent::recreateAccompanimentPlayerForSelectedPlayer()
{
    // Preserve current tempo / transpose / count-in / loop settings across the switch.
    double preservedTempo = tempoSlider.getValue();
    int preservedTranspose = (int) transposeSlider.getValue();
    int preservedCountIn = currentCountInBars();
    bool preservedLoopOn = loopToggle.getToggleState();
    int preservedLoopStart = (int) loopStartBar.getValue() - 1;
    int preservedLoopEnd = (int) loopEndBar.getValue() - 1;

    accompanimentPlayer.reset();

    auto* player = findSelectedPlayer();
    if (player == nullptr)
    {
        updatePlayButtonText();
        return;
    }

    accompanimentPlayer = std::make_unique<AccompanimentPlayer> (*player);
    accompanimentPlayer->onPlaybackFinished = [this] { onPlaybackFinished(); };

    accompanimentPlayer->setTempoBpm (preservedTempo);
    accompanimentPlayer->setTranspose (preservedTranspose);
    accompanimentPlayer->setCountInBars (preservedCountIn);

    if (preservedLoopOn)
        accompanimentPlayer->setLoopBars (preservedLoopStart, preservedLoopEnd);

    updatePlayButtonText();
}

// -----------------------------------------------------------------------------
// Lesson loading
// -----------------------------------------------------------------------------

void PracticeModeComponent::loadLessonFile (const juce::File& file)
{
    auto lessonOpt = LessonLoader::loadFromFile (file);
    if (! lessonOpt.has_value())
    {
        statusLabel.setText ("Failed to load lesson: " + file.getFileName(), juce::dontSendNotification);
        return;
    }

    currentLesson = lessonOpt;

    tempoSlider.setValue (currentLesson->tempoBpm, juce::dontSendNotification);
    transposeSlider.setValue (0.0, juce::dontSendNotification);
    transposeLabel.setText ("+0 st", juce::dontSendNotification);

    auto numBars = juce::jmax (1, currentLesson->numBars());
    loopStartBar.setRange (1.0, (double) numBars, 1.0);
    loopEndBar.setRange (1.0, (double) numBars, 1.0);
    loopStartBar.setValue (1.0, juce::dontSendNotification);
    loopEndBar.setValue ((double) numBars, juce::dontSendNotification);

    applyLessonToPlayerAndRoll();
    updateStatusLabel();
}

void PracticeModeComponent::applyLessonToPlayerAndRoll()
{
    if (! currentLesson.has_value())
        return;

    auto countIn = currentCountInBars();

    pianoRoll.setLesson (*currentLesson, countIn);
    pianoRoll.setPlayheadBeat (-1.0e9);

    if (accompanimentPlayer == nullptr)
        return;

    accompanimentPlayer->load (*currentLesson);
    accompanimentPlayer->setTempoBpm (tempoSlider.getValue());
    accompanimentPlayer->setTranspose ((int) transposeSlider.getValue());
    accompanimentPlayer->setCountInBars (countIn);

    if (loopToggle.getToggleState())
        onLoopRangeChanged();
    else
        accompanimentPlayer->clearLoop();
}

int PracticeModeComponent::currentCountInBars() const
{
    switch (countInSelector.getSelectedId())
    {
        case 1: return 0;
        case 3: return 2;
        case 2:
        default: return 1;
    }
}

// -----------------------------------------------------------------------------
// Controls
// -----------------------------------------------------------------------------

void PracticeModeComponent::onTempoChanged()
{
    if (accompanimentPlayer != nullptr)
        accompanimentPlayer->setTempoBpm (tempoSlider.getValue());
}

void PracticeModeComponent::onTempoResetClicked()
{
    if (currentLesson.has_value())
        tempoSlider.setValue (currentLesson->tempoBpm, juce::sendNotification);
}

void PracticeModeComponent::onTransposeChanged()
{
    int semitones = (int) transposeSlider.getValue();
    transposeLabel.setText ((semitones >= 0 ? "+" : "") + juce::String (semitones) + " st", juce::dontSendNotification);

    if (accompanimentPlayer != nullptr)
        accompanimentPlayer->setTranspose (semitones);
}

void PracticeModeComponent::onCountInChanged()
{
    auto countIn = currentCountInBars();

    if (accompanimentPlayer != nullptr)
        accompanimentPlayer->setCountInBars (countIn);

    if (currentLesson.has_value())
        pianoRoll.setLesson (*currentLesson, countIn);
}

void PracticeModeComponent::onLoopToggled()
{
    if (loopToggle.getToggleState())
        onLoopRangeChanged();
    else
    {
        if (accompanimentPlayer != nullptr)
            accompanimentPlayer->clearLoop();

        pianoRoll.clearLoop();
    }
}

void PracticeModeComponent::onLoopRangeChanged()
{
    int startBar = (int) loopStartBar.getValue();
    int endBar = (int) loopEndBar.getValue();

    if (endBar < startBar)
    {
        endBar = startBar;
        loopEndBar.setValue ((double) endBar, juce::dontSendNotification);
    }

    if (! loopToggle.getToggleState())
        return;

    int firstBar0 = startBar - 1;
    int lastBar0 = endBar - 1;

    if (accompanimentPlayer != nullptr)
        accompanimentPlayer->setLoopBars (firstBar0, lastBar0);

    pianoRoll.setLoopBars (firstBar0, lastBar0);
}

void PracticeModeComponent::onPlayButtonClicked()
{
    if (accompanimentPlayer == nullptr)
        return;

    if (accompanimentPlayer->isPlaying())
    {
        accompanimentPlayer->stop();
        stopTimer();
        endTake();
    }
    else
    {
        startTake();
        accompanimentPlayer->play();
        startTimer (30);
    }

    updatePlayButtonText();
    updateStatusLabel();
}

void PracticeModeComponent::onRewindButtonClicked()
{
    endTake();

    if (accompanimentPlayer == nullptr)
        return;

    auto countIn = currentCountInBars();
    auto beatsPerBar = currentLesson.has_value() ? currentLesson->beatsPerBar() : 4.0;
    accompanimentPlayer->setPositionBeats (-(countIn * beatsPerBar));
    pianoRoll.setPlayheadBeat (accompanimentPlayer->getPositionBeats());
    updateStatusLabel();
}

void PracticeModeComponent::onPlaybackFinished()
{
    stopTimer();
    endTake();
    updatePlayButtonText();
    onRewindButtonClicked();
}

// -----------------------------------------------------------------------------
// Mic / pitch tracking (step 4)
// -----------------------------------------------------------------------------

void PracticeModeComponent::onMicToggleChanged()
{
    updateMicListenerRegistration();

    if (! micToggle.getToggleState())
        endTake();
}

void PracticeModeComponent::updateMicListenerRegistration()
{
    auto& engine = TracktionAudioEngine::getInstance();

    if (micToggle.getToggleState())
        engine.setAudioInputListener (this);
    else if (engine.getAudioInputListener() == this)
        engine.setAudioInputListener (nullptr);
}

double PracticeModeComponent::getMicLatencySeconds() const
{
    auto ms = getPracticeProperties().getDoubleValue ("practice.micLatencyMs", 0.0);
    return ms / 1000.0;
}

void PracticeModeComponent::startTake()
{
    if (! micToggle.getToggleState())
        return;

    // Make sure the audio thread isn't mid-push into whatever tracker we
    // currently have before we potentially replace or reset it.
    takeActive.store (false);
    waitForAudioThreadToLeavePush();

    auto sampleRate = TracktionAudioEngine::getInstance().getSampleRate();

    if (pitchTracker == nullptr || ! juce::approximatelyEqual (pitchTracker->getSampleRate(), sampleRate))
        pitchTracker = std::make_unique<PitchTracker> (sampleRate);

    pitchTracker->reset();

    takeStartBeat = accompanimentPlayer != nullptr ? accompanimentPlayer->getPositionBeats() : 0.0;
    takeStartTempoBpm = accompanimentPlayer != nullptr ? accompanimentPlayer->getTempoBpm() : 120.0;

    currentTrace.clear();
    pianoRoll.clearPitchTrace();
    pitchReadout.setText (juce::String::fromUTF8 ("\xe2\x80\x94"), juce::dontSendNotification);

    // Published last: the audio thread only touches pitchTracker while this
    // is true, and by now the tracker is fully built and reset.
    takeActive.store (true);
}

void PracticeModeComponent::endTake()
{
    takeActive.store (false);
}

void PracticeModeComponent::audioInputBlock (const float* const* inputChannels, int numInputChannels, int numSamples, double sampleRate)
{
    juce::ignoreUnused (sampleRate);

    if (! takeActive.load())
        return;

    audioThreadInPush.store (true);

    // Re-check after raising the flag: startTake() clears takeActive first and
    // then waits for this flag, so either we see the clear here and bail, or
    // startTake() sees our flag and waits for us to finish.
    if (takeActive.load() && pitchTracker != nullptr)
        pitchTracker->pushInterleavedOrMultichannel (inputChannels, numInputChannels, numSamples);

    audioThreadInPush.store (false);
}

void PracticeModeComponent::waitForAudioThreadToLeavePush()
{
    // Bounded: a callback is a few ms; 50 ms is far more than one block.
    for (int i = 0; i < 50 && audioThreadInPush.load(); ++i)
        juce::Thread::sleep (1);
}

void PracticeModeComponent::ingestPitchSamples (const std::vector<PitchSample>& samples)
{
    if (samples.empty())
        return;

    auto micLatencySeconds = getMicLatencySeconds();

    for (auto& sample : samples)
        currentTrace.push_back (pitchSampleToPoint (sample, takeStartBeat, takeStartTempoBpm, micLatencySeconds));

    pianoRoll.setPitchTrace (currentTrace);

    auto& last = samples.back();
    if (last.voiced)
        pitchReadout.setText (covertracks::practice::PitchDetector::noteNameWithCents (last.midiPitch), juce::dontSendNotification);
    else
        pitchReadout.setText (juce::String::fromUTF8 ("\xe2\x80\x94"), juce::dontSendNotification);
}

void PracticeModeComponent::updatePlayButtonText()
{
    bool canPlay = accompanimentPlayer != nullptr;
    playButton.setEnabled (canPlay);

    if (canPlay && accompanimentPlayer->isPlaying())
        playButton.setButtonText ("Stop");
    else
        playButton.setButtonText ("Play");
}

void PracticeModeComponent::updateStatusLabel()
{
    if (playerManager == nullptr || playerManager->getPlayers().isEmpty())
    {
        statusLabel.setText ("No players available - add a Player to practice with.", juce::dontSendNotification);
        return;
    }

    if (! currentLesson.has_value())
    {
        statusLabel.setText ("Choose a lesson to begin.", juce::dontSendNotification);
        return;
    }

    juce::String text = currentLesson->title;
    text << "  (" << (int) currentLesson->tempoBpm << " bpm, "
         << currentLesson->timeSigNumerator << "/" << currentLesson->timeSigDenominator << ")";

    if (accompanimentPlayer != nullptr)
    {
        auto beats = accompanimentPlayer->getPositionBeats();
        text << "   -   " << formatPosition (beats, currentLesson->beatsPerBar());
    }

    statusLabel.setText (text, juce::dontSendNotification);
}

} // namespace practice
} // namespace covertracks
