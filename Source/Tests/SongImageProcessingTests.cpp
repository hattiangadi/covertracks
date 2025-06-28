#include <JuceHeader.h>
#include "../Musician/Core/Song.h"
#include "../LLM/Core/LLMClient.h"

namespace covertracks {

/**
 * @brief Mock LLM Client for testing Song image processing
 */
class MockLLMClient : public LLMClient
{
public:
    MockLLMClient() = default;
    
    LLMClient* clone() const override
    {
        return new MockLLMClient();
    }
    
    void sendMessage(const juce::String& message, ResponseCallback callback) override
    {
        // Simple mock response
        callback(true, "Mock LLM response");
    }
    
    void sendMessageWithHistory(const juce::String& message, 
                              const juce::Array<std::pair<juce::String, juce::String>>& history,
                              ResponseCallback callback) override
    {
        // Mock response incorporating history
        juce::String mockResponse = "Mock response to: " + message + " (with " + juce::String(history.size()) + " history items)";
        callback(true, mockResponse);
    }
    
    void sendMessageWithImage(const juce::String& message, 
                              const juce::String& imageBase64,
                              const juce::String& imageType,
                              ResponseCallback callback) override
    {
        // Mock JSON response for image processing
        juce::String mockResponse = R"({
            "title": "Test Song",
            "tempo": 120,
            "sections": [
                {
                    "name": "intro", 
                    "measures": 4,
                    "chords": ["C", "Am", "F", "G"]
                },
                {
                    "name": "verse",
                    "measures": 8, 
                    "chords": ["C", "G", "Am", "F"]
                }
            ]
        })";
        
        callback(true, mockResponse);
    }
    
    bool isConnected() const override
    {
        return true; // Mock client is always "connected"
    }
    
    juce::String getServiceName() const override
    {
        return "MockLLM";
    }
};

/**
 * @brief Tests for Song image processing functionality
 */
class SongImageProcessingTests : public juce::UnitTest
{
public:
    SongImageProcessingTests() : juce::UnitTest("Song Image Processing", "Song Backend") {}

    void runTest() override
    {
        testImagePreprocessing();
        testImageProcessingWithMockLLM();
        testExtractedStructureProcessing();
    }

private:
    void testImagePreprocessing()
    {
        beginTest("Image preprocessing for LLM");
        
        // Create a test image
        juce::Image testImage(juce::Image::RGB, 2000, 1500, true);
        juce::Graphics g(testImage);
        g.fillAll(juce::Colours::white);
        g.setColour(juce::Colours::black);
        g.drawText("Test Sheet Music", testImage.getBounds(), juce::Justification::centred);
        
        // Test preprocessing
        juce::Image processed = Song::preprocessImageForLLM(testImage);
        
        // Should be converted to grayscale and resized
        expect(processed.getFormat() == juce::Image::SingleChannel, "Image should be converted to grayscale");
        expect(processed.getWidth() <= testImage.getWidth(), "Image width should be <= original");
        expect(processed.getHeight() <= testImage.getHeight(), "Image height should be <= original");
        
        // Should be under 2 million pixels
        int totalPixels = processed.getWidth() * processed.getHeight();
        expect(totalPixels <= 2000000, "Processed image should be under 2M pixels");
    }
    
    void testImageProcessingWithMockLLM()
    {
        beginTest("Image processing with mock LLM");
        
        // Create test image
        juce::Image testImage(juce::Image::RGB, 800, 600, true);
        juce::Graphics g(testImage);
        g.fillAll(juce::Colours::white);
        g.setColour(juce::Colours::black);
        g.drawText("Mock Sheet Music", testImage.getBounds(), juce::Justification::centred);
        
        auto mockClient = std::make_shared<MockLLMClient>();
        bool callbackCalled = false;
        bool success = false;
        juce::var extractedData;
        
        // Test image processing
        auto result = Song::processImageForStructure(testImage, mockClient, 
            [&](bool callbackSuccess, const juce::var& data) {
                callbackCalled = true;
                success = callbackSuccess;
                extractedData = data;
            });
        
        expect(result.wasOk(), "processImageForStructure should return success");
        
        // Wait a bit for async callback
        juce::Thread::sleep(100);
        
        expect(callbackCalled, "Callback should be called");
        expect(success, "Callback should indicate success");
        expect(extractedData.isObject(), "Extracted data should be an object");
        
        if (extractedData.isObject())
        {
            expect(extractedData.hasProperty("title"), "Should have title property");
            expect(extractedData.hasProperty("tempo"), "Should have tempo property");
            expect(extractedData.hasProperty("sections"), "Should have sections property");
        }
    }
    
    void testExtractedStructureProcessing()
    {
        beginTest("Processing extracted structure in Song");
        
        Song song;
        
        // Create mock extracted structure
        juce::DynamicObject::Ptr extractedObj = new juce::DynamicObject();
        extractedObj->setProperty("title", "Extracted Song");
        extractedObj->setProperty("tempo", 140.0);
        
        juce::Array<juce::var> sections;
        juce::DynamicObject::Ptr section1 = new juce::DynamicObject();
        section1->setProperty("name", "verse");
        section1->setProperty("measures", 8);
        sections.add(juce::var(section1.get()));
        
        extractedObj->setProperty("sections", sections);
        
        juce::var extractedData(extractedObj.get());
        
        // Test processing
        auto result = song.processExtractedStructure(extractedData);
        expect(result.wasOk(), "processExtractedStructure should succeed");
        
        // Verify song was updated
        expect(song.getTitle() == "Extracted Song", "Song title should be updated");
        expect(std::abs(song.getTempo() - 140.0) < 0.1, "Song tempo should be updated");
        expect(song.getSourceStructureJson().isNotEmpty(), "Source structure JSON should be set");
    }
};

// Register the test
static SongImageProcessingTests songImageProcessingTests;

} // namespace covertracks