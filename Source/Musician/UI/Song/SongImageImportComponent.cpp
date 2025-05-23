#include "SongImageImportComponent.h"

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
    
    // Set up cancel button (initially hidden)
    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    cancelButton.setVisible(false);
    addAndMakeVisible(cancelButton);
    
    // Set up process button (initially hidden)
    processButton.setButtonText("Extract Song Structure");
    processButton.addListener(this);
    processButton.setVisible(false);
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
    int buttonWidth = 150;
    int buttonSpacing = 10;
    
    #if JUCE_IOS || JUCE_ANDROID
    // With camera button
    importButton.setBounds(buttonArea.getCentreX() - buttonWidth - buttonSpacing / 2, buttonArea.getY(),
                          buttonWidth, buttonArea.getHeight());
    cameraButton.setBounds(buttonArea.getCentreX() + buttonSpacing / 2, buttonArea.getY(),
                          buttonWidth, buttonArea.getHeight());
    #else
    // Without camera button
    importButton.setBounds(buttonArea.getCentreX() - buttonWidth / 2, buttonArea.getY(),
                          buttonWidth, buttonArea.getHeight());
    #endif
    
    // Position cancel and process buttons
    cancelButton.setBounds(buttonArea.getCentreX() - buttonWidth - buttonSpacing / 2, buttonArea.getY(),
                          buttonWidth, buttonArea.getHeight());
    processButton.setBounds(buttonArea.getCentreX() + buttonSpacing / 2, buttonArea.getY(),
                           buttonWidth, buttonArea.getHeight());
    
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
    
    // Update UI
    importButton.setVisible(false);
    #if JUCE_IOS || JUCE_ANDROID
    cameraButton.setVisible(false);
    #endif
    cancelButton.setVisible(true);
    processButton.setVisible(true);
    
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
    
    // Preprocess the image to reduce size (Claude has a 5MB limit)
    juce::Image processedImage = preprocessImageForLLM(loadedImage);
    
    // Convert image to base64 for sending to LLM
    juce::MemoryOutputStream mos;
    juce::JPEGImageFormat jpegFormat;
    
    // Start with moderate quality
    float quality = 0.7f;
    jpegFormat.setQuality(quality);
    jpegFormat.writeImageToStream(processedImage, mos);
    
    // Check size and log
    size_t imageDataSize = mos.getDataSize();
    std::cout << "Image size before base64 encoding: " << (imageDataSize / 1024) << " KB" << std::endl;
    
    // Claude has a 5MB limit, which is about 3.75MB before base64 encoding
    // If the image is still too large, try reducing quality or dimensions further
    const size_t maxSizeBytes = 3 * 1024 * 1024; // 3MB safety margin
    
    if (imageDataSize > maxSizeBytes)
    {
        std::cout << "Image still too large, applying additional compression..." << std::endl;
        
        // Try with lower quality first
        for (quality = 0.5f; quality >= 0.2f && imageDataSize > maxSizeBytes; quality -= 0.1f)
        {
            mos.reset();
            jpegFormat.setQuality(quality);
            jpegFormat.writeImageToStream(processedImage, mos);
            imageDataSize = mos.getDataSize();
            std::cout << "Reduced quality to " << quality << ", new size: " << (imageDataSize / 1024) << " KB" << std::endl;
        }
        
        // If still too large, reduce dimensions further
        if (imageDataSize > maxSizeBytes)
        {
            std::cout << "Still too large, reducing dimensions further..." << std::endl;
            
            // Create a smaller image
            int newWidth = processedImage.getWidth() / 2;
            int newHeight = processedImage.getHeight() / 2;
            
            juce::Image smallerImage(processedImage.getFormat(), newWidth, newHeight, true);
            juce::Graphics g(smallerImage);
            g.drawImage(processedImage, 0, 0, newWidth, newHeight, 
                       0, 0, processedImage.getWidth(), processedImage.getHeight());
            
            // Try again with the smaller image
            mos.reset();
            jpegFormat.setQuality(0.5f); // Reset quality to a reasonable value
            jpegFormat.writeImageToStream(smallerImage, mos);
            imageDataSize = mos.getDataSize();
            std::cout << "Reduced dimensions to " << newWidth << "x" << newHeight 
                      << ", new size: " << (imageDataSize / 1024) << " KB" << std::endl;
            
            // If it's still too large, give up and let the ClaudeClient handle the error
        }
    }
    
    juce::String base64Image = juce::Base64::toBase64(mos.getData(), mos.getDataSize());
    std::cout << "Base64 encoded size: " << (base64Image.length() / 1024) << " KB" << std::endl;
    
    // Prepare the prompt for the LLM
    juce::String prompt = 
        "You are a musician's assistant, performing the first step of converting the given image to a detailed lead sheet "
        "that will later be used to generate an improvised midi performance. Please convert this to json in as much detail as possible, "
        "following this example. Use the following convention for notes: \"1:3\" would be play this note on the first count "
        "(of 4 for 4/4) for 3(of the 4) notes. Indicate rests by setting the note to r. Provide only the complete json in the response, "
        "no additional preamble or final details. Indicate sections with variations to describe the form of the piece at the beginning "
        "(A1, A2, B, A1) A1: 1-4, A2 1-4 measure 4 Chords/notes, B, A1)\n\n"
        "{\n"
        "\"title\": \"Take the \\\"A\\\" Train\",\n"
        "\"composer\": \"Billy Strayhorn\",\n"
        "\"key\": \"C\",\n"
        "\"timeSignature\": \"4/4\",\n"
        "\"tempo\": 132,\n"
        "\"style\": \"Medium Swing\",\n"
        "  \"measures\": [\n"
        "    {\n"
        "      \"meas\": 1,\n"
        "      \"ch\": \"C6\",\n"
        "      \"mel\": {\n"
        "        \"nts\": [\"G4\"],\n"
        "        \"dur\": [\"1:4\"]\n"
        "      },\n"
        "      \"lyr\": \"You-\"\n"
        "    },\n"
        "    {\n"
        "      \"meas\": 2,\n"
        "      \"ch\": \"\",\n"
        "      \"mel\": {\n"
        "        \"nts\": [\"G4\",\"E4\", \"G4\", \"C4\"],\n"
        "        \"dur\": [\"1:.5\", \"1.5:1.5\", \"3:1\", \"4:1\"]\n"
        "      },\n"
        "      \"lyr\": \"-ou must take the\"\n"
        "    }\n"
        "  ]\n"
        "}\n\n"
        "If you cannot determine some information, use null or empty values. For sheet music, focus on extracting the melody, "
        "chords, time signature, key, and any other musical elements visible in the image.";
    
    // Send the request to the LLM using the new image-specific method
    llmClient->sendMessageWithImage(prompt, base64Image, "image/jpeg", [this](bool success, const juce::String& responseText) {
        // Reset processing state
        isProcessing = false;
        processButton.setEnabled(true);
        cancelButton.setEnabled(true);
        
        if (!success)
        {
            // Handle error
            juce::MessageManager::callAsync([this]() {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                      "Error",
                                                      "Failed to process image with LLM.");
                repaint();
            });
            return;
        }
        
        // Try to parse the response as JSON
        juce::var parsedJson;
        
        // Log the raw response for debugging
        std::cout << "Raw Claude response: " << responseText << std::endl;
        
        // Extract JSON from the response text
        // Claude 4 responses are structured differently with content blocks
        juce::String jsonStr = responseText;
        
        // First check if this is a Claude API response with content blocks
        if (responseText.contains("\"content\":[{\"type\":\"text\",\"text\":\""))
        {
            // This is a Claude API response with the new format
            juce::var apiResponse;
            juce::Result apiResult = juce::JSON::parse(responseText, apiResponse);
            
            if (!apiResult.failed() && apiResponse.isObject())
            {
                // Extract the text content from the first content block
                if (apiResponse.hasProperty("content") && apiResponse["content"].isArray())
                {
                    auto contentArray = apiResponse["content"].getArray();
                    if (contentArray && contentArray->size() > 0)
                    {
                        auto firstContent = contentArray->getReference(0);
                        if (firstContent.hasProperty("text"))
                        {
                            jsonStr = firstContent["text"].toString();
                            std::cout << "Extracted JSON from content block: " << jsonStr << std::endl;
                        }
                    }
                }
            }
        }
        
        // Now extract the actual JSON object from the text
        int jsonStart = jsonStr.indexOf("{");
        int jsonEnd = jsonStr.lastIndexOf("}");
        
        if (jsonStart >= 0 && jsonEnd >= 0 && jsonEnd > jsonStart)
        {
            jsonStr = jsonStr.substring(jsonStart, jsonEnd + 1);
            std::cout << "Extracted JSON object: " << jsonStr << std::endl;
        }
        
        juce::Result result = juce::JSON::parse(jsonStr, parsedJson);
        
        if (result.failed())
        {
            // Handle parsing error
            juce::MessageManager::callAsync([this, responseText]() {
                // If JSON parsing failed, we'll just pass the raw text to the callback
                // and let the SongEditorComponent handle it
                juce::var rawResponse;
                rawResponse.getDynamicObject()->setProperty("rawText", responseText);
                
                // Call the callback with the raw response
                if (structureExtractedCallback)
                {
                    structureExtractedCallback(rawResponse);
                }
                
                // Reset the component
                reset();
                repaint();
            });
            return;
        }
        
        // Call the callback with the parsed JSON
        juce::MessageManager::callAsync([this, parsedJson]() {
            if (structureExtractedCallback)
            {
                structureExtractedCallback(parsedJson);
            }
            
            // Reset the component
            reset();
            repaint();
        });
    });
}

