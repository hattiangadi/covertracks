#include "MusicPatternComponents.h"

namespace covertracks {

//==============================================================================
// Bar implementation
//==============================================================================

Bar::Bar(int timeSignatureNumerator, int timeSignatureDenominator)
    : timeSignatureNumerator(timeSignatureNumerator),
      timeSignatureDenominator(timeSignatureDenominator)
{
}

Bar::~Bar()
{
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
    
    // Calculate the total number of ticks per quarter note
    const int ticksPerQuarterNote = 960;
    
    // Process each event
    for (const auto& event : events)
    {
        // Convert beat position to ticks
        // In a 4/4 time signature, 1 beat = 1 quarter note
        // In other time signatures, we need to adjust accordingly
        double beatsPerQuarterNote = 4.0 / timeSignatureDenominator;
        double tickPosition = event.beat * beatsPerQuarterNote * ticksPerQuarterNote;
        
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
                double noteOffPosition = tickPosition + (event.duration * beatsPerQuarterNote * ticksPerQuarterNote);
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

Phrase::Phrase(const String& name)
    : name(name)
{
}

Phrase::~Phrase()
{
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
    
    // Calculate the total number of ticks per quarter note
    const int ticksPerQuarterNote = 960;
    
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
        currentTick += beatsInBar * ticksPerQuarterNote;
    }
    
    // Sort the sequence by timestamp
    sequence->sort();
    
    return sequence;
}

//==============================================================================
// Section implementation
//==============================================================================

Section::Section(const String& name)
    : name(name)
{
}

Section::~Section()
{
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
    
    // Calculate the total number of ticks per quarter note
    const int ticksPerQuarterNote = 960;
    
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

Song::Song(const String& title, double tempo)
    : title(title), tempo(tempo)
{
}

Song::~Song()
{
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

} // namespace covertracks
