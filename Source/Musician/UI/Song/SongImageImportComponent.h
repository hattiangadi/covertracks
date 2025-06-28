#pragma once

#include <JuceHeader.h>
#include "../../../LLM/Core/LLMClient.h"

namespace covertracks {

/**
 * @brief Component for importing song structure from images
 * 
 * Allows users to upload images of sheet music or fakebooks and
 * uses an LLM with vision capabilities to extract song structure.
 */
class SongImageImportComponent : public juce::Component,
                                 public juce::FileDragAndDropTarget,
                                 public juce::Button::Listener
{
public:
    /**
     * @brief Callback for when a song structure is extracted
     */
    using SongStructureExtractedCallback = std::function<void(const juce::var&)>;

    /**
     * @brief Construct a new Song Image Import Component
     * 
     * @param llmClient The LLM client to use for image analysis
     * @param callback Callback to call when a song structure is extracted
     */
    SongImageImportComponent(std::shared_ptr<LLMClient> llmClient, 
                             SongStructureExtractedCallback callback);
    
    /**
     * @brief Destroy the Song Image Import Component
     */
    ~SongImageImportComponent() override;
    
    /**
     * @brief Paint the component
     * 
     * @param g The graphics context
     */
    void paint(juce::Graphics& g) override;
    
    /**
     * @brief Handle component resizing
     */
    void resized() override;
    
    /**
     * @brief Check if the component is interested in the dragged file
     * 
     * @param files The files being dragged
     * @return true If the component is interested in the files
     * @return false If the component is not interested in the files
     */
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    
    /**
     * @brief Handle files being dropped onto the component
     * 
     * @param files The files being dropped
     * @param x The x coordinate of the drop
     * @param y The y coordinate of the drop
     */
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    /**
     * @brief Handle button clicks
     * 
     * @param button The button that was clicked
     */
    void buttonClicked(juce::Button* button) override;
    
    /**
     * @brief Set the LLM client to use for image analysis
     * 
     * @param client The LLM client
     */
    void setLLMClient(std::shared_ptr<LLMClient> client);
    
private:
    /**
     * @brief Load an image from a file
     * 
     * @param file The file to load
     * @return true If the image was loaded successfully
     * @return false If the image could not be loaded
     */
    bool loadImageFromFile(const juce::File& file);
    
    /**
     * @brief Process the loaded image with the LLM
     */
    void processImageWithLLM();
    
    
    /**
     * @brief Reset the component to its initial state
     */
    void reset();
    
    // UI Components
    juce::Label instructionsLabel;
    juce::TextButton importButton;
    juce::TextButton cameraButton;
    juce::TextButton cancelButton;
    juce::TextButton processButton;
    
    // Image handling
    juce::Image loadedImage;
    juce::String imagePath;
    bool hasImage;
    
    // LLM client
    std::shared_ptr<LLMClient> llmClient;
    
    // Callback
    SongStructureExtractedCallback structureExtractedCallback;
    
    // Processing state
    bool isProcessing;
    
    // File chooser (needs to be stored as a member to prevent destruction during async operations)
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongImageImportComponent)
};

} // namespace covertracks
