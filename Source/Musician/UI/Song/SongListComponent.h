#pragma once

#include <JuceHeader.h>
#include "../../Core/Song.h"

namespace covertracks {

/**
 * @brief Component for displaying and managing a list of songs
 */
class SongListComponent : public juce::Component,
                         public juce::Button::Listener,
                         public juce::TableListBoxModel
{
public:
    /**
     * @brief Construct a new Song List Component
     */
    SongListComponent();
    
    /**
     * @brief Destroy the Song List Component
     */
    ~SongListComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    
    /**
     * @brief Get the currently selected song
     * 
     * @return std::shared_ptr<Song> The selected song, or nullptr if none selected
     */
    std::shared_ptr<Song> getSelectedSong() const;
    
    /**
     * @brief Get all songs in the list
     * 
     * @return const juce::Array<std::shared_ptr<Song>>& All songs in the list
     */
    const juce::Array<std::shared_ptr<Song>>& getSongs() const { return songs; }
    
    /**
     * @brief Set the selected song
     * 
     * @param song The song to select
     */
    void setSelectedSong(std::shared_ptr<Song> song);
    
    /**
     * @brief Load songs from a directory
     * 
     * @param directory The directory to load from
     */
    void loadSongs(const juce::File& directory);
    
    /**
     * @brief Save songs to a directory
     * 
     * @param directory The directory to save to
     */
    void saveSongs(const juce::File& directory);
    
    /**
     * @brief Save songs to the default location
     */
    void saveSongsToDefaultLocation()
    {
        saveSongs(getDefaultSongsDirectory());
    }
    
    /**
     * @brief Add a song
     * 
     * @param song The song to add
     */
    void addSong(std::shared_ptr<Song> song);
    
    /**
     * @brief Remove a song
     * 
     * @param index The index of the song to remove
     */
    void removeSong(int index);
    
    /**
     * @brief Create a new empty song
     * 
     * @return std::shared_ptr<Song> The new song
     */
    std::shared_ptr<Song> createNewSong();
    
    /**
     * @brief Listener interface for song list changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /**
         * @brief Called when a song is selected
         * 
         * @param source The SongListComponent that triggered the event
         * @param song The selected song
         */
        virtual void songSelectionChanged(SongListComponent* /*source*/, std::shared_ptr<Song> /*song*/) {}
        
        /**
         * @brief Called when the song list changes
         * 
         * @param source The SongListComponent that triggered the event
         */
        virtual void songListChanged(SongListComponent* /*source*/) {}
    };
    
    /**
     * @brief Add a listener
     * 
     * @param listener The listener to add
     */
    void addListener(Listener* listener);
    
    /**
     * @brief Remove a listener
     * 
     * @param listener The listener to remove
     */
    void removeListener(Listener* listener);
    
private:
    // UI components
    juce::TableListBox songTable;
    juce::TextButton newButton;
    juce::TextButton editButton;
    juce::TextButton deleteButton;
    
    // Data
    juce::Array<std::shared_ptr<Song>> songs;
    int selectedRow = -1;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    /**
     * @brief Get the default songs directory
     * 
     * @return juce::File The default directory for storing songs
     */
    juce::File getDefaultSongsDirectory() const
    {
        // Use the app data directory to store songs
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Covertracks")
            .getChildFile("Songs");
    }
    
    /**
     * @brief Load songs from the default location
     */
    void loadSongsFromDefaultLocation()
    {
        loadSongs(getDefaultSongsDirectory());
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongListComponent)
};

} // namespace covertracks
