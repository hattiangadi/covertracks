#pragma once

#include <JuceHeader.h>

namespace covertracks {

/**
 * @brief Represents a song with musical structure
 * 
 * A Song contains basic information like title and tempo, along with
 * the musical structure represented as JSON. This is the base representation
 * that can be edited in the SongEditor and displayed in the SongList.
 * It can later be converted to a Chart for performance.
 */
class Song
{
public:
    /**
     * @brief Construct a new Song object
     */
    Song();
    
    /**
     * @brief Construct a new Song with title and tempo
     * 
     * @param title The song title
     * @param tempo The song tempo in BPM
     */
    Song(const juce::String& title, double tempo);
    
    /**
     * @brief Destroy the Song object
     */
    ~Song();
    
    /**
     * @brief Get the song's unique ID
     * 
     * @return juce::String The unique ID
     */
    juce::String getUniqueID() const { return uniqueID; }
    
    /**
     * @brief Get the song title
     * 
     * @return juce::String The title
     */
    juce::String getTitle() const { return title; }
    
    /**
     * @brief Set the song title
     * 
     * @param newTitle The new title
     */
    void setTitle(const juce::String& newTitle) { title = newTitle; }
    
    /**
     * @brief Get the song tempo
     * 
     * @return double The tempo in BPM
     */
    double getTempo() const { return tempo; }
    
    /**
     * @brief Set the song tempo
     * 
     * @param newTempo The new tempo in BPM
     */
    void setTempo(double newTempo) { tempo = newTempo; }
    
    /**
     * @brief Get the source structure JSON
     * 
     * @return juce::String The structure JSON
     */
    juce::String getSourceStructureJson() const { return sourceStructureJson; }
    
    /**
     * @brief Set the source structure JSON
     * 
     * @param json The structure JSON
     */
    void setSourceStructureJson(const juce::String& json) { sourceStructureJson = json; }
    
    /**
     * @brief Get the number of sections in the song
     * 
     * @return int The number of sections
     */
    int getNumSections() const;
    
    /**
     * @brief Get the number of measures in the song
     * 
     * @return int The number of measures
     */
    int getNumMeasures() const;
    
    /**
     * @brief Convert the song to a var object for serialization
     * 
     * @return juce::var The var representation
     */
    juce::var toVar() const;
    
    /**
     * @brief Create a Song from a var object
     * 
     * @param v The var object
     * @return std::shared_ptr<Song> The created Song
     */
    static std::shared_ptr<Song> fromVar(const juce::var& v);
    
    /**
     * @brief Update song from UI editor data (backend method)
     * @param title Title from UI editor
     * @param tempo Tempo from UI editor
     * @param structureJson Structure JSON from UI editor
     * @return juce::Result Success/failure with error message
     */
    juce::Result updateFromUIData(const juce::String& newTitle, double newTempo, const juce::String& structureJson);
    
    /**
     * @brief Process and update song from extracted structure JSON (backend method)
     * @param extractedJson JSON structure extracted from image/LLM
     * @return juce::Result Success/failure with error message
     */
    juce::Result processExtractedStructure(const juce::var& extractedJson);
    
    /**
     * @brief Process image to extract song structure using LLM (backend method)
     * @param image The image to process
     * @param llmClient The LLM client to use
     * @param callback Callback for completion (success, extracted JSON)
     * @return juce::Result Success/failure with error message
     */
    static juce::Result processImageForStructure(const juce::Image& image, 
                                                std::shared_ptr<class LLMClient> llmClient,
                                                std::function<void(bool, const juce::var&)> callback);
    
    /**
     * @brief Preprocess image for LLM processing (backend method)
     * @param sourceImage The source image
     * @return juce::Image The preprocessed image ready for LLM
     */
    static juce::Image preprocessImageForLLM(const juce::Image& sourceImage);
    
    /**
     * @brief Create a deep copy of this song (backend method)
     * @return std::shared_ptr<Song> Cloned song instance
     */
    std::shared_ptr<Song> clone() const;
    
private:
    juce::String uniqueID;
    juce::String title;
    double tempo;
    juce::String sourceStructureJson;
};

} // namespace covertracks
