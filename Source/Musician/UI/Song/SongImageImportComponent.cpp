#include "SongImageImportComponent.h"
#include "../../Core/Song.h"

namespace covertracks {

SongImageImportComponent::SongImageImportComponent(std::shared_ptr<LLMClient> clientPtr, 
                                                   SongStructureExtractedCallback callback)
    : hasImage(false),
      isProcessing(false),
      llmClient(clientPtr),
      structureExtractedCallback(callback)
{
    // Set up instructions label
    instructionsLabel.setText("Drag and drop a sheet music image or click Import to select a file",
                              juce::dontSendNotification);
    juce::FontOptions fontOptions;
    instructionsLabel.setFont(fontOptions.withHeight(16.0f).withStyle("bold"));
    instructionsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(instructionsLabel);
    
    // Set up import button
    importButton.setButtonText("Import Image");
    importButton.addListener(this);
    addAndMakeVisible(importButton);
    
    // Set up camera button (if available on platform)
    #if JUCE_IOS || JUCE_ANDROID
    cameraButton.setButtonText("Take Photo");
    cameraButton.addListener(this);
    addAndMakeVisible(cameraButton);
    #endif
    
    // Set up cancel button
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);
    
    // Set up process button (initially disabled)
    processButton.setButtonText("Extract Song Structure");
    processButton.addListener(this);
    processButton.setEnabled(false); // Disabled until an image is loaded
    addAndMakeVisible(processButton);
    
    // Set size constraints
    setSize(600, 400);
}

SongImageImportComponent::~SongImageImportComponent()
{
    // Clean up resources
}

void SongImageImportComponent::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
    
    // Draw drop area
    juce::Rectangle<int> dropArea = getLocalBounds().reduced(20).withTrimmedBottom(60);
    g.setColour(hasImage ? juce::Colours::transparentBlack : juce::Colours::lightgrey.withAlpha(0.3f));
    g.fillRect(dropArea);
    g.setColour(juce::Colours::grey);
    g.drawRect(dropArea, 1);
    
    // Draw image if loaded
    if (hasImage && !loadedImage.isNull())
    {
        // Calculate aspect ratio
        float imageAspect = (float)loadedImage.getWidth() / (float)loadedImage.getHeight();
        float areaAspect = (float)dropArea.getWidth() / (float)dropArea.getHeight();
        
        juce::Rectangle<int> imageBounds;
        if (imageAspect > areaAspect)
        {
            // Image is wider than area
            int height = (int)((float)dropArea.getWidth() / imageAspect);
            imageBounds = juce::Rectangle<int>(dropArea.getX(), dropArea.getY() + (dropArea.getHeight() - height) / 2,
                                              dropArea.getWidth(), height);
        }
        else
        {
            // Image is taller than area
            int width = (int)((float)dropArea.getHeight() * imageAspect);
            imageBounds = juce::Rectangle<int>(dropArea.getX() + (dropArea.getWidth() - width) / 2, dropArea.getY(),
                                              width, dropArea.getHeight());
        }
        
        g.drawImageWithin(loadedImage, imageBounds.getX(), imageBounds.getY(),
                          imageBounds.getWidth(), imageBounds.getHeight(),
                          juce::RectanglePlacement::centred);
    }
    else if (!hasImage)
    {
        // Draw drop instructions
        g.setColour(juce::Colours::grey);
        g.setFont(16.0f);
        g.drawText("Drop sheet music image here", dropArea, juce::Justification::centred, true);
    }
    
    // Draw processing indicator
    if (isProcessing)
    {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRect(getLocalBounds());
        
        g.setColour(juce::Colours::white);
        g.setFont(18.0f);
        g.drawText("Processing image...", getLocalBounds(), juce::Justification::centred, true);
    }
}

void SongImageImportComponent::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    auto buttonArea = bounds.removeFromBottom(40);
    
    // Position buttons
    int buttonWidth = 120;
    int buttonSpacing = 10;
    int totalWidth = 3 * buttonWidth + 2 * buttonSpacing;
    int startX = buttonArea.getCentreX() - totalWidth / 2;
    
    // Position all three buttons in a row
    importButton.setBounds(startX, buttonArea.getY(), buttonWidth, buttonArea.getHeight());
    cancelButton.setBounds(startX + buttonWidth + buttonSpacing, buttonArea.getY(), buttonWidth, buttonArea.getHeight());
    processButton.setBounds(startX + 2 * (buttonWidth + buttonSpacing), buttonArea.getY(), buttonWidth, buttonArea.getHeight());
    
    #if JUCE_IOS || JUCE_ANDROID
    // Adjust for camera button if needed
    cameraButton.setBounds(buttonArea.getCentreX() - buttonWidth / 2, buttonArea.getY() + buttonArea.getHeight() + 10,
                          buttonWidth, buttonArea.getHeight());
    #endif
    
    // Position instructions label
    instructionsLabel.setBounds(bounds.removeFromTop(30));
}

