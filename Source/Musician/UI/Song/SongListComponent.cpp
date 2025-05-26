#include "SongListComponent.h"

namespace covertracks {

SongListComponent::SongListComponent()
{
    // Set up the table
    songTable.setModel(this);
    songTable.setMultipleSelectionEnabled(false);
    songTable.setHeaderHeight(24);
    songTable.getHeader().addColumn("Title", 1, 200, 100, -1, juce::TableHeaderComponent::defaultFlags);
    songTable.getHeader().addColumn("Tempo", 2, 100, 50, -1, juce::TableHeaderComponent::defaultFlags);
    songTable.getHeader().addColumn("Structure", 3, 100, 50, -1, juce::TableHeaderComponent::defaultFlags);
    addAndMakeVisible(songTable);
    
    // Set up buttons
    newButton.setButtonText("New");
    newButton.addListener(this);
    addAndMakeVisible(newButton);
    
    editButton.setButtonText("Edit");
    editButton.addListener(this);
    addAndMakeVisible(editButton);
    
    deleteButton.setButtonText("Delete");
    deleteButton.addListener(this);
    addAndMakeVisible(deleteButton);
    
    // Load songs from the default location
    loadSongsFromDefaultLocation();
    
    // Update the table
    songTable.updateContent();
}

SongListComponent::~SongListComponent()
{
    songTable.setModel(nullptr);
}

void SongListComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Songs", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
}

void SongListComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(30);
    
    // Button area
    auto buttonArea = bounds.removeFromBottom(40);
    auto buttonWidth = buttonArea.getWidth() / 3;
    
    newButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    editButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(5));
    deleteButton.setBounds(buttonArea.reduced(5));
    
    // Table area
    songTable.setBounds(bounds.reduced(5));
}

void SongListComponent::buttonClicked(juce::Button* button)
{
    if (button == &newButton)
    {
        auto newSong = createNewSong();
        songs.add(newSong);
        songTable.updateContent();
        songTable.selectRow(songs.size() - 1);
        listeners.call(&Listener::songListChanged, this);
        
        // Save songs after adding a new one
        saveSongsToDefaultLocation();
    }
    else if (button == &editButton)
    {
        // Trigger song editing
        if (selectedRow >= 0 && selectedRow < songs.size())
        {
            auto song = songs[selectedRow];
            listeners.call(&Listener::songSelectionChanged, this, song);
        }
    }
    else if (button == &deleteButton)
    {
        if (selectedRow >= 0 && selectedRow < songs.size())
        {
            songs.remove(selectedRow);
            songTable.updateContent();
            selectedRow = -1;
            listeners.call(&Listener::songListChanged, this);
            
            // Save songs after deleting one
            saveSongsToDefaultLocation();
        }
    }
}

int SongListComponent::getNumRows()
{
    return songs.size();
}

void SongListComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::lightblue);
    }
    else if (rowNumber % 2)
    {
        g.fillAll(juce::Colours::lightgrey.withAlpha(0.2f));
    }
}

void SongListComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(rowIsSelected ? juce::Colours::darkblue : juce::Colours::black);
    g.setFont(14.0f);
    
    if (rowNumber < songs.size())
    {
        auto song = songs[rowNumber];
        juce::String text;
        
        switch (columnId)
        {
            case 1: // Title
                text = song->getTitle();
                break;
                
            case 2: // Tempo
                text = juce::String(song->getTempo()) + " BPM";
                break;
                
            case 3: // Structure
                // Just indicate if the song has structure data
                text = song->getSourceStructureJson().isNotEmpty() ? "Yes" : "No";
                break;
        }
        
        g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

juce::Component* SongListComponent::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    // We're not using custom components for cells in this simple implementation
    return nullptr;
}

void SongListComponent::cellDoubleClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    // Handle double-click to edit song
    if (rowNumber >= 0 && rowNumber < songs.size())
    {
        auto song = songs[rowNumber];
        selectedRow = rowNumber;
        listeners.call(&Listener::songSelectionChanged, this, song);
    }
}

