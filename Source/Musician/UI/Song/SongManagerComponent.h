#pragma once

#include <JuceHeader.h>
#include "SongListComponent.h"
#include "SongEditorComponent.h"
#include "../../../LLM/Core/LLMClient.h"

namespace covertracks {

/**
 * @brief Component for managing songs, including listing and editing
 */
class SongManagerComponent : public juce::Component,
                            public SongListComponent::Listener,
                            public SongEditorComponent::Listener
{
public:
    /**
     * @brief Listener interface for SongManagerComponent
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /**
         * @brief Called when a song is selected in the SongManagerComponent
         * 
         * @param source The SongManagerComponent that triggered the event
         * @param song The selected song
         */
        virtual void songSelected(SongManagerComponent* source, std::shared_ptr<Song> song) = 0;
    };
    
    /**
     * @brief Add a listener to be notified of song selection events
     * 
     * @param listener The listener to add
     */
    void addListener(Listener* listener) { listeners.add(listener); }
    
    /**
     * @brief Remove a listener
     * 
     * @param listener The listener to remove
     */
    void removeListener(Listener* listener) { listeners.remove(listener); }
    
    /**
     * @brief Get the currently selected song
     * 
     * @return std::shared_ptr<Song> The currently selected song, or nullptr if none
     */
    std::shared_ptr<Song> getCurrentSong() const { return currentSong; }

    /**
     * @brief Construct a new Song Manager Component
     */
    SongManagerComponent();
    
    /**
     * @brief Destroy the Song Manager Component
     */
    ~SongManagerComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // SongListComponent::Listener
    void songSelectionChanged(SongListComponent* source, std::shared_ptr<Song> song) override;
    void songListChanged(SongListComponent* source) override;
    
    // SongEditorComponent::Listener
    void songEdited(SongEditorComponent* source, std::shared_ptr<Song> song) override;
    void editingCancelled(SongEditorComponent* source) override;
    
    /**
     * @brief Initialize the SongEditorComponent with an LLMClient
     * 
     * @param llmClient The LLM client to use for image analysis
     */
    void initializeWithLLMClient(std::shared_ptr<LLMClient> llmClient);
    
    /**
     * @brief Register a listener with the internal SongListComponent
     * 
     * This allows external components to receive updates when songs are added, removed, or changed
     * without having to poll periodically.
     * 
     * @param listener The listener to register
     */
    void registerSongListListener(SongListComponent::Listener* listener);
    
    /**
     * @brief Unregister a listener from the internal SongListComponent
     * 
     * @param listener The listener to unregister
     */
    void unregisterSongListListener(SongListComponent::Listener* listener);
    
private:
    // UI components
    SongListComponent songListComponent;
    SongEditorComponent songEditorComponent;
    
    // Resources
    std::shared_ptr<LLMClient> llmClient;
    
    // State
    bool isEditingMode = false;
    std::shared_ptr<Song> currentSong;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    // Helper methods
    void showSongList();
    void showSongEditor(std::shared_ptr<Song> song);
    
    // Notify listeners of song selection
    void notifyListenersSongSelected(std::shared_ptr<Song> song);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongManagerComponent)
};

} // namespace covertracks