bool SongImageImportComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Only interested in image files
    for (auto& file : files)
    {
        juce::File f(file);
        if (f.existsAsFile() && 
            (f.hasFileExtension("jpg") || 
             f.hasFileExtension("jpeg") || 
             f.hasFileExtension("png") || 
             f.hasFileExtension("gif") || 
             f.hasFileExtension("bmp")))
        {
            return true;
        }
    }
    
    return false;
}

void SongImageImportComponent::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    // Try to load the first valid image file
    for (auto& file : files)
    {
        juce::File f(file);
        if (loadImageFromFile(f))
        {
            break;
        }
    }
}

void SongImageImportComponent::buttonClicked(juce::Button* button)
{
    if (button == &importButton)
    {
        // Open file chooser to select an image
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a sheet music image",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.jpg;*.jpeg;*.png;*.gif;*.bmp");
        
        auto chooserFlags = juce::FileBrowserComponent::openMode | 
                            juce::FileBrowserComponent::canSelectFiles;
        
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
        {
            auto result = chooser.getResult();
            if (result.existsAsFile())
            {
                loadImageFromFile(result);
            }
        });
    }
    #if JUCE_IOS || JUCE_ANDROID
    else if (button == &cameraButton)
    {
        // TODO: Implement camera capture
        // This requires platform-specific code
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                              "Not Implemented",
                                              "Camera capture is not yet implemented.");
    }
    #endif
    else if (button == &cancelButton)
    {
        // Reset the component
        reset();
    }
    else if (button == &processButton)
    {
        // Process the image with the LLM
        processImageWithLLM();
    }
}

bool SongImageImportComponent::loadImageFromFile(const juce::File& file)
{
    // Check if file exists and is an image
    if (!file.existsAsFile())
    {
        return false;
    }
    
    // Load the image
    juce::Image image = juce::ImageFileFormat::loadFrom(file);
    if (image.isNull())
    {
        return false;
    }
    
    // Store the image and path
    loadedImage = image;
    imagePath = file.getFullPathName();
    hasImage = true;
    
    // Enable the process button now that we have an image
    processButton.setEnabled(true);
    
    instructionsLabel.setText("Image loaded. Click Extract Song Structure to process.",
                             juce::dontSendNotification);
    
    repaint();
    return true;
}

void SongImageImportComponent::processImageWithLLM()
{
    // Check if we have an image and LLM client
    if (!hasImage || loadedImage.isNull() || !llmClient)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                              "Error",
                                              "No image loaded or LLM client not available.");
        return;
    }
    
    // Set processing state
    isProcessing = true;
    processButton.setEnabled(false);
    cancelButton.setEnabled(false);
    repaint();
    
    // Use backend method for image processing
    auto result = Song::processImageForStructure(loadedImage, llmClient, [this](bool success, const juce::var& extractedJson) {
        // Reset processing state
        juce::MessageManager::callAsync([this, success, extractedJson]() {
            isProcessing = false;
            processButton.setEnabled(true);
            cancelButton.setEnabled(true);
            
            if (success)
            {
                // Call the callback with the extracted structure
                if (structureExtractedCallback)
                {
                    structureExtractedCallback(extractedJson);
                }
                
                // Reset the component
                reset();
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                      "Processing Failed",
                                                      "Failed to extract song structure from image.");
            }
            
            repaint();
        });
    });
    
    if (result.failed())
    {
        isProcessing = false;
        processButton.setEnabled(true);
        cancelButton.setEnabled(true);
        
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                              "Error",
                                              result.getErrorMessage());
        repaint();
    }
}

void SongImageImportComponent::setLLMClient(std::shared_ptr<LLMClient> client)
{
    llmClient = client;
}

void SongImageImportComponent::reset()
{
    // Reset state
    hasImage = false;
    loadedImage = juce::Image();
    imagePath = "";
    isProcessing = false;
    
    // Reset UI
    importButton.setEnabled(true);
    #if JUCE_IOS || JUCE_ANDROID
    cameraButton.setEnabled(true);
    #endif
    // Disable the process button since we no longer have an image
    processButton.setEnabled(false);
    
    instructionsLabel.setText("Drag and drop a sheet music image or click Import to select a file",
                             juce::dontSendNotification);
    
    repaint();
}

} // namespace covertracks