juce::Image SongImageImportComponent::preprocessImageForLLM(const juce::Image& sourceImage)
{
    // Claude has a 5MB limit for images, so we need to ensure our image is under this limit
    // First, check if the image is already grayscale, if not convert it
    juce::Image processedImage;
    
    if (sourceImage.getFormat() != juce::Image::PixelFormat::SingleChannel)
    {
        // Convert to grayscale (single channel) for sheet music
        // Sheet music is typically black and white, so this preserves quality while reducing size
        processedImage = juce::Image(juce::Image::PixelFormat::SingleChannel, 
                                   sourceImage.getWidth(), 
                                   sourceImage.getHeight(), 
                                   true);
        
        juce::Graphics g(processedImage);
        g.drawImageAt(sourceImage, 0, 0);
    }
    else
    {
        // Already grayscale, just use as is
        processedImage = sourceImage;
    }
    
    // Check if we need to resize the image
    // A good rule of thumb is to keep the total pixel count under 2 million for sheet music
    // This preserves enough detail while keeping file size reasonable
    const int maxPixels = 2000000; // 2 million pixels (e.g., ~1400x1400)
    const int currentPixels = processedImage.getWidth() * processedImage.getHeight();
    
    if (currentPixels > maxPixels)
    {
        // Calculate the scale factor to reduce to target size
        float scaleFactor = std::sqrt(static_cast<float>(maxPixels) / currentPixels);
        
        int newWidth = static_cast<int>(processedImage.getWidth() * scaleFactor);
        int newHeight = static_cast<int>(processedImage.getHeight() * scaleFactor);
        
        // Ensure dimensions are at least 100 pixels
        newWidth = std::max(100, newWidth);
        newHeight = std::max(100, newHeight);
        
        // Create the resized image
        juce::Image resizedImage(processedImage.getFormat(), newWidth, newHeight, true);
        juce::Graphics g(resizedImage);
        
        // Use high quality resampling for sheet music to preserve details
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(processedImage, 0, 0, newWidth, newHeight, 
                   0, 0, processedImage.getWidth(), processedImage.getHeight());
        
        processedImage = resizedImage;
        
        std::cout << "Resized image from " << sourceImage.getWidth() << "x" << sourceImage.getHeight() 
                  << " to " << newWidth << "x" << newHeight << std::endl;
    }
    
    return processedImage;
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
    importButton.setVisible(true);
    importButton.setEnabled(true);
    #if JUCE_IOS || JUCE_ANDROID
    cameraButton.setVisible(true);
    cameraButton.setEnabled(true);
    #endif
    cancelButton.setVisible(false);
    processButton.setVisible(false);
    
    instructionsLabel.setText("Drag and drop a sheet music image or click Import to select a file",
                             juce::dontSendNotification);
    
    repaint();
}

} // namespace covertracks
