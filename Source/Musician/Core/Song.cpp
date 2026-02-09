#include "Song.h"
#include "../../LLM/Core/LLMClient.h"

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

juce::Result Song::updateFromUIData(const juce::String& newTitle, double newTempo, const juce::String& structureJson)
{
    // Validate JSON structure before applying changes
    juce::var parsedJson;
    juce::Result result = juce::JSON::parse(structureJson, parsedJson);
    
    if (result.failed())
    {
        return juce::Result::fail("Invalid JSON structure: " + result.getErrorMessage());
    }
    
    // Apply changes
    setTitle(newTitle);
    setTempo(newTempo);
    setSourceStructureJson(structureJson);
    
    return juce::Result::ok();
}

juce::Result Song::processExtractedStructure(const juce::var& extractedJson)
{
    // Handle different types of input
    if (!extractedJson.isObject())
    {
        // If it's not a valid JSON object, check if it's raw text
        if (extractedJson.hasProperty("rawText"))
        {
            // Just update with the raw text
            setSourceStructureJson(extractedJson["rawText"].toString());
            return juce::Result::ok();
        }
        else
        {
            return juce::Result::fail("Failed to parse song structure - invalid format");
        }
    }
    else
    {
        // Set basic properties from the JSON
        setTitle(extractedJson.getProperty("title", "New Song").toString());
        setTempo(extractedJson.getProperty("tempo", 120.0));
        
        // Store the pretty-printed JSON structure
        juce::String structureJson = juce::JSON::toString(extractedJson, true);
        setSourceStructureJson(structureJson);
        
        return juce::Result::ok();
    }
}

std::shared_ptr<Song> Song::clone() const
{
    // Create a new song with the same title and tempo
    auto clonedSong = std::make_shared<Song>(title, tempo);
    
    // Copy the structure JSON
    clonedSong->setSourceStructureJson(sourceStructureJson);
    
    return clonedSong;
}

