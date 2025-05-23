#pragma once

#include <JuceHeader.h>

namespace covertracks {

// Forward declaration of the cache cleanup function
void cleanBarCache();

// Forward declarations
class MusicPatternSystem;
class PatternParser;
class PatternGenerator;

/**
 * @brief Represents a single bar of music
 * 
 * A bar contains a series of notes or events with timing information
 */
class Bar {
public:
    Bar(int timeSignatureNumerator = 4, int timeSignatureDenominator = 4);
    ~Bar();
    
    // Static methods for instance tracking and cleanup
    static void printInstanceCount();
    static int getInstanceCount() { return instanceCount; }
    static void forceCleanupLeakedInstances();
    static void registerStaticInstance(Bar* instance);
    
    void addNote(int beat, int channel, int noteNumber, float velocity, float duration);
    void addController(int beat, int channel, int controllerNumber, int value);
    void addProgramChange(int beat, int channel, int programNumber);
    
    std::unique_ptr<MidiMessageSequence> toMidiSequence() const;
    
    int getTimeSignatureNumerator() const { return timeSignatureNumerator; }
    int getTimeSignatureDenominator() const { return timeSignatureDenominator; }
    void setTimeSignature(int numerator, int denominator);
    
    var toVar() const;
    static std::shared_ptr<Bar> fromVar(const var& v);
    
private:
    struct Event {
        enum Type { Note, Controller, ProgramChange };
        
        Type type;
        float beat;
        int channel;
        int noteNumber;
        float velocity;
        float duration;
        int controllerNumber;
        int value;
        
        Event(Type eventType, float beatPos, int channelNum)
            : type(eventType), beat(beatPos), channel(channelNum), noteNumber(0), 
              velocity(0.0f), duration(0.0f), controllerNumber(0), value(0) {}
    };
    
    int timeSignatureNumerator;
    int timeSignatureDenominator;
    Array<Event> events;
    
    // Static counter for tracking instances
    static int instanceCount;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Bar)
};

/**
 * @brief Represents a musical phrase
 * 
 * A phrase is a sequence of bars that form a musical idea
 */
class Phrase {
public:
    Phrase(const String& name = "");
    ~Phrase();
    
    // Static methods for instance tracking
    static void printInstanceCount();
    static int getInstanceCount() { return instanceCount; }
    
    void addBar(std::shared_ptr<Bar> bar);
    int getNumBars() const;
    std::shared_ptr<Bar> getBar(int index) const;
    
    String getName() const { return name; }
    void setName(const String& newName) { name = newName; }
    
    std::unique_ptr<MidiMessageSequence> toMidiSequence() const;
    
    var toVar() const;
    static std::shared_ptr<Phrase> fromVar(const var& v);
    
private:
    String name;
    Array<std::shared_ptr<Bar>> bars;
    
    // Static counter for tracking instances
    static int instanceCount;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Phrase)
};

/**
 * @brief Represents a section of music
 * 
 * A section is a collection of phrases that form a structural element of a song
 * (e.g., verse, chorus, bridge)
 */
class Section {
public:
    Section(const String& name = "");
    ~Section();
    
    // Static methods for instance tracking
    static void printInstanceCount();
    static int getInstanceCount() { return instanceCount; }
    
    void addPhrase(std::shared_ptr<Phrase> phrase);
    int getNumPhrases() const;
    std::shared_ptr<Phrase> getPhrase(int index) const;
    
    String getName() const { return name; }
    void setName(const String& newName) { name = newName; }
    
    std::unique_ptr<MidiMessageSequence> toMidiSequence() const;
    
    var toVar() const;
    static std::shared_ptr<Section> fromVar(const var& v);
    
private:
    String name;
    Array<std::shared_ptr<Phrase>> phrases;
    
    // Static counter for tracking instances
    static int instanceCount;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
};

/**
 * @brief Represents a complete song
 * 
 * A song is a collection of sections arranged in a specific order
 */


class Song {
public:
    Song(const String& title = "", double tempo = 120.0);
    ~Song();
    
    // Get the unique ID for this song
    juce::String getUniqueID() const { return uniqueID; }
    
    // Static methods for instance tracking
    static void printInstanceCount();
    static int getInstanceCount() { return instanceCount; }
    
    void addSection(std::shared_ptr<Section> section);
    int getNumSections() const;
    std::shared_ptr<Section> getSection(int index) const;
    
    // Method to break reference cycles
    void clearReferences() {
        // Clear all sections to break potential reference cycles
        sections.clear();
    }
    
    String getTitle() const { return title; }
    void setTitle(const String& newTitle) { title = newTitle; }
    
    double getTempo() const { return tempo; }
    void setTempo(double newTempo) { tempo = newTempo; }
    
    std::unique_ptr<MidiMessageSequence> toMidiSequence() const;
    
    var toVar() const;
    static std::shared_ptr<Song> fromVar(const var& v);
    
    // Methods to get/set the source structure JSON string (from Claude/sheet music analysis)
    void setSourceStructureJson(const juce::String& json) { sourceStructureJson = json; }
    juce::String getSourceStructureJson() const { return sourceStructureJson; }
    
private:
    String title;
    double tempo;
    Array<std::shared_ptr<Section>> sections;
    juce::String uniqueID;
    
    // Store the source structure JSON string (from Claude/sheet music analysis)
    juce::String sourceStructureJson;
    
    // Static counter for tracking instances
    static int instanceCount;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Song)
};

} // namespace covertracks