std::shared_ptr<Song> SongListComponent::getSelectedSong() const
{
    if (selectedRow >= 0 && selectedRow < songs.size())
    {
        return songs[selectedRow];
    }
    return nullptr;
}

void SongListComponent::setSelectedSong(std::shared_ptr<Song> song)
{
    // Find the song in the list and select it
    for (int i = 0; i < songs.size(); ++i)
    {
        if (songs[i] == song)
        {
            songTable.selectRow(i);
            selectedRow = i;
            listeners.call(&Listener::songSelectionChanged, this, song);
            break;
        }
    }
}

void SongListComponent::loadSongs(const juce::File& directory)
{
    // Clear existing songs
    songs.clear();
    
    // Create the directory if it doesn't exist
    if (!directory.exists())
    {
        directory.createDirectory();
        DBG("Created songs directory: " + directory.getFullPathName());
    }
    else
    {
        // Find all .json files in the directory
        juce::Array<juce::File> songFiles;
        directory.findChildFiles(songFiles, juce::File::findFiles, false, "*.json");
        
        // Load each song file
        for (const auto& file : songFiles)
        {
            try
            {
                // Read the file content
                juce::String jsonContent = file.loadFileAsString();
                
                // Parse the JSON
                juce::var jsonVar = juce::JSON::parse(jsonContent);
                
                // Create a song from the JSON
                auto song = Song::fromVar(jsonVar);
                if (song)
                {
                    songs.add(song);
                    DBG("Loaded song: " + song->getTitle());
                }
            }
            catch (const std::exception& e)
            {
                DBG("Error loading song from " + file.getFileName() + ": " + e.what());
            }
        }
    }
    
    // Update the UI
    songTable.updateContent();
    listeners.call(&Listener::songListChanged, this);
    
    DBG("Loaded " + juce::String(songs.size()) + " songs from " + directory.getFullPathName());
}

void SongListComponent::saveSongs(const juce::File& directory)
{
    // Create the directory if it doesn't exist
    if (!directory.exists())
    {
        bool success = directory.createDirectory();
        if (!success)
        {
            DBG("Failed to create directory: " + directory.getFullPathName());
            return;
        }
    }
    
    // First, delete all existing song files to avoid orphaned files
    juce::Array<juce::File> existingFiles;
    directory.findChildFiles(existingFiles, juce::File::findFiles, false, "*.json");
    for (const auto& file : existingFiles)
    {
        file.deleteFile();
    }
    
    // Save each song to a separate file
    int savedCount = 0;
    for (int i = 0; i < songs.size(); ++i)
    {
        auto song = songs[i];
        if (!song) continue;
        
        // Use the song's unique ID for the filename
        juce::String filename = song->getUniqueID() + ".json";
        
        // Convert song to JSON
        juce::var songVar = song->toVar();
        juce::String jsonString = juce::JSON::toString(songVar);
        
        // Write to file
        juce::File songFile = directory.getChildFile(filename);
        if (songFile.replaceWithText(jsonString))
        {
            savedCount++;
            DBG("Saved song: " + song->getTitle() + " to " + songFile.getFullPathName());
        }
        else
        {
            DBG("Failed to save song: " + song->getTitle());
        }
    }
    
    DBG("Saved " + juce::String(savedCount) + " songs to " + directory.getFullPathName());
}

void SongListComponent::addSong(std::shared_ptr<Song> song)
{
    songs.add(song);
    songTable.updateContent();
    listeners.call(&Listener::songListChanged, this);
}

void SongListComponent::removeSong(int index)
{
    if (index >= 0 && index < songs.size())
    {
        songs.remove(index);
        songTable.updateContent();
        listeners.call(&Listener::songListChanged, this);
    }
}

std::shared_ptr<Song> SongListComponent::createNewSong()
{
    // Create a new song with a default name and tempo
    return std::make_shared<Song>("New Song", 120.0);
}

void SongListComponent::addListener(Listener* listener)
{
    listeners.add(listener);
}

void SongListComponent::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace covertracks