juce::Result Song::processImageForStructure(const juce::Image& image, 
                                          std::shared_ptr<LLMClient> llmClient,
                                          std::function<void(bool, const juce::var&)> callback)
{
    if (image.isNull())
        return juce::Result::fail("No image provided");
    
    if (!llmClient)
        return juce::Result::fail("No LLM client available");
    
    if (!callback)
        return juce::Result::fail("No callback provided");
    
    // Preprocess the image to reduce size for LLM
    juce::Image processedImage = preprocessImageForLLM(image);
    
    // Convert image to base64 for sending to LLM
    juce::MemoryOutputStream mos;
    juce::JPEGImageFormat jpegFormat;
    
    // Start with moderate quality
    float quality = 0.7f;
    jpegFormat.setQuality(quality);
    jpegFormat.writeImageToStream(processedImage, mos);
    
    // Check size and apply compression if needed
    size_t imageDataSize = mos.getDataSize();
    const size_t maxSizeBytes = 3 * 1024 * 1024; // 3MB safety margin
    
    if (imageDataSize > maxSizeBytes)
    {
        // Try with lower quality
        for (quality = 0.5f; quality >= 0.2f && imageDataSize > maxSizeBytes; quality -= 0.1f)
        {
            mos.reset();
            jpegFormat.setQuality(quality);
            jpegFormat.writeImageToStream(processedImage, mos);
            imageDataSize = mos.getDataSize();
        }
        
        // If still too large, reduce dimensions further
        if (imageDataSize > maxSizeBytes)
        {
            int targetWidth = static_cast<int>(processedImage.getWidth() * 0.8);
            int targetHeight = static_cast<int>(processedImage.getHeight() * 0.8);
            processedImage = processedImage.rescaled(targetWidth, targetHeight);
            
            mos.reset();
            jpegFormat.setQuality(0.5f);
            jpegFormat.writeImageToStream(processedImage, mos);
            imageDataSize = mos.getDataSize();
        }
    }
    
    // Convert to base64
    juce::String base64Image = juce::Base64::toBase64(mos.getData(), mos.getDataSize());
    
    // Create the prompt for structure extraction — per-measure lead sheet format
    juce::String prompt = "Analyze this sheet music image and extract the complete lead sheet. "
                         "Output one entry per measure with the chord symbol and melody notes.\n"
                         "Provide a JSON response with EXACTLY this format:\n"
                         "{\n"
                         "  \"title\": \"Song Title\",\n"
                         "  \"tempo\": 120,\n"
                         "  \"measures\": [\n"
                         "    {\"meas\": 1, \"ch\": \"C\", \"mel\": {\"nts\": [\"C4\", \"C4\", \"G4\", \"G4\"], \"dur\": [\"1:1\", \"2:1\", \"3:1\", \"4:1\"]}},\n"
                         "    {\"meas\": 2, \"ch\": \"F\", \"mel\": {\"nts\": [\"A4\", \"A4\", \"G4\"], \"dur\": [\"1:1\", \"2:1\", \"3:2\"]}}\n"
                         "  ]\n"
                         "}\n\n"
                         "Rules:\n"
                         "- One entry per measure (bar) in order — do NOT skip or merge measures\n"
                         "- \"ch\": the chord symbol above that measure (e.g. \"C\", \"Dm7\", \"G7\", \"FMaj7\"). "
                         "If no chord is written, leave empty string and the previous chord continues.\n"
                         "- \"mel\": melody notes in that measure. \"nts\" = note names with octave (e.g. \"C4\", \"Eb5\"), "
                         "\"dur\" = \"beat:duration\" pairs where beat is position in measure (1-based) and duration is in beats (1=quarter, 0.5=eighth, 2=half). "
                         "Use \"r\" for rests.\n"
                         "- Estimate tempo from the style if not marked\n"
                         "- Output raw JSON only, no markdown fences or explanations";
    
    // Send to LLM with image
    llmClient->sendMessageWithImage(prompt, base64Image, "image/jpeg", [callback](bool success, const juce::String& responseText) {
        if (!success)
        {
            callback(false, juce::var());
            return;
        }
        
        // Parse the response to extract JSON
        juce::String jsonStr = responseText;
        juce::var parsedJson;
        
        // Try to parse directly first
        juce::Result result = juce::JSON::parse(jsonStr, parsedJson);
        
        if (result.failed())
        {
            // Try extracting from API response format
            juce::var apiResponse;
            juce::Result apiResult = juce::JSON::parse(responseText, apiResponse);
            
            if (!apiResult.failed() && apiResponse.isObject())
            {
                if (apiResponse.hasProperty("content") && apiResponse["content"].isArray())
                {
                    auto contentArray = apiResponse["content"].getArray();
                    if (contentArray && contentArray->size() > 0)
                    {
                        auto firstContent = contentArray->getReference(0);
                        if (firstContent.hasProperty("text"))
                        {
                            jsonStr = firstContent["text"].toString();
                        }
                    }
                }
            }
            
            // Extract JSON object from text
            int jsonStart = jsonStr.indexOf("{");
            int jsonEnd = jsonStr.lastIndexOf("}");
            
            if (jsonStart >= 0 && jsonEnd >= 0 && jsonEnd > jsonStart)
            {
                jsonStr = jsonStr.substring(jsonStart, jsonEnd + 1);
                result = juce::JSON::parse(jsonStr, parsedJson);
            }
        }
        
        if (result.failed())
        {
            // If JSON parsing failed, pass raw text
            auto* obj = new juce::DynamicObject();
            obj->setProperty("rawText", responseText);
            callback(true, juce::var(obj));
        }
        else
        {
            // Successfully parsed JSON
            callback(true, parsedJson);
        }
    });
    
    return juce::Result::ok();
}

juce::Image Song::preprocessImageForLLM(const juce::Image& sourceImage)
{
    juce::Image processedImage;
    
    // Convert to grayscale for sheet music (preserves quality while reducing size)
    if (sourceImage.getFormat() != juce::Image::PixelFormat::SingleChannel)
    {
        processedImage = juce::Image(juce::Image::PixelFormat::SingleChannel, 
                                   sourceImage.getWidth(), 
                                   sourceImage.getHeight(), 
                                   true);
        
        juce::Graphics g(processedImage);
        g.drawImageAt(sourceImage, 0, 0);
    }
    else
    {
        processedImage = sourceImage;
    }
    
    // Check if we need to resize the image
    // Keep total pixel count under 2 million for sheet music
    const int maxPixels = 2000000;
    const int currentPixels = processedImage.getWidth() * processedImage.getHeight();
    
    if (currentPixels > maxPixels)
    {
        double scaleFactor = juce::jmin(1.0, std::sqrt(static_cast<double>(maxPixels) / currentPixels));
        int newWidth = static_cast<int>(processedImage.getWidth() * scaleFactor);
        int newHeight = static_cast<int>(processedImage.getHeight() * scaleFactor);
        
        processedImage = processedImage.rescaled(newWidth, newHeight);
    }
    
    return processedImage;
}

} // namespace covertracks
