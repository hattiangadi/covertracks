#include "MusicPatternComponents.h"

namespace covertracks {

// Define a constant for ticks per quarter note used throughout the MIDI sequence generation
const int TICKS_PER_QUARTER_NOTE = 960;

//==============================================================================
// Bar implementation
//==============================================================================

// Initialize static members
int Bar::instanceCount = 0;
static std::vector<Bar*> staticBarInstances;

Bar::Bar(int numerator, int denominator)
    : timeSignatureNumerator(numerator),
      timeSignatureDenominator(denominator)
{
    // Increment instance counter
    ++instanceCount;
    
    // Debug output
    DBG("Bar created: " + String::toHexString((intptr_t)this) + ", total instances: " + String(instanceCount));
}

Bar::~Bar()
{
    // Clear the events array to ensure proper cleanup
    events.clear();
    
    // Decrement instance counter
    --instanceCount;
    
    // Debug output to track Bar destruction
    DBG("Bar destroyed: " + String::toHexString((intptr_t)this) + ", remaining instances: " + String(instanceCount));
}

void Bar::addNote(int beat, int channel, int noteNumber, float velocity, float duration)
{
    Event event(Event::Type::Note, static_cast<float>(beat), channel);
    event.noteNumber = noteNumber;
    event.velocity = velocity;
    event.duration = duration;
    events.add(event);
}

void Bar::addController(int beat, int channel, int controllerNumber, int value)
{
    Event event(Event::Type::Controller, static_cast<float>(beat), channel);
    event.controllerNumber = controllerNumber;
    event.value = value;
    events.add(event);
}

void Bar::addProgramChange(int beat, int channel, int programNumber)
{
    Event event(Event::Type::ProgramChange, static_cast<float>(beat), channel);
    event.value = programNumber;
    events.add(event);
}

std::unique_ptr<MidiMessageSequence> Bar::toMidiSequence() const
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    // Note: We use 960 ticks per quarter note in the MIDI sequence
    
    // Process each event
    for (const auto& event : events)
    {
        // Convert beat position to ticks
        // In a 4/4 time signature, 1 beat = 1 quarter note
        // In other time signatures, we need to adjust accordingly
        double beatsPerQuarterNote = 4.0 / timeSignatureDenominator;
        double tickPosition = event.beat * beatsPerQuarterNote * TICKS_PER_QUARTER_NOTE;
        
        switch (event.type)
        {
            case Event::Type::Note:
            {
                // Note on
                MidiMessage noteOn = MidiMessage::noteOn(event.channel + 1, 
                                                       event.noteNumber, 
                                                       event.velocity);
                noteOn.setTimeStamp(tickPosition);
                sequence->addEvent(noteOn);
                
                // Note off
                double noteOffPosition = tickPosition + (event.duration * beatsPerQuarterNote * TICKS_PER_QUARTER_NOTE);
                MidiMessage noteOff = MidiMessage::noteOff(event.channel + 1, 
                                                         event.noteNumber);
                noteOff.setTimeStamp(noteOffPosition);
                sequence->addEvent(noteOff);
                break;
            }
            
            case Event::Type::Controller:
            {
                MidiMessage cc = MidiMessage::controllerEvent(event.channel + 1, 
                                                            event.controllerNumber, 
                                                            event.value);
                cc.setTimeStamp(tickPosition);
                sequence->addEvent(cc);
                break;
            }
            
            case Event::Type::ProgramChange:
            {
                MidiMessage pc = MidiMessage::programChange(event.channel + 1, 
                                                          event.value);
                pc.setTimeStamp(tickPosition);
                sequence->addEvent(pc);
                break;
            }
        }
    }
    
    // Sort the sequence by timestamp
    sequence->sort();
    
    return sequence;
}

void Bar::setTimeSignature(int numerator, int denominator)
{
    timeSignatureNumerator = numerator;
    timeSignatureDenominator = denominator;
}

//==============================================================================
// Phrase implementation
//==============================================================================

// Initialize static member
int Phrase::instanceCount = 0;

Phrase::Phrase(const String& phraseName)
    : name(phraseName)
{
    // Increment instance counter
    ++instanceCount;
    
    // Debug output
    DBG("Phrase created: " + String::toHexString((intptr_t)this) + ", total instances: " + String(instanceCount));
}

Phrase::~Phrase()
{
    // Clear all references to Bar objects
    bars.clear();
    
    // Decrement instance counter
    --instanceCount;
    
    // Debug output to track Phrase destruction
    DBG("Phrase destroyed: " + String::toHexString((intptr_t)this) + ", remaining instances: " + String(instanceCount));
}

void Phrase::addBar(std::shared_ptr<Bar> bar)
{
    if (bar)
        bars.add(bar);
}

int Phrase::getNumBars() const
{
    return bars.size();
}

std::shared_ptr<Bar> Phrase::getBar(int index) const
{
    if (index >= 0 && index < bars.size())
        return bars[index];
    return nullptr;
}

