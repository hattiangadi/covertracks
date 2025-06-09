#pragma once

#include <JuceHeader.h>
#include "Performance.h"
#include "MusicPatternComponents.h"

namespace covertracks {

/**
 * @brief Manages the saving and loading of performances
 */
class PerformanceManager
{
public:
    /**
     * @brief Construct a new Performance Manager
     */
    PerformanceManager();
    
    /**
     * @brief Destroy the Performance Manager
     */
    ~PerformanceManager();
    
    /**
     * @brief Get the default performances directory
     * 
     * @return juce::File The default directory for storing performances
     */
    juce::File getDefaultPerformancesDirectory() const;
    
    /**
     * @brief Load performances from the default location
     * 
     * @return juce::Array<std::shared_ptr<Performance>> The loaded performances
     */
    juce::Array<std::shared_ptr<Performance>> loadPerformancesFromDefaultLocation();
    
    /**
     * @brief Load performances from a directory
     * 
     * @param directory The directory to load from
     * @return juce::Array<std::shared_ptr<Performance>> The loaded performances
     */
    juce::Array<std::shared_ptr<Performance>> loadPerformances(const juce::File& directory);
    
    /**
     * @brief Save performances to the default location
     * 
     * @param performances The performances to save
     */
    void savePerformancesToDefaultLocation(const juce::Array<std::shared_ptr<Performance>>& performances);
    
    /**
     * @brief Save performances to a directory
     * 
     * @param directory The directory to save to
     * @param performances The performances to save
     */
    void savePerformances(const juce::File& directory, const juce::Array<std::shared_ptr<Performance>>& performances);
    
    /**
     * @brief Save a single performance to the default location
     * 
     * @param performance The performance to save
     */
    void savePerformance(std::shared_ptr<Performance> performance);
    
    /**
     * @brief Get all performances from the default location
     * 
     * @return juce::Array<Performance*> Array of raw pointers to performances
     */
    juce::Array<Performance*> getAllPerformances();
    
    /**
     * @brief Remove a performance by ID
     * 
     * @param performanceId The ID of the performance to remove
     * @return bool True if the performance was found and removed
     */
    bool removePerformance(const juce::String& performanceId);
    
    /**
     * @brief Find a song by its unique ID
     * 
     * This method searches the songs directory for a song with the given ID.
     * 
     * @param songId The unique ID of the song to find
     * @return std::shared_ptr<Song> The found song, or nullptr if not found
     */
    std::shared_ptr<Song> findSongById(const juce::String& songId);
    
private:
    // Cache of loaded performances
    juce::Array<std::shared_ptr<Performance>> loadedPerformances;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceManager)
};

} // namespace covertracks
