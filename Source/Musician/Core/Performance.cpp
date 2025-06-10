#include "Performance.h"

namespace covertracks {

Performance::Performance(const juce::String& performanceName)
    : name(performanceName)
{
    // Generate a unique ID for this performance
    juce::Uuid uuid;
    uniqueID = uuid.toString();
}

Performance::~Performance()
{
    // Clear references to break potential reference cycles
    song = nullptr;
    chart = nullptr;
    generatedCharts.clear();
}

// The setSong method is now implemented inline in the header file

juce::var Performance::getGeneratedChart(const juce::String& roleName) const
{
    auto it = generatedCharts.find(roleName);
    if (it != generatedCharts.end())
    {
        return it->second;
    }
    return juce::var();
}

void Performance::setGeneratedChart(const juce::String& roleName, const juce::var& generatedChart)
{
    generatedCharts[roleName] = generatedChart;
}

juce::String Performance::getStatusMessage(const juce::String& roleName) const
{
    auto it = statusMessages.find(roleName);
    if (it != statusMessages.end())
    {
        return it->second;
    }
    return "";
}

void Performance::setStatusMessage(const juce::String& roleName, const juce::String& status)
{
    statusMessages[roleName] = status;
}

void Performance::clearGeneratedCharts()
{
    generatedCharts.clear();
    statusMessages.clear();
}

juce::Array<MusicianRole> Performance::getEnsemble() const
{
    // Return cached roles if valid
    if (ensembleCacheValid)
    {
        return cachedEnsembleRoles;
    }
    
    // Load ensemble roles and cache them
    const_cast<Performance*>(this)->refreshEnsembleCache();
    return cachedEnsembleRoles;
}

void Performance::refreshEnsembleCache()
{
    cachedEnsembleRoles.clear();
    ensembleCacheValid = false;
    
    if (ensembleName.isEmpty())
    {
        ensembleCacheValid = true;
        return;
    }
    
    // Load the actual ensemble from disk
    juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    for (const auto& file : ensembleFiles)
    {
        // Load the ensemble to check its name
        auto ensemble = EnsembleManager::loadFromFile(file, nullptr); // We don't need PlayerManager just to get roles
        if (ensemble && ensemble->getName() == ensembleName)
        {
            // Found the matching ensemble, get its musician roles but clear player pointers
            // since we're not managing the full ensemble lifecycle here
            auto ensembleRoles = ensemble->getMusicianRoles();
            for (auto role : ensembleRoles)
            {
                // Clear the player pointer - it will be resolved by PerformanceEditorComponent
                // when it needs to actually work with players
                role.player = nullptr;
                cachedEnsembleRoles.add(role);
            }
            ensembleCacheValid = true;
            return;
        }
    }
    
    // If no ensemble found, cache is still valid (empty)
    ensembleCacheValid = true;
}

juce::var Performance::toVar() const
{
    juce::DynamicObject* obj = new juce::DynamicObject();
    
    obj->setProperty("id", uniqueID);
    obj->setProperty("name", name);
    obj->setProperty("description", description);
    
    // Save song ID if we have a song
    if (song)
    {
        obj->setProperty("songId", song->getUniqueID());
    }
    
    // Save chart ID if we have a chart
    if (chart)
    {
        obj->setProperty("chartId", chart->getUniqueID());
    }
    
    obj->setProperty("ensembleName", ensembleName);
    obj->setProperty("instructions", instructions);
    
    // Save performance attributes
    obj->setProperty("key", key);
    obj->setProperty("genre", genre);
    obj->setProperty("mood", mood);
    
    // Save generated charts
    juce::DynamicObject* chartsObj = new juce::DynamicObject();
    for (const auto& pair : generatedCharts)
    {
        if (!pair.second.isVoid())
        {
            // Store the chart directly as a var
            chartsObj->setProperty(pair.first, pair.second);
        }
    }
    obj->setProperty("charts", var(chartsObj));
    
    // Save status messages
    juce::DynamicObject* statusObj = new juce::DynamicObject();
    for (const auto& pair : statusMessages)
    {
        statusObj->setProperty(pair.first, pair.second);
    }
    obj->setProperty("status", var(statusObj));
    
    return var(obj);
}

std::shared_ptr<Performance> Performance::fromVar(const juce::var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        auto performance = std::make_shared<Performance>();
        
        // Load basic properties
        if (obj->hasProperty("id"))
        {
            performance->uniqueID = obj->getProperty("id").toString();
        }
        
        if (obj->hasProperty("name"))
        {
            performance->name = obj->getProperty("name").toString();
        }
        
        if (obj->hasProperty("description"))
        {
            performance->description = obj->getProperty("description").toString();
        }
        
        // Note: We don't load the song or chart here, as we need the SongListComponent to find them by ID
        // This will be handled by the PerformanceComponent when loading
        
        if (obj->hasProperty("ensembleName"))
        {
            performance->ensembleName = obj->getProperty("ensembleName").toString();
        }
        
        if (obj->hasProperty("instructions"))
        {
            performance->instructions = obj->getProperty("instructions").toString();
        }
        
        // Load performance attributes
        if (obj->hasProperty("key"))
        {
            performance->key = obj->getProperty("key").toString();
        }
        
        if (obj->hasProperty("genre"))
        {
            performance->genre = obj->getProperty("genre").toString();
        }
        
        if (obj->hasProperty("mood"))
        {
            performance->mood = obj->getProperty("mood").toString();
        }
        
        // Load generated charts
        if (auto* chartsObj = obj->getProperty("charts").getDynamicObject())
        {
            for (auto& prop : chartsObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                performance->generatedCharts[roleName] = prop.value;
            }
        }
        
        // Load status messages
        if (auto* statusObj = obj->getProperty("status").getDynamicObject())
        {
            for (auto& prop : statusObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                juce::String status = prop.value.toString();
                
                performance->statusMessages[roleName] = status;
            }
        }
        
        return performance;
    }
    
    return nullptr;
}

} // namespace covertracks