std::unique_ptr<MidiMessageSequence> Phrase::toMidiSequence() const
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    // Note: We use 960 ticks per quarter note in the MIDI sequence
    
    // Calculate the current position in ticks
    double currentTick = 0.0;
    
    // Process each bar
    for (const auto& bar : bars)
    {
        if (!bar)
            continue;
        
        // Get the MIDI sequence for this bar
        auto barSequence = bar->toMidiSequence();
        if (!barSequence)
            continue;
        
        // Add the bar's events to the phrase sequence with the current offset
        for (int i = 0; i < barSequence->getNumEvents(); ++i)
        {
            auto event = barSequence->getEventPointer(i);
            if (event)
            {
                // Clone the event and adjust its timestamp
                auto newEvent = event->message;
                newEvent.setTimeStamp(newEvent.getTimeStamp() + currentTick);
                sequence->addEvent(newEvent);
            }
        }
        
        // Update the current position
        // For simplicity, assume each bar is 4 quarter notes (1 bar in 4/4 time)
        // In a more complete implementation, this would depend on the time signature
        double beatsInBar = 4.0 * (bar->getTimeSignatureNumerator() / static_cast<double>(bar->getTimeSignatureDenominator())) * 4.0;
        currentTick += beatsInBar * TICKS_PER_QUARTER_NOTE;
    }
    
    // Sort the sequence by timestamp
    sequence->sort();
    
    return sequence;
}

//==============================================================================
// Section implementation
//==============================================================================

// Initialize static member
int Section::instanceCount = 0;

Section::Section(const String& sectionName)
    : name(sectionName)
{
    // Increment instance counter
    ++instanceCount;
    
    // Debug output
    DBG("Section created: " + String::toHexString((intptr_t)this) + ", total instances: " + String(instanceCount));
}

Section::~Section()
{
    // Clear all references to Phrase objects
    phrases.clear();
    
    // Decrement instance counter
    --instanceCount;
    
    // Debug output to track Section destruction
    DBG("Section destroyed: " + String::toHexString((intptr_t)this) + ", remaining instances: " + String(instanceCount));
}

void Section::addPhrase(std::shared_ptr<Phrase> phrase)
{
    if (phrase)
        phrases.add(phrase);
}

int Section::getNumPhrases() const
{
    return phrases.size();
}

std::shared_ptr<Phrase> Section::getPhrase(int index) const
{
    if (index >= 0 && index < phrases.size())
        return phrases[index];
    return nullptr;
}

std::unique_ptr<MidiMessageSequence> Section::toMidiSequence() const
{
    auto sequence = std::make_unique<MidiMessageSequence>();
    
    // Note: We use 960 ticks per quarter note in the MIDI sequence
    
    // Calculate the current position in ticks
    double currentTick = 0.0;
    
    // Process each phrase
    for (const auto& phrase : phrases)
    {
        if (!phrase)
            continue;
        
        // Get the MIDI sequence for this phrase
        auto phraseSequence = phrase->toMidiSequence();
        if (!phraseSequence)
            continue;
        
        // Add the phrase's events to the section sequence with the current offset
        for (int i = 0; i < phraseSequence->getNumEvents(); ++i)
        {
            auto event = phraseSequence->getEventPointer(i);
            if (event)
            {
                // Clone the event and adjust its timestamp
                auto newEvent = event->message;
                newEvent.setTimeStamp(newEvent.getTimeStamp() + currentTick);
                sequence->addEvent(newEvent);
            }
        }
        
        // Update the current position based on the phrase's length
        // This is a simplified approach; in a real implementation, you would
        // calculate the actual length of the phrase based on its bars
        currentTick += phraseSequence->getEndTime();
    }
    
    // Sort the sequence by timestamp
    sequence->sort();
    
    return sequence;
}

//==============================================================================
// Song implementation
//==============================================================================

// Initialize static member
int Song::instanceCount = 0;



Song::Song(const String& songTitle, double songTempo)
    : title(songTitle), tempo(songTempo)
{
    // Generate a unique ID for this song (using JUCE's UUID generator)
    juce::Uuid uuid;
    uniqueID = uuid.toString();
    
    // Increment instance counter
    ++instanceCount;
    
    // Enhanced debug output to help identify where the Song is being created
    String creationInfo;
    
    if (title == "Default Song") {
        creationInfo = "from default initialization";
    } else if (title.isEmpty()) {
        creationInfo = "from unknown location (empty title)";
    } else {
        creationInfo = "with title: " + title;
    }
    
    // Add a breakpoint here to see the call stack when a Song is created
    DBG("[Song] Created " + creationInfo + ", ID: " + uniqueID + ", address: " + String::toHexString((intptr_t)this) + ", total instances: " + String(instanceCount));
    
    // You can set a breakpoint here to inspect the call stack
    // Intentionally left empty for breakpoint placement
}

