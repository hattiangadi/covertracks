#include "PerformanceManager.h"

namespace covertracks {

PerformanceManager::PerformanceManager()
{
}

PerformanceManager::~PerformanceManager()
{
}

juce::File PerformanceManager::getDefaultPerformancesDirectory() const
{
    // Use the app data directory to store performances
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Covertracks")
        .getChildFile("Performances");
}

juce::Array<std::shared_ptr<Performance>> PerformanceManager::loadPerformancesFromDefaultLocation()
{
    return loadPerformances(getDefaultPerformancesDirectory());
}

juce::Array<std::shared_ptr<Performance>> PerformanceManager::loadPerformances(const juce::File& directory)
{
    juce::Array<std::shared_ptr<Performance>> performances;
    
    // Create the directory if it doesn't exist
    if (!directory.exists())
    {
        directory.createDirectory();
        DBG("Created performances directory: " + directory.getFullPathName());
        return performances; // Return empty array since there are no performances yet
    }
    
    // Find all .json files in the directory
    juce::Array<juce::File> performanceFiles;
    directory.findChildFiles(performanceFiles, juce::File::findFiles, false, "*.json");
    
    // Load each performance file
    for (const auto& file : performanceFiles)
    {
        try
        {
            // Read the file content
            juce::String jsonContent = file.loadFileAsString();
            
            // Parse the JSON
            juce::var jsonVar = juce::JSON::parse(jsonContent);
            
            // Create a performance from the JSON
            auto performance = Performance::fromVar(jsonVar);
            if (performance)
            {
                // If the performance has a song ID, try to find the song
                if (jsonVar.hasProperty("songId"))
                {
                    juce::String songId = jsonVar["songId"].toString();
                    auto song = findSongById(songId);
                    if (song)
                    {
                        performance->setSong(song);
                    }
                }
                
                performances.add(performance);
                DBG("Loaded performance: " + performance->getName());
            }
        }
        catch (const std::exception& e)
        {
            DBG("Error loading performance from " + file.getFileName() + ": " + e.what());
        }
    }
    
    return performances;
}

void PerformanceManager::savePerformancesToDefaultLocation(const juce::Array<std::shared_ptr<Performance>>& performances)
{
    savePerformances(getDefaultPerformancesDirectory(), performances);
}

void PerformanceManager::savePerformances(const juce::File& directory, const juce::Array<std::shared_ptr<Performance>>& performances)
{
    // Create the directory if it doesn't exist
    if (!directory.exists())
    {
        directory.createDirectory();
        DBG("Created performances directory: " + directory.getFullPathName());
    }
    
    // Save each performance to a separate file
    for (auto& performance : performances)
    {
        if (performance)
        {
            // Create a filename based on the performance ID
            juce::String filename = performance->getUniqueID() + ".json";
            juce::File file = directory.getChildFile(filename);
            
            // Convert the performance to JSON
            juce::var json = performance->toVar();
            
            // Write the JSON to the file
            juce::String jsonString = juce::JSON::toString(json, true);
            file.replaceWithText(jsonString);
            
            DBG("Saved performance: " + performance->getName() + " to " + file.getFullPathName());
        }
    }
}

void PerformanceManager::savePerformance(std::shared_ptr<Performance> performance)
{
    if (performance)
    {
        juce::Array<std::shared_ptr<Performance>> performances;
        performances.add(performance);
        savePerformancesToDefaultLocation(performances);
    }
}

bool PerformanceManager::deletePerformance(std::shared_ptr<Performance> performance)
{
    if (!performance)
    {
        return false;
    }
    
    // Get the file path for this performance
    juce::File directory = getDefaultPerformancesDirectory();
    juce::String filename = performance->getUniqueID() + ".json";
    juce::File file = directory.getChildFile(filename);
    
    // Check if the file exists
    if (!file.exists())
    {
        DBG("Performance file does not exist: " + file.getFullPathName());
        return false;
    }
    
    // Delete the file
    bool success = file.deleteFile();
    
    if (success)
    {
        DBG("Deleted performance: " + performance->getName() + " from " + file.getFullPathName());
    }
    else
    {
        DBG("Failed to delete performance: " + performance->getName() + " at " + file.getFullPathName());
    }
    
    return success;
}

std::shared_ptr<Song> PerformanceManager::findSongById(const juce::String& songId)
{
    // Get the songs directory
    juce::File songsDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Covertracks")
        .getChildFile("Songs");
    
    // Find all .json files in the directory
    juce::Array<juce::File> songFiles;
    songsDir.findChildFiles(songFiles, juce::File::findFiles, false, "*.json");
    
    // Search each song file for the matching ID
    for (const auto& file : songFiles)
    {
        try
        {
            // Read the file content
            juce::String jsonContent = file.loadFileAsString();
            
            // Parse the JSON
            juce::var jsonVar = juce::JSON::parse(jsonContent);
            
            // Check if this song has the matching ID
            if (jsonVar.hasProperty("id") && jsonVar["id"].toString() == songId)
            {
                // Create a song from the JSON
                auto song = Song::fromVar(jsonVar);
                if (song)
                {
                    DBG("Found song with ID: " + songId);
                    return song;
                }
            }
        }
        catch (const std::exception& e)
        {
            DBG("Error checking song file " + file.getFileName() + ": " + e.what());
        }
    }
    
    DBG("Could not find song with ID: " + songId);
    return nullptr;
}

} // namespace covertracks
