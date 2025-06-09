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
    generatedPatterns.clear();
}

// The setSong method is now implemented inline in the header file

std::shared_ptr<juce::MidiMessageSequence> Performance::getGeneratedPattern(const juce::String& roleName) const
{
    auto it = generatedPatterns.find(roleName);
    if (it != generatedPatterns.end())
    {
        return it->second;
    }
    return nullptr;
}

void Performance::setGeneratedPattern(const juce::String& roleName, std::shared_ptr<juce::MidiMessageSequence> pattern)
{
    generatedPatterns[roleName] = pattern;
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

void Performance::clearGeneratedPatterns()
{
    generatedPatterns.clear();
    statusMessages.clear();
}

juce::Array<MusicianRole> Performance::getEnsemble() const
{
    juce::Array<MusicianRole> roles;
    
    if (ensembleName.isEmpty())
        return roles;
    
    // Load the actual ensemble from disk
    juce::Array<juce::File> ensembleFiles = EnsembleManager::getEnsembleFiles();
    
    for (const auto& file : ensembleFiles)
    {
        // Load the ensemble to check its name
        auto ensemble = EnsembleManager::loadFromFile(file, nullptr); // We don't need PlayerManager just to get roles
        if (ensemble && ensemble->getName() == ensembleName)
        {
            // Found the matching ensemble, return its musician roles
            return ensemble->getMusicianRoles();
        }
    }
    
    // If no ensemble found, return empty array
    return roles;
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
    
    // Save generated patterns
    juce::DynamicObject* patternsObj = new juce::DynamicObject();
    for (const auto& pair : generatedPatterns)
    {
        if (pair.second)
        {
            // Convert MidiMessageSequence to a storable format
            juce::MemoryBlock block;
            juce::MemoryOutputStream stream(block, false);
            
            // Write the number of events
            stream.writeInt(pair.second->getNumEvents());
            
            // Write each event
            for (int i = 0; i < pair.second->getNumEvents(); ++i)
            {
                auto* event = pair.second->getEventPointer(i);
                
                // Write the message
                auto& message = event->message;
                stream.writeInt(message.getRawDataSize());
                stream.write(message.getRawData(), static_cast<size_t>(message.getRawDataSize()));
                
                // Write the timestamp
                stream.writeDouble(message.getTimeStamp());
            }
            
            // Convert to Base64 string
            juce::String base64 = block.toBase64Encoding();
            patternsObj->setProperty(pair.first, base64);
        }
    }
    obj->setProperty("patterns", var(patternsObj));
    
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
        
        // Load generated patterns
        if (auto* patternsObj = obj->getProperty("patterns").getDynamicObject())
        {
            for (auto& prop : patternsObj->getProperties())
            {
                juce::String roleName = prop.name.toString();
                juce::String base64 = prop.value.toString();
                
                // Convert from Base64 string
                juce::MemoryBlock block;
                block.fromBase64Encoding(base64);
                
                juce::MemoryInputStream stream(block, false);
                
                // Create a new MidiMessageSequence
                auto sequence = std::make_shared<juce::MidiMessageSequence>();
                
                // Read the number of events
                int numEvents = stream.readInt();
                
                // Read each event
                for (int i = 0; i < numEvents; ++i)
                {
                    // Read the message
                    int dataSize = stream.readInt();
                    juce::HeapBlock<uint8> data(dataSize);
                    stream.read(data, dataSize);
                    
                    // Read the timestamp
                    double timestamp = stream.readDouble();
                    
                    // Create the message
                    juce::MidiMessage message(data, dataSize, timestamp);
                    
                    // Add to the sequence
                    sequence->addEvent(message);
                }
                
                performance->generatedPatterns[roleName] = sequence;
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
