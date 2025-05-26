#include "Song.h"

namespace covertracks {

Song::Song()
    : tempo(120.0)
{
    // Generate a unique ID for this song
    juce::Uuid uuid;
    uniqueID = uuid.toString();
    title = "New Song";
}

Song::Song(const juce::String& songTitle, double songTempo)
    : title(songTitle), tempo(songTempo)
{
    // Generate a unique ID for this song
    juce::Uuid uuid;
    uniqueID = uuid.toString();
}

Song::~Song()
{
    // Nothing to clean up
}

int Song::getNumSections() const
{
    // Parse the JSON and count sections if available
    if (sourceStructureJson.isNotEmpty())
    {
        juce::var parsedJson;
        juce::Result result = juce::JSON::parse(sourceStructureJson, parsedJson);
        
        if (!result.failed() && parsedJson.isObject())
        {
            auto* obj = parsedJson.getDynamicObject();
            
            // If the JSON has a sections array, return its size
            if (obj->hasProperty("sections") && obj->getProperty("sections").isArray())
            {
                return obj->getProperty("sections").getArray()->size();
            }
        }
    }
    
    // Default to 1 section if we can't determine from JSON
    return 1;
}

int Song::getNumMeasures() const
{
    // Parse the JSON and count measures if available
    if (sourceStructureJson.isNotEmpty())
    {
        juce::var parsedJson;
        juce::Result result = juce::JSON::parse(sourceStructureJson, parsedJson);
        
        if (!result.failed() && parsedJson.isObject())
        {
            auto* obj = parsedJson.getDynamicObject();
            
            // If the JSON has a measures array, return its size
            if (obj->hasProperty("measures") && obj->getProperty("measures").isArray())
            {
                return obj->getProperty("measures").getArray()->size();
            }
        }
    }
    
    // Default to 4 measures if we can't determine from JSON
    return 4;
}

juce::var Song::toVar() const
{
    juce::DynamicObject* obj = new juce::DynamicObject();
    
    obj->setProperty("id", uniqueID);
    obj->setProperty("title", title);
    obj->setProperty("tempo", tempo);
    
    // Store the source structure JSON if available
    if (sourceStructureJson.isNotEmpty())
    {
        obj->setProperty("sourceStructureJson", sourceStructureJson);
    }
    
    return var(obj);
}

std::shared_ptr<Song> Song::fromVar(const juce::var& v)
{
    if (auto* obj = v.getDynamicObject())
    {
        auto song = std::make_shared<Song>();
        
        // Load basic properties
        if (obj->hasProperty("id"))
        {
            song->uniqueID = obj->getProperty("id").toString();
        }
        
        if (obj->hasProperty("title"))
        {
            song->title = obj->getProperty("title").toString();
        }
        
        if (obj->hasProperty("tempo"))
        {
            song->tempo = obj->getProperty("tempo");
        }
        
        // Load source structure JSON if available
        if (obj->hasProperty("sourceStructureJson"))
        {
            song->sourceStructureJson = obj->getProperty("sourceStructureJson").toString();
        }
        
        return song;
    }
    
    return nullptr;
}

} // namespace covertracks