Song::~Song()
{
    // Clear all references to Section objects
    sections.clear();
    
    // Decrement instance counter
    --instanceCount;
    
    // Enhanced debug output
    DBG("[Song] Destroyed: " + title + ", address: " + String::toHexString((intptr_t)this) + ", remaining instances: " + String(instanceCount));
    
    // You can set a breakpoint here to see where Songs are being destroyed
    // Intentionally left empty for breakpoint placement
}

void Song::addSection(std::shared_ptr<Section> section)
{
    if (section)
        sections.add(section);
}

int Song::getNumSections() const
{
    return sections.size();
}

std::shared_ptr<Section> Song::getSection(int index) const
{
    if (index >= 0 && index < sections.size())
        return sections[index];
    return nullptr;
}

void Bar::printInstanceCount()
{
    DBG("Current Bar instance count: " + String(instanceCount));
}

void Bar::forceCleanupLeakedInstances()
{
    DBG("[Bar] Checking for leaked instances");
    
    // If there are still Bar instances, log a warning
    if (instanceCount > 0)
    {
        DBG("[Bar] WARNING: " + String(instanceCount) + " Bar instances still exist");
    }
}

void Bar::registerStaticInstance(Bar* instance)
{
    if (instance)
    {
        DBG("[Bar] Registered Bar instance: " + String::toHexString((intptr_t)instance));
    }
}

// Static cache for tracking Bar instances
static std::vector<std::weak_ptr<Bar>> barCache;

// Static method to clean up the cache and report any leaks
void cleanBarCache()
{
    // Remove expired weak pointers
    barCache.erase(
        std::remove_if(barCache.begin(), barCache.end(),
                      [](const std::weak_ptr<Bar>& wp) { return wp.expired(); }),
        barCache.end());
    
    // Report any remaining instances
    if (!barCache.empty())
    {
        DBG("[Bar] WARNING: " + String(barCache.size()) + " Bar instances still in cache after cleanup");
        
        // Force cleanup of remaining instances
        for (auto& weakPtr : barCache)
        {
            if (auto bar = weakPtr.lock())
            {
                DBG("[Bar] Forcing cleanup of Bar: " + String::toHexString((intptr_t)bar.get()));
                // The shared_ptr will be reset when it goes out of scope
            }
        }
        
        // Clear the cache
        barCache.clear();
    }
    
    // Also call the static instance cleanup
    Bar::forceCleanupLeakedInstances();
}

std::shared_ptr<Bar> Bar::fromVar(const var& v)
{
    // Clean up expired weak pointers from the cache
    barCache.erase(
        std::remove_if(barCache.begin(), barCache.end(),
                      [](const std::weak_ptr<Bar>& wp) { return wp.expired(); }),
        barCache.end());
    
    auto result = std::make_shared<Bar>();
    
    // Add to cache for tracking
    barCache.push_back(result);
    
    if (auto* obj = v.getDynamicObject())
    {
        // Get time signature
        result->timeSignatureNumerator = obj->hasProperty("timeSignatureNumerator") ? static_cast<int>(obj->getProperty("timeSignatureNumerator")) : 4;
        result->timeSignatureDenominator = obj->hasProperty("timeSignatureDenominator") ? static_cast<int>(obj->getProperty("timeSignatureDenominator")) : 4;
        
        // Get events
        var eventsVar = obj->getProperty("events");
        if (eventsVar.isArray())
        {
            for (const auto& eventVar : *eventsVar.getArray())
            {
                if (auto* eventObj = eventVar.getDynamicObject())
                {
                    Event event(static_cast<Event::Type>(static_cast<int>(eventObj->getProperty("type"))),
                               static_cast<float>(eventObj->getProperty("beat")),
                               static_cast<int>(eventObj->getProperty("channel")));
                    
                    // Set properties based on event type
                    event.noteNumber = eventObj->hasProperty("noteNumber") ? static_cast<int>(eventObj->getProperty("noteNumber")) : 0;
                    event.velocity = eventObj->hasProperty("velocity") ? static_cast<float>(eventObj->getProperty("velocity")) : 0.0f;
                    event.duration = eventObj->hasProperty("duration") ? static_cast<float>(eventObj->getProperty("duration")) : 0.0f;
                    event.controllerNumber = eventObj->hasProperty("controllerNumber") ? static_cast<int>(eventObj->getProperty("controllerNumber")) : 0;
                    event.value = eventObj->hasProperty("value") ? static_cast<int>(eventObj->getProperty("value")) : 0;
                    
                    result->events.add(event);
                }
            }
        }
    }
    
    DBG("[Bar::fromVar] Created Bar: " + String::toHexString((intptr_t)result.get()) + ", total instances: " + String(instanceCount) + ", cache size: " + String(barCache.size()));
    return result;
}

void Phrase::printInstanceCount()
{
    DBG("Current Phrase instance count: " + String(instanceCount));
}

void Section::printInstanceCount()
{
    DBG("Current Section instance count: " + String(instanceCount));
}

void Song::printInstanceCount()
{
    DBG("Current Song instance count: " + String(instanceCount));
}



} // namespace covertracks
