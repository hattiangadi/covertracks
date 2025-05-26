#include "SongManagerComponent.h"

namespace covertracks {

SongManagerComponent::SongManagerComponent()
    : songEditorComponent(nullptr) // Initialize with null LLMClient, will be set later
{
    // Set up the song list component
    songListComponent.addListener(this);
    addAndMakeVisible(songListComponent);
    
    // The song editor component will be properly initialized when initializeWithLLMClient is called
    songEditorComponent.addListener(this);
    songEditorComponent.setVisible(false);
    addAndMakeVisible(songEditorComponent);
    
    // Show the song list by default
    showSongList();
    
    // Set size constraints
    setSize(800, 600);
}

SongManagerComponent::~SongManagerComponent()
{
    songListComponent.removeListener(this);
    songEditorComponent.removeListener(this);
}

void SongManagerComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SongManagerComponent::resized()
{
    auto bounds = getLocalBounds();
    
    if (isEditingMode)
    {
        songEditorComponent.setBounds(bounds);
    }
    else
    {
        songListComponent.setBounds(bounds);
    }
}

void SongManagerComponent::songSelectionChanged(SongListComponent* source, std::shared_ptr<Song> song)
{
    // This is called when a song is selected in the list
    // When the edit button is clicked, this is also called with the selected song
    // We'll check if the song is valid and show the editor
    if (song)
    {
        // Store the current song
        currentSong = song;
        
        // Notify listeners that a song was selected
        notifyListenersSongSelected(song);
        
        // Show the song editor
        showSongEditor(song);
    }
}

void SongManagerComponent::songListChanged(SongListComponent* source)
{
    // This is called when the song list changes (add, remove, etc.)
    // If a new song was just created, we should immediately edit it
    auto selectedSong = source->getSelectedSong();
    if (selectedSong && selectedSong->getTitle() == "New Song")
    {
        // A new song was just created, open it in edit mode
        showSongEditor(selectedSong);
    }
}

void SongManagerComponent::songEdited(SongEditorComponent* source, std::shared_ptr<Song> song)
{
    // The song has been edited, update the song list and go back to it
    if (song)
    {
        // Update the song in the list
        songListComponent.setSelectedSong(song);
        
        // Save the songs to disk
        songListComponent.saveSongsToDefaultLocation();
        
        DBG("Song edited and saved: " + song->getTitle());
    }
    
    // Go back to the song list
    showSongList();
}

void SongManagerComponent::editingCancelled(SongEditorComponent* source)
{
    // User cancelled editing, go back to the song list
    showSongList();
}

void SongManagerComponent::initializeWithLLMClient(std::shared_ptr<LLMClient> client)
{
    // Store the LLM client for future use
    llmClient = client;
    
    // We don't need to recreate the SongEditorComponent since it already exists
    // and was initialized with a null LLMClient. We just need to update its LLM client.
    
    // The SongEditorComponent will get the LLMClient when showSongEditor is called
}

void SongManagerComponent::showSongList()
{
    isEditingMode = false;
    songListComponent.setVisible(true);
    songEditorComponent.setVisible(false);
    resized();
}

void SongManagerComponent::showSongEditor(std::shared_ptr<Song> song)
{
    if (song)
    {
        isEditingMode = true;
        
        // Make sure the SongEditorComponent has the LLMClient
        // This is a workaround since we can't recreate the component
        if (llmClient)
        {
            // We need to access the LLMClient through a getter/setter in SongEditorComponent
            songEditorComponent.setLLMClient(llmClient);
        }
        
        songEditorComponent.setSong(song);
        songEditorComponent.setVisible(true);
        songListComponent.setVisible(false);
        resized();
    }
}

void SongManagerComponent::notifyListenersSongSelected(std::shared_ptr<Song> song)
{
    // Notify all listeners that a song was selected
    listeners.call(&Listener::songSelected, this, song);
}

void SongManagerComponent::registerSongListListener(SongListComponent::Listener* listener)
{
    if (listener)
    {
        songListComponent.addListener(listener);
    }
}

void SongManagerComponent::unregisterSongListListener(SongListComponent::Listener* listener)
{
    if (listener)
    {
        songListComponent.removeListener(listener);
    }
}

} // namespace covertracks
